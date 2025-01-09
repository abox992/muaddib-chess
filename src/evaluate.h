#ifndef EVALUATE_H
#define EVALUATE_H

#include "board.h"

int evaluation(const Board& board);
int materialValue(const Board& board, const Color color);
int piecePosValue(const Board& board, const Color color);
int pieceScope(const Board& board);
int castlePenalty(const Board& board, const Color color);

#endif
