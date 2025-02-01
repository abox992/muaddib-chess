#include "uci.h"
#include "board.h"
#include "search.h"
#include "move_list.h"
#include <sstream>

int runUCI() {
    Board board;
    board.setStartPos();

    Searcher searcher;

    std::cout << "id name mangoEngine" << std::endl;
    std::cout << "id author abox992" << std::endl;
    std::cout << "uciok" << std::endl;

    std::string input = "";
    while (input != "isready") {
        std::cin >> input;
    }
    std::cout << "readyok" << std::endl;

    while (input != "quit") {
        input = "";
        std::cin >> input;

        // switch on input
        if (input == "ucinewgame") {}
        if (input.substr(0, 2) == "go") {
            using namespace std::chrono_literals;
            auto [bestMove, bestEval] = searcher.iterativeDeepening(board, 3000ms);
            std::cout << "bestmove " << bestMove << std::endl;
        } if (input == "position") {
            std::string startPos;
            std::cin >> startPos;

            if (startPos == "fen") {

            } else {
                assert(startPos == "startpos");
                board.setStartPos();

                std::string moves;
                std::getline(std::cin, moves);

                if (moves == "") break;

                std::istringstream moveStream(moves);

                std::string move;
                while (std::getline(moveStream, move, ' ')) {
                    MoveList<ALL> moveList(board);

                    for (auto m : moveList) {
                        if (move == toString(m)) {
                            board.makeMove(m);
                            break;
                        }
                    }
                }
            }
        } else {
        }
    }

    return 0;
}
