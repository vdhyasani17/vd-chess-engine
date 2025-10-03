#include "../include/board.hpp"
#include <stack>   // for std::stack
#include <cstring> // for memset
#include <cstdint> // for uint64_t

using namespace std;

void Board::set_square(int i, int type)
{
    squares[i] = type;
}

Color Board::get_side()
{
    return side_to_move;
}

void Board::load_fen(string fen)
{
    int i = 0; // board index
    int j = 0;
    for (j = 0; i < 64 && fen[j] != ' '; ++j)
    {
        if (fen[j] == '/')
        {
            continue; // skip rank separator
        }
        else if (isdigit(fen[j]))
        {
            int num = fen[j] - '0';
            for (int k = 0; k < num; ++k)
            {
                set_square(i++, 0); // empty squares
            }
        }
        else
        {
            Color col = fen[j] >= 'A' && fen[j] <= 'Z' ? WHITE : BLACK;
            pieces[col][type_of(toupper(fen[j]))] ^= one_bit << i;
            blockers[col] ^= one_bit << i;
            set_square(i++, type_of(toupper(fen[j])));
        }
    }
    j++;
    if (fen[j] == 'w' || fen[j] == 'b')
    {
        side_to_move = fen[j] == 'w' ? WHITE : BLACK;
    }

    j++;
    // 3. Castling Rights (Part 3 of FEN) - SKIPPING for now
    while (j < fen.length() && fen[j] == ' ')
    {
        j++;
    }
    while (j < fen.length() && fen[j] != ' ')
    {
        j++;
    } // Skip the castling rights field

    // 4. En Passant Square (Part 4 of FEN)
    while (j < fen.length() && fen[j] == ' ')
    {
        j++;
    }

    if (j < fen.length())
    {
        if (fen[j] == '-')
        {
            enpassant_square = 0ULL;
        }
        else
        {
            // The square is two characters long (e.g., "e3")
            std::string sq_str = fen.substr(j, 2);
            int fileValue = sq_str[0] - 'a';       // 'a' → 0, 'b' → 1, ..., 'h' → 7
            int rankValue = 8 - (sq_str[1] - '0'); // '1' → 7, '8' → 0
            Square sq_index = (Square)(8 * rankValue + fileValue);
            cout << sq_str[0] << endl;

            // Store the en passant square as a single bitboard
            // Assuming enpassant_square is a u64 variable.
            enpassant_square = (1ULL << sq_index);
        }
    }
    AttackTables::init();
}

int Board::type_of(char c)
{
    // 8 = 1000, which represents color bit
    switch (c)
    {
    case 'P':
        return PAWN;
    case 'N':
        return KNIGHT;
    case 'B':
        return BISHOP;
    case 'R':
        return ROOK;
    case 'Q':
        return QUEEN;
    case 'K':
        return KING;
    default:
        return 0; // if no piece, then encode with 0
    }
}

void Board::print()
{
    // Map your encoded piece values to characters
    auto piece_char = [](int val) -> char
    {
        switch (val)
        {
        case 0:
            return ' '; // empty
        case 1:
            return 'P';
        case 2:
            return 'N';
        case 3:
            return 'B';
        case 4:
            return 'R';
        case 5:
            return 'Q';
        case 6:
            return 'K';
        case 9:
            return 'p';
        case 10:
            return 'n';
        case 11:
            return 'b';
        case 12:
            return 'r';
        case 13:
            return 'q';
        case 14:
            return 'k';
        default:
            return '?';
        }
    };

    for (int i = 0; i < 8; i++)
    {
        cout << "+";
        for (int j = 0; j < 8; j++)
            cout << "---+";
        cout << endl;

        cout << "|";
        for (int j = 0; j < 8; j++)
        {
            Color color = (1ULL << ((8 * i) + j)) & blockers[WHITE] ? WHITE : BLACK;
            char pc = piece_types[color][get_piece_at_square((Square)((8 * i) + j)) & 0b111];
            cout << " " << pc << " |";
        }
        cout << " " << (8 - i) << endl;
    }

    cout << "+";
    for (int j = 0; j < 8; j++)
        cout << "---+";
    cout << endl;

    cout << "  a   b   c   d   e   f   g   h" << endl;
}

