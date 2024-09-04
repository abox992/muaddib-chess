#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "board_state.h"
#include <cstdint>

class Zobrist {
private:
public:
    static uint64_t randomTable[64][12];
    static uint64_t randomBlackToMove;

    static void initZobrist();
    static uint64_t zhash(const BoardState&);
};

#endif
