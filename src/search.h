#ifndef SEARCH_H
#define SEARCH_H

#include "move.h"
#include "board.h"

struct SearchInfo {
    Move bestMove;
    int bestEval;
};

Move getBestMove(Board& board, int depth);
SearchInfo alphaBeta(Board& board, int depth, const int, int alpha, int beta);

#endif
