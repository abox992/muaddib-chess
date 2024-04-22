#ifndef PRECOMPUTE_MASKS_H
#define PRECOMPUTE_MASKS_H

#include <cstdint>

extern uint64_t pawnMoveMasks[2][64];
extern uint64_t pawnAttackMasks[2][64];
extern uint64_t knightMasks[64];
extern uint64_t kingMasks[64];
extern uint64_t bishopMasks[64];
extern uint64_t rookMasks[64];

extern uint64_t rookLegalMoves[64][16384];
extern uint64_t bishopLegalMoves[64][16384];

extern uint64_t checkMasksHV[64][16384];
extern uint64_t checkMasksDiag[64][16384];

extern uint64_t castleMasks[4]; // wk, bk, wq, bq 
extern uint64_t castleSquares[4]; // wk, bk, wq, bq 
extern uint64_t castleRookSquares[4]; // wk, bk, wq, bq 
extern uint64_t originalRookSquares[4]; // wk, bk, wq, bq 

extern int promoPieces[4]; // n,b,r,q pos in pieces array

extern uint64_t rowMasks[64];
extern uint64_t colMasks[64];

extern uint64_t directionMasks[8][64];

void initPawnMasks();
void initKnightMasks();
void initKingMasks();
void initBishopMasks();
void initRookMasks();

void initBishopMovesTable();
void initRookMovesTable();

void initCastleMasks();

void initRowColMasks();

void initMasks();


#endif