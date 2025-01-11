#include "cli.h"
#include <string>
#include <iostream>
#include "uci.h"
#include "board.h"
#include "search.h"
#include <sstream>
#include <fstream>
#include <chrono>
#include "helpers.h"
#include "test_suite.h"

#define LOG 1

namespace cli {

void loop() {
    std::string input;

    while (input != "exit") {
        std::cin >> input;
        std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) { return std::tolower(c); });

        if (input == "uci") {
            runUCI();
        } else if (input == "play") {
            asciiGameLoop();
        } else if (input == "test") {
            runTests();
        }
    }
}

void asciiGameLoop() {
    std::ofstream logfile("log.txt");
    Board         board;

    Searcher searcher;

    auto start = std::chrono::high_resolution_clock::now();

    // ascii console game loop
    std::stringstream pgn;
    int               moveNum = 1;
    while (board.getHalfMoves() < 100 && board.getRepeats(board.hash()) < 3) {
        //auto [bestMove, bestEval] = searcher.getBestMove(board, 5);

        using namespace std::chrono_literals;
        auto [bestMove, bestEval] = searcher.iterativeDeepening(board, 500ms);
        int perspectiveEval = board.blackToMove() ? bestEval * -1 : bestEval;

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
        std::cout << "Eval: " << perspectiveEval << '\n';
        std::cout << "Repeats: " << board.getRepeats(board.hash()) << '\n';
        std::cout << "Hash: " << board.hash() << '\n';

        logfile << board << std::endl;
        logfile << moveNum << ". " << bestMove << std::endl;
        logfile << "Eval: " << perspectiveEval << '\n';
        logfile << "Repeats: " << board.getRepeats(board.hash()) << '\n';
        logfile << "Hash: " << board.hash() << '\n';

        if (board.inCheck()) {
            pgn << "+";
        }
    }

    std::cout << pgn.str() << std::endl;
    logfile << pgn.str() << std::endl;

    auto end = std::chrono::high_resolution_clock::now();

    auto time_span = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Game took: " << time_span.count() << " seconds." << std::endl;
}

}  // namespace
