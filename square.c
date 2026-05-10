#include <math.h>

#include "square.h"

int square_to_idx(square_t sq)
{
    return sq.rank * 8 + sq.file;
}

square_t idx_to_square(int idx)
{
    int rank = (int)round(idx / 8);
    int file = idx % 8;
    return (square_t){
        .file = file,
        .rank = rank,
    };
}

bool squares_eq(square_t sq1, square_t sq2)
{
    return (
        sq1.file == sq2.file &&
        sq1.rank == sq2.rank
    );
}