#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#include "board.h"
#include "squares.h"
#include "common.h"
#include "movegen.h"
#include "attacks.h"

uint64_t zobrist_from_board(board_t* board)
{
    uint64_t new_key = 0;
    zobrist_t* z = &board->zobrist;

    for (size_t sq = 0; sq < 64; sq++)
    {
        piece_t p = board->squares[sq];
        if (piece_type(p) != NO_PIECE)
            new_key ^= z->pieces[p][sq];
    }

    if (board->ep_square_idx != -1)
        new_key ^= z->en_passant[idx_to_square(board->ep_square_idx).file];

    if (board->turn == BLACK)
        new_key ^= z->turn;

    new_key ^= z->castling[board->castling];

    return new_key;
}

void board_init(board_t* board)
{
    board_from_fen(board, DEFAULT_FEN);
    board->turn = WHITE;

    zobrist_init(&board->zobrist);
    board->hash = zobrist_from_board(board);
}

void board_from_fen(board_t* board, const char* fen)
{
    memset(board, 0, sizeof(*board));
    int file = 0;
    int rank = 7;
    int space_count = 0;
    board->ep_square_idx = -1;

    for (const char* p = fen;* p != '\0'; p++)
    {
        char c =* p;
        if (c == ' ')
            space_count++;

        // pieces position
        if (space_count == 0)
        {
            if (c == '/')
            {
                file = 0;
                rank--;
            }
            else
            {
                if (isdigit(c))
                    file += c - '0';
                else
                {
                    int color = isupper(c) ? WHITE : BLACK;
                    int type = CHAR_TO_PIECE[tolower(c)];
                    int sq_idx = rank*  RANKS + file;
                    if (type == KING)
                        board->king_sq[COLOR_IDX(color)] = sq_idx;
                    board->squares[sq_idx] = type | color;
                    file++;
                }
            }
        }

        // side to move
        if (space_count == 1)
        {
            if (c == 'b')
                board->turn = BLACK;

            if (c == 'w')
                board->turn = WHITE;
        }

        // castling ability
        if (space_count == 2)
        {
            if (c == 'K')
                board->castling |= W_KSIDE;
            else if (c == 'Q')
                board->castling |= W_QSIDE;
            else if (c == 'k')
                board->castling |= B_KSIDE;
            else if (c == 'q')
                board->castling |= B_QSIDE;
            else if (c == '-')
                board->castling = 0;
        }

        // en passant square
        if (space_count == 3)
        {
            int ep_file = -1;
            int ep_rank = -1;

            if (c == '-')
                board->ep_square_idx = -1;
            else if (islower(c))
            {
                ep_file = c - 'a';
                ep_rank =* (p + 1) - '1';
                board->ep_square_idx = ep_rank*  RANKS + ep_file;
            }
        }

        // half move
        if (space_count == 4 && isdigit(c))
            board->halfmove = board->halfmove*  10 + (c - '0');

        // full move
        if (space_count == 5 && isdigit(c))
            board->fullmove = board->fullmove*  10 + (c - '0');
    }

    zobrist_init(&board->zobrist);
    board->hash = zobrist_from_board(board);
}

piece_t board_at(board_t* board, square_t sq)
{
    return board->squares[square_to_idx(sq)];
}

void board_print(board_t* b)
{
    for (int rank = RANKS - 1; rank >= 0; rank--)
    {
        for (int file = 0; file < FILES; file++)
        {
            int piece = b->squares[rank*  RANKS + file];
            printf("%c ", piece_string(piece));
        }
        printf("\n");
    }
}

void board_flip_turn(board_t* board)
{
    board->turn = board->turn == WHITE ? BLACK : WHITE;
}

static int perft_inner(board_t* board, int depth)
{
    if (depth == 0)
        return 1;

    move_t moves[512];
    int n = gen_legal_moves(board, moves, 0);

    int num_pos = 0;
    for (size_t i = 0; i < n; i++)
    {
        board_make_move(board, moves[i]);
        num_pos += perft_inner(board, depth - 1);
        board_unmake_move(board, moves[i]);
    }
    return num_pos;
}

int board_perft(board_t* board, int depth, bool verbose)
{
    if (depth == 0)
        return 1;

    move_t moves[512];
    int n = gen_legal_moves(board, moves, 0);

    int num_pos = 0;
    for (size_t i = 0; i < n; i++)
    {
        board_make_move(board, moves[i]);
        int inc = perft_inner(board, depth - 1);
        num_pos += inc;
        board_unmake_move(board, moves[i]);

        if (verbose)
            printf("%s: %d\n", move_to_uci(moves[i]), inc);
    }

    if (verbose)
        printf("\nNodes searched: %d\n", num_pos);

    return num_pos;
}

