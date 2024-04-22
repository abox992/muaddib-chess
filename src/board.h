#ifndef BOARD_H
#define BOARD_H

#include <iostream>
#include <cstdint>
#include "move.h"
#include <vector>

struct BoardFlags {
    const bool wCastleL;

    const bool blackToMove;

    constexpr BoardFlags(bool a, bool b) : wCastleL(a), blackToMove(b) {}
};

struct BoardState {
    // 0 = pawns, 2 = knights, 4 = bishops, 6 = rooks, 8 = queens, 10 = kings
    uint64_t pieces[12];

    uint64_t empty;

    // 0 = white 1 = black
    uint64_t allPieces[2];
    
    // legal square a pawn can move to to take with enpassant
    int      enpassantPos;

    // wk, bk, wq, bq
    bool     canCastle[4];

    // 0 = white 1 = black
    bool     blackToMove;

    int      halfMoves;
    int      fullMoves;
};

struct Board {
    public:
        BoardState state;

        std::vector<BoardState> stateHistory;

        std::vector<uint64_t> seenPositions;
        int highestRepeat = 0;

        Board();

        Board(const Board& copy);

        void setPieceSet(int i, uint64_t num);

        void setStartPos();

        void updateAllPieces();

        void makeMove(const Move& move);
        void unmakeMove();

        bool inCheck() const;

        friend std::ostream& operator<<(std::ostream& o, Board& board);
};

#endif