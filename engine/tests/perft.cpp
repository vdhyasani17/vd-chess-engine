#include <chrono>
#include "../include/board.hpp"
#include <iomanip>

int main()
{
    /*
    {
        5: 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1
        4: 8/2p5/3p4/KP5r/5p1k/8/1R2P1P1/8 b - - 0 1
        3: 7r/2p5/3p4/KP6/5p1k/8/1R2P1P1/8 w - - 0 1
        2: 7r/2p5/3p4/KP6/1R3p1k/8/4P1P1/8 b - - 0 1
        1: r7/2p5/3p4/KP6/1R3p1k/8/4P1P1/8 w - - 0 1
    }
    */
    Board board;
    board.load_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    string move;
    Square squares[2];
    int depth = 5;

    board.print();
    cout << endl;
    auto start = chrono::high_resolution_clock::now();
    long total_nodes = board.perft(depth, depth);
    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double> elapsed_seconds = end - start;
    double nps = total_nodes / elapsed_seconds.count();

    cout << endl;
    cout << "Total nodes: " << total_nodes << endl;
    cout << "Elapsed time: " << elapsed_seconds.count() << " seconds" << endl;
    cout << fixed << setprecision(0) << "Nodes per second (NPS): " << nps << endl;

    return 0;
}

/*
b4b1: 69653(-12)
b4b2: 48499(+1)
---h5h8: 3565(+1)

---h5f5: 3032(-1)
---h5g5: 3311(-1)
---h5h6: 2907(-1)
---h5h7: 3127(-1)
---h5h8: 3563(-1)
---h4g3: 2987(-1)
---h4g5: 3642(-1)
---h4g4: 3539(-1)

b4b3: 59708(-11)
b4a4: 45580(-11)


total nodes: 674641(+18)
*/