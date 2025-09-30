#pragma once

#include "common/types.hpp"
#include "piece.hpp"

class Pawn : public Piece
{
public:
    Pawn(Color color) : Piece(color)
    {
        value_ = 1;
        type_ = PAWN;
    }

    // other methods to write later
};