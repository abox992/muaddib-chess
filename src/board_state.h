#ifndef BOARD_STATE_H
#define BOARD_STATE_H

#include "types.h"
#include <cstdint>
#include <memory>
#include <array>

class BoardState {
public:
    BoardState() = default;
    BoardState& operator=(const BoardState&) = delete;
    explicit BoardState(const BoardState&);

    // 0 = pawns, 2 = knights, 4 = bishops, 6 = rooks, 8 = queens, 10 = kings
    std::array<uint64_t, 12> pieces;

    uint64_t empty;

    // 0 = white 1 = black
    std::array<uint64_t, 2> allPieces;

    // legal square a pawn can move to to take with enpassant
    int enpassantPos;

    // wk, bk, wq, bq
    std::array<bool, 4> canCastle;

    // 0 = white 1 = black
    bool blackToMove;

    int halfMoves;
    int fullMoves;

    std::array<PieceType, 64> pieceOnSquare;

    // helpful for making/unmaking moves
    std::unique_ptr<BoardState> prevState;
    uint64_t hash;
};

#endif