int Board::get_piece_at_square(Square sq)
{
    u64 bitboard = 1ULL << sq;
    if (blockers[WHITE] & bitboard)
    {
        if (pieces[WHITE][PAWN] & bitboard)
        {
            return PAWN;
        }
        else if (pieces[WHITE][KNIGHT] & bitboard)
        {
            return KNIGHT;
        }
        else if (pieces[WHITE][BISHOP] & bitboard)
        {
            return BISHOP;
        }
        else if (pieces[WHITE][ROOK] & bitboard)
        {
            return ROOK;
        }
        else if (pieces[WHITE][QUEEN] & bitboard)
        {
            return QUEEN;
        }
        else if (pieces[WHITE][KING] & bitboard)
        {
            return KING;
        }
    }
    else if (blockers[BLACK] & bitboard)
    {
        if (pieces[BLACK][PAWN] & bitboard)
        {
            return 8 + PAWN;
        }
        else if (pieces[BLACK][KNIGHT] & bitboard)
        {
            return 8 + KNIGHT;
        }
        else if (pieces[BLACK][BISHOP] & bitboard)
        {
            return 8 + BISHOP;
        }
        else if (pieces[BLACK][ROOK] & bitboard)
        {
            return 8 + ROOK;
        }
        else if (pieces[BLACK][QUEEN] & bitboard)
        {
            return 8 + QUEEN;
        }
        else if ((pieces[WHITE][KING] | pieces[BLACK][KING]) & bitboard)
        {
            return 8 + KING;
        }
    }
    return NO_PIECE;
}

/*
 * Move encoding:
 *  0-5 -> start square
 *  6-11 -> target square
 *  12-14 -> moving piece
 *  15-17 -> captured piece (could be 0)
 *  18-20 -> promotion piece (could be 0)
 *  21-23 -> special flags (could be 0, indicating no special move)
 *
 *  (white pawn double pushed from d2-d4)
 *  001 000 000 001 100011 110011
 *
 *  Special moves:
 *      000 -> no special move
 *      001 -> double pawn push
 *      010 -> enpassant capture
 *      011 -> kingside castle
 *      100 -> queenside castle
 */

// change return type to void later
bool Board::make_move(Square start, Square target, Color turn, vector<int> legal_moves)
{
    int valid_move = 0;

    for (int move : legal_moves)
    {
        int from = move & 0x3F;      // bits 0-5
        int to = (move >> 6) & 0x3F; // bits 6-11
        if (from == static_cast<int>(start) && to == static_cast<int>(target))
        {
            valid_move = move;
            break;
        }
    }
    if (valid_move == 0)
        return false;

    PieceType piece_on_start = (PieceType)squares[start];
    PieceType pt = (PieceType)((valid_move & (0b111 << 12)) >> 12);

    if ((1ULL << target) & blockers[!turn])
    {
        // Get the captured piece's type using a helper function or bitmask
        PieceType captured_piece_type = (PieceType)(get_piece_at_square(target) & 0b111);

        valid_move |= (captured_piece_type << 15);

        // Remove the captured piece from the opponent's bitboards
        pieces[!turn][captured_piece_type] &= ~(1ULL << target);
        blockers[!turn] &= ~(1ULL << target);
    }

    // Remove the piece from its starting square
    pieces[turn][pt] &= ~(1ULL << start);
    blockers[turn] &= ~(1ULL << start);

    // Place the piece on the target square
    pieces[turn][pt] |= 1ULL << target;
    blockers[turn] |= 1ULL << target;

    // handle special moves
    int special_moves_flag = (valid_move & (0b111 << 21)) >> 21;

    // cout << "special_moves_flag: " << special_moves_flag << endl;
    if (special_moves_flag == 2) // handle enpassant capture
    {
        int enpassant_capture = turn == WHITE ? target + 8 : target - 8;
        pieces[!turn][PAWN] &= ~(1ULL << enpassant_capture);
        blockers[!turn] &= ~(1ULL << enpassant_capture);
        valid_move |= (PAWN << 15); // put PAWN as captured piece
    }
    else if ((valid_move & (0b111 << 18)) >> 18) // handle promotion
    {
        pieces[turn][PAWN] &= ~(1ULL << target);                              // remove pawn from promotion square
        pieces[turn][(valid_move & (0b111 << 18)) >> 18] |= (1ULL << target); // add promotion piece
    }
    // handle rest later

    side_to_move = side_to_move == WHITE ? BLACK : WHITE;

    // print_move_encoding("move", valid_move);
    //  push move to move stack
    move_stack.push(valid_move);

    return true;
}

