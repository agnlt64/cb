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

static const piece_type_t ALL_PIECES[12] = {
    KING | WHITE,
    PAWN | WHITE,
    KNIGHT | WHITE,
    BISHOP | WHITE,
    ROOK | WHITE,
    QUEEN | WHITE,
    KING | BLACK,
    PAWN | BLACK,
    KNIGHT | BLACK,
    BISHOP | BLACK,
    ROOK | BLACK,
    QUEEN | BLACK,
};

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

static const int PIECE_VALUES[7] = {
    [NO_PIECE] = 0,
    [KING] = 0,
    [PAWN] = 100,
    [KNIGHT] = 300,
    [BISHOP] = 320,
    [ROOK] = 500,
    [QUEEN] = 900
};

typedef int piece_t;

piece_type_t piece_type(piece_t piece);
color_t piece_color(piece_t piece);
char piece_string(piece_t piece);
int piece_value(piece_t piece);
int piece_square_value(piece_t piece, int sq);