#ifndef MOVE
#define MOVE

#include <iostream>
#include <immintrin.h>
#include "board.h"

struct Move {
    uint8_t from;
    uint8_t to;
    bool color;
    uint8_t piece; // 0 = pawns, 2 = knights, 4 = bishops, 6 = rooks, 8 = queens, 10 = kings
    bool enpessant = false;
    uint8_t castle = 0; // wk, bk, wq, bq
    uint8_t promotion = 0; // knight, bishop, rook, queen

    friend std::ostream& operator << (std::ostream& o, const Move& move) {
        char file[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

        int fromRank = unsigned(move.from) / 8;
        int fromFile = 7 - unsigned(move.from) % 8;

        int toRank = unsigned(move.to) / 8;
        int toFile = 7 - unsigned(move.to) % 8;

        o << file[fromFile] << (fromRank + 1) << file[toFile] << (toRank + 1);

        if (move.promotion != 0) {
            switch (_tzcnt_u64(move.promotion)) {
                case 0:
                    o << "->N";
                    break;
                case 1:
                    o << "->B";
                    break;
                case 2:
                    o << "->R";
                    break;
                case 3:
                    o << "->Q";
                    break;
            }
        }

        return o;
    }
};

#endif