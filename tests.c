#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "squares.h"
#include "piece.h"
#include "board.h"

static bool is_piece_at_square(board_t* board, square_t sq, piece_t piece)
{
    piece_t at = board_at(board, sq);
    return piece_type(piece) == piece_type(at) && piece_color(piece) == piece_color(at);
}

static bool has_move(move_t* moves, int n, square_t from, square_t to)
{
    int from_idx = square_to_idx(from);
    int to_idx   = square_to_idx(to);
    for (int i = 0; i < n; i++)
        if (MOVE_FROM(moves[i]) == from_idx && MOVE_TO(moves[i]) == to_idx)
            return true;
    return false;
}

void test_initial_state()
{
    board_t board = {0};
    board_init(&board);

    assert(is_piece_at_square(&board, E2, WHITE | PAWN) && "e2 must be a white pawn");
    assert(is_piece_at_square(&board, E7, BLACK | PAWN) && "e7 must be a black pawn");

    assert(is_piece_at_square(&board, E1, WHITE | KING) && "e1 must be the white king");
    assert(is_piece_at_square(&board, E8, BLACK | KING) && "e8 must be the black king");
    
    assert(is_piece_at_square(&board, D1, WHITE | QUEEN) && "d1 must be the white queen");
    assert(is_piece_at_square(&board, D8, BLACK | QUEEN) && "d8 must be the black queen");

    assert(is_piece_at_square(&board, A1, WHITE | ROOK) && "a1 must be a white rook");
    assert(is_piece_at_square(&board, A8, BLACK | ROOK) && "a8 must be a black rook");

    assert(is_piece_at_square(&board, H1, WHITE | ROOK) && "h1 must be a white rook");
    assert(is_piece_at_square(&board, H8, BLACK | ROOK) && "h8 must be a black rook");

    printf("initial state is ok ✅\n");
}

