#include "search.h"
#include "board.h"
#include "evaluate.h"
#include "move_list.h"
#include "transpose_table.h"
#include <algorithm>

#define DEBUG 0 

#define INF (20000)
#define NEG_INF (-20000)
#define WIN (10000)


Searcher::SearchInfo Searcher::getBestMove(Board& board, int depth)
{
    SearchInfo result = alphaBeta(board, depth, depth, NEG_INF, INF);

    return result;
}



Searcher::SearchInfo Searcher::alphaBeta(Board& board, int depth, const int startDepth, int alpha, int beta)
{

    SearchInfo info;
    info.bestMove = Move(0);

    MoveList<MoveFilter::ALL> moveList(board);
    if (depth == startDepth) {
        std::cout << moveList.size() << '\n';

    }

    __builtin_prefetch(&ttable);

    const int perspective = board.blackToMove() ? -1 : 1;

    if (moveList.size() == 0) {
        if (board.inCheck()) {
            /*info.bestEval = board.blackToMove() ? WIN + depth : (-WIN) - depth;*/
            info.bestEval = -(WIN + depth);
            /*info.bestEval *= perspective;*/

            return info;
        }
        info.bestEval = 0;
        return info;
    }

    if (board.getHighestRepeat() == 3) {
        info.bestEval = 0;
        return info;
    }

    // check transposition table
    int originalAlpha = alpha;
    const int curHash = board.hash();
    if (depth == startDepth) {
        if (ttable.contains(curHash)) {
            TTEntry entry = ttable.get(curHash);

            if (entry.depth >= depth) {
                switch (entry.flag) {
                    case TTEntry::EXACT:
                        info.bestEval = entry.eval;
                        info.bestMove = entry.move;
                        return info;
                        break;
                    case TTEntry::UPPER:
                        beta = std::min(beta, entry.eval);
                        break;
                    case TTEntry::LOWER:
                        alpha = std::max(alpha, entry.eval);
                        break;
                }

                if (alpha >= beta) {
                    info.bestEval = entry.eval;
                    info.bestMove = entry.move;
                    return info;
                }
            }
        }
    }

    if (depth == 0) {
        info.bestEval = evaluation(board) * perspective;

        /*ttable.put(board.hash(), info.bestEval);*/

        return info;
    }

    int maxEval = NEG_INF;

    for (const auto& move : moveList) {
        int extension = 0;

        board.makeMove(move);

        if (board.inCheck() && depth == 1) { // if the move puts the king in check, extend search
            //extension = 1;
        }
        SearchInfo result;

        if ((maskForPos(move.to()) & board.getOccupied()) && depth == 1) { // if the node is a capture, extend search
            result.bestEval = -quiesce(board, -beta, -alpha);
        } else {
            result.bestEval = -alphaBeta(board, depth - 1 + extension, startDepth, -beta, -alpha).bestEval;
        }
        int eval = result.bestEval;

        board.unmakeMove();

        if constexpr (DEBUG) {
            if (depth == startDepth) {
                std::cout << move << " " << eval << " " << alpha << " " << beta << '\n';
            }
        }

        if (eval > maxEval) {
            maxEval = eval;
            info.bestMove = move;
            info.bestEval = maxEval;
        }

        alpha = std::max(alpha, eval);
        if (beta <= alpha)
            break;
    }

    // update transposition table with new values
    TTEntry entry;
    entry.eval = info.bestEval;
    entry.move = info.bestMove;
    
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

    ttable.put(curHash, entry);

    return info;
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

    MoveList<MoveFilter::CAPTURES> moveList(board);

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



