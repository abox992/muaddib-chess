#include "search.h"
#include "board.h"
#include "evaluate.h"
#include "movegen.h"
#include "check_pin_masks.h"
#include "zobrist.h"
#include <cstdint>
#include <unordered_map>

#define DEBUG 1

#define INF (20000)
#define NEG_INF (-20000)
#define WIN_WHITE (10000)
#define WIN_BLACK (10000)

Move getBestMove(Board& board, int depth) {
    SearchInfo result;
    if (board.curState->blackToMove) {
        result = alphaBeta(board, depth, depth, NEG_INF, INF, false);
    } else {
        result = alphaBeta(board, depth, depth, NEG_INF, INF, true);
    }

    return result.bestMove;

}

//std::unordered_map<uint64_t, int> transpositionTable;

SearchInfo alphaBeta(Board& board, int depth, const int startDepth, int alpha, int beta, bool maximizing) {

    SearchInfo info;
    info.bestMove = Move(0);

    std::vector<Move> moveList;
    moveList.reserve(256);

    if (board.curState->blackToMove) {
        generateMoves<ALL, Color::BLACK>(board, moveList);
    } else {
        generateMoves<ALL, Color::WHITE>(board, moveList);
    }

    if (moveList.size() == 0) {
        if (board.inCheck()) {
            info.bestEval = board.curState->blackToMove ? WIN_WHITE + depth : WIN_BLACK - depth;
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
        info.bestEval = evaluation(board);
        return info;
    }

    if (maximizing) {
        //generateMoves<ALL, Color::WHITE>(board, moveList);
        int maxEval = NEG_INF;

        for (auto& move : moveList) {
            board.makeMove(move);
            int eval;
            if (std::popcount(board.curState->prevState->empty) == std::popcount(board.curState->empty) && depth == 0) {
                eval = alphaBeta(board, depth, startDepth, alpha, beta, false).bestEval;
            } else {
                eval = alphaBeta(board, depth - 1, startDepth, alpha, beta, false).bestEval;
            }
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

    } else {

        int minEval = INF;

        for (auto& move : moveList) {
            board.makeMove(move);
            int eval;
            if (std::popcount(board.curState->prevState->empty) == std::popcount(board.curState->empty) && depth == 0) {
                eval = alphaBeta(board, depth, startDepth, alpha, beta, true).bestEval;
            } else {
                eval = alphaBeta(board, depth - 1, startDepth, alpha, beta, true).bestEval;
            }
            board.unmakeMove();

            if constexpr (DEBUG) {
                if (depth == startDepth) {
                    std::cout << move << " " << eval << " " << alpha << " " << beta << '\n';
                }
            }

            if (eval < minEval) {
                minEval = eval;
                info.bestMove = move;
                info.bestEval = minEval;
            }

            beta = std::min(beta, eval);
            if (beta <= alpha) break;
        }

        return info;
    }

}