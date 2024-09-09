#ifndef BOARD_STATE_H
#define BOARD_STATE_H

#include <cstdint>
#include <memory>

class BoardState {
public:
    BoardState() = default;
    BoardState& operator=(const BoardState&) = delete;
    BoardState(const BoardState&);

    // 0 = pawns, 2 = knights, 4 = bishops, 6 = rooks, 8 = queens, 10 = kings
    uint64_t pieces[12];

    uint64_t empty;

    // 0 = white 1 = black
    uint64_t allPieces[2];

    // legal square a pawn can move to to take with enpassant
    int enpassantPos;

    // wk, bk, wq, bq
    bool canCastle[4];

    // 0 = white 1 = black
    bool blackToMove;

    int halfMoves;
    int fullMoves;

    // helpful for making/unmaking moves
    std::unique_ptr<BoardState> prevState;
    uint64_t hash;
    int highestRepeat;

};

#endif
