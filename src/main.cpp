#include "board.h"
#include "evaluate.h"
#include "helpers.h"
#include "move_list.h"
#include "precompute_masks.h"
#include "search.h"
#include "test_suite.h"
#include "zobrist.h"
#include "transpose_table.h"
#include <iostream>

// #include "gui/game.h"

#include <chrono>
#include <sstream>

void asciiGameLoop();
void benchmarkMoveGen();
void benchmarkPerft();
void benchmarkMakeMove();

int main()
{
    
    initMasks();
    Zobrist::initZobrist();
    
    Board board;

    MoveList<MoveFilter::ALL> moveList(board);

    std::cout << board << '\n';

    for (auto& m : moveList) {
        std::cout << m << '\n';
    }

    std::cout << "initial hash: " << board.hash() << '\n';
    // benchmarkMoveGen();
    // benchmarkMakeMove();
    // benchmarkPerft();

    //runTests();

    asciiGameLoop();
    //std::cout << sizeof(BoardState) << std::endl;

    return 0;
}

void benchmarkMoveGen()
{
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

void benchmarkMakeMove()
{
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

void benchmarkPerft()
{
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

void asciiGameLoop()
{
    Board board;
    board.setStartPos();

    Searcher searcher;

    // board.set("r1bqk2r/p2nppbp/2pp1np1/1p6/3PP3/2N1BP2/PPPQN1PP/R3KB1R w KQkq - 2 8");

    // ascii console game loop
    std::stringstream pgn;
    int moveNum = 1;
    while (board.getHalfMoves() < 100 && board.getHighestRepeat() < 3) {
        Searcher::SearchInfo result = searcher.getBestMove(board, 5);
        Move bestMove = result.bestMove;

        if (result.bestMove.isNull()) { // no moves available
            //std::cout << "here" << std::endl;
            break;
        }


        // MoveList<MoveFilter::ALL> moveList(board);

        // for (auto& m : moveList) {
        //     std::cout << m << '\n';
        // }

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
        std::cout << "Eval: " << result.bestEval << '\n';
        std::cout << "Repeats: " << board.getHighestRepeat() << '\n';

        /*MoveList<MoveFilter::ALL> moveList(board);*/
        /**/
        /*for (auto& m : moveList) {*/
        /*     std::cout << m << '\n';*/
        /*}*/


        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        //  std::string x;
        //  std::cin >> x;

        // std::cout << board.curState->halfMoves << '\n';
        // std::cout << board.curState->highestRepeat << '\n';

        if (board.inCheck()) {
            pgn << "+";
        }
    }

    std::cout << pgn.str() << std::endl;
}
