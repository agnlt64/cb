#include <math.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "search.h"
#include "board.h"
#include "evaluation.h"
#include "movegen.h"
#include "attacks.h"
#include "precomputed_eval_data.h"

#define MAX_HISTORY 16384

void search_ctx_init(search_ctx_t* ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->tt = malloc(TT_SIZE*  sizeof(tt_entry_t));
}

void search_ctx_destroy(search_ctx_t* ctx)
{
    free(ctx->tt);
    ctx->tt = NULL;
}

long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec*  1000 + ts.tv_nsec / 1000000;
}

void check_time(search_ctx_t* ctx)
{
    if (ctx->search_end_ms != 0 && now_ms() >= ctx->search_end_ms)
        ctx->canceled = true;
}

int quiescence_search(search_ctx_t* ctx, int alpha, int beta)
{
    ctx->total_positions++;
    int eval = evaluate(&ctx->board);
    if (eval >= beta)
        return beta;

    if (eval + piece_value(QUEEN) < alpha)
        return alpha;

    alpha = (int)fmax(alpha, eval);

    move_t capture_moves[256];
    int n = gen_capture_moves(&ctx->board, capture_moves);
    order_moves(&ctx->board, capture_moves, n, ctx->killers, -1, false, 0, NULL);

    for (size_t i = 0; i < n; i++)
    {
        if (ctx->canceled)
            return alpha;
        move_t move = capture_moves[i];
        if (see(&ctx->board, move) < 0)
            continue; // skip losing captures

        board_make_move(&ctx->board, move);
        eval = -quiescence_search(ctx, -beta, -alpha);
        board_unmake_move(&ctx->board, move);

        if (ctx->canceled)
            return alpha;

        if (eval >= beta)
            return beta;

        alpha = (int)fmax(alpha, eval);
    }

    return alpha;
}

bool is_repetition(board_t* board)
{
    int limit = board->history_top - board->halfmove;
    if (limit < 0)
        limit = 0;
    for (int i = board->history_top - 2; i >= limit; i -= 2)
    {
        if (board->history[i].hash == board->hash)
            return true;
    }
    return false;
}

int get_extension(board_t* board, move_t move)
{
    int extension = 0;
    int to = MOVE_TO(move);
    piece_t p = board->squares[to];

    if (board_in_check(board))
        extension = 1;

    return extension;
}

