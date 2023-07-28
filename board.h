#ifndef BOARD_H
#define BOARD_H

#include <iostream>
#include <cstdint>

enum Piece {
    wpawns,
    bpawns, 
    wknights, 
    bknights, 
    wbishops, 
    bbishops,
    wrooks,
    brooks, 
    wqueens, 
    bqueens, 
    wking, 
    bking
};

enum File {
    a, b, c, d, e, f, g, h
};

enum Color {
    white, black
};

class Board {
    public:
        uint64_t pieces[12];   // 0 = wpawns, 1 = bpawns, ... knights, bishops, rooks, queens, kings
        uint64_t empty;
        uint64_t allPieces[2]; // 0 = white 1 = black
        int      enpassantPos; // legal square a pawn can move to to take with enpassant
        bool     castle[4];    // wk, bk, wq, bq
        bool     blackToMove;  // 0 = white 1 = black

        Board();

        uint64_t getPieceSet(int i);

        void setPieceSet(int i, uint64_t num);

        int getPiecePos(int i);

        void setStartPos();

        void updateAllPieces();

        void makeMove(struct Move move);

        friend std::ostream& operator << (std::ostream& o, Board& board);
};

#endif