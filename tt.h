#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "move.h"

#define TT_SIZE (1<<20)

typedef enum {
    TT_EXACT = 0,
    TT_LOWER = 1,
    TT_UPPER = 2,
} tt_flag_t;

typedef struct tt_entry {
    uint64_t hash;
    int depth;
    int eval;
    move_t best_move;
    tt_flag_t flag;
} tt_entry_t;

tt_entry_t* tt_get(tt_entry_t* tt, uint64_t hash);
void tt_set(tt_entry_t* tt, uint64_t hash, int depth, int eval, tt_flag_t flag, move_t move);