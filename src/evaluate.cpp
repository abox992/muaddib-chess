#include "evaluate.h"
#include "bit_manip.h"
#include "board.h"
#include "types.h"
#include "move_list.h"
#include <bit>
#include <cstdint>

// returns an evaluation of board where positive is good for white and negative is good for black
int evaluation(const Board& board) {

    int whiteValue = 2 * materialValue(board, Color::WHITE);
    int blackValue = 2 * materialValue(board, Color::BLACK);

    int whitePosValue = piecePosValue(board, Color::WHITE);
    int blackPosValue = piecePosValue(board, Color::BLACK);

    whiteValue += whitePosValue;
    blackValue += blackPosValue;

    int eval = whiteValue - blackValue;

    return eval;
}

const int pieceTables[6][64] = {
    { /* pawn start */
        0, 0, 0, 0, 0, 0, 0, 0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
        5, 5, 10, 25, 25, 10, 5, 5,
        0, 0, 0, 20, 20, 0, 0, 0,
        5, -5, -10, 0, 0, -10, -5, 5,
        5, 10, 10, -20, -20, 10, 10, 5,
        0, 0, 0, 0, 0, 0, 0, 0 },
    { /* knight */
        -50, -40, -30, -30, -30, -30, -40, -50,
        -40, -20, 0, 0, 0, 0, -20, -40,
        -30, 0, 10, 15, 15, 10, 0, -30,
        -30, 5, 15, 20, 20, 15, 5, -30,
        -30, 0, 15, 20, 20, 15, 0, -30,
        -30, 5, 10, 15, 15, 10, 5, -30,
        -40, -20, 0, 5, 5, 0, -20, -40,
        -50, -40, -30, -30, -30, -30, -40, -50 },
    { /* bishop */
        -20, -10, -10, -10, -10, -10, -10, -20,
        -10, 0, 0, 0, 0, 0, 0, -10,
        -10, 0, 5, 10, 10, 5, 0, -10,
        -10, 5, 5, 10, 10, 5, 5, -10,
        -10, 0, 10, 10, 10, 10, 0, -10,
        -10, 10, 10, 10, 10, 10, 10, -10,
        -10, 5, 0, 0, 0, 0, 5, -10,
        -20, -10, -10, -10, -10, -10, -10, -20 },
    { /* rook */
        0, 0, 0, 0, 0, 0, 0, 0,
        5, 10, 10, 10, 10, 10, 10, 5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        0, 0, 0, 5, 5, 0, 0, 0 },
    { /* queen */
        -20, -10, -10, -5, -5, -10, -10, -20,
        -10, 0, 0, 0, 0, 0, 0, -10,
        -10, 0, 5, 5, 5, 5, 0, -10,
        -5, 0, 5, 5, 5, 5, 0, -5,
        0, 0, 5, 5, 5, 5, 0, -5,
        -10, 5, 5, 5, 5, 5, 0, -10,
        -10, 0, 5, 0, 0, 0, 0, -10,
        -20, -10, -10, -5, -5, -10, -10, -20 },
    { /* king start */
        -80, -70, -70, -70, -70, -70, -70, -80,
        -60, -60, -60, -60, -60, -60, -60, -60,
        -40, -50, -50, -60, -60, -50, -50, -40,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -20, -30, -30, -40, -40, -30, -30, -20,
        -10, -20, -20, -20, -20, -20, -20, -10,
        20, 20, -5, -5, -5, -5, 20, 20,
        20, 30, 10, 0, 0, 10, 30, 20 }
};

int piecePosValue(const Board& board, const int color) {
    int value = 0;
    for (int i = 0; i < 5; i++) {
        uint64_t bb = board.getBB(i * 2 + color);
        while (bb) {
            int curSquare = tz_count(bb);
            pop_lsb(bb);

            if (color == Color::WHITE) {
                curSquare = 63 - curSquare;
            }

            value += pieceTables[i][curSquare];
        }
    }

    return value;
}

int materialValue(const Board& board, const int color) {
    const int pieceValue[] = { 100, 300, 320, 500, 900 }; // pawn, knight, bishop, rook, queen
    int totalValue = 0;
    for (int i = 0; i < 5; i++) {
        int count = std::popcount(board.getBB(i * 2 + color)); // std::popcount(board.curState->pieces[i * 2 + color]);
        totalValue += count * pieceValue[i];
    }

    return totalValue;
}

int pieceScope(const Board& board) {
    MoveList<ALL> moveListWhite(board, Color::WHITE);
    MoveList<ALL> moveListBlack(board, Color::BLACK);

    return (moveListWhite.size() - moveListBlack.size()) / 2;
}
