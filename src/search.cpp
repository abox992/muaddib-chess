#include "search.h"
#include "board.h"
#include "evaluate.h"
#include "move_list.h"
#include "transpose_table.h"
#include <algorithm>
#include <map>

#define DEBUG 0

#define INF (20000)
#define NEG_INF (-20000)
#define WIN (10000)

TranspositionTable ttable(10000);
std::map<int, int> cacheHits;
int totalNodes;

SearchInfo getBestMove(Board& board, int depth)
{
    cacheHits.clear();
    totalNodes = 0;
    SearchInfo result = alphaBeta(board, depth, depth, NEG_INF, INF);

    // print cache results
    for (auto& v : cacheHits) {
        std::cout << v.first << ": " << v.second << '\n';
    }
    std::cout << totalNodes << '\n';

    return result;
}

struct Line {
    int count; // line length
    Move moves[256];
};

SearchInfo alphaBeta(Board& board, int depth, const int startDepth, int alpha, int beta)
{

    SearchInfo info;
    info.bestMove = Move(0);

    MoveList<MoveFilter::ALL> moveList(board);

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
    if (ttable.contains(curHash)) {
        TtEntry entry = ttable.get(curHash);

        if (entry.depth >= depth) {
            switch (entry.flag) {
            case TtEntry::EXACT:
                info.bestEval = entry.eval;
                return info;
                break;
            case TtEntry::UPPER:
                beta = std::min(beta, entry.eval);
                break;
            case TtEntry::LOWER:
                alpha = std::max(alpha, entry.eval);
                break;
            }

            if (alpha >= beta) {
                info.bestEval = entry.eval;
                return info;
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
        if ((maskForPos(move.to()) & board.getOccupied()) && depth == 1) { // if the node is a capture, extend search
            // extension = 1;
        }

        board.makeMove(move);

        if (board.inCheck()) { // if the move puts the king in check, extend search
            // extension = 1;
        }

        int eval = -alphaBeta(board, depth - 1 + extension, startDepth, -beta, -alpha).bestEval;

        totalNodes++;

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
    TtEntry entry;
    entry.eval = maxEval;
    
    // set flag
    if (maxEval <= originalAlpha) {
        entry.flag = TtEntry::UPPER;
    } else if (maxEval >= beta) {
        entry.flag = TtEntry::LOWER;
    } else {
        entry.flag = TtEntry::EXACT;
    }

    // set depth
    entry.depth = depth;

    ttable.put(curHash, entry);

    return info;
}
