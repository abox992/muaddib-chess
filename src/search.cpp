#include "search.h"
#include "board.h"
#include "evaluate.h"
#include "movegen.h"
#include "check_pin_masks.h"
#include "zobrist.h"
#include <cstdint>
#include <climits>

#define DEBUG 0

Move bestMove;
int bestEval = NEGATIVE_INFINITY;

Move getBestMove(Board& board, int depth) {
    bestMove = Move(0);
    search(board, depth, depth, NEGATIVE_INFINITY, INFINITY, false);

    return bestMove;
}

bool flag = false;

// (* Initial call *)
// alphabeta(origin, depth, −∞, +∞, TRUE)
int search(Board& board, int startDepth, int depth, int alpha, int beta, bool capture) {

    if (depth == 0) {

        if (capture) {
            return quiesce(board, alpha, beta);
        } else {
            return evaluation(board);
        }

    }

    if (board.curState->halfMoves >= 100) {
        return 0; // 50 move rule, stalemate
    }

    if (board.curState->highestRepeat == 3) {
        return 0;
    }

    std::vector<Move> moveList;
    moveList.reserve(256);

    if (board.curState->blackToMove) {
        generateMoves<MoveFilter::CAPTURES, Color::BLACK>(board, moveList);
    } else {
        generateMoves<MoveFilter::CAPTURES, Color::WHITE>(board, moveList);
    }

    int captureMoveCount = moveList.size();

    for (int i = 0; i < captureMoveCount; i++) {

        board.makeMove(moveList[i]);
        int eval = -search(board, startDepth, depth - 1, -beta, -alpha, true);
        board.unmakeMove();

        if (DEBUG) {
            if (depth == startDepth) {
                std::cout << i << " " << moveList[i] << " " << eval << std::endl;
            }
        }

        if (eval >= beta) {
            return beta;
        }

        if (eval > alpha) {
            alpha = eval;
            if (depth == startDepth) {
                bestMove = moveList[i];
            }
        }

    }

    if (board.curState->blackToMove) {
        generateMoves<MoveFilter::QUIET, Color::BLACK>(board, moveList);
    } else {
        generateMoves<MoveFilter::QUIET, Color::WHITE>(board, moveList);
    }

    int quietMoveCount = moveList.size() - captureMoveCount;
    for (int i = 0; i < quietMoveCount; i++) {

        board.makeMove(moveList[i]);
        int eval = -search(board, startDepth, depth - 1, -beta, -alpha, false);
        board.unmakeMove();

        if (DEBUG) {
            if (depth == startDepth) {
                std::cout << i << " " << moveList[i] << " " << eval << std::endl;
            }
        }

        if (eval >= beta) {
            return beta;
        }

        if (eval > alpha) {
            alpha = eval;
            if (depth == startDepth) {
                bestMove = moveList[i];
            }
        }

    }

    int moveCount = quietMoveCount + captureMoveCount;
    if (moveCount == 0) {
        if (board.inCheck()) {
            return NEGATIVE_INFINITY + (startDepth - depth); // loss, + (startDepth - depth) makes faster mates worse
        } else {
            return 0; // stalemate
        }
    }

    return alpha;

}

int quiesce(Board& board, int alpha, int beta) {
    int eval = evaluation(board);

    if (eval >= beta) {
        return beta;
    }
    if (alpha < eval) {
        alpha = eval;
    }

    // struct Move moveList[256];
    std::vector<Move> moveList;
    moveList.reserve(256);

    if (board.curState->blackToMove) {
        generateMoves<MoveFilter::CAPTURES, Color::BLACK>(board, moveList);
    } else {
        generateMoves<MoveFilter::CAPTURES, Color::WHITE>(board, moveList);
    }

    //generateMoves<MoveType::CAPTURES>(board, moveList, board.curState->blackToMove);

    for (size_t i = 0; i < moveList.size(); i++) {

        board.makeMove(moveList[i]);
        int eval = -quiesce(board, -beta, -alpha);
        board.unmakeMove();

        if (eval >= beta) {
            return beta;
        }
        if (alpha < eval) {
            alpha = eval;
        }

    }

    return alpha;
}