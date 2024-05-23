#include "evaluate.h"
#include "board.h"
#include "types.h"
#include "move_list.h"
#include <bit>

int evaluation(const Board& board) {

    int whiteValue = materialValue(board, Color::WHITE);
    int blackValue = materialValue(board, Color::BLACK);

    int eval = whiteValue - blackValue;

    //eval += pieceScope(board);

    return eval;
    //return board.curState->blackToMove ? -eval : eval;
}

int materialValue(const Board& board, const int color) {
    const int pieceValue[] = {100, 300, 320, 500, 900}; // pawn, knight, bishop, rook, queen
    int totalValue = 0;
    for (int i = 0; i < 5; i++) {
        int count = std::popcount(board.curState->pieces[i * 2 + color]);
        totalValue += count * pieceValue[i];
    }

    return totalValue;
}

int pieceScope(const Board& board) {
    MoveList<MoveFilter::ALL> moveListWhite(board, Color::WHITE);
    MoveList<MoveFilter::ALL> moveListBlack(board, Color::BLACK);

    return (moveListWhite.size() - moveListBlack.size()) / 2;
}