#include <iostream>
#include "board.h"
#include "helpers.h"
#include "precompute_masks.h"
#include "movegen.h"
#include "test_suite.h"
#include "zobrist.h"
#include "search.h"
#include "evaluate.h"
#include "move_list.h"

//#include "gui/game.h"

#include <bit>
#include <sstream>
#include <chrono>

void asciiGameLoop();
void benchmarkMoveGen();
void benchmarkPerft();
void benchmarkMakeMove();

int main() {
    Board board;
    board.setStartPos();
    initMasks();
    initZobrist();

    MoveList<MoveFilter::ALL> moveList(board);

    std::cout << board << '\n';

    for (auto& m : moveList) {
        std::cout << m << '\n';
    }

    //benchmarkMoveGen();
    //benchmarkMakeMove();
    //benchmarkPerft();

    runTests();

    // board = generateBoardFromFen("rnb1kbn1/ppp5/4p3/8/7q/1PP1P3/P2K3p/RNBQ1B1R b q - 0 14");
    // getBestMove(board, 7);

    //asciiGameLoop();
    //board.set("rn3bnr/p2q4/4k3/2pb1p2/P1N4P/1Q2PB2/5N2/R1B1K2R b KQ - 3 23");
    // Move move = getBestMove(board, 5);
    // std::cout << move << '\n';

    //Game game;
    //game.runGameLoop();

    return 0;
}

void benchmarkMoveGen() {
    Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

    int iterations = 1'000'000;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        MoveList<MoveFilter::ALL> moveList(board);
    }
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    auto time_span = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Movegen speed: " << time_span.count() / static_cast<double>(iterations) << " microseconds" << std::endl;
}

void benchmarkMakeMove() {
    Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

    MoveList<MoveFilter::ALL> moveList(board);

    int iterations = 1'000'000;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        board.makeMove(moveList.get(0));
        board.unmakeMove();
    }
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    auto time_span = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Make/unmake speed: " << time_span.count() / static_cast<double>(iterations) << " microseconds" << std::endl;
}

void benchmarkPerft() {
    //uint64_t answer = 3'195'901'860;

    Board board;
    board.setStartPos();

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    uint64_t result = moveGenTest(6, board);

    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    //assert(answer == result);

    auto time_span = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Engine speed: " << (static_cast<double>(result) / time_span.count()) / 1000000 << " MN/sec" << std::endl;
}

void asciiGameLoop() {
    Board board;
    board.setStartPos();

    //board.set("r1bqk2r/p2nppbp/2pp1np1/1p6/3PP3/2N1BP2/PPPQN1PP/R3KB1R w KQkq - 2 8");

    // ascii console game loop
    std::stringstream pgn;
    int moveNum = 1;
    while (board.curState->halfMoves < 100 && board.curState->highestRepeat < 3) {
        Move bestMove = getBestMove(board, 5);

        if (bestMove.isNull()) { // no moves available
            break;
        }

        std::cout << moveNum << ". " << bestMove << std::endl;
        std::cout << evaluation(board) << '\n';

        MoveList<MoveFilter::ALL> moveList(board);

        for (auto& m : moveList) {
            std::cout << m << '\n';
        }

        // update pgn
        if (!board.curState->blackToMove) {
            pgn << " " << moveNum << ". ";
        }

        pgn << movePretty(board, bestMove);

        if (board.curState->blackToMove) {
            moveNum++;
        }

        board.makeMove(bestMove);

        std::cout << "\x1B[2J\x1B[H"; // clear screen
        std::cout << board << std::endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        // std::string x;
        // std::cin >> x;

        // std::cout << board.curState->halfMoves << '\n';
        // std::cout << board.curState->highestRepeat << '\n';

        if (board.inCheck()) {
            pgn << "+";
        }
    }

    std::cout << pgn.str() << std::endl;
}