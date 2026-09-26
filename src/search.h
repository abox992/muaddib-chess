#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "move.h"
#include "transpose_table.h"
#include <array>
#include <chrono>
#include <tuple>
#include <utility>
#include <vector>

class Searcher {
private:
    static constexpr int kMaxSearchPly = 256;

    TranspositionTable                ttable;
    std::vector<Move>                 pv;
    std::vector<std::pair<Move, int>> rootMoves;
    Move                              bestMove;
    bool                              stopSearch;
    bool                              collectRoot;
    // Quiet moves that caused a beta cutoff at this ply. First slot is the newer one.
    std::array<std::array<Move, 2>, kMaxSearchPly> killers{};

public:
    Searcher() :
        ttable(64),
        stopSearch(false),
        collectRoot(false) { }

    std::tuple<Move, int> search(Board& board, const int depth, const int ply, int alpha, int beta);
    std::tuple<Move, int> getBestMove(Board& board, int depth);
    std::tuple<Move, int> searchDepth(Board& board, int depth, bool collectRootMoves = false);
    std::tuple<Move, int> iterativeDeepening(Board& board, std::chrono::milliseconds timeMs);

    void newSearch() { ttable.NewSearch(); }

    const std::vector<std::pair<Move, int>>& getRootMoves() const { return rootMoves; }

    int quiesce(Board& board, int alpha, int beta, int ply);
};

#endif
