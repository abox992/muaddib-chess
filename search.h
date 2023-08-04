#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "move.h"

Move getBestMove(Board& board, int depth);
int search(Board& board, int startDepth, int depth, int alpha, int beta);
int quiesce(Board& board, int alpha, int beta);

#endif