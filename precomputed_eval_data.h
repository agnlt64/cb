#pragma once

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)
#define CLAMP(x, low, up) (MIN(up, MAX(x, low)))

typedef struct pawn_shield {
    int squares[6];
    int length;
} pawn_shield_t;

extern pawn_shield_t pawn_shield_squares_white[64];
extern pawn_shield_t pawn_shield_squares_black[64];

void init_pawn_shields();