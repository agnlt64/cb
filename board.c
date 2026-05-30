#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>

#include "board.h"
#include "squares.h"

#define W_KSIDE 0b1000
#define W_QSIDE 0b0100
#define B_KSIDE 0b0010
#define B_QSIDE 0b0001

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

static const int knight_offsets[8] = {17, 15, 10, 6, -6, -10, -15, -17};
static const int knight_file_offsets[8] = {1, -1, 2, -2, 2, -2, 1, -1};

static const int king_offsets[8] = {1, -1, 8, -8, 9, -9, 7, -7};
static const int king_file_offsets[8] = {1, -1, 0, 0, 1, -1, -1, 1};

static const int diag_offsets[4] = {9, -9, 7, -7};
static const int diag_file_offsets[4] = {1, -1, -1, 1};

static const int orth_offsets[4] = {8, -8, 1, -1};
static const int orth_file_offsets[4] = {0, 0, 1, -1};

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
                    board->squares[rank * RANKS + file] = type | color;
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
                board->ep_square_idx = ep_rank * RANKS + ep_file;
            }
        }

        // half move
        if (space_count == 4 && isdigit(c))
            board->halfmove = board->halfmove * 10 + (c - '0');

        // full move
        if (space_count == 5 && isdigit(c))
            board->fullmove = board->fullmove * 10 + (c - '0');
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
            int piece = b->squares[rank * RANKS + file];
            printf("%c ", piece_string(piece));
        }
        printf("\n");
    }
}

int gen_king_moves(board_t* board, int sq, move_t* moves)
{
    piece_t p = board->squares[sq];
    assert(piece_type(p) == KING && "invalid piece type for gen_king_moves");
    if (piece_color(p) != board->turn)
        return 0;

    int count = 0;
    int sq_file = sq % FILES;

    for (size_t i = 0; i < 8; i++)
    {
        int target = sq + king_offsets[i];
        if (target < 0 || target >= FILES * RANKS)
            continue;

        if ((target % FILES) - sq_file != king_file_offsets[i])
            continue; // wrap around

        piece_t at = board->squares[target];
        if (piece_color(at) == board->turn)
            continue; // ally piece

        int flag = piece_type(at) == NO_PIECE ? FLAG_QUIET : FLAG_CAPTURE;
        int captured = piece_type(at);
        moves[count++] = MOVE_ENCODE(sq, target, flag, captured);
    }

    // todo: refactor this
    if (board->turn == WHITE && sq == square_to_idx(E1))
    {
        if (board->castling & W_KSIDE &&
            board->squares[square_to_idx(F1)] == NO_PIECE &&
            board->squares[square_to_idx(G1)] == NO_PIECE &&
            !is_square_attacked(board, E1, BLACK) &&
            !is_square_attacked(board, F1, BLACK) &&
            !is_square_attacked(board, G1, BLACK))
            moves[count++] = MOVE_ENCODE(square_to_idx(E1), square_to_idx(G1), FLAG_CASTLE_K, NO_PIECE);

        if (board->castling & W_QSIDE &&
            board->squares[square_to_idx(B1)] == NO_PIECE &&
            board->squares[square_to_idx(C1)] == NO_PIECE &&
            board->squares[square_to_idx(D1)] == NO_PIECE &&
            !is_square_attacked(board, E1, BLACK) &&
            !is_square_attacked(board, D1, BLACK) &&
            !is_square_attacked(board, C1, BLACK))
            moves[count++] = MOVE_ENCODE(square_to_idx(E1), square_to_idx(C1), FLAG_CASTLE_Q, NO_PIECE);
    }
    else if (board->turn == BLACK && sq == square_to_idx(E8))
    {
        if (board->castling & B_KSIDE &&
            board->squares[square_to_idx(F8)] == NO_PIECE &&
            board->squares[square_to_idx(G8)] == NO_PIECE &&
            !is_square_attacked(board, E8, WHITE) &&
            !is_square_attacked(board, F8, WHITE) &&
            !is_square_attacked(board, G8, WHITE))
            moves[count++] = MOVE_ENCODE(square_to_idx(E8), square_to_idx(G8), FLAG_CASTLE_K, NO_PIECE);

        if (board->castling & B_QSIDE &&
            board->squares[square_to_idx(B8)] == NO_PIECE &&
            board->squares[square_to_idx(C8)] == NO_PIECE &&
            board->squares[square_to_idx(D8)] == NO_PIECE &&
            !is_square_attacked(board, E8, WHITE) &&
            !is_square_attacked(board, D8, WHITE) &&
            !is_square_attacked(board, C8, WHITE))
            moves[count++] = MOVE_ENCODE(square_to_idx(E8), square_to_idx(C8), FLAG_CASTLE_Q, NO_PIECE);
    }

    return count;
}

