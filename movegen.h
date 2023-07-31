#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <cstdint>
#include "board.h"
#include "move.h"

int generateMoves(Board& board, struct Move moveList[], int color);

#endif