#include "cli.h"
#include <string>
#include <iostream>
#include "uci.h"
#include "board.h"
#include "search.h"
#include <sstream>
#include "helpers.h"
#include "test_suite.h"

namespace cli {

void loop() {
    std::string input;

    while (input != "exit") {
        std::cin >> input;
        std::transform(input.begin(), input.end(), input.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (input == "uci") {
            runUCI();
        } else if (input == "selfplay") {
            std::chrono::high_resolution_clock::time_point start =
              std::chrono::high_resolution_clock::now();

            asciiGameLoop();

            std::chrono::high_resolution_clock::time_point end =
              std::chrono::high_resolution_clock::now();

            auto time_span = std::chrono::duration_cast<std::chrono::seconds>(end - start);
            std::cout << "Game took: " << time_span.count() << " seconds." << std::endl;
        } else if (input == "test") {
            runTests();
        }
    }
}

void asciiGameLoop() {
    Board board;
    board.setStartPos();

    Searcher searcher;

    // ascii console game loop
    std::stringstream pgn;
    int               moveNum = 1;
    while (board.getHalfMoves() < 100 && board.getRepeats(board.hash()) < 3) {
        auto [bestMove, bestEval] = searcher.getBestMove(board, 5);

        if (bestMove.isNull()) {  // no moves available
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

        std::cout << "\x1B[2J\x1B[H";  // clear screen
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

}  // namespace
