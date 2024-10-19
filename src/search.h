#ifndef SEARCH_H
#define SEARCH_H

#include "move.h"
#include "board.h"
#include "transpose_table.h"

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

public:
    struct SearchInfo {
        Move bestMove;
        int bestEval;
        Line line;
    };

    Searcher() : ttable(64) {}

    SearchInfo alphaBeta(Board& board, int depth, const int, int alpha, int beta);
    SearchInfo getBestMove(Board& board, int depth);

    int quiesce(Board& board, int alpha, int beta); 
};




#endif
