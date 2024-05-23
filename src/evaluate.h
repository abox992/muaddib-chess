#ifndef EVALUATE_H
#define EVALUATE_H

#include "board.h"

int evaluation(const Board& board);
int materialValue(const Board& board, const int color);
int pieceScope(const Board& board);

#endif