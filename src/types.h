#ifndef TYPES_H
#define TYPES_H

enum ColorPiece {
    WPAWNS,
    BPAWNS,
    WKNIGHTS,
    BKNIGHTS,
    WBISHOPS,
    BBISHOPS,
    WROOKS,
    BROOKS,
    WQUEENS,
    BQUEENS,
    WKING,
    BKING
};

enum Piece {
    PAWNS = 0,
    KNIGHTS = 2,
    BISHOPS = 4,
    ROOKS = 6,
    QUEENS = 8,
    KINGS = 10
};

enum File {
    A, B, C, D, E, F, G, H
};

enum Color {
    WHITE, BLACK
};

enum Direction {
    UP, DOWN, LEFT, RIGHT, TOP_LEFT, BOTTOM_RIGHT, TOP_RIGHT, BOTTOM_LEFT
};

enum MoveFilter {
    ALL,
    CAPTURES,
    QUIET
};

#endif