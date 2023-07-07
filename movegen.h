#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <cstdint>
#include "board.h"
#include "move.h"

uint64_t generateKingLosMask(Board board, int color);

// ex input white = 0 -> needs to check what black pieces attack white
uint64_t generateCheckMask(Board board, int color);

uint64_t generatePinMask(Board board, int color);

void generateMoves(Board board, struct Move moveList[], int color);

#endif