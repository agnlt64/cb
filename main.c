#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef UCI_DEBUG
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#include "board.h"
#include "tt.h"
#include "squares.h"
#include "evaluation.h"
#include "precomputed_move_data.h"

#define MAX_DEPTH 20
#define MATE_SCORE 100000

// ===== Debug log file (per-process) =====
// Active uniquement en build UCI_DEBUG. cutechess avale le stderr des
// moteurs, donc on écrit dans /tmp/cb-dbg-<pid>.log. Après un match en
// debug, `grep -l "BUG\|HASH\|PARSE FAIL" /tmp/cb-dbg-*.log` localise
// les crashes.
#ifdef UCI_DEBUG
static FILE *dbg_fp = NULL;

static void dbg_init(void)
{
    if (dbg_fp) return;
    char path[128];
    snprintf(path, sizeof(path), "/tmp/cb-dbg-%d.log", (int)getpid());
    dbg_fp = fopen(path, "w");
    if (dbg_fp)
    {
        setvbuf(dbg_fp, NULL, _IONBF, 0);  // unbuffered: survive abort()
        fprintf(dbg_fp, "=== cb debug log, pid=%d ===\n", (int)getpid());
    }
}

#define DBG(...) do { dbg_init(); if (dbg_fp) fprintf(dbg_fp, __VA_ARGS__); } while(0)

static void dbg_dump_board(board_t *b)
{
    dbg_init();
    if (!dbg_fp) return;
    for (int rank = 7; rank >= 0; rank--)
    {
        for (int file = 0; file < 8; file++)
            fprintf(dbg_fp, "%c ", piece_string(b->squares[rank * 8 + file]));
        fprintf(dbg_fp, "\n");
    }
    fprintf(dbg_fp, "turn=%s castling=0x%x ep=%d halfmove=%d fullmove=%d hash=0x%llx\n",
            b->turn == WHITE ? "w" : "b", b->castling, b->ep_square_idx,
            b->halfmove, b->fullmove, (unsigned long long)b->hash);
}
#else
#define DBG(...) do { } while (0)
#define dbg_dump_board(b) do { (void)(b); } while (0)
#endif

int total_positions = 0;
tt_entry_t tt[TT_SIZE];
// volatile to make sure they don't get
// optimized out by -O3
volatile bool canceled = false;
static volatile clock_t search_end;

typedef struct killer
{
    move_t a;
    move_t b;
} killer_t;

killer_t killers[MAX_DEPTH] = {0};

void add_killer(int depth, move_t move)
{
    if (move != killers[depth].a)
    {
        killers[depth].b = killers[depth].a;
        killers[depth].a = move;
    }
}

bool killer_contains(int depth, move_t move)
{
    return move == killers[depth].a || move == killers[depth].b;
}

void order_moves(board_t *board, move_t *moves, int n, int depth)
{
    color_t opp = board->turn == WHITE ? BLACK : WHITE;
    int scores[256];

    for (int i = 0; i < n; i++)
    {
        move_t move = moves[i];
        int score_guess = 0;
        piece_type_t move_piece_type = piece_type(board->squares[MOVE_FROM(move)]);
        piece_type_t capture_piece_type = MOVE_CAPTURED(move);

        if (capture_piece_type != NO_PIECE)
        {
            score_guess = 10 * piece_value(capture_piece_type) - piece_value(move_piece_type);
        }
        else
        {
            // detect killer move
            score_guess += (depth >= 0 && depth < MAX_DEPTH && killer_contains(depth, move)) ? 4000000 : 0;
        }

        switch (MOVE_FLAGS(move))
        {
        case FLAG_PROMO_B:
            score_guess += piece_value(BISHOP);
            break;
        case FLAG_PROMO_N:
            score_guess += piece_value(KNIGHT);
            break;
        case FLAG_PROMO_R:
            score_guess += piece_value(ROOK);
            break;
        case FLAG_PROMO_Q:
            score_guess += piece_value(QUEEN);
            break;
        default:
            break;
        }

        if (can_pawn_attack(board, idx_to_square(MOVE_TO(move)), opp))
            score_guess -= piece_value(move_piece_type);

        scores[i] = score_guess;
    }

    for (int i = 1; i < n; i++)
    {
        move_t m = moves[i];
        int s = scores[i];
        int j = i - 1;
        while (j >= 0 && scores[j] < s)
        {
            moves[j + 1] = moves[j];
            scores[j + 1] = scores[j];
            j--;
        }
        moves[j + 1] = m;
        scores[j + 1] = s;
    }
}

