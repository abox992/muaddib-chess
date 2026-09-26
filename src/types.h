#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

enum class PieceType : uint8_t {
    PAWNS   = 0,
    KNIGHTS = 2,
    BISHOPS = 4,
    ROOKS   = 6,
    QUEENS  = 8,
    KINGS   = 10,
};

enum class Piece : uint8_t {
    WHITE_PAWN   = 0,
    WHITE_KNIGHT = 2,
    WHITE_BISHOP = 4,
    WHITE_ROOK   = 6,
    WHITE_QUEEN  = 8,
    WHITE_KING   = 10,

    BLACK_PAWN   = 1,
    BLACK_KNIGHT = 3,
    BLACK_BISHOP = 5,
    BLACK_ROOK   = 7,
    BLACK_QUEEN  = 9,
    BLACK_KING   = 11,
};

// Note that this is used in comparison to both PieceType and Piece, so it needs to be a separate type.
struct NoPiece {
    static constexpr uint8_t value = 16;

    constexpr operator PieceType() const noexcept { return static_cast<PieceType>(value); }
    constexpr operator Piece() const noexcept { return static_cast<Piece>(value); }
    constexpr operator uint8_t() const noexcept { return value; }
};

inline constexpr NoPiece NO_PIECE{};

// Piece and PieceType values are bitboard indexes. Enum class does not convert to int.
constexpr int toInt(Piece piece) noexcept { return static_cast<int>(piece); }
constexpr int toInt(PieceType type) noexcept { return static_cast<int>(type); }

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
