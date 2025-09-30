#include <chrono>
#include "../include/board.hpp"
#include <iomanip>

int main()
{
    Board board;
    board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    string move;
    Square squares[2];
    int depth = 5;

    board.print();
    cout << endl;
    auto start = chrono::high_resolution_clock::now();
    int total_nodes = board.perft(depth, depth, false, false);
    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double> elapsed_seconds = end - start;
    double nps = total_nodes / elapsed_seconds.count();

    cout << "Total nodes: " << total_nodes << endl;
    cout << "Elapsed time: " << elapsed_seconds.count() << " seconds" << endl;
    cout << fixed << setprecision(0) << "Nodes per second (NPS): " << nps << endl;

    return 0;
}