void board_make_move(board_t* board, move_t move)
{
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    int flag = MOVE_FLAGS(move);
    piece_t moving = board->squares[from];
    zobrist_t* z = &board->zobrist;

    if (piece_type(moving) == NO_PIECE || piece_color(moving) != board->turn)
    {
#ifdef UCI_DEBUG
        assert(false && "unreachable");
#else
        // push dummy history entry so that unmake_move
        // never corrupts the actual history
        board->history[board->history_top++] = (board_history_t){
            .castling = board->castling,
            .ep_square_idx = board->ep_square_idx,
            .halfmove = board->halfmove,
            .hash = board->hash};
        board_flip_turn(board);
        return;
#endif
    }

    board->history[board->history_top++] = (board_history_t){
        .castling = board->castling,
        .ep_square_idx = board->ep_square_idx,
        .halfmove = board->halfmove,
        .hash = board->hash,
    };

    board->hash ^= z->pieces[moving][from];
    board->hash ^= z->castling[board->castling];
    if (board->ep_square_idx != -1)
        board->hash ^= z->en_passant[board->ep_square_idx % 8];

    if (MOVE_CAPTURED(move) != NO_PIECE && flag != FLAG_EP)
        board->hash ^= z->pieces[board->squares[to]][to];

    if (piece_type(moving) == PAWN || flag == FLAG_CAPTURE || flag == FLAG_EP)
        board->halfmove = 0;
    else
        board->halfmove++;

    if (board->turn == BLACK)
        board->fullmove++;

    board->ep_square_idx = -1;
    board->squares[to] = moving;
    board->squares[from] = NO_PIECE;

    if (piece_type(moving) == KING)
        board->king_sq[COLOR_IDX(board->turn)] = to;

    switch (flag)
    {
    case FLAG_EP:
    {
        int cap_sq = to + (board->turn == WHITE ? -8 : 8);
        board->squares[cap_sq] = NO_PIECE;
        board->hash ^= z->pieces[board->turn == WHITE ? (BLACK | PAWN) : (WHITE | PAWN)][cap_sq];
        break;
    }
    case FLAG_CASTLE_K:
    {
        int rf = (board->turn == WHITE ? square_to_idx(H1) : square_to_idx(H8));
        int rt = (board->turn == WHITE ? square_to_idx(F1) : square_to_idx(F8));
        board->squares[rt] = board->squares[rf];
        board->squares[rf] = NO_PIECE;
        board->hash ^= z->pieces[board->turn | ROOK][rf];
        board->hash ^= z->pieces[board->turn | ROOK][rt];
        break;
    }
    case FLAG_CASTLE_Q:
    {
        int rf = (board->turn == WHITE ? square_to_idx(A1) : square_to_idx(A8));
        int rt = (board->turn == WHITE ? square_to_idx(D1) : square_to_idx(D8));
        board->squares[rt] = board->squares[rf];
        board->squares[rf] = NO_PIECE;
        board->hash ^= z->pieces[board->turn | ROOK][rf];
        board->hash ^= z->pieces[board->turn | ROOK][rt];
        break;
    }
    case FLAG_PROMO_N:
        board->squares[to] = board->turn | KNIGHT;
        break;
    case FLAG_PROMO_B:
        board->squares[to] = board->turn | BISHOP;
        break;
    case FLAG_PROMO_R:
        board->squares[to] = board->turn | ROOK;
        break;
    case FLAG_PROMO_Q:
        board->squares[to] = board->turn | QUEEN;
        break;
    default:
    {
        // double pawn push
        if (piece_type(moving) == PAWN && (to - from == 16 || from - to == 16))
            board->ep_square_idx = (from + to) / 2;
        break;
    }
    }

    board->hash ^= z->pieces[board->squares[to]][to];

    if (piece_type(moving) == KING)
    {
        if (board->turn == WHITE)
            board->castling &= ~(W_KSIDE | W_QSIDE);
        else
            board->castling &= ~(B_KSIDE | B_QSIDE);
    }
    if (from == square_to_idx(A1) || to == square_to_idx(A1))
        board->castling &= ~W_QSIDE;
    if (from == square_to_idx(H1) || to == square_to_idx(H1))
        board->castling &= ~W_KSIDE;
    if (from == square_to_idx(A8) || to == square_to_idx(A8))
        board->castling &= ~B_QSIDE;
    if (from == square_to_idx(H8) || to == square_to_idx(H8))
        board->castling &= ~B_KSIDE;

    board->hash ^= z->castling[board->castling];

    if (board->ep_square_idx != -1)
        board->hash ^= z->en_passant[board->ep_square_idx % 8];

    board_flip_turn(board);
    board->hash ^= z->turn;
}

