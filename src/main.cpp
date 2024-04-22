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

int moveGenTest(int depth, Board& board);

#define MAX_DEPTH 6

int main() {
    Board board;
    board.setStartPos();
    initMasks();

    //board = generateBoardFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board = generateBoardFromFen("3r4/4p3/8/1PpP1P2/3K4/5k2/8/8 w - c6 0 1");
    std::cout << board << std::endl;

    uint64_t check = generateCheckMask(board, Color::WHITE);
    printBitboard(check);
    std::cout << std::endl;

    //struct Move moveList[256];
    std::vector<Move> moveList;
    moveList.reserve(256);

    if (board.state.blackToMove) {
        generateMoves<MoveFilter::ALL, Color::BLACK>(board, moveList);
    } else {
        generateMoves<MoveFilter::ALL, Color::WHITE>(board, moveList);
    }

    board = generateBoardFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
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