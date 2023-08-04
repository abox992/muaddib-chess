#include "search.h"
#include "board.h"
#include "evaluate.h"
#include "movegen.h"
#include "check_pin_masks.h"
#include <cstdint>
#include <climits>

using namespace std;

#define INFINITY (200000)
#define NEGATIVE_INFINITY (-200000)

struct Move bestMove;

Move getBestMove(Board& board, int depth) {
    search(board, depth, depth, NEGATIVE_INFINITY, INFINITY);

    return bestMove;
}

// (* Initial call *)
// alphabeta(origin, depth, −∞, +∞, TRUE)
int search(Board& board, int startDepth, int depth, int alpha, int beta) { //, bool maximizing
    int bestValue = NEGATIVE_INFINITY;

    if (depth == 0) {
        return quiesce(board, alpha, beta);
    }

    if (board.halfMoves >= 100) {
        return 0; // 50 move rule, stalemate
    }

    struct Move moveList[256];
    int moveCount = generateMoves(ALL_MOVES, board, moveList, board.blackToMove);

    if (moveCount == 0) {
        if (board.inCheck()) {
            return NEGATIVE_INFINITY + startDepth - depth; // loss
        } else {
            return 0; // stalemate
        }
    }

    for (int i = 0; i < moveCount; i++) {
        Board child = board;
        child.makeMove(moveList[i]);
        int value = -1 * search(child, startDepth, depth - 1, -beta, -alpha);

        if (value >= beta) {

            // if (depth == startDepth) {
            //     bestMove = moveList[i];
            // }

            return value;
        }

        if (value > bestValue) {
            bestValue = value;
            if (depth == startDepth) {
                bestMove = moveList[i];
            }
            if (value > alpha) {
                alpha = value;
            }
        }

        // if (depth == startDepth) {
        //     cout << moveList[i] << " " << value << endl;
        // }
    }

    return bestValue;

}

int quiesce(Board& board, int alpha, int beta) {
    int stand_pat = evaluation(board);
    if( stand_pat >= beta )
        return beta;
    if( alpha < stand_pat )
        alpha = stand_pat;

    struct Move moveList[256];
    int moveCount = generateMoves(CAPTURES(board), board, moveList, board.blackToMove);

    for (int i = 0; i < moveCount; i++)  {
        Board child = board;
        child.makeMove(moveList[i]);
        int score = -1 * quiesce(child, -beta, -alpha);

        if( score >= beta )
            return beta;
        if( score > alpha )
           alpha = score;
    }

    // if (moveCount == 0) {
    //     return stand_pat;
    // }

    return alpha;
}