#include <stdio.h>
#include <math.h>
#include <string.h>

#include "board.h"

#define PAWN_VAL 100
#define KNIGHT_VAL 300
#define BISHOP_VAL 300
#define ROOK_VAL 500
#define QUEEN_VAL 900

int total_positions = 0;

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
    int w_mat = count_material(board, WHITE);
    int b_mat = count_material(board, BLACK);

    int sign = board->turn == WHITE ? 1 : -1;
    return sign * (w_mat - b_mat);
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

int negamax(board_t* board, int depth, int alpha, int beta)
{
    total_positions++;
    move_t moves[256];
    int n = gen_legal_moves(board, moves);
    if (n == 0)
    {
        int king_sq = find_king(board, board->turn);
        return is_square_attacked(board, idx_to_square(king_sq), board->turn == WHITE ? BLACK : WHITE) ? -100000 : 0;
    }

    if (depth == 0)
    {
        return search_captures(board, alpha, beta);
        // return evaluate(board);
    }

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

    return alpha;
}

move_t search(board_t* board, int depth)
{
    move_t moves[256];
    int n = gen_legal_moves(board, moves);

    move_t best = moves[0];
    int best_eval = -200000;

    for (size_t i = 0; i < n; i++)
    {
        board_make_move(board, moves[i]);
        int eval = -negamax(board, depth - 1, -200000, 200000);
        board_unmake_move(board, moves[i]);

        if (eval > best_eval)
        {
            best_eval = eval;
            best = moves[i];
        }
    }

    return best;
}

static move_t parse_uci_move(board_t* board, const char* s)
{
    int from = (s[1] - '1') * 8 + (s[0] - 'a');
    int to   = (s[3] - '1') * 8 + (s[2] - 'a');
    char promo = (s[4] && s[4] != ' ' && s[4] != '\n' && s[4] != '\r') ? s[4] : 0;

    move_t moves[256];
    int n = gen_legal_moves(board, moves);
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
            move_t best = search(&board, 4);
            printf("bestmove %s\n", move_to_uci(best));
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
    uci_loop();

    return 0;
}