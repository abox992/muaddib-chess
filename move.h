#ifndef MOVE
#define MOVE

#include <iostream>
#include "board.h"

struct Move {
    uint8_t from;
    uint8_t to;
    bool color;
    uint16_t piece; // 0 = pawns, 1 = knights, 2 = bishops, 3 = rooks, 4 = queens, 5 = kings

    friend std::ostream& operator << (std::ostream& o, const Move& move) {
        char file[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

        int fromRank = unsigned(move.from) / 8;
        int fromFile = 7 - unsigned(move.from) % 8;

        int toRank = unsigned(move.to) / 8;
        int toFile = 7 - unsigned(move.to) % 8;

        o << file[fromFile] << (fromRank + 1) << file[toFile] << (toRank + 1);
        return o;
    }
};

#endif