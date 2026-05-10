
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>

#include "board.h"

void board_init(board_t* board)
{
    board_from_fen(board, DEFAULT_FEN);
}

static const int knight_offsets[8] = {17, 15, 10, 6, -6, -10, -15, -17};
static const int knight_file_offsets[8] = {1, -1, 2, -2, 2, -2, 1, -1};

static const int king_offsets[8] = {1, -1,  8, -8,  9, -9,  7, -7};
static const int king_file_offsets[8] = {1, -1, 0, 0, 1, -1, -1, 1};

static const int diag_offsets[4] = {9, -9, 7, -7};
static const int diag_file_offsets[4] = { 1, -1, -1, 1};

static const int orth_offsets[4] = {8, -8, 1, -1};
static const int orth_file_offsets[4] = { 0, 0, 1, -1};

void board_from_fen(board_t* board, const char* fen)
{
    memset(board, 0, sizeof(*board));
    int file = 0;
    int rank = 7;
    int space_count = 0;
    board->ep_square_idx = -1;

    for (const char* p = fen; *p != '\0'; p++)
    {
        char c = *p;
        if (c == ' ')
            space_count++;
        
        // pieces position
        if (space_count == 0)
        {
            if (c == '/')
            {
                file = 0;
                rank--;
            }
            else
            {
                if (isdigit(c))
                    file += c - '0';
                else
                {
                    int color = isupper(c) ? WHITE : BLACK;
                    int type = CHAR_TO_PIECE[tolower(c)];
                    board->squares[rank * RANKS + file] = type | color;
                    file++;
                }
            }
        }

        // side to move
        if (space_count == 1)
        {
            if (c == 'b')
                board->turn = BLACK;
    
            if (c == 'w')
                board->turn = WHITE;
        }

        // castling ability
        if (space_count == 2)
        {
            if (c == 'K')
                board->castling |= 8; // 0b1000
            else if (c == 'Q')
                board->castling |= 4; // 0b0100
            else if (c == 'k')
                board->castling |= 2; // 0b0010
            else if (c == 'q')
                board->castling |= 1; // 0b0001
            else if (c == '-')
                board->castling = 0;
        }

        // en passant square
        if (space_count == 3)
        {
            int ep_file = -1;
            int ep_rank = -1;

            if (c == '-')
                board->ep_square_idx = -1;
            else if (islower(c))
            {
                ep_file = c - 'a';
                ep_rank = *(p+1) - '1';
                board->ep_square_idx = ep_rank * RANKS + ep_file;
            }
        }

        // half move
        if (space_count == 4 && isdigit(c))
            board->halfmove = board->halfmove * 10 + (c - '0');

        // full move
        if (space_count == 5 && isdigit(c))
            board->fullmove = board->fullmove * 10 + (c - '0');
    }
}

piece_t board_at(board_t* board, square_t sq)
{
    return board->squares[square_to_idx(sq)];
}

void board_print(board_t* b)
{
    for (int rank = RANKS - 1; rank >= 0; rank--)
    {
        for (int file = 0; file < FILES; file++)
        {
            int piece = b->squares[rank * RANKS + file];
            printf("%c ", piece_string(piece));
        }
        printf("\n");
    }
}

int gen_king_moves(board_t* board, int sq, move_t* moves)
{
    piece_t p = board->squares[sq];
    assert(piece_type(p) == KING && "invalid piece type for gen_king_moves");
    if (piece_color(p) != board->turn) return 0;

    int count = 0;
    int sq_file = sq % FILES;

    for (size_t i = 0; i < 8; i++)
    {
        int target = sq + king_offsets[i];
        if (target < 0 || target >= FILES * RANKS)
            continue;
        
        if ((target % FILES) - sq_file != king_file_offsets[i])
            continue; // wrap around

        piece_t at = board->squares[target];
        if (piece_color(at) == board->turn)
            continue; // ally piece

        int flag = piece_type(at) == NO_PIECE ? FLAG_QUIET : FLAG_CAPTURE;
        int captured = piece_type(at);
        moves[count++] = MOVE_ENCODE(sq, target, flag, captured);
    }
    return count;
}

