#include <stdlib.h>
#include <string.h>

#include "attacks.h"
#include "common.h"
#include "precomputed_eval_data.h"

bool is_square_attacked(board_t* board, square_t sq, color_t color)
{
    return (
        can_knight_attack(board, sq, color) ||
        can_orth_attack(board, sq, color) ||
        can_diag_attack(board, sq, color) ||
        can_pawn_attack(board, sq, color) ||
        can_king_attack(board, sq, color));
}

int internal__knight_attack(piece_t squares[64], square_t sq, color_t color)
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

        piece_t piece = squares[target];
        if (piece_type(piece) == KNIGHT && piece_color(piece) == color)
            return target;
    }
    return -1;
}

bool can_knight_attack(board_t* board, square_t sq, color_t color)
{
    return internal__knight_attack(board->squares, sq, color) >= 0;
}

int internal__king_attack(piece_t squares[64], square_t sq, color_t color)
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

        piece_t piece = squares[target];
        if (piece_type(piece) == KING && piece_color(piece) == color)
            return target;
    }
    return -1;
}

bool can_king_attack(board_t* board, square_t sq, color_t color)
{
    return internal__king_attack(board->squares, sq, color) >= 0;
}

int internal__pawn_attack(piece_t squares[64], square_t sq, color_t color)
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

        piece_t piece = squares[target];
        if (piece_type(piece) == PAWN && piece_color(piece) == color)
            return target;
    }
    return -1;
}

bool can_pawn_attack(board_t* board, square_t sq, color_t color)
{
    return internal__pawn_attack(board->squares, sq, color) >= 0;
}

int internal__diag_attack(piece_t squares[64], square_t sq, color_t color)
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

            piece_t piece = squares[next];
            if (piece_color(piece) == color)
            {
                if (piece_type(piece) == BISHOP || piece_type(piece) == QUEEN)
                    return next;
                break; // ally piece blocks
            }
            if (piece_type(piece) != NO_PIECE)
                break; // enemy piece blocks

            curr = next;
            curr_file = next_file;
        }
    }
    return -1;
}

bool can_diag_attack(board_t* board, square_t sq, color_t color)
{
    return internal__diag_attack(board->squares, sq, color) >= 0;
}

int internal__orth_attack(piece_t squares[64], square_t sq, color_t color)
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

            piece_t piece = squares[next];
            if (piece_color(piece) == color)
            {
                if (piece_type(piece) == ROOK || piece_type(piece) == QUEEN)
                    return next;
                break; // ally piece blocks
            }
            if (piece_type(piece) != NO_PIECE)
                break; // enemy piece blocks

            curr = next;
            curr_file = next_file;
        }
    }
    return -1;
}

bool can_orth_attack(board_t* board, square_t sq, color_t color)
{
    return internal__orth_attack(board->squares, sq, color) >= 0;
}

int find_lva(piece_t squares[64], int to, color_t side)
{
    int lva = -1;
    square_t sq = idx_to_square(to);

    if ((lva = internal__pawn_attack(squares, sq, side)) >= 0)
        return lva;
    else if ((lva = internal__knight_attack(squares, sq, side)) >= 0)
        return lva;
   
    int potential_bishop = internal__diag_attack(squares, sq, side);
    if (potential_bishop >= 0 && piece_type(squares[potential_bishop]) == BISHOP)
        return potential_bishop;

    int potential_rook = internal__orth_attack(squares, sq, side);
    if (potential_rook >= 0 && piece_type(squares[potential_rook]) == ROOK)
        return potential_rook;

    if (potential_bishop >= 0 && piece_type(squares[potential_bishop]) == QUEEN)
        return potential_bishop;

    if (potential_rook >= 0 && piece_type(squares[potential_rook]) == QUEEN)
        return potential_rook;

    if ((lva = internal__king_attack(squares, sq, side)) >= 0)
        return lva;

    return -1;
}


int see(board_t* board, move_t move)
{
    piece_t squares[64];
    memcpy(squares, board->squares, sizeof(squares));

    int to = MOVE_TO(move);
    int from = MOVE_FROM(move);
    color_t side = board->turn;

    int gain[32];
    int d = 0;

    // get value of captured piece
    gain[0] = piece_value(piece_type(squares[to]));
    // play first capture
    squares[to] = squares[from];
    squares[from] = NO_PIECE;
    side = side == WHITE ? BLACK : WHITE;

    while (true)
    {
        int att = find_lva(squares, to, side);
        if (att <= 0) break;
        // get piece capture relative to the previouis captured piece
        d++;
        gain[d] = piece_value(piece_type(squares[to])) - gain[d - 1];
        // play capture
        squares[to] = squares[att];
        squares[att] = NO_PIECE;
        side = side == WHITE ? BLACK : WHITE;
    }

    while (--d >= 0)
        gain[d] = -MAX(-gain[d], gain[d + 1]);

    return gain[0];
}
