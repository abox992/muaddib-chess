#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "board.h"
#include "move.h"
#include "types.h"

void printBitboard(uint64_t bitboard) {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int i = ((7 - file) + (8 * rank));
            uint64_t mask = uint64_t(1) << i;
            int bit = ((bitboard & mask) >> i);
            std::cout << bit << " ";
        }

        std::cout << std::endl;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::string movePretty(const Board& board, const Move& move) {
    std::stringstream result;

    uint64_t fromMask = maskForPos(move.from());
    uint64_t toMask = maskForPos(move.from());
    const Color color = static_cast<Color>(board.blackToMove());
    const Color opColor = static_cast<Color>(!color);

    // figure out piece
    int pieceNum = 0;
    for (int i = 0; i < 12; i += 2) {
        if (board.getBB(i + color) & fromMask) {
            pieceNum = i;
        }
    }
    pieceNum /= 2;

    const std::string capture = toMask & board.getAll(opColor) ? "x" : "";

    const char file[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

    int fromFile = 7 - unsigned(move.from()) % 8;

    int toRank = unsigned(move.to()) / 8;
    int toFile = 7 - unsigned(move.to()) % 8;

    const std::string pieceChars[] = {"", "N", "B", "R", "Q", "K"};
    if (move.moveType() != MoveType::CASTLE) {
        result << " " << pieceChars[pieceNum] << file[fromFile] << capture << file[toFile] << toRank + 1;
    } else {
        result << " " << "O-O";

        if (toFile == 0) { // queen side
            result << "-O";
        }
    }

    if ((move.moveType() == MoveType::PROMOTION)) {
        result << "=" << pieceChars[move.promotionPiece() + 1];
    }

    return result.str();
}
