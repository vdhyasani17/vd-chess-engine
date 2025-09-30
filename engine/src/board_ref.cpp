#include <iostream>
#include <vector>
#include <array>
#include "Board.h"

// remove "string.h" from all piece header files

using namespace std;

// encode move
#define encode_move(source, target, piece, promoted, capture, double_push, enpassant, castling) \
    (source) |                                                                                  \
        (target << 6) |                                                                         \
        (piece << 12) |                                                                         \
        (promoted << 16) |                                                                      \
        (capture << 20) |                                                                       \
        (double_push << 21) |                                                                   \
        (enpassant << 22) |                                                                     \
        (castling << 23)

Board::Board(const char *fen)
{
    parse_fen(fen);
    spa = spa->getInstance();
    init_leapers_attacks();
}

// ************ New code for generate and make move ************ //
#define U64 unsigned long long

#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

// parse FEN string
void Board::parse_fen(const char *fen)
{
    // reset board position (bitboards)
    memset(bitboards, 0ULL, sizeof(bitboards));

    // reset occupancies (bitboards)
    memset(occupancies, 0ULL, sizeof(occupancies));

    // reset game state variables
    side = 0;
    enpassant = no_sq;
    castle = 0;

    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over board files
        for (int file = 0; file < 8; file++)
        {
            // init current square
            int square = rank * 8 + file;

            // match ascii pieces within FEN string
            if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z'))
            {
                // init piece type
                int piece = char_pieces[*fen];

                // set piece on corresponding bitboard
                set_bit(bitboards[piece], square);

                // increment pointer to FEN string
                fen++;
            }

            // match empty square numbers within FEN string
            if (*fen >= '0' && *fen <= '9')
            {
                // init offset (convert char 0 to int 0)
                int offset = *fen - '0';

                // define piece variable
                int piece = -1;

                // loop over all piece bitboards
                for (int bb_piece = P; bb_piece <= k; bb_piece++)
                {
                    // if there is a piece on current square
                    if (get_bit(bitboards[bb_piece], square))
                        // get piece code
                        piece = bb_piece;
                }

                // on empty current square
                if (piece == -1)
                    // decrement file
                    file--;

                // adjust file counter
                file += offset;

                // increment pointer to FEN string
                fen++;
            }

            // match rank separator
            if (*fen == '/')
                // increment pointer to FEN string
                fen++;
        }
    }

    // got to parsing side to move (increment pointer to FEN string)
    fen++;

    // parse side to move
    (*fen == 'w') ? (side = white) : (side = black);

    // go to parsing castling rights
    fen += 2;

    // parse castling rights
    while (*fen != ' ')
    {
        switch (*fen)
        {
        case 'K':
            castle |= wk;
            break;
        case 'Q':
            castle |= wq;
            break;
        case 'k':
            castle |= bk;
            break;
        case 'q':
            castle |= bq;
            break;
        case '-':
            break;
        }

        // increment pointer to FEN string
        fen++;
    }

    // got to parsing enpassant square (increment pointer to FEN string)
    fen++;

    // parse enpassant square
    if (*fen != '-')
    {
        // parse enpassant file & rank
        int file = fen[0] - 'a';
        int rank = 8 - (fen[1] - '0');

        // init enpassant square
        enpassant = rank * 8 + file;
    }

    // no enpassant square
    else
        enpassant = no_sq;

    // loop over white pieces bitboards
    for (int piece = P; piece <= K; piece++)
        // populate white occupancy bitboard
        occupancies[white] |= bitboards[piece];

    // loop over black pieces bitboards
    for (int piece = p; piece <= k; piece++)
        // populate white occupancy bitboard
        occupancies[black] |= bitboards[piece];

    // init all occupancies
    occupancies[both] |= occupancies[white];
    occupancies[both] |= occupancies[black];
}

