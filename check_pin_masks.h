#ifndef CHECK_PIN_MASKS_H
#define CHECK_PIN_MASKS_H

#include <cstdint>
#include <tuple>
#include "board.h"

uint64_t attacksOnSquare(Board& board, int color, int pos);

uint64_t attacksToKing(Board& board, int color);
uint64_t attacksToKingXray(Board& board, int color);

// ex input white = 0 -> needs to check what black pieces attack white
uint64_t generateCheckMask(Board& board, int color);
uint64_t generatePinMask(Board& board, int color); // note that this includes checks as well

// checkmask, pinHV, pinDiag
std::tuple<uint64_t, uint64_t, uint64_t> generateCheckAndPinMask(Board& board, int color);

#endif