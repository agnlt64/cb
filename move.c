
#include <stdio.h>

#include "move.h"

void move_print(move_t move)
{
    int idx_from = MOVE_FROM(move);
    int idx_to = MOVE_TO(move);

    square_t sq_from = idx_to_square(idx_from);
    square_t sq_to = idx_to_square(idx_to);

    printf("%c%c%c%c\n", sq_from.file + 'a', sq_from.rank + '1', sq_to.file + 'a', sq_to.rank + '1');
}