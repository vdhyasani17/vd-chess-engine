#pragma once

#include "common/types.hpp"
#include "piece.hpp"

class Rook : public Piece
{
public:
    Rook(Color color) : Piece(color)
    {
        value_ = 5;
        type_ = ROOK;
    }

    // other methods to write later
};