bool Board::unmake_move()
{
    if (move_stack.size() == 0)
    {
        return false;
    }

    int move = move_stack.top();

    Square start = (Square)(move & 0x3f);
    Square target = (Square)((move & 0xfc0) >> 6);
    PieceType moved_piece = (PieceType)(get_piece_at_square(target) & 0b111);
    PieceType captured_piece = (PieceType)((move & (0b111 << 15)) >> 15);
    PieceType promoted_piece = (PieceType)((move & (0b111 << 18)) >> 18);
    int special_moves_flag = (move & (0b111 << 21)) >> 21;

    // remove piece that just moved from target square
    pieces[!side_to_move][moved_piece] &= ~(1ULL << target);
    blockers[!side_to_move] &= ~(1ULL << target);

    // re-add piece that just moved to start square
    pieces[!side_to_move][moved_piece] |= (1ULL << start);
    blockers[!side_to_move] |= (1ULL << start);

    if (special_moves_flag == 2) // if last move was an enpassant capture
    {
        target = (Square)(side_to_move == WHITE ? target - 8 : target + 8); // change target square to be where captured pawn should be re-added
    }

    // add captured piece back to bitboard(s) if there is one
    if (captured_piece)
    {
        pieces[side_to_move][captured_piece] |= (1ULL << target);
        blockers[side_to_move] |= (1ULL << target);
    }

    if (promoted_piece) // if last move was promotion
    {
        pieces[!side_to_move][promoted_piece] &= ~(1ULL << start); // remove promoted piece bit from its bitboard
        pieces[!side_to_move][PAWN] |= (1ULL << start);            // add pawn back to board
    }

    move_stack.pop();

    side_to_move = side_to_move == WHITE ? BLACK : WHITE;

    return true;
}

/*
 * Move encoding:
 *  0-5 -> start square
 *  6-11 -> target square
 *  12-14 -> moving piece
 *  15-17 -> captured piece (could be 0)
 *  18-20 -> promotion piece (could be 0)
 *  21-23 -> special flags (could be 0, indicating no special move)
 *
 *  000 000 000 000 000000 000000
 *
 *  Special moves:
 *      000 -> no special move
 *      001 -> double pawn push
 *      010 -> enpassant capture
 *      011 -> kingside castle
 *      100 -> queenside castle
 */