int search_captures(board_t *board, int alpha, int beta)
{
    total_positions++;
    int eval = evaluate(board);
    if (eval >= beta)
        return beta;

    if (eval + piece_value(QUEEN) < alpha)
        return alpha;

    alpha = (int)fmax(alpha, eval);

    move_t capture_moves[256];
    int n = gen_capture_moves(board, capture_moves);
    order_moves(board, capture_moves, n, -1);

    for (size_t i = 0; i < n; i++)
    {
        if (canceled)
            return alpha;
        move_t move = capture_moves[i];
        board_make_move(board, move);
        eval = -search_captures(board, -beta, -alpha);
        board_unmake_move(board, move);

        if (eval >= beta)
            return beta;

        alpha = (int)fmax(alpha, eval);
    }

    return alpha;
}

bool is_repetition(board_t *board)
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

static void check_time()
{
    if (search_end != 0 && clock() >= search_end)
        canceled = true;
}

int get_extension(board_t *board, move_t move)
{
    int extension = 0;
    int to = MOVE_TO(move);
    piece_t p = board->squares[to];

    if (board_in_check(board))
        extension = 1;

    return extension;
}

int negamax(board_t *board, int depth, int alpha, int beta, int ply)
{
    total_positions++;

    if ((total_positions & 2047) == 0)
        check_time();

    // important: always keep this at the top of the function
    move_t moves[256];
    int n = gen_legal_moves(board, moves, 0);

    if (n == 0)
    {
        int king_sq = FIND_KING(board, board->turn);
        return is_square_attacked(board, idx_to_square(king_sq), board->turn == WHITE ? BLACK : WHITE) ? -MATE_SCORE + ply : 0;
    }

    // 50-move rule and threefold repetition (one fold here)
    if (board->halfmove >= 100 || is_repetition(board))
        return 0;

    tt_entry_t *entry = tt_get(tt, board->hash);
    if (entry && entry->depth >= depth)
    {
        int e = entry->eval;
        if (e >= MATE_SCORE - 1000)
            e -= ply;
        if (e <= -MATE_SCORE + 1000)
            e += ply;
        return e;
    }

    if (depth == 0)
        return search_captures(board, alpha, beta);

    order_moves(board, moves, n, depth);

    // null move pruning
    if (depth >= 3 && !board_in_check(board))
    {
        int saved_ep = board->ep_square_idx;
        if (saved_ep != -1)
            board->hash ^= board->zobrist.en_passant[saved_ep % 8];
        board->ep_square_idx = -1;

        board_flip_turn(board);
        board->hash ^= board->zobrist.turn;

        int eval = -negamax(board, depth - 3, -beta, -beta + 1, ply + 1);

        board_flip_turn(board);
        board->hash ^= board->zobrist.turn;

        board->ep_square_idx = saved_ep;
        if (saved_ep != -1)
            board->hash ^= board->zobrist.en_passant[saved_ep % 8];

        if (eval >= beta)
            return beta;
    }

    for (size_t i = 0; i < n; i++)
    {
        if (canceled) return alpha;
        move_t move = moves[i];
        board_make_move(board, move);
        int extension = get_extension(board, move);

        // late move reduction
        int eval;
        bool is_late = depth >= 3 && i >= 3 && extension == 0 &&
                       MOVE_FLAGS(move) != FLAG_CAPTURE && MOVE_FLAGS(move) != FLAG_EP;

        if (is_late)
        {
            eval = -negamax(board, depth - 2, -alpha - 1, -alpha, ply + 1);
            if (eval > alpha)
                eval = -negamax(board, depth - 1, -beta, -alpha, ply + 1);
        }
        else
        {
            eval = -negamax(board, depth - 1 + extension, -beta, -alpha, ply + 1);
        }

        board_unmake_move(board, move);

        // killer move detection
        if (eval >= beta)
        {
            if (depth < MAX_DEPTH && MOVE_FLAGS(move) != FLAG_CAPTURE && MOVE_FLAGS(move) != FLAG_EP)
                add_killer(depth, move);
            return beta;
        }
        if (eval > alpha)
            alpha = eval;
    }

    int stored = alpha;
    if (stored >= MATE_SCORE - 1000)
        stored += ply;
    if (stored <= -MATE_SCORE + 1000)
        stored -= ply;
    tt_set(tt, board->hash, depth, stored);
    return alpha;
}

