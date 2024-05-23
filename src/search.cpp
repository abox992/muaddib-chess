#include "search.h"
#include "board.h"
#include "evaluate.h"
#include "movegen.h"
#include "check_pin_masks.h"
#include "zobrist.h"
#include "move_list.h"
#include <cstdint>
#include "transpose_table.h"

#define DEBUG 0

#define INF (20000)
#define NEG_INF (-20000)
#define WIN_WHITE (10000)
#define WIN_BLACK (10000)

Move getBestMove(Board& board, int depth) {
    SearchInfo result = alphaBeta(board, depth, depth, NEG_INF, INF);
    return result.bestMove;
}

struct Line {
    int count; // line length
    Move moves[256];
};

SearchInfo alphaBeta(Board& board, int depth, const int startDepth, int alpha, int beta) {

    SearchInfo info;
    info.bestMove = Move(0);

    MoveList<MoveFilter::ALL> moveList(board);

    if (moveList.size() == 0) {
        if (board.inCheck()) {
            const int perspective = board.curState->blackToMove ? -1 : 1;
            info.bestEval = board.curState->blackToMove ? WIN_WHITE + depth : WIN_BLACK - depth;
            info.bestEval *= perspective;
            return info;
        }
        info.bestEval = 0;
        return info;
    }

    if (board.curState->highestRepeat == 3) {
        info.bestEval = 0;
        return info;
    }

    if (depth == 0) {
        const int perspective = board.curState->blackToMove ? -1 : 1;
        info.bestEval = evaluation(board) * perspective;
        return info;
    }

    int maxEval = NEG_INF;

    for (auto& move : moveList) {
        int extension = 0;
        if (maskForPos(move.to()) & !board.curState->empty) { // if the node is a capture, extend search
            extension++;
        }

        board.makeMove(move);

        if (board.inCheck()) { // if the move puts the king in check, extend search
            extension++;
        }

        int eval = -alphaBeta(board, depth - 1 + extension, startDepth, -beta, -alpha).bestEval;
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
        if (beta <= alpha) break;
    }

    return info;

}