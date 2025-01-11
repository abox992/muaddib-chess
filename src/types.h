#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

enum PieceType : uint8_t {
    PAWNS    = 0,
    KNIGHTS  = 2,
    BISHOPS  = 4,
    ROOKS    = 6,
    QUEENS   = 8,
    KINGS    = 10,
    NO_PIECE = 16
};

enum File {
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H
};

enum Color {
    WHITE,
    BLACK
};

enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    TOP_LEFT,
    BOTTOM_RIGHT,
    TOP_RIGHT,
    BOTTOM_LEFT
};

enum GenType {
    ALL,
    CAPTURES,
    QUIET
};

#endif