int negamax(search_ctx_t* ctx, int depth, int alpha, int beta, int ply)
{
    ctx->total_positions++;

    if ((ctx->total_positions & 2047) == 0)
        check_time(ctx);

    // important: always keep this at the top of the function
    move_t moves[256];
    int n = gen_legal_moves(&ctx->board, moves, 0);

    if (n == 0)
    {
        int king_sq = FIND_KING(&ctx->board, ctx->board.turn);
        return is_square_attacked(&ctx->board, idx_to_square(king_sq), ctx->board.turn == WHITE ? BLACK : WHITE) ? -MATE_SCORE + ply : 0;
    }

    // 50-move rule and threefold repetition (one fold here)
    if (ctx->board.halfmove >= 100 || is_repetition(&ctx->board))
        return 0;

    int original_alpha = alpha;
    move_t best_move = 0;

    tt_entry_t* entry = tt_get(ctx->tt, ctx->board.hash);
    move_t tt_move = 0;
    if (entry)
    {
        tt_move = entry->best_move;

        if (entry->depth >= depth)
        {
            int e = entry->eval;
            if (e >= MATE_SCORE - 1000)
                e -= ply;
            if (e <= -MATE_SCORE + 1000)
                e += ply;
    
            if (entry->flag == TT_EXACT)
                return e;
            if (entry->flag == TT_LOWER && e >= beta)
                return e;
            if (entry->flag == TT_UPPER && e <= alpha)
                return e;
        }
    }

    // internal iterative reduction
    if (tt_move == 0 && depth >= 4)
        depth--;

    if (depth == 0)
        return quiescence_search(ctx, alpha, beta);

    // reverse futility pruning
    if (depth <= 6 && !board_in_check(&ctx->board))
    {
        int static_eval = evaluate(&ctx->board);
        if (static_eval - 150*  depth >= beta)
            return static_eval;
    }

    order_moves(&ctx->board, moves, n, ctx->killers, depth, false, tt_move, ctx->history);

    // null move pruning
    if (depth >= 3 && !board_in_check(&ctx->board))
    {
        int saved_ep = ctx->board.ep_square_idx;
        if (saved_ep != -1)
            ctx->board.hash ^= ctx->board.zobrist.en_passant[saved_ep % 8];
        ctx->board.ep_square_idx = -1;

        board_flip_turn(&ctx->board);
        ctx->board.hash ^= ctx->board.zobrist.turn;

        int eval = -negamax(ctx, depth - 3, -beta, -beta + 1, ply + 1);

        // ALWAYS restore board state, even on cancellation. otherwise the
        // caller's board_unmake_move operates on a corrupted board (wrong
        // turn, stale hash, missing ep), which silently corrupts every
        // unmake up the stack and triggers HASH DRIFT at the top.
        board_flip_turn(&ctx->board);
        ctx->board.hash ^= ctx->board.zobrist.turn;

        ctx->board.ep_square_idx = saved_ep;
        if (saved_ep != -1)
            ctx->board.hash ^= ctx->board.zobrist.en_passant[saved_ep % 8];

        if (ctx->canceled) return alpha;

        if (eval >= beta)
            return beta;
    }

    for (size_t i = 0; i < n; i++)
    {
        if (ctx->canceled) return alpha;

        move_t move = moves[i];

        // forward futility pruning
        if (depth == 1 && !board_in_check(&ctx->board) && i > 0)
        {
            int static_eval = evaluate(&ctx->board);
            // if position with one more pawn is less than alpha,
            // it's not worth continuing
            if (static_eval + 150 < alpha)
                continue;
        }

        board_make_move(&ctx->board, move);

        int eval;
        int extension = get_extension(&ctx->board, move);
        int new_depth = depth - 1 + extension;

        // late move reduction
        bool can_reduce = extension == 0 && MOVE_FLAGS(move) != FLAG_CAPTURE && MOVE_FLAGS(move) != FLAG_EP;
        int reduction = can_reduce ? (int)(log(depth) * log(i + 1) / 2.0) : 0;

        if (i == 0)
        {
            // PVS
            eval = -negamax(ctx, new_depth, -beta, -alpha, ply + 1);
        }
        else
        {
            // null window
            eval = -negamax(ctx, new_depth - reduction, -alpha - 1, -alpha, ply + 1);

            // null window again to confirm fail-high
            if (reduction > 0 && eval > alpha)
                eval = -negamax(ctx, new_depth, -alpha - 1, -alpha, ply + 1);

            // if null window fails in a PV windos, full search
            if (eval > alpha && eval < beta)
                eval = -negamax(ctx, new_depth, -beta, -alpha, ply + 1);
        }

        board_unmake_move(&ctx->board, move);

        if (ctx->canceled) return alpha;

        if (eval >= beta)
        {
            // killer move detection
            if (depth < MAX_DEPTH && MOVE_FLAGS(move) != FLAG_CAPTURE && MOVE_FLAGS(move) != FLAG_EP)
                add_killer(ctx->killers, depth, move);

            // history heuristics
            if (MOVE_FLAGS(move) != FLAG_CAPTURE)
            {
                int bonus = depth*  depth;
                int clamped_bonus = CLAMP(bonus, -MAX_HISTORY, MAX_HISTORY);
                int from = MOVE_FROM(move);
                int to = MOVE_TO(move);
                ctx->history[from][to] += clamped_bonus - ctx->history[from][to]*  abs(clamped_bonus) / MAX_HISTORY;
            }

            int stored = beta;
            if (stored >= MATE_SCORE - 1000)  stored += ply;
            if (stored <= -MATE_SCORE + 1000) stored -= ply;
            tt_set(ctx->tt, ctx->board.hash, depth, stored, TT_LOWER, move);
            return beta;
        }
        if (eval > alpha)
        {
            alpha = eval;
            best_move = move;
        }
    }

    int stored = alpha;
    if (stored >= MATE_SCORE - 1000)
        stored += ply;
    if (stored <= -MATE_SCORE + 1000)
        stored -= ply;
    
    tt_flag_t flag = alpha > original_alpha ? TT_EXACT : TT_UPPER;
    tt_set(ctx->tt, ctx->board.hash, depth, stored, flag, best_move);
    
    return alpha;
}

