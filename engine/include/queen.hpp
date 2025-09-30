#pragma once

#include "common/types.hpp"
#include "piece.hpp"

class Queen : public Piece
{
public:
    Queen(Color color) : Piece(color)
    {
        value_ = 9;
        type_ = QUEEN;
    }

    // other methods to write later
};