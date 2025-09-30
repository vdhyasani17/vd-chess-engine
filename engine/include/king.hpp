#pragma once

#include "common/types.hpp"
#include "piece.hpp"

class King : public Piece
{
public:
    King(Color color) : Piece(color)
    {
        value_ = 99;
        type_ = KING;
    }

    // other methods to write later
};