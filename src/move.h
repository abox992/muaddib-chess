#ifndef MOVE_H
#define MOVE_H

#include <cassert>
#include <cstdint>
#include <iostream>

enum MoveType {
    NORMAL,
    PROMOTION,
    EN_PASSANT,
    CASTLE
};

enum PromoPiece {
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN
};

/*
    bits  0 -  5: from (0-63)
    bits  6 - 11: to (0-63)
    bits 12 - 13: promotion piece type (knight(0) bishop(1) rook(2) queen(3))
    bits 14 - 15: special move flag (promotion(1) enpassant(2) castle(3))
*/
class Move {
private:
    uint16_t data = 0;

public:
    Move() = default;
    constexpr explicit Move(std::uint16_t d)
        : data(d) { }

    template <MoveType moveType>
    static constexpr Move make(int from, int to, PromoPiece pt = PromoPiece::KNIGHT) {
        return Move((moveType << 14) | (pt << 12) | (to << 6) | from);
    }

    inline constexpr int from() const {
        return data & 0x3F;
    }

    inline constexpr int to() const {
        return (data >> 6) & 0x3F;
    }

    inline constexpr PromoPiece promotionPiece() const {
        return PromoPiece((data >> 12) & 3);
    }

    inline constexpr MoveType moveType() const {
        return MoveType((data >> 14) & 3);
    }

    inline constexpr bool isNull() const {
        return data == 0;
    }

    inline constexpr bool operator==(const Move& m) const { return data == m.data; }
    constexpr bool operator!=(const Move& m) const { return data != m.data; }

    friend std::ostream& operator<<(std::ostream& o, const Move& move);
};

#endif