vector<int> Board::generate_legal_moves(Color color)
{
    vector<int> legal_moves;
    u64 bitboard;
    u64 blockers_all = blockers[WHITE] | blockers[BLACK];
    u64 empty = ~blockers_all;
    Color opposite_color = color == WHITE ? BLACK : WHITE;

    /*** generate check mask and if there is single/double check ***/
    u64 checkmask = 0, check_square = 0;
    bool double_check = false;
    Square king_square = (Square)__builtin_ctzll(pieces[color][KING]);

    checkmask |= ((AttackTables::pawn_attacks(color, king_square) & pieces[opposite_color][PAWN]) ? AttackTables::pawn_attacks(color, king_square) & pieces[opposite_color][PAWN] : 0); // if pawn attacking current king

    checkmask |= ((AttackTables::knight_attacks(king_square) & pieces[opposite_color][KNIGHT]) ? AttackTables::knight_attacks(king_square) & pieces[opposite_color][KNIGHT] : 0); // if knight attacking current king

    check_square = AttackTables::bishop_attacks(king_square, blockers_all) & pieces[opposite_color][BISHOP];
    checkmask |= (check_square ? AttackTables::bishop_attacks(king_square, blockers_all) & AttackTables::bishop_attacks((Square)__builtin_ctzll(check_square), blockers_all) | check_square : 0); // if bishop attacking current king

    check_square = AttackTables::rook_attacks(king_square, blockers_all) & pieces[opposite_color][ROOK];
    checkmask |= (check_square ? AttackTables::rook_attacks(king_square, blockers_all) & AttackTables::rook_attacks((Square)__builtin_ctzll(check_square), blockers_all) | check_square : 0); // if rook attacking current king

    check_square = AttackTables::bishop_attacks(king_square, blockers_all) & pieces[opposite_color][QUEEN];
    checkmask |= check_square ? AttackTables::bishop_attacks(king_square, blockers_all) & AttackTables::bishop_attacks((Square)__builtin_ctzll(check_square), blockers_all) | check_square : 0; // if queen attacking current king

    check_square = AttackTables::rook_attacks(king_square, blockers_all) & pieces[opposite_color][QUEEN];
    checkmask |= check_square ? AttackTables::rook_attacks(king_square, blockers_all) & AttackTables::rook_attacks((Square)__builtin_ctzll(check_square), blockers_all) | check_square : 0; // if queen attacking current king

    if (__builtin_popcountll(checkmask & blockers[opposite_color]) > 1) // if double check
    {
        double_check = true;
    }
    checkmask = checkmask == 0 ? ~0 : checkmask; // toggle checkmask to be all 1's if no checks

    // print_bitboard("checkmask", checkmask);

    // generate king legal moves
    bitboard = pieces[color][KING];
    while (bitboard)
    {
        int start = __builtin_ctzll(bitboard);
        u64 enemy_attacks = get_attacks(color == WHITE ? BLACK : WHITE);
        // handle castling
        bool king_in_check = (enemy_attacks & (1ULL << king_square)) != 0;

        // 2. King-side Castling Check
        bool kingside_clear = (castle_masks[color][KINGSIDE] & blockers_all) == 0;
        bool kingside_safe = (castle_masks[color][KINGSIDE] & enemy_attacks) == 0;

        cout << "king in check: " << king_in_check << endl;
        cout << "kingside safe: " << kingside_safe << endl;
        cout << "kingside clear: " << kingside_clear << endl;
        bool can_kingside_castle = !king_in_check && kingside_clear && kingside_safe;

        // 3. Queen-side Castling Check
        bool queenside_clear = (castle_masks[color][QUEENSIDE] & blockers_all) == 0;
        bool queenside_safe = (castle_masks[color][QUEENSIDE] & enemy_attacks) == 0;

        bool can_queenside_castle = !king_in_check && queenside_clear && queenside_safe;
        print_bitboard("kingside castle mask", castle_masks[color][KINGSIDE]);
        print_bitboard("kingside blockers", (castle_masks[color][KINGSIDE] & blockers_all));
        print_bitboard("kingside attacks", (castle_masks[color][KINGSIDE] & enemy_attacks));
        cout << "kingside castle: " << can_kingside_castle << endl;
        cout << "queenside castle: " << can_queenside_castle << endl;

        u64 kingside_castle = can_kingside_castle ? castle_square[color][KINGSIDE] : 0ULL;
        u64 queenside_castle = can_queenside_castle ? castle_square[color][QUEENSIDE] : 0ULL;

        u64 attacks = (AttackTables::king_attacks((Square)start) & ~blockers[color] & ~enemy_attacks);
        while (attacks)
        {
            int target = __builtin_ctzll(attacks); // compiler instruction to get position of rightmost set bit
            legal_moves.push_back(KING << 12 | target << 6 | start);
            attacks &= attacks - 1;
        }
        /* TODO: FINISH CASTLING LOGIC */
        int special_moves_flag = 0;
        if (can_kingside_castle)
        {
            special_moves_flag = 3;
            legal_moves.push_back(special_moves_flag << 21 | __builtin_ctz(kingside_castle) << 6 | king_square);
        }
        if (can_queenside_castle)
        {
            special_moves_flag = 4;
            legal_moves.push_back(special_moves_flag << 21 | __builtin_ctz(kingside_castle) << 6 | king_square);
        }
        bitboard &= bitboard - 1;
    }
    if (double_check)
    {
        return legal_moves;
    }

    /*** generate pins ***/
    u64 pin_masks[64];
    memset(pin_masks, 0xff, sizeof(pin_masks));
    u64 temp_board = pieces[color][KING] | blockers[opposite_color];
    u64 pin_square = 0, pin_hv = 0, pin_diag = 0, temp_pieces = 0, pin_ray = 0;

    // populate pin_mask array
    u64 sliders_bitboard = pieces[!color][BISHOP] | pieces[!color][QUEEN];
    while (sliders_bitboard)
    {
        pin_square = AttackTables::bishop_attacks(king_square, sliders_bitboard) & (1ULL << __builtin_ctzll(sliders_bitboard));
        pin_ray = pin_square ? AttackTables::bishop_attacks(king_square, sliders_bitboard) & AttackTables::bishop_attacks((Square)__builtin_ctzll(pin_square), temp_board) | pin_square : 0ULL;
        int num_blockers_on_mask = __builtin_popcountll((pin_ray & blockers_all) ^ pin_square);
        if (num_blockers_on_mask == 1 && (pin_ray & blockers[color]))
        {
            pin_masks[__builtin_ctzll(pin_ray & blockers[color])] = pin_ray;
        }
        sliders_bitboard &= sliders_bitboard - 1;
    }

    sliders_bitboard = pieces[!color][ROOK] | pieces[!color][QUEEN];
    while (sliders_bitboard)
    {
        pin_square = AttackTables::rook_attacks(king_square, sliders_bitboard) & (1ULL << __builtin_ctzll(sliders_bitboard));
        pin_ray = pin_square ? AttackTables::rook_attacks(king_square, sliders_bitboard) & AttackTables::rook_attacks((Square)__builtin_ctzll(pin_square), temp_board) | pin_square : 0ULL;
        int num_blockers_on_mask = __builtin_popcountll((pin_ray & blockers_all) ^ pin_square);
        if (num_blockers_on_mask == 1 && (pin_ray & blockers[color]))
        {
            pin_masks[__builtin_ctzll(pin_ray & blockers[color])] = pin_ray;
        }
        sliders_bitboard &= sliders_bitboard - 1;
    }

    int prev_move = move_stack.size() ? move_stack.top() : 0;

    // generate pawn legal moves
    bitboard = pieces[color][PAWN];
    while (bitboard)
    {

        if (prev_move)
        {
            enpassant_square = (((0b001 << 21) & prev_move) != 0) ? 1ULL << (((0x3f & prev_move) + ((0xfc0 & prev_move) >> 6)) >> 1) : 0ULL;
        }

        int start = __builtin_ctzll(bitboard);
        u64 single_push, double_push, double_push_rank;
        if (color == WHITE)
        {
            single_push = ((1ULL << start) >> 8) & empty;
            double_push_rank = (0xffULL << 32);
            double_push = ((single_push & (double_push_rank << 8)) >> 8) & empty; // check if white pawn on 3rd rank
        }
        else
        {
            single_push = ((1ULL << start) << 8) & empty;
            double_push_rank = (0xffULL << 24);
            double_push = ((single_push & (double_push_rank >> 8)) << 8) & empty; // check if black pawn on 6th rank
            // print_bitboard("double push", double_push);
        }
        u64 attacks = AttackTables::pawn_attacks(color, (Square)start);
        u64 normal_captures = attacks & blockers[!color];
        u64 enpassant_capture = attacks & enpassant_square;
        if (enpassant_capture) // check enpassant capture special case for rooks/queens
        {
            u64 enpassant_sq_capture = color == WHITE ? enpassant_capture << 8 : enpassant_capture >> 8;
            u64 is_attack = AttackTables::rook_attacks(king_square, (blockers_all & ~((1ULL << start) | enpassant_sq_capture))) & (pieces[!color][ROOK] | pieces[!color][QUEEN]);
            if (is_attack)
            {
                enpassant_capture = 0ULL;
            }
        }
        u64 moves = (single_push | double_push | normal_captures | enpassant_capture) & (checkmask | enpassant_capture) & pin_masks[start];

        while (moves)
        {
            int target = __builtin_ctzll(moves);
            int is_double_push = ((1ULL << target) & double_push_rank) && double_push;
            int is_enpassant_capture = ((1ULL << target) & enpassant_square) != 0 ? 0b010 : 0;
            int special_flags = is_double_push | is_enpassant_capture;
            int promotion_piece = (((1ULL << target) & 0xff) || ((1ULL << target) & (0xffULL << 56)))
                                      ? QUEEN
                                      : NO_PIECE;
            int legal_move = special_flags << 21 | promotion_piece << 18 | PAWN << 12 | target << 6 | start;
            legal_moves.push_back(legal_move);
            moves &= moves - 1;
        }
        bitboard &= bitboard - 1;
    }

    // generate knight legal moves
    bitboard = pieces[color][KNIGHT];
    while (bitboard)
    {
        int start = __builtin_ctzll(bitboard);
        u64 attacks = AttackTables::knight_attacks((Square)start) & ~blockers[color] & checkmask & pin_masks[start];
        while (attacks)
        {
            int target = __builtin_ctzll(attacks);
            legal_moves.push_back(KNIGHT << 12 | target << 6 | start);
            attacks &= attacks - 1;
        }
        bitboard &= bitboard - 1;
    }

    // generate bishop legal moves
    bitboard = pieces[color][BISHOP];
    while (bitboard)
    {
        int start = __builtin_ctzll(bitboard);
        u64 attacks = AttackTables::bishop_attacks((Square)start, blockers_all) & ~blockers[color] & checkmask & pin_masks[start];
        while (attacks)
        {
            int target = __builtin_ctzll(attacks);
            legal_moves.push_back(BISHOP << 12 | target << 6 | start);
            attacks &= attacks - 1;
        }
        bitboard &= bitboard - 1;
    }

    // generate rook legal moves
    bitboard = pieces[color][ROOK];
    while (bitboard)
    {
        int start = __builtin_ctzll(bitboard);
        u64 attacks = AttackTables::rook_attacks((Square)start, blockers_all) & ~blockers[color] & checkmask & pin_masks[start];
        while (attacks)
        {
            int target = __builtin_ctzll(attacks);
            legal_moves.push_back(ROOK << 12 | target << 6 | start);
            attacks &= attacks - 1;
        }
        bitboard &= bitboard - 1;
    }

    // generate queen legal moves
    bitboard = pieces[color][QUEEN];
    while (bitboard)
    {
        int start = __builtin_ctzll(bitboard);
        u64 attacks = AttackTables::queen_attacks((Square)start, blockers_all) & ~blockers[color] & checkmask & pin_masks[start];
        while (attacks)
        {
            int target = __builtin_ctzll(attacks);
            legal_moves.push_back(QUEEN << 12 | target << 6 | start);
            attacks &= attacks - 1;
        }
        bitboard &= bitboard - 1;
    }

    return legal_moves;
}

