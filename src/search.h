#ifndef SEARCH_H
#define SEARCH_H

#include "move.h"

struct SearchInfo {
    Move bestMove;
    int bestEval;
};

Move getBestMove(Board& board, int depth);
SearchInfo alphaBeta(Board& board, int depth, int alpha, int beta, bool);

#endif