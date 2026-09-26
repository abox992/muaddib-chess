#include "uci.h"
#include "board.h"
#include "move_list.h"
#include "search.h"

#include <iostream>
#include <sstream>
#include <string>

void runUCI() {
    Board board;
    board.setStartPos();

    Searcher searcher;

    std::cout << "id name muaddibChess" << std::endl;
    std::cout << "id author abox992" << std::endl;
    std::cout << "uciok" << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        std::istringstream stream(line);
        std::string        command;
        stream >> command;

        if (command == "quit") {
            break;
        }

        if (command == "isready") {
            std::cout << "readyok" << std::endl;
            continue;
        }

        if (command == "ucinewgame") {
            board.setStartPos();
            continue;
        }

        if (command == "position") {
            std::string type;
            stream >> type;
            if (type != "startpos") {
                continue;
            }

            board.setStartPos();

            std::string token;
            stream >> token;
            if (token != "moves") {
                continue;
            }

            while (stream >> token) {
                MoveList<ALL> moveList(board);
                for (auto move : moveList) {
                    if (token == toString(move)) {
                        board.makeMove(move);
                        break;
                    }
                }
            }
            continue;
        }

        if (command == "go") {
            using namespace std::chrono_literals;
            auto [bestMove, bestEval] = searcher.iterativeDeepening(board, 3000ms);
            std::cout << "bestmove " << bestMove << std::endl;
        }
    }
}
