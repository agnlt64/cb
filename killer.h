#pragma once

#include "move.h"

typedef struct killer
{
    move_t a;
    move_t b;
} killer_t;

void add_killer(killer_t* killers, int depth, move_t move);
bool killer_contains(killer_t* killers, int depth, move_t move);