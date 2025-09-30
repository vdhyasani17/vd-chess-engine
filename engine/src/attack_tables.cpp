#include "../include/attack_tables.hpp"

u64 AttackTables::pawn_attack_table[2][64];
u64 AttackTables::knight_attack_table[64];
u64 AttackTables::king_attack_table[64];
u64 AttackTables::rook_attack_table[64][4096];
u64 AttackTables::bishop_attack_table[64][512];
u64 AttackTables::rook_magics[64];
u64 AttackTables::bishop_magics[64];

void AttackTables::init()
{
    init_pawn_tables();
    init_knight_table();
    init_king_table();
    MagicBitboards::init_sliders(rook_magics, bishop_magics, rook_attack_table, bishop_attack_table);
}

void AttackTables::init_pawn_tables()
{
    // init WHITE pawn attacks (skip 8th rank)
    for (int i = 8; i < BOARD_SIZE; i++)
    {
        if (i % 8 > 0)
            pawn_attack_table[0][i] ^= 1ULL << (i - 9);
        if (i % 8 < 7)
            pawn_attack_table[0][i] ^= 1ULL << (i - 7);
    }
    // init BLACK pawn attacks (skip 1st rank)
    for (int i = 0; i < BOARD_SIZE - 8; i++)
    {
        if (i % 8 > 0)
            pawn_attack_table[1][i] ^= 1ULL << (i + 7);
        if (i % 8 < 7)
            pawn_attack_table[1][i] ^= 1ULL << (i + 9);
    }
}

void AttackTables::init_knight_table()
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        if (i / 8 > 0 && i % 8 > 1)
            knight_attack_table[i] ^= 1ULL << (i - 10); // 10 o'clock
        if (i / 8 > 1 && i % 8 > 0)
            knight_attack_table[i] ^= 1ULL << (i - 17); // 11 o'clock
        if (i / 8 < 7 && i % 8 > 1)
            knight_attack_table[i] ^= 1ULL << (i + 6); // 8 o'clock
        if (i / 8 < 6 && i % 8 > 0)
            knight_attack_table[i] ^= 1ULL << (i + 15); // 7 o'clock
        if (i / 8 > 1 && i % 8 < 7)
            knight_attack_table[i] ^= 1ULL << (i - 15); // 1 o'clock
        if (i / 8 > 0 && i % 8 < 6)
            knight_attack_table[i] ^= 1ULL << (i - 6); // 2 o'clock
        if (i / 8 < 7 && i % 8 < 6)
            knight_attack_table[i] ^= 1ULL << (i + 10); // 4 o'clock
        if (i / 8 < 6 && i % 8 < 7)
            knight_attack_table[i] ^= 1ULL << (i + 17); // 5 o'clock
    }
}

void AttackTables::init_king_table()
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        if (i / 8 > 0 && i % 8 > 0)
            king_attack_table[i] ^= 1ULL << (i - 9); // top left
        if (i / 8 > 0)
            king_attack_table[i] ^= 1ULL << (i - 8); // top middle
        if (i / 8 > 0 && i % 8 < 7)
            king_attack_table[i] ^= 1ULL << (i - 7); // top right
        if (i % 8 < 7)
            king_attack_table[i] ^= 1ULL << (i + 1); // middle right
        if (i / 8 < 7 && i % 8 < 7)
            king_attack_table[i] ^= 1ULL << (i + 9); // bottom right
        if (i / 8 < 7)
            king_attack_table[i] ^= 1ULL << (i + 8); // middle bottom
        if (i / 8 < 7 && i % 8 > 0)
            king_attack_table[i] ^= 1ULL << (i + 7); // bottom left
        if (i % 8 > 0)
            king_attack_table[i] ^= 1ULL << (i - 1); // middle left
    }
}

u64 AttackTables::pawn_attacks(Color color, Square square)
{
    return pawn_attack_table[color][square];
}

u64 AttackTables::knight_attacks(Square square)
{
    return knight_attack_table[square];
}

u64 AttackTables::king_attacks(Square square)
{
    return king_attack_table[square];
}

u64 AttackTables::rook_attacks(Square square, u64 occupancy)
{
    occupancy &= rook_masks[square];
    occupancy *= rook_magics[square];
    occupancy >>= (64 - rook_relevant_bits[square]);
    return rook_attack_table[square][occupancy];
}

u64 AttackTables::bishop_attacks(Square square, u64 occupancy)
{
    occupancy &= bishop_masks[square];
    occupancy *= bishop_magics[square];
    occupancy >>= (64 - bishop_relevant_bits[square]);
    return bishop_attack_table[square][occupancy];
}

u64 AttackTables::queen_attacks(Square square, u64 occupancy)
{
    return rook_attacks(square, occupancy) | bishop_attacks(square, occupancy);
}