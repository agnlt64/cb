#pragma once

#define W_KSIDE 0b1000
#define W_QSIDE 0b0100
#define B_KSIDE 0b0010
#define B_QSIDE 0b0001

static const int knight_offsets[8] = {17, 15, 10, 6, -6, -10, -15, -17};
static const int knight_file_offsets[8] = {1, -1, 2, -2, 2, -2, 1, -1};

static const int king_offsets[8] = {1, -1, 8, -8, 9, -9, 7, -7};
static const int king_file_offsets[8] = {1, -1, 0, 0, 1, -1, -1, 1};

static const int diag_offsets[4] = {9, -9, 7, -7};
static const int diag_file_offsets[4] = {1, -1, -1, 1};

static const int orth_offsets[4] = {8, -8, 1, -1};
static const int orth_file_offsets[4] = {0, 0, 1, -1};