// count bits within a bitboard (Brian Kernighan's way)
inline int Board::count_bits(uint64_t bitboard)
{
    // bit counter
    int count = 0;

    // consecutively reset least significant 1st bit
    while (bitboard)
    {
        // increment count
        count++;

        // reset least significant 1st bit
        bitboard &= bitboard - 1;
    }

    // return bit count
    return count;
}

// get least significant 1st bit index
inline int Board::get_ls1b_index(uint64_t bitboard)
{
    // make sure bitboard is not 0
    if (bitboard)
    {
        // count trailing bits before LS1B
        return count_bits((bitboard & -bitboard) - 1);
    }

    // otherwise
    else
        // return illegal index
        return -1;
}

// add move to the move list
inline void Board::add_move(moves *move_list, int move)
{
    // strore move
    move_list->moves[move_list->count] = move;

    // increment move count
    move_list->count++;
}

// TODO: have to populate attack arrays for each piece
// is square current given attacked by the current given side
inline int Board::is_square_attacked(int square, int side)
{
    // attacked by white pawns
    if ((side == white) && (pawn_attacks[black][square] & bitboards[P]))
        return 1;

    // attacked by black pawns
    if ((side == black) && (pawn_attacks[white][square] & bitboards[p]))
        return 1;

    // attacked by knights
    if (knight_attacks[square] & ((side == white) ? bitboards[N] : bitboards[n]))
        return 1;

    // attacked by bishops
    if (spa->bishop_attack_bitboard(square, occupancies[both]) & ((side == white) ? bitboards[B] : bitboards[b]))
        return 1;

    // attacked by rooks
    if (spa->rook_attack_bitboard(square, occupancies[both]) & ((side == white) ? bitboards[R] : bitboards[r]))
        return 1;

    // attacked by queen
    if ((spa->bishop_attack_bitboard(square, occupancies[both]) | spa->rook_attack_bitboard(square, occupancies[both])) & ((side == white) ? bitboards[Q] : bitboards[q]))
        return 1;

    // attacked by kings
    if (king_attacks[square] & ((side == white) ? bitboards[K] : bitboards[k]))
        return 1;

    // by default return false
    return 0;
}

// generate knight attacks
uint64_t Board::mask_knight_attacks(int square)
{
    // result attacks bitboard
    uint64_t attacks = 0ULL;

    // piece bitboard
    uint64_t bitboard = 0ULL;

    // set piece on board
    set_bit(bitboard, square);

    // generate knight attacks
    if ((bitboard >> 17) & not_h_file)
        attacks |= (bitboard >> 17);
    if ((bitboard >> 15) & not_a_file)
        attacks |= (bitboard >> 15);
    if ((bitboard >> 10) & not_hg_file)
        attacks |= (bitboard >> 10);
    if ((bitboard >> 6) & not_ab_file)
        attacks |= (bitboard >> 6);
    if ((bitboard << 17) & not_a_file)
        attacks |= (bitboard << 17);
    if ((bitboard << 15) & not_h_file)
        attacks |= (bitboard << 15);
    if ((bitboard << 10) & not_ab_file)
        attacks |= (bitboard << 10);
    if ((bitboard << 6) & not_hg_file)
        attacks |= (bitboard << 6);

    // return attack map
    return attacks;
}

// generate king attacks
uint64_t Board::mask_king_attacks(int square)
{
    // result attacks bitboard
    uint64_t attacks = 0ULL;

    // piece bitboard
    uint64_t bitboard = 0ULL;

    // set piece on board
    set_bit(bitboard, square);

    // generate king attacks
    if (bitboard >> 8)
        attacks |= (bitboard >> 8);
    if ((bitboard >> 9) & not_h_file)
        attacks |= (bitboard >> 9);
    if ((bitboard >> 7) & not_a_file)
        attacks |= (bitboard >> 7);
    if ((bitboard >> 1) & not_h_file)
        attacks |= (bitboard >> 1);
    if (bitboard << 8)
        attacks |= (bitboard << 8);
    if ((bitboard << 9) & not_a_file)
        attacks |= (bitboard << 9);
    if ((bitboard << 7) & not_h_file)
        attacks |= (bitboard << 7);
    if ((bitboard << 1) & not_a_file)
        attacks |= (bitboard << 1);

    // return attack map
    return attacks;
}

