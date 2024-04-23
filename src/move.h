#ifndef MOVE_H
#define MOVE_H

#include <iostream>
#include "board.h"
#include <cassert>
#include "bit_manip.h"

struct Move {
    uint8_t from;
    uint8_t to;
    bool color;
    uint8_t piece; // 0 = pawns, 2 = knights, 4 = bishops, 6 = rooks, 8 = queens, 10 = kings
    uint8_t capturedPiece = 16; // 0 = pawns, 2 = knights, 4 = bishops, 6 = rooks, 8 = queens, 10 = kings
    bool enpessant = false;
    uint8_t castle = 0; // wk, bk, wq, bq
    uint8_t promotion = 0; // knight, bishop, rook, queen

    friend std::ostream& operator << (std::ostream& o, const Move& move);
};

// enum MoveType {
//     NORMAL,
//     PROMOTION  = 1 << 14,
//     EN_PASSANT = 2 << 14,
//     CASTLING   = 3 << 14
// };

// /*
//     bits  0 -  5: from (0-63)
//     bits  6 - 11: to (0-63)
//     bits 12 - 13: promotion piece type (knight(0) bishop(1) rook(2) queen(3))
//     bits 14 - 15: special move flag (promotion(1) enpassant(2) castle(3))
// */
// class Move {
// private:
//     const uint16_t data;

// public:
//     constexpr explicit Move(std::uint16_t d) : data(d) {}

//     constexpr int from_sq() const {
//         return (data >> 6) & 0x3F;
//     }

//     constexpr int to_sq() const {
//         return data & 0x3F;
//     }

//     //friend std::ostream& operator << (std::ostream& o, const Move& move);
// };

#endif