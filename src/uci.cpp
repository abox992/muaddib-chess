#include "uci.h"
#include "board.h"

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
