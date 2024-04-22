#include "evaluate.h"
#include "board.h"
#include "constants.h"
#include <bit>

const int pieceValue[] = {100, 300, 320, 500, 900}; // pawn, knight, bishop, rook, queen

int evaluation(Board& board) {
    int whiteValue = materialValue(board, Color::WHITE);
    int blackValue = materialValue(board, Color::BLACK);

    int eval = whiteValue - blackValue;

    return board.state.blackToMove ? -eval : eval;
}

int materialValue(const Board& board, const int color) {
    int totalValue = 0;
    for (int i = 0; i < 5; i+=2) {
        int count = std::popcount(board.state.pieces[i * 2 + color]);
        totalValue += count * pieceValue[i];
    }

    return totalValue;
}