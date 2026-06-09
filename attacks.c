#include <stdlib.h>

#include "attacks.h"
#include "common.h"

bool is_square_attacked(board_t *board, square_t sq, color_t color)
{
    return (
        can_knight_attack(board, sq, color) ||
        can_orth_attack(board, sq, color) ||
        can_diag_attack(board, sq, color) ||
        can_pawn_attack(board, sq, color) ||
        can_king_attack(board, sq, color));
}

bool can_knight_attack(board_t *board, square_t sq, color_t color)
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

bool can_king_attack(board_t *board, square_t sq, color_t color)
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

bool can_pawn_attack(board_t *board, square_t sq, color_t color)
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

bool can_diag_attack(board_t *board, square_t sq, color_t color)
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
            if (piece_type(piece) != NO_PIECE)
                break; // enemy piece blocks

            curr = next;
            curr_file = next_file;
        }
    }
    return false;
}

bool can_orth_attack(board_t *board, square_t sq, color_t color)
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
            if (piece_type(piece) != NO_PIECE)
                break; // enemy piece blocks

            curr = next;
            curr_file = next_file;
        }
    }
    return false;
}
