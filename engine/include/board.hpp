#include <iostream>
#include <string>
#include <vector>
#include <stack>   // for std::stack
#include <cstring> // for memset
#include <cstdint> // for uint64_t
#include "common/types.hpp"
#include "attack_tables.hpp"

using namespace std;

using u64 = unsigned long long;

class Board
{
private:
    int squares[64] = {};
    u64 pieces[2][7] = {};
    u64 blockers[2] = {};
    u64 one_bit = 1;
    Color side_to_move = WHITE;
    std::stack<int> move_stack;
    char piece_types[2][7] = {{' ', 'P', 'N', 'B', 'R', 'Q', 'K'},
                              {' ', 'p', 'n', 'b', 'r', 'q', 'k'}};
    u64 enpassant_square;
    u64 castle_masks[2][2] = {{0b11ULL << 61, 0b111ULL << 57}, {0b11ULL << 5, 0b111ULL << 1}};
    u64 castle_square[2][2] = {{1ULL << 62, 1ULL << 58}, {1ULL << 6, 1ULL << 2}};

public:
    void set_square(int i, int value);
    void load_fen(string fen);
    int type_of(char p);
    void print();
    bool make_move(Square start, Square target, Color turn, vector<int> legal_moves);
    bool unmake_move();
    vector<int> generate_legal_moves(Color color);
    bool is_legal_move(Square start, Square target, Color turn);
    u64 get_attacks(Color color);
    void print_bitboard(string label, u64 bitboard);
    void print_move_encoding(string label, int number);
    Color get_side();
    bool is_checkmate(Color turn);
    bool is_stalemate(Color turn);
    u64 generate_checkmask(Color turn);
    int get_piece_at_square(Square sq);
    long perft(int depth, int max_depth);
    string coordinates(int square);
    void print_profiling();
};