#ifndef PRECOMPUTE_MASKS_H
#define PRECOMPUTE_MASKS_H

#include <cstdint>

extern uint64_t pawnMasks[64];
extern uint64_t knightMasks[64];
extern uint64_t kingMasks[64];
extern uint64_t bishopMasks[64];
extern uint64_t rookMasks[64];
extern uint64_t queenMasks[64];

extern uint64_t rookLegalMoves[64][16384];
extern uint64_t bishopLegalMoves[64][16384];

#endif