void board_unmake_move(board_t* board, move_t move)
{
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    int flag = MOVE_FLAGS(move);
    int captured = MOVE_CAPTURED(move);

    color_t opp = board->turn;
    board_flip_turn(board);

    board_history_t s = board->history[--board->history_top];
    board->castling = s.castling;
    board->ep_square_idx = s.ep_square_idx;
    board->halfmove = s.halfmove;
    board->hash = s.hash;

    if (board->turn == BLACK)
        board->fullmove--;

    board->squares[from] = board->squares[to];
    board->squares[to] = captured ? (captured | opp) : NO_PIECE;

    if (piece_type(board->squares[from]) == KING)
        board->king_sq[COLOR_IDX(board->turn)] = from;

    switch (flag)
    {
    case FLAG_EP:
    {
        int cap_sq = to + (board->turn == WHITE ? -8 : 8);
        board->squares[cap_sq] = opp | PAWN;
        board->squares[to] = NO_PIECE;
        break;
    }
    case FLAG_CASTLE_K:
    {
        int rf = (board->turn == WHITE ? square_to_idx(H1) : square_to_idx(H8));
        int rt = (board->turn == WHITE ? square_to_idx(F1) : square_to_idx(F8));
        board->squares[rf] = board->squares[rt];
        board->squares[rt] = NO_PIECE;
        break;
    }
    case FLAG_CASTLE_Q:
    {
        int rf = (board->turn == WHITE ? square_to_idx(A1) : square_to_idx(A8));
        int rt = (board->turn == WHITE ? square_to_idx(D1) : square_to_idx(D8));
        board->squares[rf] = board->squares[rt];
        board->squares[rt] = NO_PIECE;
        break;
    }
    case FLAG_PROMO_N:
    case FLAG_PROMO_B:
    case FLAG_PROMO_R:
    case FLAG_PROMO_Q:
        board->squares[from] = board->turn | PAWN;
        break;
    default:
        break;
    }
}

void order_moves(board_t* board, move_t* moves, int moves_size, killer_t* killers, int depth, bool q_search, move_t pv_move, int history[64][64])
{
    color_t opp = board->turn == WHITE ? BLACK : WHITE;
    int scores[256];

    for (int i = 0; i < moves_size; i++)
    {
        move_t move = moves[i];
        if (move == pv_move)
        {
            scores[i] = INT_MAX;
            continue;
        }
        int score = 0;
        int start_sq = MOVE_FROM(move);
        int target_sq = MOVE_TO(move);

        int move_piece = board->squares[start_sq];
        int move_piece_type = piece_type(move_piece);
        int captured_piece_type = piece_type(board->squares[target_sq]);
        bool is_capture = MOVE_CAPTURED(move) != 0;
        int flag = MOVE_FLAGS(move);
        int value = piece_value(move_piece_type);

        if (is_capture)
        {
            int see_val = see(board, move);
            if (see_val >= 0)
                score += 8000000 + see_val;
            else
                score -= 2000000 - see_val;
        }

        if (move_piece_type == PAWN)
        {
            if (flag == FLAG_PROMO_Q && !is_capture)
                score += 6000000;
        }
        else
        {
            int to_score = piece_square_value(move_piece, target_sq);
            int from_score = piece_square_value(move_piece, start_sq);
            score += to_score - from_score;

            if (!is_capture && is_square_attacked(board, idx_to_square(target_sq), opp))
                score -= 50;
        }

        if (!is_capture)
        {
            bool is_killer = !q_search && depth < MAX_DEPTH && killer_contains(killers, depth, move);
            if (is_killer)
                score += 4000000;
            else if (history)
                score += history[start_sq][target_sq];
        }

        scores[i] = score;
    }

    for (int i = 1; i < moves_size; i++)
    {
        move_t m = moves[i];
        int s = scores[i];
        int j = i - 1;
        while (j >= 0 && scores[j] < s)
        {
            moves[j + 1] = moves[j];
            scores[j + 1] = scores[j];
            j--;
        }
        moves[j + 1] = m;
        scores[j + 1] = s;
    }
}