move_t search(board_t *board, int depth, move_t pre_best_move, int *out_eval)
{
#ifdef UCI_DEBUG
    printf("pre best move = %s\n", move_to_uci(pre_best_move));
#endif

    move_t moves[256];
    int n = gen_legal_moves(board, moves, pre_best_move);

    move_t best = moves[0];
    int best_eval = -200000;

    for (size_t i = 0; i < n; i++)
    {
        move_t move = moves[i];
        board_make_move(board, move);
        int eval = -negamax(board, depth - 1, -200000, 200000, 0);
        board_unmake_move(board, move);

#ifdef UCI_DEBUG
        fprintf(stderr, "    d=%d  %s  eval=%d\n", depth, move_to_uci(move), eval);
#endif

        if (eval > best_eval)
        {
            best_eval = eval;
            best = move;
        }
    }

#ifdef UCI_DEBUG
    fprintf(stderr, "depth %d -> best %s eval=%d\n", depth, move_to_uci(best), best_eval);
#endif

    *out_eval = best_eval;
    return best;
}

move_t iterative_deepening(board_t *board)
{
    move_t best = 0;
    for (int depth = 1; depth < 256; depth++)
    {
        int eval;
        move_t best_move_curr = search(board, depth, best, &eval);
        // always commit the latest move we got, even if we were canceled
        // mid-iteration — otherwise a cancel at depth=1 makes us return 0
        // which would print "bestmove a1a1" and lose on illegal move
        if (best_move_curr != 0)
            best = best_move_curr;
        if (canceled)
            break;
    }
    return best;
}

static move_t parse_uci_move(board_t *board, const char *s)
{
#ifdef UCI_DEBUG
    assert(board->hash == zobrist_from_board(board) && "hash drift detected");
#endif
    int from = (s[1] - '1') * 8 + (s[0] - 'a');
    int to = (s[3] - '1') * 8 + (s[2] - 'a');
    char promo = (s[4] && s[4] != ' ' && s[4] != '\n' && s[4] != '\r') ? s[4] : 0;

    // e1h1 (and friends) is valid UCI rook-target castle form. We
    // translate it to e1g1 (king-target form). But ONLY when the piece
    // on `from` is actually a king — otherwise a normal rook move from
    // E1 to H1 (king has already moved away earlier in the game) would
    // be wrongly reinterpreted as a castle attempt and resolved to a
    // different legal move (e8g8 in place of e8h8), causing a silent
    // desynchro with the GUI and an illegal move several plies later.
    piece_t mover = board->squares[from];
    if (piece_type(mover) == KING)
    {
        if (from == square_to_idx(E1) && to == square_to_idx(H1))
            to = square_to_idx(G1);
        if (from == square_to_idx(E1) && to == square_to_idx(A1))
            to = square_to_idx(C1);
        if (from == square_to_idx(E8) && to == square_to_idx(H8))
            to = square_to_idx(G8);
        if (from == square_to_idx(E8) && to == square_to_idx(A8))
            to = square_to_idx(C8);
    }

    move_t moves[256];
    int n = gen_legal_moves(board, moves, 0);
    for (int i = 0; i < n; i++)
    {
        move_t m = moves[i];

        if (MOVE_FROM(m) != from || MOVE_TO(m) != to)
            continue;

        int flag = MOVE_FLAGS(m);
        if (promo)
        {
            if (promo == 'q' && flag != FLAG_PROMO_Q)
                continue;
            if (promo == 'r' && flag != FLAG_PROMO_R)
                continue;
            if (promo == 'b' && flag != FLAG_PROMO_B)
                continue;
            if (promo == 'n' && flag != FLAG_PROMO_N)
                continue;
        }
        return m;
    }
    return 0;
}

