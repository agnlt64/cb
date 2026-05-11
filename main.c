#include <stdio.h>

#include "board.h"

#define PAWN_VAL 100
#define KNIGHT_VAL 300
#define BISHOP_VAL 300
#define ROOK_VAL 500
#define QUEEN_VAL 900
#define KING_VAL 10000

int count_material(board_t* board, color_t color)
{
    int total = 0;

    for (int i = 0; i < FILES * RANKS; i++)
    {
        piece_t p = board->squares[i];
        if (piece_color(p) != color) continue;

        switch (piece_type(p))
        {
            case PAWN: total += PAWN_VAL; break;
            case KNIGHT: total += KNIGHT_VAL; break;
            case BISHOP: total += BISHOP_VAL; break;
            case ROOK: total += ROOK_VAL; break;
            case QUEEN: total += QUEEN_VAL; break;
            // case KING: total += KING_VAL; break;
            default: break;
        }
    }

    return total;
}

int evaluate(board_t* board)
{
    int w_mat = count_material(board, WHITE);
    int b_mat = count_material(board, BLACK);

    return w_mat - b_mat;
}

int main()
{
    board_t board = {0};
    board_init(&board);
    printf("material for white = %d\n", count_material(&board, WHITE));
    printf("material for black = %d\n", count_material(&board, BLACK));
    printf("position evaluation = %d\n", evaluate(&board));
    
    return 0;
}