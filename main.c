#include <stdio.h>
#include <math.h>

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

int main()
{
    board_t board = {0};
    board_from_fen(&board, "r3k2r/p1ppqpb1/Bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R b KQkq - 0 1");

    printf("best move = ");
    move_print(search(&board, 4));
    printf("searched %d positions\n", total_positions);

    return 0;
}