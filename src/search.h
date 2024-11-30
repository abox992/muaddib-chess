#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "move.h"
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
    /*struct SearchInfo {*/
    /*    Move bestMove;*/
    /*    int bestEval;*/
    /*    //Line line;*/
    /*};*/

    Searcher()
        : ttable(256) { }

    std::tuple<Move, int> alphaBeta(Board& board, int depth, const int, int alpha, int beta);
    std::tuple<Move, int> getBestMove(Board& board, int depth);

    int quiesce(Board& board, int alpha, int beta);
};

#endif
