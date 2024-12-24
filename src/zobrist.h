#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "board_state.h"
#include <cstdint>

namespace Zobrist {
inline uint64_t randomTable[64][12];
inline uint64_t randomBlackToMove;

void     initZobrist();
uint64_t zhash(const BoardState&);
}

#endif
