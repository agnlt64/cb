#include <stdlib.h>

#include "zobrist.h"
#include "mt19937-64.h"
#include "piece.h"

void zobrist_init(zobrist_t* z)
{
    init_genrand64(8112007);
    for (size_t sq = 0; sq < 64; sq++)
    {
        // todo: 6 is a magic number, unmagic it
        // it's because there are 12 entries in ALL_PIECES
        for (size_t piece_idx = 0; piece_idx < 12; piece_idx++)
        {
            piece_type_t pt = ALL_PIECES[piece_idx];
            z->pieces[pt][sq] = genrand64_int64();
        }
    }

    // 16 entries in z->castling
    for (size_t i = 0; i < 16; i++)
    {
        z->castling[i] = genrand64_int64();
    }

    // 9 entries in z->castling
    for (size_t i = 0; i < 9; i++)
    {
        z->en_passant[i] = genrand64_int64();
    }

    z->turn = genrand64_int64();
}
