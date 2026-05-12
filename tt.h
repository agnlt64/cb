#pragma once

#include <stdint.h>
#include <stdlib.h>

#define TT_SIZE 1<<24

typedef struct tt_entry {
    uint64_t hash;
    int depth;
    int eval;
} tt_entry_t;

tt_entry_t tt[TT_SIZE];

tt_entry_t* tt_get(uint64_t hash);
void tt_set(uint64_t hash, int depth, int eval);