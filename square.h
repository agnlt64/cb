#pragma once

#include <stdbool.h>

typedef struct square {
    // 1, 2, 3, etc
    int rank;
    // A, B, C, etc
    int file;
} square_t;

int square_to_idx(square_t sq);
square_t idx_to_square(int idx);
bool squares_eq(square_t sq1, square_t sq2);