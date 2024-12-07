#include "bitboard.h"
#include "board.h"
#include "helpers.h"
#include "move_list.h"
#include "search.h"
#include "test_suite.h"
#include "transpose_table.h"
#include "zobrist.h"
#include <iostream>

// #include "gui/game.h"

#include <chrono>
#include <sstream>

void asciiGameLoop();
void benchmarkMoveGen();
void benchmarkPerft();
void benchmarkMakeMove();

void runUCI();

int main() {

    Bitboard::init();
    Zobrist::initZobrist();

    std::cout << sizeof(Cluster) << std::endl;

    std::string mode;
    std::cin >> mode;
    if (mode == "uci") {
        runUCI();
        return 0;
    }

    Board board;

    MoveList<ALL> moveList(board);

    std::cout << board << '\n';

    for (auto& m : moveList) {
        std::cout << m << '\n';
    }

    std::cout << "initial hash: " << board.hash() << '\n';
    std::cout << sizeof(TTEntry) << '\n';
    // benchmarkMoveGen();
    // benchmarkMakeMove();
    // benchmarkPerft();

    runTests();

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    // asciiGameLoop();

    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    auto time_span = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Game took: " << time_span.count() << " seconds." << std::endl;

    return 0;
}

void benchmarkMoveGen() {
    Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

    int iterations = 1'000'000;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        MoveList<ALL> moveList(board);
    }
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    auto time_span = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Movegen speed: " << time_span.count() / static_cast<double>(iterations) << " microseconds" << std::endl;
}

void benchmarkMakeMove() {
    Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

    MoveList<ALL> moveList(board);

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
    // uint64_t answer = 3'195'901'860;

    Board board;
    board.setStartPos();

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    uint64_t result = moveGenTest(6, board);

    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    // assert(answer == result);

    auto time_span = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Engine speed: " << (static_cast<double>(result) / time_span.count()) / 1000000 << " MN/sec" << std::endl;
}

void runUCI() {
    Board board;
    board.setStartPos();

    std::cout << "id name mangoEngine" << std::endl;
    std::cout << "id author abox992" << std::endl;
    std::cout << "uciok" << std::endl;

    std::string input = "";
    while (input != "isready") {
        std::cin >> input;
    }
    std::cout << "readyok" << std::endl;

    while (input != "quit") {
        std::cin >> input;
    }
}

void asciiGameLoop() {
    Board board;
    board.setStartPos();

    Searcher searcher;

    // board.set("r1bqk2r/p2nppbp/2pp1np1/1p6/3PP3/2N1BP2/PPPQN1PP/R3KB1R w KQkq - 2 8");

    // ascii console game loop
    std::stringstream pgn;
    int moveNum = 1;
    while (board.getHalfMoves() < 100 && board.getRepeats(board.hash()) < 3) {
        auto [bestMove, bestEval] = searcher.getBestMove(board, 5);

        if (bestMove.isNull()) { // no moves available
            break;
        }

        // update pgn
        if (!board.blackToMove()) {
            pgn << " " << moveNum << ".";
        }

        pgn << movePretty(board, bestMove);

        if (board.blackToMove()) {
            moveNum++;
        }

        board.makeMove(bestMove);

        std::cout << "\x1B[2J\x1B[H"; // clear screen
        std::cout << board << std::endl;
        std::cout << moveNum << ". " << bestMove << std::endl;
        std::cout << "Eval: " << bestEval << '\n';
        std::cout << "Repeats: " << board.getRepeats(board.hash()) << '\n';
        std::cout << "Hash: " << board.hash() << '\n';

        if (board.inCheck()) {
            pgn << "+";
        }
    }

    std::cout << pgn.str() << std::endl;
}