int gen_knight_moves(board_t* board, int sq, move_t* moves)
{
    piece_t p = board->squares[sq];
    assert(piece_type(p) == KNIGHT && "invalid piece type for gen_knight_moves");
    if (piece_color(p) != board->turn)
        return 0;

    int count = 0;
    int sq_file = sq % FILES;

    for (size_t i = 0; i < 8; i++)
    {
        int target = sq + knight_offsets[i];
        if (target < 0 || target >= FILES * RANKS)
            continue;

        if ((target % FILES) - sq_file != knight_file_offsets[i])
            continue; // wrap around

        piece_t at = board->squares[target];
        if (piece_color(at) == board->turn)
            continue; // ally piece

        int flag = piece_type(at) == NO_PIECE ? FLAG_QUIET : FLAG_CAPTURE;
        int captured = piece_type(at);
        moves[count++] = MOVE_ENCODE(sq, target, flag, captured);
    }
    return count;
}

int gen_diag_moves(board_t* board, int sq, move_t* moves)
{
    piece_t p = board->squares[sq];
    if (piece_color(p) != board->turn)
        return 0;

    int count = 0;

    for (size_t d = 0; d < 4; d++)
    {
        int curr = sq;
        int curr_file = sq % FILES;

        while (true)
        {
            int next = curr + diag_offsets[d];
            int next_file = next % FILES;

            if (next < 0 || next >= FILES * RANKS)
                break; // out of board

            if (next_file - curr_file != diag_file_offsets[d])
                break; // wrap around

            piece_t at = board->squares[next];
            if (piece_color(at) == board->turn)
                break; // ally piece

            int flag = piece_type(at) == NO_PIECE ? FLAG_QUIET : FLAG_CAPTURE;
            int captured = piece_type(at);
            moves[count++] = MOVE_ENCODE(sq, next, flag, captured);

            if (flag == FLAG_CAPTURE)
                break; // piece captured, end of move

            curr = next;
            curr_file = next_file;
        }
    }
    return count;
}

int gen_orth_moves(board_t* board, int sq, move_t* moves)
{
    piece_t p = board->squares[sq];
    if (piece_color(p) != board->turn)
        return 0;

    int count = 0;

    for (size_t d = 0; d < 4; d++)
    {
        int curr = sq;
        int curr_file = sq % FILES;

        while (true)
        {
            int next = curr + orth_offsets[d];
            int next_file = next % FILES;

            if (next < 0 || next >= FILES * RANKS)
                break; // out of board

            if (next_file - curr_file != orth_file_offsets[d])
                break; // wrap around

            piece_t at = board->squares[next];
            if (piece_color(at) == board->turn)
                break; // ally piece

            int flag = piece_type(at) == NO_PIECE ? FLAG_QUIET : FLAG_CAPTURE;
            int captured = piece_type(at);
            moves[count++] = MOVE_ENCODE(sq, next, flag, captured);

            if (flag == FLAG_CAPTURE)
                break; // piece captured, end of move

            curr = next;
            curr_file = next_file;
        }
    }
    return count;
}

