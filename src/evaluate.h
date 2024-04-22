#ifndef EVALUATE_H
#define EVALUATE_H

#include "board.h"

int evaluation(Board& board);
int materialValue(const Board& board, const int color);

#endif