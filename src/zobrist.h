#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <cstdint>
#include "board_state.h"
#include "move.h"

void initZobrist();
uint64_t zhash(const BoardState&);
uint64_t zhash(const Move& move, const uint64_t oldHash);

#endif