// root_search handles ONLY the root node. It:
//   - never cuts off via TT or null move (we must produce an actual best move)
//   - calls negamax for child nodes (never recurses on itself)
//   - always writes* best_move_out before returning, so the caller never reads
//     an uninitialized move or "a1a1" (move_t = 0).
int root_search(search_ctx_t* ctx, int depth, int alpha, int beta, move_t* best_move_out)
{
    ctx->total_positions++;

    if ((ctx->total_positions & 2047) == 0)
        check_time(ctx);

    move_t moves[256];
    int n = gen_legal_moves(&ctx->board, moves, 0);

    if (n == 0)
    {
        *best_move_out = 0;
        int king_sq = FIND_KING(&ctx->board, ctx->board.turn);
        return is_square_attacked(&ctx->board, idx_to_square(king_sq), ctx->board.turn == WHITE ? BLACK : WHITE) ? -MATE_SCORE : 0;
    }

    // legal fallback so we never return move_t = 0 ("a1a1")
    *best_move_out = moves[0];

    // TT lookup at root: ordering ONLY, never cut off
    tt_entry_t* entry = tt_get(ctx->tt, ctx->board.hash);
    move_t tt_move = entry ? entry->best_move : 0;

    order_moves(&ctx->board, moves, n, ctx->killers, depth, false, tt_move, ctx->history);

    int original_alpha = alpha;
    move_t best_move = moves[0];
    int best_eval = -200000;

    for (size_t i = 0; i < n; i++)
    {
        move_t move = moves[i];
        board_make_move(&ctx->board, move);

        int extension = get_extension(&ctx->board, move);
        int new_depth = depth - 1 + extension;

        bool can_reduce = extension == 0 && MOVE_FLAGS(move) != FLAG_CAPTURE && MOVE_FLAGS(move) != FLAG_EP;
        int reduction = can_reduce ? (int)(log(depth) * log(i + 1) / 2.0) : 0;

        int eval;
        if (i == 0)
        {
            eval = -negamax(ctx, new_depth, -beta, -alpha, 1);
        }
        else
        {
            eval = -negamax(ctx, new_depth - reduction, -alpha - 1, -alpha, 1);
            if (reduction > 0 && eval > alpha)
                eval = -negamax(ctx, new_depth, -alpha - 1, -alpha, 1);
            if (eval > alpha && eval < beta)
                eval = -negamax(ctx, new_depth, -beta, -alpha, 1);
        }

        board_unmake_move(&ctx->board, move);

        if (ctx->canceled)
        {
            // partial ctx->canceled result: keep best move found so far (legal
            // fallback to moves[0] if nothing improved yet).
            *best_move_out = best_eval > -200000 ? best_move : moves[0];
            return alpha;
        }

        // track best move independently of alpha so we always have a real
        // answer even if every move fails low against the aspiration window
        if (eval > best_eval)
        {
            best_eval = eval;
            best_move = move;
        }

        if (eval > alpha)
            alpha = eval;

        if (alpha >= beta)
        {
            // fail-high at root: aspiration window too narrow.
            // store as lower bound and let iterative_deepening widen + re-search.
            *best_move_out = best_move;
            tt_set(ctx->tt, ctx->board.hash, depth, beta, TT_LOWER, best_move);
            return beta;
        }
    }

    *best_move_out = best_move;

    tt_flag_t flag = alpha > original_alpha ? TT_EXACT : TT_UPPER;
    tt_set(ctx->tt, ctx->board.hash, depth, alpha, flag, best_move);

#ifdef UCI_DEBUG
    fprintf(stderr, "depth=%d  bestmove=%s  eval=%d\n", depth, move_to_uci(best_move), alpha);
#endif

    return alpha;
}

// thin wrapper kept for `go depth N` UCI command. delegates to root_search.
move_t search(search_ctx_t* ctx, int depth)
{
    move_t best;
    root_search(ctx, depth, -200000, 200000, &best);
    return best;
}

move_t iterative_deepening(search_ctx_t* ctx)
{
    move_t best = 0;
    int pre_eval = 0;

    for (int depth = 1; depth < 256; depth++)
    {
        // aspiration window centered on previous iteration's eval
        int alpha = depth > 4 ? pre_eval - 50 : -200000;
        int beta  = depth > 4 ? pre_eval + 50 :  200000;
        int widen = 200;

        int eval = 0;
        move_t best_candidate = 0;
        int retries = 0;

        while (true)
        {
            eval = root_search(ctx, depth, alpha, beta, &best_candidate);
            if (ctx->canceled) break;

            if (eval <= alpha)
            {
                // fail-low: widen DOWN
                alpha -= widen;
                if (alpha < -200000) alpha = -200000;
            }
            else if (eval >= beta)
            {
                // fail-high: widen UP
                beta += widen;
                if (beta > 200000) beta = 200000;
            }
            else
            {
                break; // eval inside [alpha, beta]: trust it
            }

            widen *= 2;
            retries++;

            // safety net: after 3 failures, escalate to full window
            if (retries >= 3)
            {
                alpha = -200000;
                beta  =  200000;
            }
        }

        if (ctx->canceled)
        {
            // partial result of an interrupted depth is unreliable.
            // only fall back on it if we have nothing from a previous depth.
            if (best == 0)
                best = best_candidate;
            break;
        }

        best = best_candidate;
        pre_eval = eval;
    }
    return best;
}