bool Board::is_legal_move(Square start, Square target, Color turn)
{
    vector<int> legal_moves = generate_legal_moves(turn);

    for (int move : legal_moves)
    {
        int from = move & 0x3F;      // bits 0-5
        int to = (move >> 6) & 0x3F; // bits 6-11
        if (from == static_cast<int>(start) && to == static_cast<int>(target))
            return true;
    }
    return false;
}

u64 Board::get_attacks(Color color)
{
    u64 attacks = 0, bitboard;
    u64 blockers_all = blockers[WHITE] | blockers[BLACK];

    // generate pawn legal moves
    bitboard = pieces[color][PAWN];
    while (bitboard)
    {
        int start = __builtin_ctzll(bitboard);
        attacks |= AttackTables::pawn_attacks(color, (Square)start);
        bitboard &= bitboard - 1;
    }

    // generate knight legal moves
    bitboard = pieces[color][KNIGHT];
    while (bitboard)
    {
        int start = __builtin_ctzll(bitboard);
        attacks |= AttackTables::knight_attacks((Square)start);
        bitboard &= bitboard - 1;
    }

    // generate bishop legal moves
    bitboard = pieces[color][BISHOP];
    while (bitboard)
    {
        int start = __builtin_ctzll(bitboard);
        attacks |= AttackTables::bishop_attacks((Square)start, blockers_all ^ pieces[!color][KING]);
        bitboard &= bitboard - 1;
    }

    // generate rook legal moves
    bitboard = pieces[color][ROOK];
    while (bitboard)
    {
        int start = __builtin_ctzll(bitboard);
        attacks |= AttackTables::rook_attacks((Square)start, blockers_all ^ pieces[!color][KING]);
        bitboard &= bitboard - 1;
    }

    // generate queen legal moves
    bitboard = pieces[color][QUEEN];
    while (bitboard)
    {
        int start = __builtin_ctzll(bitboard);
        attacks |= AttackTables::queen_attacks((Square)start, blockers_all ^ pieces[!color][KING]);
        bitboard &= bitboard - 1;
    }

    // generate king legal moves
    bitboard = pieces[color][KING];
    while (bitboard)
    {
        int start = __builtin_ctzll(bitboard);
        attacks |= AttackTables::king_attacks((Square)start);
        bitboard &= bitboard - 1;
    }

    return attacks;
}

