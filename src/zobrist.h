#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "board.h"
#include <cstdint>

namespace Zobrist {
inline uint64_t randomTable[64][12];
inline uint64_t randomBlackToMove;
inline uint64_t enpassantFile[8];
inline uint64_t noPawns;
inline uint64_t castling[4];

void     initZobrist();
uint64_t zhash(const Board&);
}

#endif