int gen_pawn_moves(board_t* board, int sq, move_t* moves)
{
    piece_t p = board->squares[sq];
    assert(piece_type(p) == PAWN && "invalid piece type for gen_pawn_moves");
    if (piece_color(p) != board->turn) return 0;

    int count = 0;
    int sq_file = sq % FILES;
    int sq_rank = sq / RANKS;

    int push = (board->turn == WHITE) ? 8 : -8;
    int start_rank = (board->turn == WHITE) ? 1 : 6;
    int promo_rank = (board->turn == WHITE) ? 7 : 0;

    int fwd = sq + push;
    if (fwd >= 0 && fwd < FILES * RANKS && piece_type(board->squares[fwd]) == NO_PIECE)
    {
        if (fwd / RANKS == promo_rank)
        {
            moves[count++] = MOVE_ENCODE(sq, fwd, FLAG_PROMO_N, NO_PIECE);
            moves[count++] = MOVE_ENCODE(sq, fwd, FLAG_PROMO_B, NO_PIECE);
            moves[count++] = MOVE_ENCODE(sq, fwd, FLAG_PROMO_R, NO_PIECE);
            moves[count++] = MOVE_ENCODE(sq, fwd, FLAG_PROMO_Q, NO_PIECE);
        }
        else
        {
            moves[count++] = MOVE_ENCODE(sq, fwd, FLAG_QUIET, NO_PIECE);

            if (sq_rank == start_rank)
            {
                int fwd2 = sq + 2 * push;
                if (piece_type(board->squares[fwd2]) == NO_PIECE)
                    moves[count++] = MOVE_ENCODE(sq, fwd2, FLAG_QUIET, NO_PIECE);
            }
        }
    }

    int cap_offsets[2] = {push - 1, push + 1};
    int cap_file_offsets[2] = {-1, 1};

    for (int i = 0; i < 2; i++)
    {
        int target = sq + cap_offsets[i];
        if (target < 0 || target >= FILES * RANKS) continue;
        if ((target % FILES) - sq_file != cap_file_offsets[i]) continue; // wrap

        if (target == board->ep_square_idx)
        {
            int cap_sq = target + (board->turn == WHITE ? -8 : 8);
            piece_t cap_piece = board->squares[cap_sq];
            color_t opp = board->turn == WHITE ? BLACK : WHITE;
            if (piece_type(cap_piece) == PAWN && piece_color(cap_piece) == opp)
                moves[count++] = MOVE_ENCODE(sq, target, FLAG_EP, PAWN);
            continue;
        }

        piece_t at = board->squares[target];
        if (piece_type(at) == NO_PIECE || piece_color(at) == board->turn) continue;

        if (target / RANKS == promo_rank)
        {
            moves[count++] = MOVE_ENCODE(sq, target, FLAG_PROMO_N, piece_type(at));
            moves[count++] = MOVE_ENCODE(sq, target, FLAG_PROMO_B, piece_type(at));
            moves[count++] = MOVE_ENCODE(sq, target, FLAG_PROMO_R, piece_type(at));
            moves[count++] = MOVE_ENCODE(sq, target, FLAG_PROMO_Q, piece_type(at));
        }
        else
        {
            moves[count++] = MOVE_ENCODE(sq, target, FLAG_CAPTURE, piece_type(at));
        }
    }

    return count;
}

int gen_pseudo_legal_moves(board_t* board, move_t* moves)
{
    int count = 0;

    for (size_t sq = 0; sq < FILES * RANKS; sq++)
    {
        piece_t p = board->squares[sq];
        if (piece_color(p) != board->turn) continue;

        switch (piece_type(p))
        {
            case KING: count += gen_king_moves(board, sq, moves + count); break;
            case KNIGHT: count += gen_knight_moves(board, sq, moves + count); break;
            case BISHOP: count += gen_diag_moves(board, sq, moves + count); break;
            case ROOK: count += gen_orth_moves(board, sq, moves + count); break;
            case PAWN: count += gen_pawn_moves(board, sq, moves + count); break;
            case QUEEN:
                count += gen_diag_moves(board, sq, moves + count);
                count += gen_orth_moves(board, sq, moves + count);
                break;
            case NO_PIECE:
            default:
                break;
        }
    }

    return count;
}

bool board_in_check(board_t* board)
{
    int king_sq = find_king(board, board->turn);
    return is_square_attacked(board, idx_to_square(king_sq), board->turn == WHITE ? BLACK : WHITE);
}

