#include "precomputed_move_data.h"
#include "precomputed_eval_data.h"
#include "uci.h"

int main()
{
    init_distances();
    init_pawn_shields();
    uci_loop();

    return 0;
}