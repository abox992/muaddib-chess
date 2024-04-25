#ifndef TEST_SUITE_H
#define TEST_SUITE_H

#include "board.h"
#include <string>
#include <tuple>
#include <vector>

uint64_t moveGenTest(int startDepth, Board& board);

void runTests();

#endif