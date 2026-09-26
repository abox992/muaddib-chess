#include "uci.h"
#include "board.h"
#include "eval_net.h"
#include "move_list.h"
#include "search.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>

namespace {

void sendId() {
    std::cout << "id name muaddibChess" << std::endl;
    std::cout << "id author abox992" << std::endl;
    std::cout << "option name EvalFile type string default" << std::endl;
    std::cout << "uciok" << std::endl;
}

void playMoves(Board& board, std::istringstream& stream) {
    std::string token;
    while (stream >> token) {
        MoveList<ALL> moveList(board);
        for (auto move : moveList) {
            if (token == toString(move)) {
                board.makeMove(move);
                break;
            }
        }
    }
}

}  // namespace

void runUCI() {
    Board board;
    board.setStartPos();

    Searcher searcher;

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

        if (command == "uci") {
            sendId();
            continue;
        }

        if (command == "isready") {
            std::cout << "readyok" << std::endl;
            continue;
        }

        if (command == "ucinewgame") {
            board.setStartPos();
            continue;
        }

        if (command == "setoption") {
            std::string nameTok, name, valueTok, value;
            stream >> nameTok >> name >> valueTok;
            std::getline(stream, value);
            if (!value.empty() && value.front() == ' ') {
                value.erase(0, 1);
            }
            if (nameTok == "name" && name == "EvalFile" && valueTok == "value") {
                if (!EvalNet::load(value)) {
                    std::cerr << "info string failed to load EvalFile " << value << std::endl;
                }
            }
            continue;
        }

        if (command == "position") {
            std::string type;
            stream >> type;
            if (type == "startpos") {
                board.setStartPos();
                std::string token;
                if (stream >> token && token == "moves") {
                    playMoves(board, stream);
                }
            } else if (type == "fen") {
                std::string fen;
                std::string part;
                for (int i = 0; i < 6 && stream >> part; i++) {
                    if (part == "moves") {
                        break;
                    }
                    if (!fen.empty()) {
                        fen += ' ';
                    }
                    fen += part;
                }
                board.set(fen);
                std::string token;
                if (part == "moves") {
                    playMoves(board, stream);
                } else if (stream >> token && token == "moves") {
                    playMoves(board, stream);
                }
            }
            continue;
        }

        if (command == "go") {
            using namespace std::chrono_literals;
            int         depth    = 0;
            int         movetime = 0;
            std::string token;
            while (stream >> token) {
                if (token == "depth") {
                    stream >> depth;
                } else if (token == "movetime") {
                    stream >> movetime;
                }
            }

            std::tuple<Move, int> result;
            if (depth > 0) {
                searcher.newSearch();
                result = searcher.searchDepth(board, depth);
            } else if (movetime > 0) {
                result = searcher.iterativeDeepening(board, std::chrono::milliseconds(movetime));
            } else {
                result = searcher.iterativeDeepening(board, 3000ms);
            }
            std::cout << "info score cp " << std::get<1>(result) << std::endl;
            std::cout << "bestmove " << std::get<0>(result) << std::endl;
        }
    }
}
