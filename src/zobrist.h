#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "board.h"
#include <cstdint>

void initZobrist();
uint64_t zhash(Board& board);

#endif