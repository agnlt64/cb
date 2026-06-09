#include "precomputed_eval_data.h"
#include "square.h"

// https://github.com/SebLague/Chess-Coding-Adventure/blob/Chess-V2-UCI/Chess-Coding-Adventure/src/Core/Evaluation/PrecomputedEvaluationData.cs

pawn_shield_t pawn_shield_squares_white[64];
pawn_shield_t pawn_shield_squares_black[64];

void init_pawn_shields()
{
    for (int sq = 0; sq < 64; sq++)
    {
        int rank = sq / 8;
        int file = sq % 8;
        int clamped_file = CLAMP(file, 1, 6);
        int wc = 0, bc = 0;

        // FRONT row (rank+1 for white, rank-1 for black)
        for (int file_off = -1; file_off <= 1; file_off++)
        {
            int f = file_off + clamped_file;

            int wr = rank + 1;
            if (wr >= 0 && wr < 8)
                pawn_shield_squares_white[sq].squares[wc++] = wr*  8 + f;

            int br = rank - 1;
            if (br >= 0 && br < 8)
                pawn_shield_squares_black[sq].squares[bc++] = br*  8 + f;
        }

        // BACK row (rank+2 for white, rank-2 for black) — fallback
        for (int file_off = -1; file_off <= 1; file_off++)
        {
            int f = file_off + clamped_file;

            int wr = rank + 2;
            if (wr >= 0 && wr < 8)
                pawn_shield_squares_white[sq].squares[wc++] = wr*  8 + f;

            int br = rank - 2;
            if (br >= 0 && br < 8)
                pawn_shield_squares_black[sq].squares[bc++] = br*  8 + f;
        }

        pawn_shield_squares_white[sq].length = wc;
        pawn_shield_squares_black[sq].length = bc;
    }
}