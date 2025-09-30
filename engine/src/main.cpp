#include <iostream>
#include "../include/board.hpp"

pair<Square, Square> convert_input(string coordinates);

int main()
{
    Board board;
    board.load_fen("8/3k4/8/5p2/4K3/8/8/3R4 w - - 0 1");
    string move;
    Square squares[2];

    board.print();
    cout << endl;
    bool side = board.get_side();
    string to_move;
    bool move_made;
    while (!(board.is_checkmate((Color)side) || board.is_stalemate((Color)side)))
    {
        move_made = false;
        while (!move_made)
        {
            to_move = side ? "Black" : "White";
            cout << to_move << " to move" << endl;
            cout << "Move (ex: d2d4): ";
            cin >> move;
            if (move == "undo")
            {
                if (board.unmake_move()) // unmake_move returns true if successful
                {
                    side ^= 1; // flip turn back
                    board.print();
                    cout << endl;
                }
                else
                {
                    cout << "No move to undo." << endl;
                }
                continue; // go back to waiting for input
            }

            auto [sq1, sq2] = convert_input(move);

            move_made = board.make_move(sq1, sq2, (Color)side);
            if (move_made)
            {
                board.print();
                cout << endl;
                side ^= 1;
            }
            else
                cout << "Illegal move" << endl;
        }
    }
    board.print();

    if (board.is_checkmate(((Color)side)))
    {
        string winner_msg = side == WHITE ? "Black wins by checkmate!" : "White wins by checkmate!";
        cout << winner_msg << endl;
    }
    else
    {
        cout << "Stalemate" << endl;
    }

    return 0;
}

std::pair<Square, Square> convert_input(string coordinates)
{
    if (coordinates.size() != 4)
        throw std::invalid_argument("Move must be 4 characters like e2e4");

    auto parse_square = [](char file, char rank) -> Square
    {
        int fileValue = file - 'a';       // 'a' → 0, 'b' → 1, ..., 'h' → 7
        int rankValue = 8 - (rank - '0'); // '1' → 7, '8' → 0
        return static_cast<Square>(8 * rankValue + fileValue);
    };

    Square from = parse_square(coordinates[0], coordinates[1]);
    Square to = parse_square(coordinates[2], coordinates[3]);

    return {from, to};
}