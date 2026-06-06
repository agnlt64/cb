#pragma once

#include <stdbool.h>

#include "board.h"
#include "tt.h"

#define MATE_SCORE 100000

typedef struct search_ctx {
    board_t board;
    tt_entry_t* tt;
    killer_t killers[MAX_DEPTH];
    int history[64][64];
    
    int total_positions;

    bool canceled;
    // wall-clock deadline in milliseconds since an arbitrary monotonic epoch.
    // MUST be wall-clock, not CPU time — UCI time controls are wall-clock and
    // concurrent matches share the CPU. clock() (CPU time) drifts behind wall
    // time by 10-30% under concurrency=4 and causes systematic timeouts.
    long long search_end_ms;
} search_ctx_t;

void search_ctx_init(search_ctx_t* ctx);
void search_ctx_destroy(search_ctx_t* ctx);

bool is_repetition(board_t* board);
int get_extension(board_t* board, move_t move);

move_t search(search_ctx_t* ctx, int depth);
int quiescence_search(search_ctx_t* ctx, int alpha, int beta);
int root_search(search_ctx_t* ctx, int depth, int alpha, int beta, move_t* best_move_out);
move_t iterative_deepening(search_ctx_t* ctx);
int negamax(search_ctx_t* ctx, int depth, int alpha, int beta, int ply);

long long now_ms();
void check_time(search_ctx_t* ctx);