int gen_knight_moves(board_t* board, int sq, move_t* moves)
{
    piece_t p = board->squares[sq];
    assert(piece_type(p) == KNIGHT && "invalid piece type for gen_knight_moves");
    if (piece_color(p) != board->turn) return 0;

    int count = 0;
    int sq_file = sq % FILES;

    for (size_t i = 0; i < 8; i++)
    {
        int target = sq + knight_offsets[i];
        if (target < 0 || target >= FILES * RANKS)
            continue;
        
        if ((target % FILES) - sq_file != knight_file_offsets[i])
            continue; // wrap around

        piece_t at = board->squares[target];
        if (piece_color(at) == board->turn)
            continue; // ally piece

        int flag = piece_type(at) == NO_PIECE ? FLAG_QUIET : FLAG_CAPTURE;
        int captured = piece_type(at);
        moves[count++] = MOVE_ENCODE(sq, target, flag, captured);
    }
    return count;
}

int gen_diag_moves(board_t* board, int sq, move_t* moves)
{
    piece_t p = board->squares[sq];
    if (piece_color(p) != board->turn) return 0;
    
    int count = 0;

    for (size_t d = 0; d < 4; d++)
    {
        int curr = sq;
        int curr_file = sq % FILES;

        while (true)
        {
            int next = curr + diag_offsets[d];
            int next_file = next % FILES;

            if (next < 0 || next >= FILES * RANKS)
                break; // out of board

            if (next_file - curr_file != diag_file_offsets[d])
                break; // wrap around

            piece_t at = board->squares[next];
            if (piece_color(at) == board->turn)
                break; // ally piece

            int flag = piece_type(at) == NO_PIECE ? FLAG_QUIET : FLAG_CAPTURE;
            int captured = piece_type(at);
            moves[count++] = MOVE_ENCODE(sq, next, flag, captured);

            if (flag == FLAG_CAPTURE)
                break; // piece captured, end of move

            curr = next;
            curr_file = next_file;
        }

    }
    return count;
}

int gen_orth_moves(board_t* board, int sq, move_t* moves)
{
    piece_t p = board->squares[sq];
    if (piece_color(p) != board->turn) return 0;
    
    int count = 0;

    for (size_t d = 0; d < 4; d++)
    {
        int curr = sq;
        int curr_file = sq % FILES;

        while (true)
        {
            int next = curr + orth_offsets[d];
            int next_file = next % FILES;

            if (next < 0 || next >= FILES * RANKS)
                break; // out of board

            if (next_file - curr_file != orth_file_offsets[d])
                break; // wrap around

            piece_t at = board->squares[next];
            if (piece_color(at) == board->turn)
                break; // ally piece

            int flag = piece_type(at) == NO_PIECE ? FLAG_QUIET : FLAG_CAPTURE;
            int captured = piece_type(at);
            moves[count++] = MOVE_ENCODE(sq, next, flag, captured);

            if (flag == FLAG_CAPTURE)
                break; // piece captured, end of move

            curr = next;
            curr_file = next_file;
        }

    }
    return count;
}

int gen_pawn_moves(board_t* board, int sq, move_t* moves)
{
    piece_t p = board->squares[sq];
    color_t color = piece_color(p);
    if (color != board->turn) return 0;

    int count = 0;

    int offsets[2] = {color == WHITE ? -8 : 8, color == WHITE ? -16 : 16};
    int file_offsets[2] = {color == WHITE ? -1 : 1, color == WHITE ? 1 : -1};

    int sq_file = sq % FILES;

    for (size_t i = 0; i < 2; i++)
    {
        int target = sq + offsets[i];
        printf("target = %d\n", target);

        if (target < 0 || target >= FILES * RANKS)
            continue;
        
        if ((target % FILES) - sq_file != file_offsets[i])
            continue; // wrap around

        piece_t at = board->squares[target];
        if (piece_color(at) == board->turn)
            continue; // ally piece

        int flag = piece_type(at) == NO_PIECE ? FLAG_QUIET : FLAG_CAPTURE;
        int captured = piece_type(at);
        moves[count++] = MOVE_ENCODE(sq, target, flag, captured);
    }
    return count;
}

int board_gen_moves(board_t* board, move_t* moves)
{
    int count = 0;
    
    for (size_t sq = 0; sq < FILES * RANKS; sq++)
    {
        piece_t p = board->squares[sq];
        if (piece_color(p) != board->turn) continue;

        switch (piece_type(p))
        {
            case KING: count += gen_king_moves(board, sq, moves + count); break;
            case KNIGHT: count += gen_knight_moves(board, sq, moves + count); break;
            case BISHOP: count += gen_diag_moves(board, sq, moves + count); break;
            case ROOK: count += gen_orth_moves(board, sq, moves + count); break;
            case QUEEN:
                count += gen_diag_moves(board, sq, moves + count);
                count += gen_orth_moves(board, sq, moves + count);
                break;
            case NO_PIECE:
            default:
                break;
        }
    }
    
    return count;
}

