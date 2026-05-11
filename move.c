#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "move.h"

void move_print(move_t move)
{
    char* str = move_to_uci(move);
    printf("%s\n", str);
    free(str);
}

char* move_to_uci(move_t move)
{
    // todo: support promotion
    char res[5];
    int idx_from = MOVE_FROM(move);
    int idx_to = MOVE_TO(move);

    square_t sq_from = idx_to_square(idx_from);
    square_t sq_to = idx_to_square(idx_to);
    res[0] = sq_from.file + 'a';
    res[1] = sq_from.rank + '1';
    res[2] = sq_to.file + 'a';
    res[3] = sq_to.rank + '1';
    res[4] = '\0';

    return strdup(res);
}