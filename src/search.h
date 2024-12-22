#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "move.h"
#include "transpose_table.h"

class Searcher {
private:
    struct Line {
        int                   curMove;
        std::array<Move, 256> line;
    };

    TranspositionTable ttable;

public:
    Searcher() :
        ttable(512) {}

    std::tuple<Move, int> alphaBeta(Board& board, const int depth, const int ply, int alpha,
                                    int beta, Line& line);
    std::tuple<Move, int> getBestMove(Board& board, int depth);

    int quiesce(Board& board, int alpha, int beta);
};

#endif
