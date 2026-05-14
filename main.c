#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "board.h"
#include "tt.h"

int total_positions = 0;
tt_entry_t tt[TT_SIZE];

int count_material(board_t* board, color_t color)
{
    int total = 0;

    for (int i = 0; i < FILES * RANKS; i++)
    {
        piece_t p = board->squares[i];
        if (piece_color(p) != color) continue;

        total += piece_value(p);
    }

    return total;
}

// todo: improve move ordering, check / captures first maybe?
void order_moves(board_t* board, move_t* moves, int n)
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
            score_guess = 10 * piece_value(capture_piece_type) - piece_value(move_piece_type);

        switch (MOVE_FLAGS(move))
        {
            case FLAG_PROMO_B: score_guess += piece_value(BISHOP); break;
            case FLAG_PROMO_N: score_guess += piece_value(KNIGHT); break;
            case FLAG_PROMO_R: score_guess += piece_value(ROOK);   break;
            case FLAG_PROMO_Q: score_guess += piece_value(QUEEN);  break;
            default: break;
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

int evaluate(board_t* board)
{
    int eval = 0;

    int material = count_material(board, board->turn);

    for (int sq = 0; sq < 64; sq++)
    {
        piece_t p = board->squares[sq];
        if (piece_type(p) == NO_PIECE)
            continue;

        int val = piece_value(p) + piece_square_value(p, sq, material);
        eval += piece_color(p) == WHITE ? val : -val;
    }

    int sign = board->turn == WHITE ? 1 : -1;
    return sign * eval;
}

int search_captures(board_t* board, int alpha, int beta)
{
    total_positions++;
    int eval = evaluate(board);
    if (eval >= beta)
        return beta;
    
    if (eval + piece_value(QUEEN) < alpha) return alpha;

    alpha = (int)fmax(alpha, eval);
    
    move_t capture_moves[256];
    int n = gen_capture_moves(board, capture_moves);
    order_moves(board, capture_moves, n);

    for (size_t i = 0; i < n; i++)
    {
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

bool is_repetition(board_t* board)
{
    int limit = board->history_top - board->halfmove;
    if (limit < 0) limit = 0;
    for (int i = board->history_top - 2; i >= limit; i -= 2)
    {
        if (board->history[i].hash == board->hash)
            return true;
    }
    return false;
}

// ugly global state, just to debug
bool canceled = false;
static clock_t search_end;

static void check_time()
{
    if (clock() >= search_end)
        canceled = true;
}

int negamax(board_t* board, int depth, int alpha, int beta)
{
    total_positions++;

    if (canceled) return 0;
    if ((total_positions & 2048) == 0) check_time();

    // important: always keep this at the top of the function
    move_t moves[256];
    int n = gen_legal_moves(board, moves, 0);
    if (n == 0)
    {
        int king_sq = find_king(board, board->turn);
        return is_square_attacked(board, idx_to_square(king_sq), board->turn == WHITE ? BLACK : WHITE) ? -100000 : 0;
    }

    // 50-move rule and threefold repetition (one fold here)
    if (board->halfmove >= 100 || is_repetition(board)) return 0;
    
    tt_entry_t* entry = tt_get(tt, board->hash);
    if (entry && entry->depth >= depth)
        return entry->eval;

    if (depth == 0)
        return search_captures(board, alpha, beta);

    order_moves(board, moves, n);

    for (size_t i = 0; i < n; i++)
    {
        move_t move = moves[i];
        board_make_move(board, move);
        int eval = -negamax(board, depth - 1, -beta, -alpha);
        board_unmake_move(board, move);

        if (eval >= beta) return beta;
        if (eval > alpha) alpha = eval;
    }

    tt_set(tt, board->hash, depth, alpha);
    return alpha;
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

move_t search(board_t* board, int depth, move_t pre_best_move)
{
#ifdef UCI_DEBUG
    printf("depth = %d\n", depth);
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
        int extension = get_extension(board, move);
        int eval = -negamax(board, depth - 1 + extension, -200000, 200000);
        board_unmake_move(board, move);

        if (eval > best_eval)
        {
            best_eval = eval;
            best = move;
        }
    }

    return best;
}

move_t iterative_deepening(board_t* board)
{
    move_t best = 0;
    for (int depth = 1; depth < 256; depth++)
    {
        move_t best_move_curr = search(board, depth, best);
        if (canceled) break;
        best = best_move_curr;
    }
    return best;
}

static move_t parse_uci_move(board_t* board, const char* s)
{
    int from = (s[1] - '1') * 8 + (s[0] - 'a');
    int to   = (s[3] - '1') * 8 + (s[2] - 'a');
    char promo = (s[4] && s[4] != ' ' && s[4] != '\n' && s[4] != '\r') ? s[4] : 0;

    move_t moves[256];
    int n = gen_legal_moves(board, moves, 0);
    for (int i = 0; i < n; i++)
    {
        if (MOVE_FROM(moves[i]) != from || MOVE_TO(moves[i]) != to) continue;
        if (promo)
        {
            int flag = MOVE_FLAGS(moves[i]);
            if (promo == 'q' && flag != FLAG_PROMO_Q) continue;
            if (promo == 'r' && flag != FLAG_PROMO_R) continue;
            if (promo == 'b' && flag != FLAG_PROMO_B) continue;
            if (promo == 'n' && flag != FLAG_PROMO_N) continue;
        }
        return moves[i];
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
        while (*p && spaces < 6) { if (*p++ == ' ') spaces++; }
    }

    p = strstr(p, "moves");
    if (!p) return;
    p += 6; // skip "moves "

    while (*p && *p != '\n' && *p != '\r')
    {
        move_t m = parse_uci_move(board, p);
        if (m) board_make_move(board, m);
        while (*p && *p != ' ' && *p != '\n' && *p != '\r') p++;
        while (*p == ' ') p++;
    }
}

int parse_time(const char* line, const char* side)
{
    const char* p = strstr(line, side);
    if (!p) return 0;
    p += strlen(side) + 1;
    int time_left = 0;
    while (isdigit(*p))
        time_left = 10 * time_left + (*p++ - '0');
    return time_left;
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
            int btime = parse_time(line, "btime");
            int wtime = parse_time(line, "wtime");
            int binc = parse_time(line, "binc");
            int winc = parse_time(line, "winc");

            int time_left = board.turn == WHITE ? wtime : btime;
            int inc = board.turn == WHITE ? winc : binc;
            int alloc_time = time_left / 30 + inc / 2;
            if (alloc_time == 0) alloc_time = 30000; // 30s by default

            canceled = false;
            search_end = clock() + (clock_t)(alloc_time * CLOCKS_PER_SEC / 1000);
#ifdef UCI_DEBUG
            printf("using %dms\n", alloc_time);
            printf("black = %d + %d, white = %d + %d\n", btime, binc, wtime, winc);
#endif
            move_t best = iterative_deepening(&board);
            printf("bestmove %s\n", move_to_uci(best));
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
// technically these macros cancel each other,
// but i'll keep them to distinguish when
// i'm debugging the UCI part and when the engine
// is working and using the UCI through an UI.
#if defined(NO_UCI) && !defined(UCI_DEBUG)
    board_t board = {0};
    // board_from_fen(&board, "r3k2r/p1ppqpb1/Bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R b KQkq - 0 1");
    board_from_fen(&board, "8/3Q4/8/4PK1k/8/8/8/8 w - - 1 75");

    printf("board hash = %llu\n", board.hash);
    printf("best move = %s\n", move_to_uci(search(&board)));
    printf("total positions = %d\n", total_positions);
#else
    uci_loop();
#endif

    return 0;
}