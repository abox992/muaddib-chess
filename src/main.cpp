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


    // r3k1nr/p2pp1pp/b1n1P1P1/1BK1Pp1q/8/8/2PP1PPP/6N1 w kq - 0 1
    // c5c4: r3k1nr/p2pp1pp/b1n1P1P1/1B2Pp1q/2K5/8/2PP1PPP/6N1 b kq - 0 1
    board = generateBoardFromFen("r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1");
    int total = moveGenTest(1, board);
    std::cout << total << std::endl;
    runTests();

    initZobrist();

    // ascii console game loop
    // while (board.state.halfMoves < 100 && board.highestRepeat < 3) {
    //     struct Move bestMove = getBestMove(board, 7);
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