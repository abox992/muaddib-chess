#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <cstdint>
#include "board.h"
#include "move.h"

#define ALL_MOVES (~uint64_t(0))
#define CAPTURES(X) uint64_t(X.allPieces[!X.blackToMove])

int generateMoves(uint64_t moveMask, Board& board, struct Move moveList[], int color);

#endif