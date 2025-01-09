#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "move.h"
#include "transpose_table.h"

class Searcher {
private:
    TranspositionTable ttable;
    std::vector<Move>  pv;
    Move               bestMove;
    bool               stopSearch;

public:
    Searcher() :
        ttable(512) {}

    std::tuple<Move, int> alphaBeta(Board& board, const int depth, const int ply, int alpha, int beta);
    std::tuple<Move, int> getBestMove(Board& board, int depth);
    std::tuple<Move, int> iterativeDeepening(Board& board, std::chrono::milliseconds timeMs);

    int quiesce(Board& board, int alpha, int beta);
};

#endif
