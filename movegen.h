#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <cstdint>
#include "board.h"
#include "move.h"

int lsb(uint64_t &b);
void initPawnMasks();
void initKnightMasks();
void initKingMasks();
void initBishopMasks();
void initRookMasks();
void initQueenMasks(); // bishop and rook masks must be init first

void initBishopMovesTable();
void initRookMovesTable();

void initMasks();

void generateMoves(Board board, struct Move moveList[], int color);

#endif