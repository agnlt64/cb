#include <stdio.h>
#include <stdarg.h>

#include "debug.h"

static FILE* dbg_fp = NULL;

void dbg_init(void)
{
#ifdef UCI_DEBUG
    if (dbg_fp)
        return;
    char path[128];
    snprintf(path, sizeof(path), "/tmp/cb-dbg-%d.log", (int)getpid());
    dbg_fp = fopen(path, "w");
    if (dbg_fp)
    {
        setvbuf(dbg_fp, NULL, _IONBF, 0); // unbuffered: survive abort()
        fprintf(dbg_fp, "=== cb debug log, pid=%d ===\n", (int)getpid());
    }
#endif
}

void dbg_log(const char* fmt, ...)
{
#ifdef UCI_DEBUG
    dbg_init();
    if (!dbg_fp)
        return;
    va_list args;
    va_start(args, fmt);
    vfprintf(dbg_fp, fmt, args);
    va_end(args);
#else
    (void)fmt;
#endif
}

void dbg_dump_board(board_t* b)
{
#ifdef UCI_DEBUG
    dbg_init();
    if (!dbg_fp)
        return;
    for (int rank = 7; rank >= 0; rank--)
    {
        for (int file = 0; file < 8; file++)
            fprintf(dbg_fp, "%c ", piece_string(b->squares[rank*  8 + file]));
        fprintf(dbg_fp, "\n");
    }
    fprintf(dbg_fp, "turn=%s castling=0x%x ep=%d halfmove=%d fullmove=%d hash=0x%llx\n",
            b->turn == WHITE ? "w" : "b", b->castling, b->ep_square_idx,
            b->halfmove, b->fullmove, (unsigned long long)b->hash);
#endif
}