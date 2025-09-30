#pragma once

#include "common/types.hpp"
#include "piece.hpp"

class Bishop : public Piece
{
public:
    Bishop(Color color) : Piece(color)
    {
        value_ = 3;
        type_ = BISHOP;
    }

    // other methods to write later
};