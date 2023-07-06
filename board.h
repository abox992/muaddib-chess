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
        bool  passant[2][8]; // 0 = white 1 = black, index set to true if that file is takeable with enpesant

        Board();

        uint64_t getPieceSet(int i);

        void setPieceSet(int i, uint64_t num);

        void setStartPos();

        void updateAllPieces();

        friend std::ostream& operator << (std::ostream& o, Board& board);
};

#endif