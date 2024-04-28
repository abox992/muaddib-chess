#ifndef HELPERS_H
#define HELPERS_H

#include <cstdint>
#include <string>
#include <vector>
#include "board.h"

// prints a U64 in an 8x8 format
void printBitboard(uint64_t bitboard);

std::vector<std::string> split(const std::string &s, char delim);

#endif