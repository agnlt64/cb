#pragma once

#include <stdint.h>
#include <stdlib.h>

#define TT_SIZE (1<<20)

typedef struct tt_entry {
    uint64_t hash;
    int depth;
    int eval;
} tt_entry_t;

tt_entry_t* tt_get(tt_entry_t* tt, uint64_t hash);
void tt_set(tt_entry_t* tt, uint64_t hash, int depth, int eval);