#pragma once

#include "common/types.hpp"
#include "piece.hpp"

class Knight : public Piece
{
public:
    Knight(Color color) : Piece(color)
    {
        value_ = 3;
        type_ = KNIGHT;
    }

    // other methods to write later
};