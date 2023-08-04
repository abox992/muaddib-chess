#include "evaluate.h"
#include "board.h"

#include <immintrin.h>

const int pieceValue[] = {100, 300, 350, 500, 900}; // pawn, knight, bishop, rook, queen

int evaluation(Board& board) {
    int whiteValue = materialValue(board, white);
    int blackValue = materialValue(board, black);

    int evaluation = whiteValue - blackValue;

    int perspective = board.blackToMove ? -1 : 1;

    return evaluation * perspective;
}

int materialValue(Board& board, int color) {
    int totalValue = 0;
    for (int i = 0; i < 5; i++) {
        int count = _mm_popcnt_u64(board.pieces[i * 2 + color]);
        totalValue += count * pieceValue[i];
    }

    return totalValue;
}