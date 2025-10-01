#include "../include/magic_bitboards.hpp"

#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <cstring>

// Define static arrays
u64 MagicBitboards::rook_attack_table[64][4096] = {};
u64 MagicBitboards::rook_magic_numbers[64] = {};
u64 MagicBitboards::bishop_attack_table[64][512] = {};
u64 MagicBitboards::bishop_magic_numbers[64] = {};

void MagicBitboards::init_sliders(u64 rook_magics[64], u64 bishop_magics[64], u64 rook_attacks[64][4096], u64 bishop_attacks[64][512])
{
    populate_rook_magic_numbers_array();
    populate_bishop_magic_numbers_array();
    for (int square = 0; square < 64; square++)
    {
        rook_magics[square] = rook_magic_numbers[square];
        for (int occ = 0; occ < 4096; occ++)
        {
            rook_attacks[square][occ] = rook_attack_table[square][occ];
        }
    }

    for (int square = 0; square < 64; square++)
    {
        bishop_magics[square] = bishop_magic_numbers[square];
        for (int occ = 0; occ < 512; occ++)
        {
            bishop_attacks[square][occ] = bishop_attack_table[square][occ];
        }
    }
}

/***    General Methods    ***/
u64 MagicBitboards::random_uint64()
{
    uint64_t n1 = ((uint64_t)std::rand()) & 0xFFFF;
    uint64_t n2 = ((uint64_t)std::rand()) & 0xFFFF;
    uint64_t n3 = ((uint64_t)std::rand()) & 0xFFFF;
    uint64_t n4 = ((uint64_t)std::rand()) & 0xFFFF;

    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

u64 MagicBitboards::generate_magic_number_candidate()
{
    return random_uint64() & random_uint64() & random_uint64();
}

int MagicBitboards::count_one_bits(uint64_t bitBoard)
{
    int count = 0;

    while (bitBoard > 0)
    {
        count++;
        bitBoard &= (bitBoard - 1);
    }

    return count;
}

u64 MagicBitboards::attack_bitboard(int square, u64 bitboard, bool is_rook)
{
    if (is_rook)
    {
        u64 relevant_occupancies = bitboard & rook_attack_masks[square];
        int magic_index = (relevant_occupancies * rook_magic_numbers[square]) >> (64 - rook_relevant_bits[square]);

        return rook_attack_table[square][magic_index];
    }
    else
    {
        u64 relevant_occupancies = bitboard & bishop_attack_masks[square];
        int magic_index = (relevant_occupancies * bishop_magic_numbers[square]) >> (64 - bishop_relevant_bits[square]);

        return bishop_attack_table[square][magic_index];
    }
}

/***    Rook    ***/
u64 MagicBitboards::generate_occupancies_for_rook(int square, int index)
{
    u64 attack_mask = rook_attack_masks[square];
    u64 occupancies = attack_mask;

    // Loop through the blockermask to find the indices of all set bits.
    int bitindex = 0;
    for (int i = 0; i < 64; i++)
    {
        // If relevant bit
        if (attack_mask & (1ULL << i))
        {
            // Clear the i'th bit in the blockerboard if it's clear in the index at bitindex.
            if (!(index & (1 << bitindex)))
            {
                occupancies &= ~(1ULL << i); // Clear the bit.
            }
            // Increment the bit index in the 0-4096 index, so each bit in index will correspond
            // to each set bit in blockermask.
            bitindex++;
        }
    }
    return occupancies;
}

u64 MagicBitboards::generate_rook_attacks(int square, u64 occupancies)
{
    u64 squares_attacked = 0x0;
    int increment = 0;
    int current_index = square;
    bool ran_into_piece = false;

    // up direction
    increment = -8;
    while (current_index + increment >= 0 && !ran_into_piece)
    {
        if (occupancies & (1ULL << (current_index + increment)))
        {
            ran_into_piece = true;
        }
        current_index += increment;
        squares_attacked = squares_attacked | (1ULL << current_index);
    }

    // down direction
    increment = 8;
    current_index = square;
    ran_into_piece = false;
    while (current_index + increment <= 63 && !ran_into_piece)
    {
        if (occupancies & (1ULL << (current_index + increment)))
        {
            ran_into_piece = true;
        }
        current_index += increment;
        squares_attacked = squares_attacked | (1ULL << current_index);
    }

    // left direction
    increment = -1;
    current_index = square;
    ran_into_piece = false;
    while (current_index % 8 > 0 && !ran_into_piece)
    {
        if (occupancies & (1ULL << (current_index + increment)))
        {
            ran_into_piece = true;
        }
        current_index += increment;
        squares_attacked = squares_attacked | (1ULL << current_index);
    }

    // right direction
    increment = 1;
    current_index = square;
    ran_into_piece = false;
    while (current_index % 8 < 7 && !ran_into_piece)
    {
        if (occupancies & (1ULL << (current_index + increment)))
        {
            ran_into_piece = true;
        }
        current_index += increment;
        squares_attacked = squares_attacked | (1ULL << current_index);
    }

    return squares_attacked;
}

// generate magic number for given square
u64 MagicBitboards::find_rook_magics(int square, int relevant_bits)
{
    int num_bits = 1 << relevant_bits;

    u64 occupancies[4096];
    u64 attacks[4096];
    u64 used_attacks[4096];

    for (int i = 0; i < num_bits; i++)
    {
        occupancies[i] = generate_occupancies_for_rook(square, i);
        attacks[i] = generate_rook_attacks(square, occupancies[i]);
    }

    // find magic number
    for (int i = 0; i < 10000000; i++)
    {
        uint64_t magic_number = generate_magic_number_candidate();

        // skipping over inappropriate magic numbers
        if (count_one_bits(magic_number * rook_attack_masks[square]) * 0xFF00000000000000 < 6)
        {
            continue;
        }

        std::memset(used_attacks, (uint64_t)0, sizeof(used_attacks));

        bool fail = false;
        for (int j = 0; !fail && j < num_bits; j++)
        {
            int magic_index = (int)((occupancies[j] * magic_number) >> (64 - relevant_bits));

            if (used_attacks[magic_index] == 0)
            {
                used_attacks[magic_index] = attacks[j];
                rook_attack_table[square][magic_index] = attacks[j];
            }
            else
            {
                fail = true;
                std::memset(rook_attack_table[square], (uint64_t)0, sizeof(rook_attack_table[square]));
                break;
            }
        }

        // if right magic number found, return it
        // else, generate a different number and try again
        if (!fail)
        {
            return magic_number;
        }
    }

    // magic number not found, so return 0
    return 0;
}

// print magic numbers
void MagicBitboards::populate_rook_magic_numbers_array()
{
    for (int i = 0; i < 64; i++)
    {
        // std::cout << i << ": " << find_magic_number(i, rookRelevantBits[i]) << std::endl;
        rook_magic_numbers[i] = find_rook_magics(i, rook_relevant_bits[i]);
    }
}

/***    Bishop    ***/

u64 MagicBitboards::generate_occupancies_for_bishop(int square, int index)
{
    uint64_t attack_mask = bishop_attack_masks[square];
    uint64_t occupancies = attack_mask;

    // Loop through the blockermask to find the indices of all set bits.
    int8_t bitindex = 0;
    for (int8_t i = 0; i < 64; i++)
    {
        // Check if the i'th bit is set in the mask (and thus a potential blocker).
        if (attack_mask & (1ULL << i))
        {
            // Clear the i'th bit in the blockerboard if it's clear in the index at bitindex.
            if (!(index & (1 << bitindex)))
            {
                occupancies &= ~(1ULL << i); // Clear the bit.
            }
            // Increment the bit index in the 0-4096 index, so each bit in index will correspond
            // to each set bit in blockermask.
            bitindex++;
        }
    }
    return occupancies;
}

u64 MagicBitboards::generate_bishop_attacks(int square, u64 occupancies)
{
    size_t squares_attacked = 0x0;
    int increment = 0;
    int current_index = square;
    bool ran_into_piece = false;

    // init target files & ranks
    int currRank = square / 8;
    int currFile = square % 8;

    // down-right
    for (int r = currRank + 1, f = currFile + 1; r <= 7 && f <= 7; r++, f++)
    {
        squares_attacked |= (1ULL << (r * 8 + f));
        if (occupancies & (1ULL << (r * 8 + f)))
            break;
    }

    // down-left
    for (int r = currRank + 1, f = currFile - 1; r <= 7 && f >= 0; r++, f--)
    {
        squares_attacked |= (1ULL << (r * 8 + f));
        if (occupancies & (1ULL << (r * 8 + f)))
            break;
    }

    // up-right
    for (int r = currRank - 1, f = currFile + 1; r >= 0 && f <= 7; r--, f++)
    {
        squares_attacked |= (1ULL << (r * 8 + f));
        if (occupancies & (1ULL << (r * 8 + f)))
            break;
    }

    // up-left
    for (int r = currRank - 1, f = currFile - 1; r >= 0 && f >= 0; r--, f--)
    {
        squares_attacked |= (1ULL << (r * 8 + f));
        if (occupancies & (1ULL << (r * 8 + f)))
            break;
    }

    return squares_attacked;
}

// generate magic number for given square
u64 MagicBitboards::find_bishop_magics(int square, int relevant_bits)
{
    int num_bits = 1 << relevant_bits;

    u64 occupancies[num_bits];
    u64 attacks[num_bits];
    u64 used_attacks[num_bits];

    for (int i = 0; i < num_bits; i++)
    {
        occupancies[i] = generate_occupancies_for_bishop(square, i);
        attacks[i] = generate_bishop_attacks(square, occupancies[i]);
    }

    // find magic number
    for (int i = 0; i < 10000000; i++)
    {
        u64 magic_number = generate_magic_number_candidate();

        // skipping over inappropriate magic numbers
        if (count_one_bits(magic_number * bishop_attack_masks[square]) * 0xFF00000000000000 < 6)
        {
            continue;
        }

        std::memset(used_attacks, (u64)0, sizeof(used_attacks));

        bool fail = false;
        for (int j = 0; !fail && j < num_bits; j++)
        {
            int magic_index = (int)((occupancies[j] * magic_number) >> (64 - relevant_bits));

            if (used_attacks[magic_index] == 0)
            {
                used_attacks[magic_index] = attacks[j];
                bishop_attack_table[square][magic_index] = attacks[j];
            }
            else
            {
                fail = true;
                std::memset(bishop_attack_table[square], (u64)0, sizeof(bishop_attack_table[square]));
                break;
            }
        }

        // if right magic number found, return it
        // else, generate a different number and try again
        if (!fail)
        {
            return magic_number;
        }
    }

    // magic number not found, so return 0
    return 0;
}

// print magic numbers
void MagicBitboards::populate_bishop_magic_numbers_array()
{
    for (int i = 0; i < 64; i++)
    {
        // std::cout << i << ": " << find_magic_number(i, rookRelevantBits[i]) << std::endl;
        bishop_magic_numbers[i] = find_bishop_magics(i, bishop_relevant_bits[i]);
    }
}