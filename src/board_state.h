#ifndef BOARD_STATE_H
#define BOARD_STATE_H

#include <cstdint>
#include "move.h"

struct Undo {
    uint64_t hash;      // position hash before this move
    Move move;
    uint16_t halfMoves;
    uint8_t captured;   // NO_PIECE, or the piece pos/index that was removed
    uint8_t castle;
    uint8_t ep;
};

#endif
