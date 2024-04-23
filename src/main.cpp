#include <iostream>
#include "board.h"
#include "helpers.h"
#include "precompute_masks.h"
#include "movegen.h"
#include "test_suite.h"
#include "zobrist.h"
#include "search.h"

#include "gui/game.h"

#include <bit>

#define MAX_DEPTH 6

int main() {
    Board board;
    board.setStartPos();
    initMasks();

    std::vector<Move> moveList;
    moveList.reserve(256);

    // if (board.state.blackToMove) {
    //     generateMoves<MoveFilter::ALL, Color::BLACK>(board, moveList);
    // } else {
    //     generateMoves<MoveFilter::ALL, Color::WHITE>(board, moveList);
    // }


    // 3r4/2p1p3/8/1P1P1P2/3K4/5k2/8/8 b - - 0 1
    // c7c6: 3r4/4p3/2p5/1P1P1P2/3K4/5k2/8/8 w - - 0 1
    // b5b6: 3r4/4p3/1Pp5/3P1P2/3K4/5k2/8/8 b - - 0 1
    // f3g3: 3r4/4p3/1Pp5/3P1P2/3K4/6k1/8/8 w - - 0 1
    board = generateBoardFromFen("3r4/4p3/1Pp5/3P1P2/3K4/6k1/8/8 w - - 0 1");
    int total = moveGenTest(1, board);
    std::cout << total << std::endl;
    runTests();

    initZobrist();

    // ascii console game loop
    // while (board.state.halfMoves < 100 && board.highestRepeat < 3) {
    //     struct Move bestMove = getBestMove(board, 8);
    //     std::cout << bestMove << std::endl;
    //     board.makeMove(bestMove);

    //     std::cout << "\x1B[2J\x1B[H"; // clear screen
    //     std::cout << board << std::endl;
    //     //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //     // std::string x;
    //     // std::cin >> x;
    // }

    //Game game;
    //game.runGameLoop();

    return 0;
}