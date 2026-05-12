#pragma once

#include <stdint.h>

typedef struct zobrist {
    uint64_t pieces[32][64];
    uint64_t castling[16];
    uint64_t en_passant[9];
    uint64_t turn;
} zobrist_t;

void zobrist_init(zobrist_t* z);