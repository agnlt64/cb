
#include <ctype.h>

#include "piece.h"
#include "pieces_tables.h"

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

// internal usage only
int flip(int sq)
{
    return (7 - sq / 8) * 8 + sq % 8;
}

int piece_square_value(piece_t piece, int sq, int material)
{
    int idx = piece_color(piece) == WHITE ? sq : flip(sq);

    switch (piece_type(piece))
    {
        case PAWN: return pawn_table[idx];
        case KNIGHT: return knight_table[idx];
        case BISHOP: return bishop_table[idx];
        case ROOK: return rook_table[idx];
        case QUEEN: return queen_table[idx];
        case KING: return material <= 3000 ? king_endgame_table[idx] : king_midgame_table[idx];
        default: return 0;
    }
}