// generate pawn attacks
uint64_t Board::mask_pawn_attacks(int side, int square)
{
    // result attacks bitboard
    uint64_t attacks = 0ULL;

    // piece bitboard
    uint64_t bitboard = 0ULL;

    // set piece on board
    set_bit(bitboard, square);

    // white pawns
    if (!side)
    {
        // generate pawn attacks
        if ((bitboard >> 7) & not_a_file)
            attacks |= (bitboard >> 7);
        if ((bitboard >> 9) & not_h_file)
            attacks |= (bitboard >> 9);
    }

    // black pawns
    else
    {
        // generate pawn attacks
        if ((bitboard << 7) & not_h_file)
            attacks |= (bitboard << 7);
        if ((bitboard << 9) & not_a_file)
            attacks |= (bitboard << 9);
    }

    // return attack map
    return attacks;
}

// init leaper pieces attacks
void Board::init_leapers_attacks()
{
    // loop over 64 board squares
    for (int square = 0; square < 64; square++)
    {
        // init pawn attacks
        pawn_attacks[white][square] = mask_pawn_attacks(white, square);
        pawn_attacks[black][square] = mask_pawn_attacks(black, square);

        // init knight attacks
        knight_attacks[square] = mask_knight_attacks(square);

        // init king attacks
        king_attacks[square] = mask_king_attacks(square);
    }
}

// extract source square
#define get_move_source(move) (move & 0x3f)

// extract target square
#define get_move_target(move) ((move & 0xfc0) >> 6)

// extract piece
#define get_move_piece(move) ((move & 0xf000) >> 12)

// extract promoted piece
#define get_move_promoted(move) ((move & 0xf0000) >> 16)

// extract capture flag
#define get_move_capture(move) (move & 0x100000)

// extract double pawn push flag
#define get_move_double(move) (move & 0x200000)

// extract enpassant flag
#define get_move_enpassant(move) (move & 0x400000)

// extract castling flag
#define get_move_castling(move) (move & 0x800000);

// preserve board state
#define copy_board()                             \
    U64 bitboards_copy[12], occupancies_copy[3]; \
    int side_copy, enpassant_copy, castle_copy;  \
    memcpy(bitboards_copy, bitboards, 96);       \
    memcpy(occupancies_copy, occupancies, 24);   \
    side_copy = side, enpassant_copy = enpassant, castle_copy = castle;

// restore board state
#define take_back()                            \
    memcpy(bitboards, bitboards_copy, 96);     \
    memcpy(occupancies, occupancies_copy, 24); \
    side = side_copy, enpassant = enpassant_copy, castle = castle_copy;