void test_fen_parser()
{
    board_t board = {0};
    
    board_from_fen(&board, "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    assert(board.turn == BLACK && "side to move is black");
    assert(board.castling == 0b1111 && "both side can castle");
    assert(board.ep_square_idx == square_to_idx(E3) && "en passant square is e3");
    assert(board.halfmove == 0 && "half move is 0");
    assert(board.fullmove == 1 && "full move is 1");

    board_from_fen(&board, "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2");
    assert(board.turn == WHITE && "side to move is white");
    assert(board.castling == 0b1111 && "both side can castle");
    assert(board.ep_square_idx == square_to_idx(C6) && "en passant square is c6");
    assert(board.halfmove == 0 && "half move is 0");
    assert(board.fullmove == 2 && "full move is 2");

    board_from_fen(&board, "r1bq1rk1/pppnbpp1/4pn1p/3p4/2PP3B/2N1PN2/PP3PPP/R2QKB1R w KQ - 1 8");
    assert(board.turn == WHITE && "side to move is white");
    assert(board.castling == 0b1100 && "black can't castle");
    assert(board.ep_square_idx == -1 && "no en passant is possible");
    assert(board.halfmove == 1 && "half move is 1");
    assert(board.fullmove == 8 && "full move is 8");

    board_from_fen(&board, "r1bq1rk1/pp1nbpp1/4pn1p/2pp4/2PP3B/2NBPN2/PP3PPP/R2Q1RK1 b - - 1 9");
    assert(board.turn == BLACK && "side to move is black");
    assert(board.castling == 0 && "nobody can castle");
    assert(board.ep_square_idx == -1 && "no en passant is possible");
    assert(board.halfmove == 1 && "half move is 1");
    assert(board.fullmove == 9 && "full move is 9");

    board_from_fen(&board, "r1bqkbnr/pppppppp/8/6N1/1n6/8/PPPPPPPP/RNBQKB1R w KQkq - 4 3");
    assert(board.turn == WHITE && "side to move is black");
    assert(board.castling == 0b1111 && "both sides can castle");
    assert(board.ep_square_idx == -1 && "no en passant is possible");
    assert(board.halfmove == 4 && "half move is 4");
    assert(board.fullmove == 3 && "full move is 3");

    board_from_fen(&board, "r1bqk2r/ppp1bpp1/2n1pn1p/1B2N3/3Pp3/P1N5/1PP2PPP/R1BQK2R w KQkq - 20 10");
    assert(board.turn == WHITE && "side to move is black");
    assert(board.castling == 0b1111 && "both sides can castle");
    assert(board.ep_square_idx == -1 && "no en passant is possible");
    assert(board.halfmove == 20 && "half move is 20");
    assert(board.fullmove == 10 && "full move is 10");

    printf("fen parser is ok ✅\n");
}

void test_piece_type()
{
    assert(piece_type(KING) == KING && "king must be a king");
    assert(piece_type(QUEEN) == QUEEN && "queen must be a queen");
    assert(piece_type(KNIGHT) == KNIGHT && "knight must be a knight");
    assert(piece_type(BISHOP) == BISHOP && "bishop must be a bishop");
    assert(piece_type(ROOK) == ROOK && "rook must be a rook");
    assert(piece_type(PAWN) == PAWN && "pawn must be a pawn");
    assert(piece_type(PAWN | WHITE) == PAWN && "white pawn must be a pawn");
    assert(piece_type(ROOK | BLACK) == ROOK && "black rook must be a rook");
    
    printf("piece types are ok ✅\n");
}

void test_piece_color()
{
    assert(piece_color(WHITE) == WHITE && "white must be white");
    assert(piece_color(BLACK) == BLACK && "black must be black");

    assert(piece_color(PAWN | WHITE) == WHITE && "white pawn must be white");
    assert(piece_color(ROOK | BLACK) == BLACK && "black rook must be black");

    printf("piece colors are ok ✅\n");
}

void test_piece_string()
{
    assert(piece_string(WHITE | PAWN) == 'P' && "white pawn is P");
    assert(piece_string(BLACK | PAWN) == 'p' && "black pawn is p");

    assert(piece_string(WHITE | QUEEN) == 'Q' && "white queen is Q");
    assert(piece_string(BLACK | QUEEN) == 'q' && "black queen is q");

    assert(piece_string(WHITE | KING) == 'K' && "white king is K");
    assert(piece_string(BLACK | KING) == 'k' && "black king is k");

    assert(piece_string(WHITE | ROOK) == 'R' && "white rook is R");
    assert(piece_string(BLACK | ROOK) == 'r' && "black rook is r");

    assert(piece_string(WHITE | BISHOP) == 'B' && "white pawn is B");
    assert(piece_string(BLACK | BISHOP) == 'b' && "black pawn is b");

    assert(piece_string(WHITE | KNIGHT) == 'N' && "white knight is N");
    assert(piece_string(BLACK | KNIGHT) == 'n' && "black knight is n");

    printf("piece strings are ok ✅\n");
}

void test_knight_attacks()
{
    board_t board = {0};
    board.squares[square_to_idx(D4)] = WHITE | KNIGHT;

    assert(can_knight_attack(&board, C2, WHITE) && "knight on d4 attacks c2");
    assert(can_knight_attack(&board, B3, WHITE) && "knight on d4 attacks b3");
    assert(can_knight_attack(&board, B5, WHITE) && "knight on d4 attacks b5");
    assert(can_knight_attack(&board, C6, WHITE) && "knight on d4 attacks c6");
    assert(can_knight_attack(&board, E6, WHITE) && "knight on d4 attacks e6");
    assert(can_knight_attack(&board, F5, WHITE) && "knight on d4 attacks f5");
    assert(can_knight_attack(&board, F3, WHITE) && "knight on d4 attacks f3");
    assert(can_knight_attack(&board, E2, WHITE) && "knight on d4 attacks e2");

    board.squares[square_to_idx(H1)] = BLACK | KNIGHT;
    assert(can_knight_attack(&board, G3, BLACK) && "knight on h1 attacks g2");
    assert(can_knight_attack(&board, F2, BLACK) && "knight on h1 attacks f2");
    assert(!can_knight_attack(&board, F3, BLACK) && "knight on h1 doesn't attack f3");
    assert(!can_knight_attack(&board, A3, BLACK) && "knight on h1 doesn't attack a3");
    assert(!can_knight_attack(&board, C6, BLACK) && "knight on h1 doesn't attack c6");
    assert(!can_knight_attack(&board, G3, WHITE) && "knight on h1 attacks g3 but is black");

    printf("knight attacks are ok ✅\n");
}

void test_king_attacks()
{
    board_t board = {0};
    board.squares[square_to_idx(D4)] = WHITE | KING;

    assert(can_king_attack(&board, E4, WHITE) && "king on d4 attacks e4");
    assert(can_king_attack(&board, C4, WHITE) && "king on d4 attacks c4");
    assert(can_king_attack(&board, D3, WHITE) && "king on d4 attacks d3");
    assert(can_king_attack(&board, D5, WHITE) && "king on d4 attacks d5");
    assert(can_king_attack(&board, C5, WHITE) && "king on d4 attacks c5");
    assert(can_king_attack(&board, E5, WHITE) && "king on d4 attacks e5");
    assert(can_king_attack(&board, C3, WHITE) && "king on d4 attacks c3");
    assert(can_king_attack(&board, E3, WHITE) && "king on d4 attacks e3");

    board.squares[square_to_idx(H1)] = BLACK | KING;
    assert(can_king_attack(&board, G1, BLACK) && "king on h1 attacks g1");
    assert(can_king_attack(&board, G2, BLACK) && "king on h1 attacks g2");
    assert(can_king_attack(&board, H2, BLACK) && "king on h1 attacks h2");
    assert(!can_king_attack(&board, A3, BLACK) && "king on h1 doesn't attack a3");
    assert(!can_king_attack(&board, C6, BLACK) && "king on h1 doesn't attack c6");
    assert(!can_king_attack(&board, G1, WHITE) && "king on h1 attacks g1 but is black");

    printf("king attacks are ok ✅\n");
}

void test_pawn_attacks()
{
    board_t board = {0};
    board.squares[square_to_idx(D4)] = WHITE | PAWN;

    assert(can_pawn_attack(&board, C5, WHITE) && "pawn on d4 attacks c5");
    assert(can_pawn_attack(&board, E5, WHITE) && "pawn on d4 attacks e5");
    assert(!can_pawn_attack(&board, D5, WHITE) && "pawn on d4 doesn't attack d5");
    assert(!can_pawn_attack(&board, D3, WHITE) && "white pawn doesn't attack backwards");
    assert(!can_pawn_attack(&board, C5, BLACK) && "white pawn is not a black pawn");

    board_t board2 = {0};
    board2.squares[square_to_idx(D5)] = BLACK | PAWN;
    assert(can_pawn_attack(&board2, C4, BLACK) && "black pawn on d5 attacks c4");
    assert(can_pawn_attack(&board2, E4, BLACK) && "black pawn on d5 attacks e4");
    assert(!can_pawn_attack(&board2, D4, BLACK) && "black pawn on d5 doesn't attack d4");
    assert(!can_pawn_attack(&board2, C6, BLACK) && "black pawn doesn't attack backwards");

    board_t edge = {0};
    edge.squares[square_to_idx(A4)] = WHITE | PAWN;
    assert(can_pawn_attack(&edge, B5, WHITE) && "pawn on a4 attacks b5");
    assert(!can_pawn_attack(&edge, H5, WHITE) && "pawn on a4 doesn't wrap to h5");

    edge.squares[square_to_idx(H4)] = WHITE | PAWN;
    assert(can_pawn_attack(&edge, G5, WHITE) && "pawn on h4 attacks g5");
    assert(!can_pawn_attack(&edge, A5, WHITE) && "pawn on h4 doesn't wrap to a5");

    printf("pawn attacks are ok ✅\n");
}

void test_diag_attacks()
{
    board_t board = {0};
    board.squares[square_to_idx(D5)] = WHITE | BISHOP;

    assert(can_diag_attack(&board, E6, WHITE) && "bishop on d5 attacks e6");
    assert(can_diag_attack(&board, F7, WHITE) && "bishop on d5 attacks f7");
    assert(can_diag_attack(&board, G8, WHITE) && "bishop on d5 attacks g8");
    assert(can_diag_attack(&board, C4, WHITE) && "bishop on d5 attacks c4");
    assert(can_diag_attack(&board, B3, WHITE) && "bishop on d5 attacks b3");
    assert(can_diag_attack(&board, C6, WHITE) && "bishop on d5 attacks c6");
    assert(can_diag_attack(&board, E4, WHITE) && "bishop on d5 attacks e4");

    board.squares[square_to_idx(F7)] = WHITE | PAWN;
    assert(can_diag_attack(&board, E6, WHITE) && "bishop still attacks e6 before own pawn");
    assert(!can_diag_attack(&board, G8, WHITE) && "bishop blocked by own pawn at f7");

    board_t board2 = {0};
    board2.squares[square_to_idx(D5)] = WHITE | BISHOP;
    board2.squares[square_to_idx(F7)] = BLACK | PAWN;
    assert(can_diag_attack(&board2, F7, WHITE) && "bishop attacks enemy pawn at f7");
    assert(!can_diag_attack(&board2, G8, WHITE) && "bishop blocked beyond enemy pawn at f7");

    board_t board3 = {0};
    board3.squares[square_to_idx(D5)] = WHITE | QUEEN;
    assert(can_diag_attack(&board3, G8, WHITE) && "queen on d5 attacks g8 diagonally");
    assert(can_diag_attack(&board3, A2, WHITE) && "queen on d5 attacks a2 diagonally");

    board_t board4 = {0};
    board4.squares[square_to_idx(H5)] = WHITE | BISHOP;
    assert(!can_diag_attack(&board4, A6, WHITE) && "bishop on h5 doesn't wrap to a6");
    assert(!can_diag_attack(&board4, A4, WHITE) && "bishop on h5 doesn't wrap to a4");

    printf("diagonal attacks are ok ✅\n");
}

void test_orth_attacks()
{
    board_t board = {0};
    board.squares[square_to_idx(D4)] = WHITE | ROOK;

    assert(can_orth_attack(&board, E4, WHITE) && "rook on d4 attacks e4");
    assert(can_orth_attack(&board, H4, WHITE) && "rook on d4 attacks h4");
    assert(can_orth_attack(&board, C4, WHITE) && "rook on d4 attacks c4");
    assert(can_orth_attack(&board, A4, WHITE) && "rook on d4 attacks a4");
    assert(can_orth_attack(&board, D5, WHITE) && "rook on d4 attacks d5");
    assert(can_orth_attack(&board, D8, WHITE) && "rook on d4 attacks d8");
    assert(can_orth_attack(&board, D3, WHITE) && "rook on d4 attacks d3");
    assert(can_orth_attack(&board, D1, WHITE) && "rook on d4 attacks d1");

    assert(!can_orth_attack(&board, E5, WHITE) && "rook on d4 doesn't attack e5");
    assert(!can_orth_attack(&board, C3, WHITE) && "rook on d4 doesn't attack c3");

    board.squares[square_to_idx(F4)] = WHITE | PAWN;
    assert(can_orth_attack(&board, E4, WHITE) && "rook still attacks e4 before own pawn");
    assert(!can_orth_attack(&board, G4, WHITE) && "rook blocked by own pawn at f4");

    board_t board2 = {0};
    board2.squares[square_to_idx(D4)] = WHITE | ROOK;
    board2.squares[square_to_idx(F4)] = BLACK | PAWN;
    assert(can_orth_attack(&board2, F4, WHITE) && "rook attacks enemy pawn at f4");
    assert(!can_orth_attack(&board2, G4, WHITE) && "rook blocked beyond enemy pawn at f4");

    board_t board3 = {0};
    board3.squares[square_to_idx(D4)] = WHITE | QUEEN;
    assert(can_orth_attack(&board3, D8, WHITE) && "queen on d4 attacks d8 orthogonally");
    assert(can_orth_attack(&board3, A4, WHITE) && "queen on d4 attacks a4 orthogonally");

    board_t board4 = {0};
    board4.squares[square_to_idx(H4)] = WHITE | ROOK;
    assert(can_orth_attack(&board4, A4, WHITE) && "rook on h4 attacks a4");
    assert(!can_orth_attack(&board4, A5, WHITE) && "rook on h4 doesn't wrap to a5");

    board_t board5 = {0};
    board5.squares[square_to_idx(A4)] = WHITE | ROOK;
    assert(can_orth_attack(&board5, H4, WHITE) && "rook on a4 attacks h4");

    board_t board6 = {0};
    board6.squares[square_to_idx(A1)] = WHITE | ROOK;
    assert(can_orth_attack(&board6, H1, WHITE) && "rook on a1 attacks h1");
    assert(can_orth_attack(&board6, A8, WHITE) && "rook on a1 attacks a8");
    assert(!can_orth_attack(&board6, B2, WHITE) && "rook on a1 doesn't attack b2");

    assert(!can_orth_attack(&board6, H1, BLACK) && "rook on a1 is white, not black");

    printf("orthogonal attacks are ok ✅\n");
}

void test_square_attacked()
{
    board_t board = {0};
    board_from_fen(&board, "8/1B6/3K1N2/1q5R/2Q1P3/8/8/8 w - - 0 1");
    assert(is_square_attacked(&board, D5, WHITE) && "d5 is attacked by all of the white pieces");
    assert(is_square_attacked(&board, D5, BLACK) && "d5 is attacked by the black queen");
    assert(is_square_attacked(&board, B1, BLACK) && "b1 is attacked by the black queen");
    assert(is_square_attacked(&board, H5, BLACK) && "h5 is attacked by the black queen");
    assert(is_square_attacked(&board, A5, BLACK) && "a5 is attacked by the black queen");
    assert(is_square_attacked(&board, G4, WHITE) && "g4 is attacked by the white knight");

    assert(!is_square_attacked(&board, A5, WHITE) && "white rook on h5 is blocked by black queen on b5");

    printf("attacked square is ok ✅\n");
}

void test_king_moves()
{
    move_t moves[256];
    int n;

    board_t board = {0};
    board.turn = WHITE;
    board.squares[square_to_idx(D5)] = WHITE | KING;
    n = gen_king_moves(&board, square_to_idx(D5), moves);
    assert(n == 8 && "king on d5 has 8 moves");
    assert(has_move(moves, n, D5, C4) && "king d5->c4");
    assert(has_move(moves, n, D5, C5) && "king d5->c5");
    assert(has_move(moves, n, D5, C6) && "king d5->c6");
    assert(has_move(moves, n, D5, D4) && "king d5->d4");
    assert(has_move(moves, n, D5, D6) && "king d5->d6");
    assert(has_move(moves, n, D5, E4) && "king d5->e4");
    assert(has_move(moves, n, D5, E5) && "king d5->e5");
    assert(has_move(moves, n, D5, E6) && "king d5->e6");

    board_t corner = {0};
    corner.turn = WHITE;
    corner.squares[square_to_idx(A1)] = WHITE | KING;
    n = gen_king_moves(&corner, square_to_idx(A1), moves);
    assert(n == 3 && "king on a1 has 3 moves");
    assert(has_move(moves, n, A1, A2) && "king a1->a2");
    assert(has_move(moves, n, A1, B1) && "king a1->b1");
    assert(has_move(moves, n, A1, B2) && "king a1->b2");

    board_t corner2 = {0};
    corner2.turn = WHITE;
    corner2.squares[square_to_idx(H8)] = WHITE | KING;
    n = gen_king_moves(&corner2, square_to_idx(H8), moves);
    assert(n == 3 && "king on h8 has 3 moves");

    board_t edge = {0};
    edge.turn = WHITE;
    edge.squares[square_to_idx(A4)] = WHITE | KING;
    n = gen_king_moves(&edge, square_to_idx(A4), moves);
    assert(n == 5 && "king on a4 has 5 moves");

    board_t blocked = {0};
    blocked.turn = WHITE;
    blocked.squares[square_to_idx(D5)] = WHITE | KING;
    blocked.squares[square_to_idx(C4)] = WHITE | PAWN;
    blocked.squares[square_to_idx(C5)] = WHITE | PAWN;
    blocked.squares[square_to_idx(C6)] = WHITE | PAWN;
    blocked.squares[square_to_idx(D4)] = WHITE | PAWN;
    blocked.squares[square_to_idx(D6)] = WHITE | PAWN;
    blocked.squares[square_to_idx(E4)] = WHITE | PAWN;
    blocked.squares[square_to_idx(E5)] = WHITE | PAWN;
    blocked.squares[square_to_idx(E6)] = WHITE | PAWN;
    n = gen_king_moves(&blocked, square_to_idx(D5), moves);
    assert(n == 0 && "king fully surrounded by allies has 0 moves");

    board_t cap = {0};
    cap.turn = WHITE;
    cap.squares[square_to_idx(D5)] = WHITE | KING;
    cap.squares[square_to_idx(E5)] = BLACK | ROOK;
    n = gen_king_moves(&cap, square_to_idx(D5), moves);
    assert(n == 8 && "king can capture enemy rook on e5");
    assert(has_move(moves, n, D5, E5) && "king d5 captures e5");
    for (int i = 0; i < n; i++)
        if (MOVE_TO(moves[i]) == square_to_idx(E5))
            assert(MOVE_FLAGS(moves[i]) == FLAG_CAPTURE && "e5 is a capture");

    printf("king moves are ok ✅\n");
}

// todo: finish testing, we already know movegen for knights is working
void test_knight_moves()
{
    board_t board = {0};
    board.turn = BLACK;

    board.squares[square_to_idx(D4)] = WHITE | KNIGHT;

    int n;
    move_t moves[256];

    n = gen_knight_moves(&board, square_to_idx(D4), moves);
    assert(n == 0 && "black has no pieces on the board");

    printf("knight moves are ok ✅\n");
}

// todo: finish testing
void test_diag_moves()
{
    board_t board = {0};
    board.turn = WHITE;

    board.squares[square_to_idx(D4)] = WHITE | BISHOP;

    int n;
    move_t moves[256];

    n = gen_diag_moves(&board, square_to_idx(D4), moves);

    // for (int i = 0; i < n; i++)
    // {
    //     move_print(moves[i]);
    // }

    printf("diagonal moves are ok ✅\n");
}

// todo: finish testing
void test_orth_moves()
{
    board_t board = {0};
    board.turn = WHITE;

    board.squares[square_to_idx(D4)] = WHITE | ROOK;

    int n;
    move_t moves[256];

    n = gen_orth_moves(&board, square_to_idx(D4), moves);

    // for (int i = 0; i < n; i++)
    // {
    //     move_print(moves[i]);
    // }

    printf("orthogonal moves are ok ✅\n");
}

// todo: finish testing
void test_pawn_moves()
{
    board_t board = {0};
    board.turn = WHITE;

    board.squares[square_to_idx(E7)] = WHITE | PAWN;

    int n;
    move_t moves[256];

    n = gen_pawn_moves(&board, square_to_idx(E7), moves);

    // for (int i = 0; i < n; i++)
    // {
    //     move_print(moves[i]);
    // }

    printf("pawn moves are ok ✅\n");
}

void test_gen_moves()
{
    board_t board = {0};
    board_init(&board);

    int n = 0;

    // startpos
    printf("testing startpos\n");

    n = board_perft(&board, 1, false);
    assert(n == 20 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 1 is ok\n");

    n = board_perft(&board, 2, false);
    assert(n == 400 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 2 is ok\n");

    n = board_perft(&board, 3, false);
    assert(n == 8902 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 3 is ok\n");

    n = board_perft(&board, 4, false);
    assert(n == 197281 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 4 is ok\n");

    n = board_perft(&board, 5, false);
    assert(n == 4865609 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 5 is ok\n");

    // kiwipete
    printf("testing kiwipete\n");
    board_from_fen(&board, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ");
    
    n = board_perft(&board, 1, false);
    assert(n == 48 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 1 is ok\n");

    n = board_perft(&board, 2, false);
    assert(n == 2039 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 2 is ok\n");

    n = board_perft(&board, 3, false);
    assert(n == 97862 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 3 is ok\n");

    n = board_perft(&board, 4, false);
    assert(n == 4085603 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 4 is ok\n");

    n = board_perft(&board, 5, false);
    assert(n == 193690690 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 5 is ok\n");

    // position 5 from chessprogramming perft
    printf("testing position 5\n");
    board_from_fen(&board, "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    
    n = board_perft(&board, 1, false);
    assert(n == 44 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 1 is ok\n");

    n = board_perft(&board, 2, false);
    assert(n == 1486 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 2 is ok\n");

    n = board_perft(&board, 3, false);
    assert(n == 62379 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 3 is ok\n");

    n = board_perft(&board, 4, false);
    assert(n == 2103487 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 4 is ok\n");

    n = board_perft(&board, 5, false);
    assert(n == 89941194 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 5 is ok\n");

    // position 3 from chessprogramming perft
    printf("testing position 3\n");
    board_from_fen(&board, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ");
    
    n = board_perft(&board, 1, false);
    assert(n == 14 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 1 is ok\n");
    
    n = board_perft(&board, 2, false);
    assert(n == 191 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 2 is ok\n");

    n = board_perft(&board, 3, false);
    assert(n == 2812 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 3 is ok\n");

    n = board_perft(&board, 4, false);
    assert(n == 43238 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 4 is ok\n");

    n = board_perft(&board, 5, false);
    assert(n == 674624 && "https://www.chessprogramming.org/Perft_Results");
    printf("    depth 5 is ok\n");

    // after ply 5, move gen is really slow so i don't test it further
    printf("gen moves is ok ✅\n");
}

void test_gen_capture_moves()
{
    board_t board = {0};
    board_from_fen(&board, "r1b1kb1r/ppp2ppp/4p3/4P3/1nPq4/5N2/PPQ1KpPP/R1B2B1R w kq - 0 1");

    move_t moves[64];
    int n = gen_capture_moves(&board, moves);
    assert(n == 2 && "2 captures for white in this position");

    board_from_fen(&board, "4b2K/pp2p1pp/2ppkp2/1B5Q/1P1PN2P/2P1P1P1/R7/8 w - - 0 1");
    n = gen_capture_moves(&board, moves);
    assert(n == 8 && "8 captures for white in this position");

    printf("gen capture moves is ok ✅\n");
}

// todo: finish testing
void test_make_move()
{
    board_t board = {0};
    board_init(&board);
    // board_print(&board);

    board_make_move(&board, MOVE_ENCODE(square_to_idx(D2), square_to_idx(D4), 0, 0));
    // board_print(&board);

    board_make_move(&board, MOVE_ENCODE(square_to_idx(D7), square_to_idx(D5), 0, 0));
    // board_print(&board);

    board_make_move(&board, MOVE_ENCODE(square_to_idx(G8), square_to_idx(F6), 0, 0));
    // board_print(&board);

    printf("make move is ok ✅\n");
}

// todo: finish testing
void test_unmake_move()
{
    board_t board = {0};
    board_from_fen(&board, "r1bq1rk1/pppnbpp1/4pn1p/3p4/2PP3B/2N1PN2/PP3PPP/R2QKB1R w KQ - 0 1");
    // board_print(&board);

    board_unmake_move(&board, MOVE_ENCODE(square_to_idx(G1), square_to_idx(F3), 0, 0));
    // board_print(&board);
    
    printf("unmake move is ok ✅\n");
}

void test_king_square()
{
    board_t board = {0};
    board_init(&board);

    assert(board.king_sq[0] == square_to_idx(E1) && board.king_sq[1] == square_to_idx(E8));

    board_from_fen(&board, "7k/8/8/8/8/8/1K6/8 w - - 0 1");
    assert(board.king_sq[0] == square_to_idx(B2) && board.king_sq[1] == square_to_idx(H8));

    board_from_fen(&board, "8/2K5/8/8/4k3/8/8/8 w - - 0 1");
    assert(board.king_sq[0] == square_to_idx(C7) && board.king_sq[1] == square_to_idx(E4));

    board_from_fen(&board, "8/8/8/3K4/8/3k4/8/8 w - - 0 1");
    assert(board.king_sq[0] == square_to_idx(D5) && board.king_sq[1] == square_to_idx(D3));

    printf("king squares are ok ✅\n");
}

void test_in_ckeck()
{
    board_t board = {0};
    board_from_fen(&board, "8/b4k2/5n2/1p4p1/1P1r2b1/3K4/PP1P4/1RB5 w - - 0 36");
    assert(board_in_check(&board));

    board_init(&board);
    assert(!board_in_check(&board));

    printf("in check is ok ✅\n");
}

int main()
{
    test_initial_state();
    test_fen_parser();
    test_piece_type();
    test_piece_color();
    test_piece_string();
    test_knight_attacks();
    test_king_attacks();
    test_pawn_attacks();
    test_diag_attacks();
    test_orth_attacks();
    test_square_attacked();
    test_king_moves();
    test_knight_moves();
    test_diag_moves();
    test_orth_moves();
    test_pawn_moves();
    test_gen_moves();
    test_gen_capture_moves();
    test_make_move();
    test_unmake_move();
    test_king_square();
    test_in_ckeck();

    return 0;
}