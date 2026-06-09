#include <stdlib.h>
#include <assert.h>

#include "movegen.h"
#include "common.h"
#include "squares.h"
#include "attacks.h"

int gen_king_moves(board_t *board, int sq, move_t *moves)
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

int gen_knight_moves(board_t *board, int sq, move_t *moves)
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

int gen_diag_moves(board_t *board, int sq, move_t *moves)
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

int gen_orth_moves(board_t *board, int sq, move_t *moves)
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

int gen_pawn_moves(board_t *board, int sq, move_t *moves)
{
    piece_t p = board->squares[sq];
    assert(piece_type(p) == PAWN && "invalid piece type for gen_pawn_moves");
    if (piece_color(p) != board->turn)
        return 0;

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
        if (target < 0 || target >= FILES * RANKS)
            continue;
        if ((target % FILES) - sq_file != cap_file_offsets[i])
            continue; // wrap

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
        if (piece_type(at) == NO_PIECE || piece_color(at) == board->turn)
            continue;

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

int gen_pseudo_legal_moves(board_t *board, move_t *moves)
{
    int count = 0;

    for (size_t sq = 0; sq < FILES * RANKS; sq++)
    {
        piece_t p = board->squares[sq];
        if (piece_color(p) != board->turn)
            continue;

        switch (piece_type(p))
        {
        case KING:
            count += gen_king_moves(board, sq, moves + count);
            break;
        case KNIGHT:
            count += gen_knight_moves(board, sq, moves + count);
            break;
        case BISHOP:
            count += gen_diag_moves(board, sq, moves + count);
            break;
        case ROOK:
            count += gen_orth_moves(board, sq, moves + count);
            break;
        case PAWN:
            count += gen_pawn_moves(board, sq, moves + count);
            break;
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

bool board_in_check(board_t *board)
{
    int king_sq = FIND_KING(board, board->turn);
    return is_square_attacked(board, idx_to_square(king_sq), board->turn == WHITE ? BLACK : WHITE);
}

// to_explore will be in first position in the moves array
int gen_legal_moves(board_t *board, move_t *moves, move_t to_explore)
{
    move_t pseudo[512];
    int n = gen_pseudo_legal_moves(board, pseudo);
    int count = 0;
    color_t turn = board->turn;

    if (to_explore != 0)
        moves[count++] = to_explore;

    for (size_t i = 0; i < n; i++)
    {
        if (pseudo[i] == to_explore)
            continue;

        board_make_move(board, pseudo[i]);
        int king_sq = FIND_KING(board, turn);

        square_t king = idx_to_square(king_sq);
        if (!is_square_attacked(board, king, board->turn))
            moves[count++] = pseudo[i];

        board_unmake_move(board, pseudo[i]);
    }
    return count;
}

int gen_capture_moves(board_t *board, move_t *moves)
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