void parse_position(board_t *board, const char *line)
{
    const char *p = line + 9; // skip "position "

    if (strncmp(p, "startpos", 8) == 0)
    {
        board_init(board);
        p += 8;
    }
    else if (strncmp(p, "fen", 3) == 0)
    {
        p += 4; // skip "fen "
        board_from_fen(board, p);
        int spaces = 0;
        while (*p && spaces < 6)
        {
            if (*p++ == ' ')
                spaces++;
        }
    }

#ifdef UCI_DEBUG
    // sanity: the freshly-built board must have a consistent hash
    {
        uint64_t fresh = zobrist_from_board(board);
        if (board->hash != fresh)
        {
            DBG("HASH MISMATCH after position setup\n");
            DBG("  stored = 0x%llx\n", (unsigned long long)board->hash);
            DBG("  fresh  = 0x%llx\n", (unsigned long long)fresh);
            dbg_dump_board(board);
            abort();
        }
    }
#endif

    p = strstr(p, "moves");
    if (!p)
        return;
    p += 6; // skip "moves "

    while (*p && *p != '\n' && *p != '\r')
    {
        move_t m = parse_uci_move(board, p);
        if (m)
        {
            DBG("applying %s (flag=%d cap=%d)\n",
                move_to_uci(m), MOVE_FLAGS(m), MOVE_CAPTURED(m));
            board_make_move(board, m);
#ifdef UCI_DEBUG
            // catch any make_move that leaves hash out of sync with squares
            uint64_t fresh = zobrist_from_board(board);
            if (board->hash != fresh)
            {
                DBG("HASH DRIFT after applying %s\n", move_to_uci(m));
                DBG("  incremental = 0x%llx\n", (unsigned long long)board->hash);
                DBG("  from scratch = 0x%llx\n", (unsigned long long)fresh);
                dbg_dump_board(board);
                abort();
            }
#endif
        }
#ifdef UCI_DEBUG
        else
        {
            DBG("PARSE FAIL on move starting with '%.6s'\n", p);
            dbg_dump_board(board);
            abort();
        }
#endif
        while (*p && *p != ' ' && *p != '\n' && *p != '\r')
            p++;
        while (*p == ' ')
            p++;
    }
}

int parse_int(const char *line, const char *side)
{
    const char *p = strstr(line, side);
    if (!p)
        return 0;
    p += strlen(side) + 1;
    int nb = 0;
    while (isdigit(*p))
        nb = 10 * nb + (*p++ - '0');
    return nb;
}