void board_make_move(board_t* board, move_t* move)
{
    // todo
}

void board_unmake_move(board_t* board, move_t* move)
{
    // todo
}

bool is_square_attacked(board_t* board, square_t sq, color_t color)
{
    return (
        can_knight_attack(board, sq, color) ||
        can_orth_attack(board, sq, color) ||
        can_diag_attack(board, sq, color) ||
        can_pawn_attack(board, sq, color) ||
        can_king_attack(board, sq, color)
    );
}

bool can_knight_attack(board_t* board, square_t sq, color_t color)
{
    int idx = square_to_idx(sq);
    int sq_file = sq.file;

    for (size_t i = 0; i < 8; i++)
    {
        int target = idx + knight_offsets[i];
        if (target < 0 || target >= FILES * RANKS)
            continue;
        
        if ((target % FILES) - sq_file != knight_file_offsets[i])
            continue; // wrap around

        piece_t piece = board->squares[target];
        if (piece_type(piece) == KNIGHT && piece_color(piece) == color)
            return true;
    }
    return false;
}

bool can_king_attack(board_t* board, square_t sq, color_t color)
{
    int idx = square_to_idx(sq);
    int sq_file = sq.file;

    for (size_t i = 0; i < 8; i++)
    {
        int target = idx + king_offsets[i];
        if (target < 0 || target >= FILES * RANKS)
            continue;
        
        if ((target % FILES) - sq_file != king_file_offsets[i])
            continue; // wrap around

        piece_t piece = board->squares[target];
        if (piece_type(piece) == KING && piece_color(piece) == color)
            return true;
    }
    return false;
}

bool can_pawn_attack(board_t* board, square_t sq, color_t color)
{
    int offsets[2] = {color == WHITE ? -9 : 9, color == WHITE ? -7 : 7};
    int file_offsets[2] = {color == WHITE ? -1 : 1, color == WHITE ? 1 : -1};

    int idx = square_to_idx(sq);
    int sq_file = sq.file;

    for (size_t i = 0; i < 8; i++)
    {
        int target = idx + offsets[i];
        if (target < 0 || target >= FILES * RANKS)
            continue;
        
        if ((target % FILES) - sq_file != file_offsets[i])
            continue; // wrap around

        piece_t piece = board->squares[target];
        if (piece_type(piece) == PAWN && piece_color(piece) == color)
            return true;
    }
    return false;
}

bool can_diag_attack(board_t* board, square_t sq, color_t color)
{
    int idx = square_to_idx(sq);

    for (int d = 0; d < 4; d++)
    {
        int curr = idx;
        int curr_file = idx % FILES;

        while (true)
        {
            int next = curr + diag_offsets[d];
            int next_file = next % FILES;

            if (next < 0 || next >= FILES * RANKS)
                break; // out of board

            if (next_file - curr_file != diag_file_offsets[d])
                break; // wrap around

            piece_t piece = board->squares[next];
            if (piece_color(piece) == color)
            {
                if (piece_type(piece) == BISHOP || piece_type(piece) == QUEEN)
                    return true;
                break; // ally piece blocks
            }
            if (piece_type(piece) != NO_PIECE) break; // enemy piece blocks

            curr = next;
            curr_file = next_file;
        }
    }
    return false;
}

bool can_orth_attack(board_t* board, square_t sq, color_t color)
{
    int idx = square_to_idx(sq);

    for (int d = 0; d < 4; d++)
    {
        int curr = idx;
        int curr_file = idx % FILES;

        while (true)
        {
            int next = curr + orth_offsets[d];
            int next_file = next % FILES;

            if (next < 0 || next >= FILES * RANKS)
                break; // out of board

            if (next_file - curr_file != orth_file_offsets[d])
                break; // wrap around

            piece_t piece = board->squares[next];
            if (piece_color(piece) == color)
            {
                if (piece_type(piece) == ROOK || piece_type(piece) == QUEEN)
                    return true;
                break; // ally piece blocks
            }
            if (piece_type(piece) != NO_PIECE) break; // enemy piece blocks

            curr = next;
            curr_file = next_file;
        }
    }
    return false;
}