#include <stdio.h>
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
    static char res[6];
    int idx_from = MOVE_FROM(move);
    int idx_to = MOVE_TO(move);

    square_t sq_from = idx_to_square(idx_from);
    square_t sq_to = idx_to_square(idx_to);
    res[0] = sq_from.file + 'a';
    res[1] = sq_from.rank + '1';
    res[2] = sq_to.file + 'a';
    res[3] = sq_to.rank + '1';
    res[4] = '\0';

    int flag = MOVE_FLAGS(move);
    if (flag == FLAG_PROMO_Q) { res[4] = 'q'; res[5] = 0; }
    if (flag == FLAG_PROMO_R) { res[4] = 'r'; res[5] = 0; }
    if (flag == FLAG_PROMO_B) { res[4] = 'b'; res[5] = 0; }
    if (flag == FLAG_PROMO_N) { res[4] = 'n'; res[5] = 0; }

    return res;
}