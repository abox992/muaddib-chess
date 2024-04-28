#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "move.h"

#define INFINITY (INT_MAX)
#define NEGATIVE_INFINITY (-(INT_MAX))

Move getBestMove(Board& board, int depth);
int search(Board& board, int startDepth, int depth, int alpha, int beta, bool capture);
int quiesce(Board& board, int alpha, int beta);

#endif