#include "tt.h"

tt_entry_t* tt_get(uint64_t hash)
{
    tt_entry_t* entry = &tt[hash % TT_SIZE];
    return (entry->hash == hash) ? entry : NULL;
}

void tt_set(uint64_t hash, int depth, int eval)
{
    tt_entry_t* slot = &tt[hash % TT_SIZE];
    if (slot->depth <= depth)
    {
        slot->hash = hash;
        slot->depth = depth;
        slot->eval = eval;
    }
}