// make move on chess board
// make move on chess board
int Board::make_move(int move, int move_flag)
{
    // quiet moves
    if (move_flag == all_moves)
    {
        // preserve board state
        copy_board();

        // parse move
        int source_square = get_move_source(move);
        int target_square = get_move_target(move);
        int piece = get_move_piece(move);
        int promoted_piece = get_move_promoted(move);
        int capture = get_move_capture(move);
        int double_push = get_move_double(move);
        int enpass = get_move_enpassant(move);
        int castling = get_move_castling(move);

        // move piece
        pop_bit(bitboards[piece], source_square);
        set_bit(bitboards[piece], target_square);

        // handling capture moves
        if (capture)
        {
            // pick up bitboard piece index ranges depending on side
            int start_piece, end_piece;

            // white to move
            if (side == white)
            {
                start_piece = p;
                end_piece = k;
            }

            // black to move
            else
            {
                start_piece = P;
                end_piece = K;
            }

            // loop over bitboards opposite to the current side to move
            for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++)
            {
                // if there's a piece on the target square
                if (get_bit(bitboards[bb_piece], target_square))
                {
                    // remove it from corresponding bitboard
                    pop_bit(bitboards[bb_piece], target_square);
                    break;
                }
            }
        }

        // handle pawn promotions
        if (promoted_piece)
        {
            // erase the pawn from the target square
            pop_bit(bitboards[(side == white) ? P : p], target_square);

            // set up promoted piece on chess board
            set_bit(bitboards[promoted_piece], target_square);
        }

        // handle enpassant captures
        if (enpass)
        {
            // erase the pawn depending on side to move
            (side == white) ? pop_bit(bitboards[p], target_square + 8) : pop_bit(bitboards[P], target_square - 8);
        }

        // reset enpassant square
        enpassant = no_sq;

        // handle double pawn push
        if (double_push)
        {
            // set enpassant aquare depending on side to move
            (side == white) ? (enpassant = target_square + 8) : (enpassant = target_square - 8);
        }

        // handle castling moves
        if (castling)
        {
            // switch target square
            switch (target_square)
            {
            // white castles king side
            case (g1):
                // move H rook
                pop_bit(bitboards[R], h1);
                set_bit(bitboards[R], f1);
                break;

                // white castles queen side
            case (c1):
                // move A rook
                pop_bit(bitboards[R], a1);
                set_bit(bitboards[R], d1);
                break;

                // black castles king side
            case (g8):
                // move H rook
                pop_bit(bitboards[r], h8);
                set_bit(bitboards[r], f8);
                break;

                // black castles queen side
            case (c8):
                // move A rook
                pop_bit(bitboards[r], a8);
                set_bit(bitboards[r], d8);
                break;
            }
        }

        // update castling rights
        castle &= castling_rights[source_square];
        castle &= castling_rights[target_square];

        // reset occupancies
        memset(occupancies, 0ULL, 24);

        // loop over white pieces bitboards
        for (int bb_piece = P; bb_piece <= K; bb_piece++)
            // update white occupancies
            occupancies[white] |= bitboards[bb_piece];

        // loop over black pieces bitboards
        for (int bb_piece = p; bb_piece <= k; bb_piece++)
            // update black occupancies
            occupancies[black] |= bitboards[bb_piece];

        // update both sides occupancies
        occupancies[both] |= occupancies[white];
        occupancies[both] |= occupancies[black];

        // change side
        side ^= 1;

        // make sure that king has not been exposed into a check
        if (is_square_attacked((side == white) ? get_ls1b_index(bitboards[k]) : get_ls1b_index(bitboards[K]), side))
        {
            // take move back
            take_back();

            // return illegal move
            return 0;
        }

        //
        else
            // return legal move
            return 1;
    }

    // capture moves
    else
    {
        // make sure move is the capture
        if (get_move_capture(move))
            make_move(move, all_moves);

        // otherwise the move is not a capture
        else
            // don't make it
            return 0;
    }
}