int find_king(board_t* board, color_t turn)
{
    int king_sq = -1;
    for (int sq = 0; sq < 64; sq++)
    {
        if (board->squares[sq] == (turn | KING))
        {
            king_sq = sq;
            break;
        }
    }
    return king_sq;
}

// to_explore will be in first position in the moves array
int gen_legal_moves(board_t* board, move_t* moves, move_t to_explore)
{
    move_t pseudo[512];
    int n = gen_pseudo_legal_moves(board, pseudo);
    int count = 0;
    color_t turn = board->turn;

    if (to_explore != 0)
        moves[count++] = to_explore;

    for (size_t i = 0; i < n; i++)
    {
        if (pseudo[i] == to_explore) continue;

        board_make_move(board, pseudo[i]);
        int king_sq = find_king(board, turn);

        square_t king = idx_to_square(king_sq);
        if (!is_square_attacked(board, king, board->turn))
            moves[count++] = pseudo[i];

        board_unmake_move(board, pseudo[i]);
    }
    return count;
}

int gen_capture_moves(board_t* board, move_t* moves)
{
    move_t m[512];
    int n = gen_legal_moves(board, m, 0);
    int count = 0;

    for (size_t i = 0; i < n; i++)
    {
        move_t move = m[i];
        int flag = MOVE_FLAGS(move);
        if (flag == FLAG_CAPTURE || flag == FLAG_EP)
        {
            moves[count++] = move;
        }
        else if (flag == FLAG_PROMO_Q)
        {
            moves[count++] = move;
        }
        else if (MOVE_CAPTURED(move) != NO_PIECE && (flag == FLAG_PROMO_N || flag == FLAG_PROMO_B || flag == FLAG_PROMO_R))
        {
            moves[count++] = move;
        }
    }
    return count;
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

    if (board->turn == BLACK) board->fullmove++;

    board->ep_square_idx = -1;
    board->squares[to] = moving;
    board->squares[from] = NO_PIECE;

    switch (flag)
    {
        case FLAG_EP:
        {
            int cap_sq = to + (board->turn == WHITE ? -8 : 8);
            board->squares[cap_sq] = NO_PIECE;
            board->hash ^= z->pieces[board->turn == WHITE ? (BLACK|PAWN) : (WHITE|PAWN)][cap_sq];
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
        case FLAG_PROMO_N: board->squares[to] = board->turn | KNIGHT; break;
        case FLAG_PROMO_B: board->squares[to] = board->turn | BISHOP; break;
        case FLAG_PROMO_R: board->squares[to] = board->turn | ROOK; break;
        case FLAG_PROMO_Q: board->squares[to] = board->turn | QUEEN; break;
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
        if (board->turn == WHITE) board->castling &= ~(W_KSIDE | W_QSIDE);
        else                      board->castling &= ~(B_KSIDE | B_QSIDE);
    }
    if (from == square_to_idx(A1) || to == square_to_idx(A1)) board->castling &= ~W_QSIDE;
    if (from == square_to_idx(H1) || to == square_to_idx(H1)) board->castling &= ~W_KSIDE;
    if (from == square_to_idx(A8) || to == square_to_idx(A8)) board->castling &= ~B_QSIDE;
    if (from == square_to_idx(H8) || to == square_to_idx(H8)) board->castling &= ~B_KSIDE;

    board->hash ^= z->castling[board->castling];

    if (board->ep_square_idx != -1)
        board->hash ^= z->en_passant[board->ep_square_idx % 8];

    board->turn = board->turn == WHITE ? BLACK : WHITE;
    board->hash ^= z->turn;
}

void board_unmake_move(board_t* board, move_t move)
{
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    int flag = MOVE_FLAGS(move);
    int captured = MOVE_CAPTURED(move);

    color_t opp = board->turn;
    board->turn = board->turn == WHITE ? BLACK : WHITE;

    board_history_t s = board->history[--board->history_top];
    board->castling = s.castling;
    board->ep_square_idx = s.ep_square_idx;
    board->halfmove = s.halfmove;
    board->hash = s.hash;

    if (board->turn == BLACK)
        board->fullmove--;

    board->squares[from] = board->squares[to];
    board->squares[to] = captured ? (captured | opp) : NO_PIECE;

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

bool is_square_attacked(board_t* board, square_t sq, color_t color)
{
    return (
        can_knight_attack(board, sq, color) ||
        can_orth_attack(board, sq, color) ||
        can_diag_attack(board, sq, color) ||
        can_pawn_attack(board, sq, color) ||
        can_king_attack(board, sq, color));
}

bool can_knight_attack(board_t* board, square_t sq, color_t color)
{
    int idx = square_to_idx(sq);
    int sq_file = sq.file;

    for (size_t i = 0; i < 8; i++)
    {
        int target = idx + knight_offsets[i];
        if (target < 0 || target >= FILES * RANKS)
            continue;

        if ((target % FILES) - sq_file != knight_file_offsets[i])
            continue; // wrap around

        piece_t piece = board->squares[target];
        if (piece_type(piece) == KNIGHT && piece_color(piece) == color)
            return true;
    }
    return false;
}

bool can_king_attack(board_t* board, square_t sq, color_t color)
{
    int idx = square_to_idx(sq);
    int sq_file = sq.file;

    for (size_t i = 0; i < 8; i++)
    {
        int target = idx + king_offsets[i];
        if (target < 0 || target >= FILES * RANKS)
            continue;

        if ((target % FILES) - sq_file != king_file_offsets[i])
            continue; // wrap around

        piece_t piece = board->squares[target];
        if (piece_type(piece) == KING && piece_color(piece) == color)
            return true;
    }
    return false;
}

bool can_pawn_attack(board_t* board, square_t sq, color_t color)
{
    int offsets[2] = {color == WHITE ? -9 : 9, color == WHITE ? -7 : 7};
    int file_offsets[2] = {color == WHITE ? -1 : 1, color == WHITE ? 1 : -1};

    int idx = square_to_idx(sq);
    int sq_file = sq.file;

    for (size_t i = 0; i < 2; i++)
    {
        int target = idx + offsets[i];
        if (target < 0 || target >= FILES * RANKS)
            continue;

        if ((target % FILES) - sq_file != file_offsets[i])
            continue; // wrap around

        piece_t piece = board->squares[target];
        if (piece_type(piece) == PAWN && piece_color(piece) == color)
            return true;
    }
    return false;
}

bool can_diag_attack(board_t* board, square_t sq, color_t color)
{
    int idx = square_to_idx(sq);

    for (int d = 0; d < 4; d++)
    {
        int curr = idx;
        int curr_file = idx % FILES;

        while (true)
        {
            int next = curr + diag_offsets[d];
            int next_file = next % FILES;

            if (next < 0 || next >= FILES * RANKS)
                break; // out of board

            if (next_file - curr_file != diag_file_offsets[d])
                break; // wrap around

            piece_t piece = board->squares[next];
            if (piece_color(piece) == color)
            {
                if (piece_type(piece) == BISHOP || piece_type(piece) == QUEEN)
                    return true;
                break; // ally piece blocks
            }
            if (piece_type(piece) != NO_PIECE) break; // enemy piece blocks

            curr = next;
            curr_file = next_file;
        }
    }
    return false;
}

bool can_orth_attack(board_t* board, square_t sq, color_t color)
{
    int idx = square_to_idx(sq);

    for (int d = 0; d < 4; d++)
    {
        int curr = idx;
        int curr_file = idx % FILES;

        while (true)
        {
            int next = curr + orth_offsets[d];
            int next_file = next % FILES;

            if (next < 0 || next >= FILES * RANKS)
                break; // out of board

            if (next_file - curr_file != orth_file_offsets[d])
                break; // wrap around

            piece_t piece = board->squares[next];
            if (piece_color(piece) == color)
            {
                if (piece_type(piece) == ROOK || piece_type(piece) == QUEEN)
                    return true;
                break; // ally piece blocks
            }
            if (piece_type(piece) != NO_PIECE) break; // enemy piece blocks

            curr = next;
            curr_file = next_file;
        }
    }
    return false;
}