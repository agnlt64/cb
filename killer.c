#include "killer.h"

void add_killer(killer_t* killers, int depth, move_t move)
{
    if (move != killers[depth].a)
    {
        killers[depth].b = killers[depth].a;
        killers[depth].a = move;
    }
}

bool killer_contains(killer_t* killers, int depth, move_t move)
{
    return move == killers[depth].a || move == killers[depth].b;
}