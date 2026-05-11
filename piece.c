
#include <ctype.h>

#include "piece.h"

// piece = 2 bits for color and 3 bits for type
piece_type_t piece_type(piece_t piece)
{
    return piece & 0b111;
}

color_t piece_color(piece_t piece)
{
    return piece & 0b11000;
}

char piece_string(piece_t piece)
{
    int type = piece_type(piece);
    int color = piece_color(piece);
    char c = PIECE_CHARS[type];
    return color == WHITE ? toupper(c) : c;
}

int piece_value(piece_t piece)
{
    return PIECE_VALUES[piece_type(piece)];
}