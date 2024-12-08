#include "search.h"
#include "board.h"
#include "evaluate.h"
#include "move_list.h"
#include "transpose_table.h"
#include <algorithm>

#define DEBUG 1

#define INF (20000)
#define NEG_INF (-20000)
#define WIN (10000)

std::tuple<Move, int> Searcher::getBestMove(Board& board, int depth) {
    Searcher::Line line;
    return alphaBeta(board, depth, depth, NEG_INF, INF, line);
}

std::tuple<Move, int> Searcher::alphaBeta(Board& board, int depth, const int startDepth, int alpha, int beta, Line& line) {

    Move bestMove;
    int bestEval;

    bestMove = Move(0);

    MoveList<ALL> moveList(board);
    if (depth == startDepth) {
        std::cout << moveList.size() << '\n';
    }

    moveList.sort(board);

    // const int perspective = board.blackToMove() ? -1 : 1;

    // no moves means we are either in checkmate or a stalemate
    if (moveList.size() == 0) {
        if (board.inCheck()) {
            bestEval = -(WIN + depth);

            return { bestMove, bestEval };
        }
        bestEval = 0;
        return { bestMove, bestEval };
    }

    // depth limit reached, return evaluation
    if (depth == 0) {
        /*bestEval = evaluation(board) * perspective;*/
        bestEval = quiesce(board, alpha, beta);

        return { bestMove, bestEval };
    }

    if (board.getRepeats(board.hash()) == 2) {
        return { bestMove, -1 };
    }

    // check transposition table for already computed position
    int originalAlpha = alpha;
    const int curHash = board.hash();
    if (ttable.contains(curHash)) {
        TTData entry = ttable.get(curHash);

        if (entry.depth >= depth) {
            switch (entry.flag) {
            case TTEntry::EXACT:
                bestEval = entry.eval;
                bestMove = entry.move;
                return { bestMove, bestEval };
                break;
            case TTEntry::UPPER:
                beta = std::min(beta, entry.eval);
                break;
            case TTEntry::LOWER:
                alpha = std::max(alpha, entry.eval);
                break;
            }

            if (alpha >= beta) {
                bestEval = entry.eval;
                bestMove = entry.move;
                return { bestMove, bestEval };
            }
        }
    }

    int maxEval = NEG_INF;

    for (const auto& move : moveList) {
        int extension = 0;

        board.makeMove(move);

        int curEval = -std::get<1>(alphaBeta(board, depth - 1 + extension, startDepth, -beta, -alpha, line));

        board.unmakeMove();

        if constexpr (DEBUG) {
            if (depth == startDepth) {
                std::cout << move << " " << curEval << '\n';
            }
        }

        if (curEval > maxEval) {
            maxEval = curEval;
            bestMove = move;
            bestEval = maxEval;
        }

        alpha = std::max(alpha, curEval);
        if (alpha >= beta)
            break;
    }

    // update transposition table with new values
    TTEntry entry;
    entry.eval = bestEval;
    entry.move = bestMove;

    // set flag
    if (maxEval <= originalAlpha) {
        entry.flag = TTEntry::UPPER;
    } else if (maxEval >= beta) {
        entry.flag = TTEntry::LOWER;
    } else {
        entry.flag = TTEntry::EXACT;
    }

    // set depth
    entry.depth = depth;

    /*ttable.put(curHash, entry);*/
    ttable.save(curHash, entry);

    return { bestMove, bestEval };
}

int Searcher::quiesce(Board& board, int alpha, int beta) {
    const int perspective = board.blackToMove() ? -1 : 1;

    int standPat = evaluation(board) * perspective;

    if (standPat >= beta) {
        return beta;
    }

    if (standPat > alpha) {
        alpha = standPat;
    }

    MoveList<CAPTURES> moveList(board);

    for (auto& move : moveList) {
        board.makeMove(move);
        int eval = -quiesce(board, -beta, -alpha);
        board.unmakeMove();

        if (eval >= beta) {
            return beta;
        }

        if (eval > alpha) {
            alpha = eval;
        }
    }

    return alpha;
}
