#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "uci.h"
#include "board.h"
#include "squares.h"
#include "debug.h"
#include "search.h"
#include "evaluation.h"
#include "movegen.h"

static move_t parse_uci_move(board_t* board, const char* s)
{
#ifdef UCI_DEBUG
    assert(board->hash == zobrist_from_board(board) && "hash drift detected");
#endif
    int from = (s[1] - '1')*  8 + (s[0] - 'a');
    int to = (s[3] - '1')*  8 + (s[2] - 'a');
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

void parse_position(board_t* board, const char* line)
{
    const char* p = line + 9; // skip "position "

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

    while (*p &&* p != '\n' &&* p != '\r')
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
        while (*p &&* p != ' ' &&* p != '\n' &&* p != '\r')
            p++;
        while (*p == ' ')
            p++;
    }
}

int parse_int(const char* line, const char* side)
{
    const char* p = strstr(line, side);
    if (!p)
        return 0;
    p += strlen(side) + 1;
    int nb = 0;
    while (isdigit(*p))
        nb = 10*  nb + (*p++ - '0');
    return nb;
}

void uci_loop()
{
    search_ctx_t ctx;
    search_ctx_init(&ctx);
    char line[4096];

    while (fgets(line, sizeof(line), stdin))
    {
        if (strncmp(line, "ucinewgame", 10) == 0)
        {
            board_init(&ctx.board);
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
            parse_position(&ctx.board, line);
        }
        else if (strncmp(line, "eval", 4) == 0)
        {
            printf("eval %d\n", evaluate(&ctx.board));
        }
        else if (strncmp(line, "go", 2) == 0)
        {
            int perft = parse_int(line, "perft");
            if (perft != 0)
            {
                board_perft(&ctx.board, perft, true);
                continue;
            }

            int depth = parse_int(line, "depth");
            if (depth != 0)
            {
                ctx.canceled = false;
                ctx.search_end_ms = 0;
                move_t best = search(&ctx, depth);
                printf("bestmove %s\n", move_to_uci(best));
                fflush(stdout);
                continue;
            }
            else
            {
                int btime = parse_int(line, "btime");
                int wtime = parse_int(line, "wtime");
                int binc = parse_int(line, "binc");
                int winc = parse_int(line, "winc");

                int time_left = ctx.board.turn == WHITE ? wtime : btime;
                int inc = ctx.board.turn == WHITE ? winc : binc;
                int move_overhead = 10; // 10ms
                // soft target: how much we want to spend on this move
                int soft = time_left / 50 + inc / 2;
                // hard cap: never burn through the time left minus overhead,
                // even if our soft target says otherwise (low time, etc.)
                int hard = time_left - move_overhead;
                int alloc_time = (int)fmin(soft, hard);
                if (alloc_time <= 0)
                    alloc_time = 30000; // 30s by default

                ctx.canceled = false;
                ctx.search_end_ms = now_ms() + alloc_time;
#ifdef UCI_DEBUG
                printf("using %dms\n", alloc_time);
                printf("black = %d + %d, white = %d + %d\n", btime, binc, wtime, winc);
#endif
                move_t best = iterative_deepening(&ctx);

#ifdef UCI_DEBUG
                // ===== Sanity checks before sending bestmove to GUI =====
                // 1. Hash drift after search would mean make/unmake has a bug
                //    that perft doesn't catch (hash only, squares restored)
                {
                    uint64_t fresh = zobrist_from_board(&ctx.board);
                    if (ctx.board.hash != fresh)
                    {
                        DBG("HASH DRIFT after iterative_deepening\n");
                        DBG("  incremental = 0x%llx\n", (unsigned long long)ctx.board.hash);
                        DBG("  from scratch = 0x%llx\n", (unsigned long long)fresh);
                        dbg_dump_board(&ctx.board);
                        abort();
                    }
                }

                // 2. best == 0 means we never committed a real move
                if (best == 0)
                {
                    DBG("BUG: best == 0 — would have sent illegal 'bestmove a1a1'\n");
                    DBG("turn=%s canceled=%d alloc_time=%dms\n",
                        ctx.board.turn == WHITE ? "white" : "black",
                        (int)ctx.canceled, alloc_time);
                    dbg_dump_board(&ctx.board);
                    abort();
                }

                // 3. best must be in the current legal-moves list
                {
                    move_t legal[256];
                    int nlegal = gen_legal_moves(&ctx.board, legal, 0);
                    bool found = false;
                    for (int i = 0; i < nlegal; i++)
                        if (legal[i] == best)
                        {
                            found = true;
                            break;
                        }
                    if (!found)
                    {
                        DBG("BUG: best (%s, raw=0x%x flag=%d cap=%d) not in legal moves\n",
                            move_to_uci(best), best,
                            MOVE_FLAGS(best), MOVE_CAPTURED(best));
                        dbg_dump_board(&ctx.board);
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
                    char* uci_str = move_to_uci(best);
                    strncpy(uci_buf, uci_str, sizeof(uci_buf) - 1);
                    uci_buf[sizeof(uci_buf) - 1] = '\0';

                    move_t roundtrip = parse_uci_move(&ctx.board, uci_buf);
                    if (roundtrip != best)
                    {
                        DBG("BUG: move_to_uci round-trip mismatch\n");
                        DBG("  best      = 0x%x (flag=%d cap=%d)\n",
                            best, MOVE_FLAGS(best), MOVE_CAPTURED(best));
                        DBG("  uci out   = '%s'\n", uci_buf);
                        DBG("  roundtrip = 0x%x\n", roundtrip);
                        dbg_dump_board(&ctx.board);
                        abort();
                    }

                    DBG("OK bestmove %s  turn=%s  halfmove=%d  fullmove=%d\n",
                        uci_buf,
                        ctx.board.turn == WHITE ? "white" : "black",
                        ctx.board.halfmove, ctx.board.fullmove);
                }
#endif
                printf("bestmove %s\n", move_to_uci(best));
            }
        }
        else if (strncmp(line, "d", 1) == 0)
        {
            board_print(&ctx.board);
        }
        else if (strncmp(line, "quit", 4) == 0)
        {
            break;
        }
        fflush(stdout);
    }
}
