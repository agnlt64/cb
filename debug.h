#pragma once

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "board.h"

void dbg_init(void);
void dbg_dump_board(board_t* b);
void dbg_log(const char* fmt, ...);

#ifdef UCI_DEBUG
#define DBG(...) dbg_log(__VA_ARGS__)
#else
#define DBG(...) \
    do           \
    {            \
    } while (0)
#endif