#include "pieces_tables.h"

int flip_square_idx(int sq)
{
    return (7 - sq / 8) * 8 + sq % 8;
}