// generate all moves
inline void Board::generate_moves(moves *move_list)
{
    // init move count
    move_list->count = 0;

    // define source & target squares
    int source_square, target_square;

    // define current piece's bitboard copy & it's attacks
    uint64_t bitboard, attacks;

    // loop over all the bitboards
    for (int piece = P; piece <= k; piece++)
    {
        // init piece bitboard copy
        bitboard = bitboards[piece];

        // generate white pawns & white king castling moves
        if (side == white)
        {
            // pick up white pawn bitboards index
            if (piece == P)
            {
                // loop over white pawns within white pawn bitboard
                while (bitboard)
                {
                    // init source square
                    source_square = get_ls1b_index(bitboard);

                    // init target square
                    target_square = source_square - 8;

                    // generate quite pawn moves
                    if (!(target_square < a8) && !get_bit(occupancies[both], target_square))
                    {
                        // pawn promotion
                        if (source_square >= a7 && source_square <= h7)
                        {
                            add_move(move_list, encode_move(source_square, target_square, piece, Q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, R, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, B, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, N, 0, 0, 0, 0));
                        }

                        else
                        {
                            // one square ahead pawn move
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                            // two squares ahead pawn move
                            if ((source_square >= a2 && source_square <= h2) && !get_bit(occupancies[both], target_square - 8))
                                add_move(move_list, encode_move(source_square, target_square - 8, piece, 0, 0, 1, 0, 0));
                        }
                    }

                    // init pawn attacks bitboard
                    attacks = pawn_attacks[side][source_square] & occupancies[black];

                    // generate pawn captures
                    while (attacks)
                    {
                        // init target square
                        target_square = get_ls1b_index(attacks);

                        // pawn promotion
                        if (source_square >= a7 && source_square <= h7)
                        {
                            add_move(move_list, encode_move(source_square, target_square, piece, Q, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, R, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, B, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, N, 1, 0, 0, 0));
                        }

                        else
                            // one square ahead pawn move
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                        // pop ls1b of the pawn attacks
                        pop_bit(attacks, target_square);
                    }

                    // generate enpassant captures
                    if (enpassant != no_sq)
                    {
                        // lookup pawn attacks and bitwise AND with enpassant square (bit)
                        uint64_t enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

                        // make sure enpassant capture available
                        if (enpassant_attacks)
                        {
                            // init enpassant capture target square
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    // pop ls1b from piece bitboard copy
                    pop_bit(bitboard, source_square);
                }
            }

            // castling moves
            if (piece == K)
            {
                // king side castling is available
                if (castle & wk)
                {
                    // make sure square between king and king's rook are empty
                    if (!get_bit(occupancies[both], f1) && !get_bit(occupancies[both], g1))
                    {
                        // make sure king and the f1 squares are not under attacks
                        if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black))
                            add_move(move_list, encode_move(e1, g1, piece, 0, 0, 0, 0, 1));
                    }
                }

                // queen side castling is available
                if (castle & wq)
                {
                    // make sure square between king and queen's rook are empty
                    if (!get_bit(occupancies[both], d1) && !get_bit(occupancies[both], c1) && !get_bit(occupancies[both], b1))
                    {
                        // make sure king and the d1 squares are not under attacks
                        if (!is_square_attacked(e1, black) && !is_square_attacked(d1, black))
                            add_move(move_list, encode_move(e1, c1, piece, 0, 0, 0, 0, 1));
                    }
                }
            }
        }

        // generate black pawns & black king castling moves
        else
        {
            // pick up black pawn bitboards index
            if (piece == p)
            {
                // loop over white pawns within white pawn bitboard
                while (bitboard)
                {
                    // init source square
                    source_square = get_ls1b_index(bitboard);

                    // init target square
                    target_square = source_square + 8;

                    // generate quite pawn moves
                    if (!(target_square > h1) && !get_bit(occupancies[both], target_square))
                    {
                        // pawn promotion
                        if (source_square >= a2 && source_square <= h2)
                        {
                            add_move(move_list, encode_move(source_square, target_square, piece, q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, r, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, b, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, n, 0, 0, 0, 0));
                        }

                        else
                        {
                            // one square ahead pawn move
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                            // two squares ahead pawn move
                            if ((source_square >= a7 && source_square <= h7) && !get_bit(occupancies[both], target_square + 8))
                                add_move(move_list, encode_move(source_square, target_square + 8, piece, 0, 0, 1, 0, 0));
                        }
                    }

                    // init pawn attacks bitboard
                    attacks = pawn_attacks[side][source_square] & occupancies[white];

                    // generate pawn captures
                    while (attacks)
                    {
                        // init target square
                        target_square = get_ls1b_index(attacks);

                        // pawn promotion
                        if (source_square >= a2 && source_square <= h2)
                        {
                            add_move(move_list, encode_move(source_square, target_square, piece, q, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, r, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, b, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, n, 1, 0, 0, 0));
                        }

                        else
                            // one square ahead pawn move
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                        // pop ls1b of the pawn attacks
                        pop_bit(attacks, target_square);
                    }

                    // generate enpassant captures
                    if (enpassant != no_sq)
                    {
                        // lookup pawn attacks and bitwise AND with enpassant square (bit)
                        uint64_t enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

                        // make sure enpassant capture available
                        if (enpassant_attacks)
                        {
                            // init enpassant capture target square
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    // pop ls1b from piece bitboard copy
                    pop_bit(bitboard, source_square);
                }
            }

            // castling moves
            if (piece == k)
            {
                // king side castling is available
                if (castle & bk)
                {
                    // make sure square between king and king's rook are empty
                    if (!get_bit(occupancies[both], f8) && !get_bit(occupancies[both], g8))
                    {
                        // make sure king and the f8 squares are not under attacks
                        if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white))
                            add_move(move_list, encode_move(e8, g8, piece, 0, 0, 0, 0, 1));
                    }
                }

                // queen side castling is available
                if (castle & bq)
                {
                    // make sure square between king and queen's rook are empty
                    if (!get_bit(occupancies[both], d8) && !get_bit(occupancies[both], c8) && !get_bit(occupancies[both], b8))
                    {
                        // make sure king and the d8 squares are not under attacks
                        if (!is_square_attacked(e8, white) && !is_square_attacked(d8, white))
                            add_move(move_list, encode_move(e8, c8, piece, 0, 0, 0, 0, 1));
                    }
                }
            }
        }

        // genarate knight moves
        if ((side == white) ? piece == N : piece == n)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = knight_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = get_ls1b_index(attacks);

                    // quite move
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate bishop moves
        if ((side == white) ? piece == B : piece == b)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                uint64_t bishop_attacks = spa->bishop_attack_bitboard(source_square, occupancies[both]);
                attacks = spa->bishop_attack_bitboard(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = get_ls1b_index(attacks);

                    // quite move
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate rook moves
        if ((side == white) ? piece == R : piece == r)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = spa->rook_attack_bitboard(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = get_ls1b_index(attacks);

                    // quite move
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate queen moves
        if ((side == white) ? piece == Q : piece == q)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = (spa->bishop_attack_bitboard(source_square, occupancies[both]) |
                           spa->rook_attack_bitboard(source_square, occupancies[both])) &
                          ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = get_ls1b_index(attacks);

                    // quite move
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate king moves
        if ((side == white) ? piece == K : piece == k)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = king_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = get_ls1b_index(attacks);

                    // quite move
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }
    }
}

// perft driver
U64 Board::perft_driver(int depth, int orig_depth)
{
    // reccursion escape condition
    if (depth == 0)
    {
        // increment nodes count (count reached positions)
        return ++num_nodes;
    }

    // create move list instance
    moves move_list[1];

    // generate moves
    generate_moves(move_list);

    // loop over generated moves
    for (int move_count = 0; move_count < move_list->count; move_count++)
    {
        // preserve board state
        copy_board();

        // int capture = get_move_capture(move_list->moves[move_count]);
        if (depth == orig_depth)
        {
            print_load_bar(move_list->count, move_count);
        }

        // make move
        if (!make_move(move_list->moves[move_count], all_moves))
            // skip to the next move
            continue;

        // call perft driver recursively
        perft_driver(depth - 1, depth);

        // take back
        take_back();
    }

    return num_nodes;
}

void Board::print_load_bar(int sizeOfMoves, int currentIteration)
{
    cout.flush();  // Ensure the output is written to the console
    cout << "\r["; // Move cursor back to the start of the line
    for (int i = 0; i <= currentIteration; i++)
    {
        cout << "#"; // Progress
    }
    for (int i = 0; i < sizeOfMoves - currentIteration - 1; i++)
    {
        cout << "."; // Remaining
    }
    cout << "]"; // End of the progress bar
    cout << " " << ((double)(currentIteration + 1) / sizeOfMoves) * 100 << "%";
}

void Board::print_board()
{
    for (int i = 0; i < 8; i++)
    {
        cout << "+";
        for (int j = 0; j < 8; j++)
        {
            cout << "-----+";
        }
        cout << endl;

        cout << "|";
        for (int j = 0; j < 8; j++)
        {
            if (1ULL << j + (i * 8) & occupancies[both])
            {
                if (1ULL << j + (i * 8) & bitboards[P])
                {
                    cout << "  " << 'P' << "  |";
                }
                else if (1ULL << j + (i * 8) & bitboards[N])
                {
                    cout << "  " << 'N' << "  |";
                }
                else if (1ULL << j + (i * 8) & bitboards[B])
                {
                    cout << "  " << 'B' << "  |";
                }
                else if (1ULL << j + (i * 8) & bitboards[R])
                {
                    cout << "  " << 'R' << "  |";
                }
                else if (1ULL << j + (i * 8) & bitboards[Q])
                {
                    cout << "  " << 'Q' << "  |";
                }
                else if (1ULL << j + (i * 8) & bitboards[K])
                {
                    cout << "  " << 'K' << "  |";
                }

                else if (1ULL << j + (i * 8) & bitboards[p])
                {
                    cout << "  " << 'p' << "  |";
                }
                else if (1ULL << j + (i * 8) & bitboards[n])
                {
                    cout << "  " << 'n' << "  |";
                }
                else if (1ULL << j + (i * 8) & bitboards[b])
                {
                    cout << "  " << 'b' << "  |";
                }
                else if (1ULL << j + (i * 8) & bitboards[r])
                {
                    cout << "  " << 'r' << "  |";
                }
                else if (1ULL << j + (i * 8) & bitboards[q])
                {
                    cout << "  " << 'q' << "  |";
                }
                else if (1ULL << j + (i * 8) & bitboards[k])
                {
                    cout << "  " << 'k' << "  |";
                }
            }
            else
            {
                cout << "  " << ' ' << "  |";
            }
        }
        cout << " " << (8 - i);
        cout << endl;
    }
    cout << "+";
    for (int j = 0; j < 8; j++)
    {
        cout << "-----+";
    }
    cout << endl;
    cout << "   " << 'a';
    for (char file = 'b'; file <= 'h'; file++)
    {
        cout << "     " << file;
    }
}

/*
void Board::printGameBoard()
{
    for (int i = 0; i < 8; i++)
    {
        cout << "+";
        for (int j = 0; j < 8; j++)
        {
            cout << "-----+";
        }
        cout << endl;

        cout << "|";
        for (int j = 0; j < 8; j++)
        {
            if (gameBoard[j + (i * 8)] >= 'a')
            {
                cout << "  "
                     << "\033[38;5;173m" << gameBoard[j + (i * 8)] << "\033[0m"
                     << "  |"; // Brown
            }
            else
            {
                cout << "  "
                     << "\033[38;5;220m" << gameBoard[j + (i * 8)] << "\033[0m"
                     << "  |"; // Yellow
            }
        }
        cout << " " << (8 - i);
        cout << endl;
    }
    cout << "+";
    for (int j = 0; j < 8; j++)
    {
        cout << "-----+";
    }
    cout << endl;
    cout << "   " << files[0];
    for (int i = 1; i < sizeof(files); i++)
    {
        cout << "     " << files[i];
    }
    cout << endl;
}
*/

bool Board::get_side()
{
    return side;
}

int Board::get_move(int source_square, int target_square)
{
    moves movelist[1];
    generate_moves(movelist);
    for (int move_count = 0; move_count < movelist->count; move_count++)
    {
        if (source_square == get_move_source(movelist->moves[move_count]) && target_square == get_move_target(movelist->moves[move_count]))
        {
            return movelist->moves[move_count]; // if move is in the list of generated moves
        }
    }
    return 0;
}