void Board::print_bitboard(string label, u64 number)
{
    cout << label << endl;
    for (int i = 0; i < 64; i++)
    {
        int bit = number & (uint64_t)0x1;
        number = number >> 1;
        if (i % 8 == 7)
        {
            std::cout << bit << std::endl;
            continue;
        }
        std::cout << bit << " ";
    }
    cout << endl;
}

void Board::print_move_encoding(string label, int n)
{
    cout << label << endl;
    cout << ((n & (0b111 << 21)) >> 21) << " "
         << ((n & (0b111 << 18)) >> 18) << " "
         << ((n & (0b111 << 15)) >> 15) << " "
         << ((n & (0b111 << 12)) >> 12) << " "
         << ((n & (0b111111 << 6)) >> 6) << " "
         << (n & 0b111111) << endl;
    cout << endl;
}

bool Board::is_checkmate(Color turn)
{
    return generate_legal_moves(turn).size() == 0 && (get_attacks(turn == WHITE ? BLACK : WHITE) & pieces[turn][KING]);
}

bool Board::is_stalemate(Color turn)
{
    return generate_legal_moves(turn).size() == 0 && !(get_attacks(turn == WHITE ? BLACK : WHITE) & pieces[turn][KING]);
}

#include <chrono>

// Global accumulators
uint64_t total_generate_legal_moves_ns = 0;
uint64_t total_make_move_ns = 0;
uint64_t total_unmake_move_ns = 0;

