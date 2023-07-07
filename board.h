#ifndef BOARD_H
#define BOARD_H

#include <iostream>
#include <cstdint>

enum Piece {
    wpawns,
    wknights, 
    wbishops, 
    wrooks, 
    wqueens, 
    wking,
    bpawns,
    bknights, 
    bbishops, 
    brooks, 
    bqueens, 
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
        // 0 = wpawns, 1 = wknights, 2 = wbishops, 3 = wrooks, 4 = wqueens, 5 = wkings ...
        uint64_t pieces[12];
        uint64_t empty;
        uint64_t allPieces[2]; // 0 = white 1 = black
        int enpassantPos; // legal square a pawn can move to to take with enpassant
        bool castle[4]; // 0 = white kingside, 1 = white queen side etc
        bool inCheck[2];
        bool blackToMove; // 0 = white 1 = black

        Board();

        uint64_t getPieceSet(int i);

        void setPieceSet(int i, uint64_t num);

        void setStartPos();

        void updateAllPieces();

        void makeMove(struct Move move);

        friend std::ostream& operator << (std::ostream& o, Board& board);
};

#endif