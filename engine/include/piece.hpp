#include "common/types.hpp"

class Piece
{
public:
    explicit Piece(Color color) : color_(color) {}
    virtual ~Piece() = default;

    Color get_color() const { return color_; }
    int get_value() const { return value_; }
    int get_type() const { return (color_ << 3) | type_; }

protected:
    int value_;
    PieceType type_;

private:
    Color color_;
};