// Helper macros for timing
#define TIME_BLOCK_NS(accumulator, code_block)                                                              \
    {                                                                                                       \
        auto start_time = std::chrono::high_resolution_clock::now();                                        \
        code_block;                                                                                         \
        auto end_time = std::chrono::high_resolution_clock::now();                                          \
        accumulator += std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count(); \
    }

long Board::perft(int depth, int max_depth)
{
    vector<int> legal_moves;

    // Measure generate_legal_moves
    legal_moves = generate_legal_moves(side_to_move);

    long nodes = 0, current_move_nodes = 0;

    if (depth == 1 || legal_moves.size() == 0)
    {
        return legal_moves.size();
    }
    else
    {
        for (int move : legal_moves)
        {
            int start = move & 0x3f;
            int target = (move >> 6) & 0x3f;
            int moving_piece = (move >> 12) & 0b111;

            if (depth == max_depth)
            {
                cout << coordinates(start) << coordinates(target) << ": ";
            }

            // Measure make_move
            make_move((Square)start, (Square)target, side_to_move, legal_moves);

            current_move_nodes = perft(depth - 1, max_depth);
            nodes += current_move_nodes;

            // Measure unmake_move
            unmake_move();

            if (depth == max_depth)
            {
                cout << current_move_nodes << endl;
            }
        }
    }

    return nodes;
}

// Optional: function to print profiling results
void Board::print_profiling()
{
    cout << "Total time spent in generate_legal_moves: "
         << total_generate_legal_moves_ns / 1e6 << " ms" << endl;
    cout << "Total time spent in make_move: "
         << total_make_move_ns / 1e6 << " ms" << endl;
    cout << "Total time spent in unmake_move: "
         << total_unmake_move_ns / 1e6 << " ms" << endl;
}

string Board::coordinates(int square)
{
    int fileValue = square % 8;       // 0 → 'a', 1 → 'b', ..., 7 → 'h'
    int rankValue = 8 - (square / 8); // 0 → '8', 1 → '7', ..., 7 → '1'

    char file = 'a' + fileValue;
    char rank = '0' + rankValue;

    return string(1, file) + string(1, rank); // e.g., "e4"
}