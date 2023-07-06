#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <cstdint>
#include "board.h"
#include "move.h"

int lsb(uint64_t &b);
void generateMoves(Board board, struct Move moveList[], int color);

#endif