void uci_loop()
{
    board_t board = {0};
    char line[4096];

    while (fgets(line, sizeof(line), stdin))
    {
        if (strncmp(line, "ucinewgame", 10) == 0)
        {
            board_init(&board);
            memset(tt, 0, sizeof(tt));
            memset(killers, 0, sizeof(killers));
        }
        else if (strncmp(line, "isready", 7) == 0)
        {
            printf("readyok\n");
        }
        else if (strncmp(line, "uci", 3) == 0)
        {
            printf("id name OutOfStockFish\n");
            printf("id author agnlt64\n");
            printf("uciok\n");
        }
        else if (strncmp(line, "position", 8) == 0)
        {
            parse_position(&board, line);
        }
        else if (strncmp(line, "go", 2) == 0)
        {
            int perft = parse_int(line, "perft");
            if (perft != 0)
            {
                board_perft(&board, perft, true);
            }
            else
            {
                int btime = parse_int(line, "btime");
                int wtime = parse_int(line, "wtime");
                int binc = parse_int(line, "binc");
                int winc = parse_int(line, "winc");

                int time_left = board.turn == WHITE ? wtime : btime;
                int inc = board.turn == WHITE ? winc : binc;
                int move_overhead = 10; // 10ms
                // divide by 50 when playing low time control games
                int alloc_time = (int)fmax(time_left / 50 + inc / 2, time_left - move_overhead);
                if (alloc_time == 0)
                    alloc_time = 30000; // 30s by default

                canceled = false;
                search_end = clock() + (clock_t)(alloc_time * CLOCKS_PER_SEC / 1000);
#ifdef UCI_DEBUG
                printf("using %dms\n", alloc_time);
                printf("black = %d + %d, white = %d + %d\n", btime, binc, wtime, winc);
#endif
                move_t best = iterative_deepening(&board);

#ifdef UCI_DEBUG
                // ===== Sanity checks before sending bestmove to GUI =====
                // 1. Hash drift after search would mean make/unmake has a bug
                //    that perft doesn't catch (hash only, squares restored)
                {
                    uint64_t fresh = zobrist_from_board(&board);
                    if (board.hash != fresh)
                    {
                        DBG("HASH DRIFT after iterative_deepening\n");
                        DBG("  incremental = 0x%llx\n", (unsigned long long)board.hash);
                        DBG("  from scratch = 0x%llx\n", (unsigned long long)fresh);
                        dbg_dump_board(&board);
                        abort();
                    }
                }

                // 2. best == 0 means we never committed a real move
                if (best == 0)
                {
                    DBG("BUG: best == 0 — would have sent illegal 'bestmove a1a1'\n");
                    DBG("turn=%s canceled=%d alloc_time=%dms\n",
                        board.turn == WHITE ? "white" : "black",
                        (int)canceled, alloc_time);
                    dbg_dump_board(&board);
                    abort();
                }

                // 3. best must be in the current legal-moves list
                {
                    move_t legal[256];
                    int nlegal = gen_legal_moves(&board, legal, 0);
                    bool found = false;
                    for (int i = 0; i < nlegal; i++)
                        if (legal[i] == best) { found = true; break; }
                    if (!found)
                    {
                        DBG("BUG: best (%s, raw=0x%x flag=%d cap=%d) not in legal moves\n",
                            move_to_uci(best), best,
                            MOVE_FLAGS(best), MOVE_CAPTURED(best));
                        dbg_dump_board(&board);
                        DBG("%d legal moves at this position:\n", nlegal);
                        for (int i = 0; i < nlegal; i++)
                            DBG("  %s (raw=0x%x flag=%d)\n",
                                move_to_uci(legal[i]), legal[i], MOVE_FLAGS(legal[i]));
                        abort();
                    }
                }

                // 4. round-trip: my UCI output must re-parse back to the
                //    same move. catches move_to_uci bugs (missing promo
                //    suffix, wrong castle form, etc.)
                {
                    char uci_buf[8];
                    char *uci_str = move_to_uci(best);
                    strncpy(uci_buf, uci_str, sizeof(uci_buf) - 1);
                    uci_buf[sizeof(uci_buf) - 1] = '\0';

                    move_t roundtrip = parse_uci_move(&board, uci_buf);
                    if (roundtrip != best)
                    {
                        DBG("BUG: move_to_uci round-trip mismatch\n");
                        DBG("  best      = 0x%x (flag=%d cap=%d)\n",
                            best, MOVE_FLAGS(best), MOVE_CAPTURED(best));
                        DBG("  uci out   = '%s'\n", uci_buf);
                        DBG("  roundtrip = 0x%x\n", roundtrip);
                        dbg_dump_board(&board);
                        abort();
                    }

                    DBG("OK bestmove %s  turn=%s  halfmove=%d  fullmove=%d\n",
                        uci_buf,
                        board.turn == WHITE ? "white" : "black",
                        board.halfmove, board.fullmove);
                }
#endif
                printf("bestmove %s\n", move_to_uci(best));
            }
        }
        else if (strncmp(line, "d", 1) == 0)
        {
            board_print(&board);
        }
        else if (strncmp(line, "quit", 4) == 0)
        {
            break;
        }
        fflush(stdout);
    }
}

int main()
{
    init_distances();
    uci_loop();

    return 0;
}