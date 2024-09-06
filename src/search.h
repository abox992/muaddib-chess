#ifndef SEARCH_H
#define SEARCH_H

#include "move.h"
#include "board.h"
#include "transpose_table.h"
#include <map>

class Searcher {
private:
    struct Line {
        int size; // line length
        Move moves[256];

        void add(const Move m) {
            moves[size++] = m;
        }
    };


    TranspositionTable ttable;
    std::map<int, int> cacheHits;
    int totalNodes;

public:
    struct SearchInfo {
        Move bestMove;
        int bestEval;
        Line line;
    };

    Searcher() : ttable(10000), totalNodes(0) {}

    SearchInfo alphaBeta(Board& board, int depth, const int, int alpha, int beta);
    SearchInfo getBestMove(Board& board, int depth);
};




#endif
