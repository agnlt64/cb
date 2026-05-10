#pragma once

typedef enum color {
    WHITE = 8,
    BLACK = 16,
    NO_COLOR = 32
} color_t;

typedef enum piece_type {
    NO_PIECE = 0,
    KING,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
} piece_type_t;

static const char PIECE_CHARS[] = {'.', 'k', 'p', 'n', 'b', 'r', 'q'};
static const int CHAR_TO_PIECE[128] = {
    ['.'] = NO_PIECE,
    ['k'] = KING,
    ['p'] = PAWN,
    ['n'] = KNIGHT,
    ['b'] = BISHOP,
    ['r'] = ROOK,
    ['q'] = QUEEN
};

typedef int piece_t;

piece_type_t piece_type(piece_t piece);
color_t piece_color(piece_t piece);
char piece_string(piece_t piece);