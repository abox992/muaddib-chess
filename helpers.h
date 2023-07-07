#ifndef HELPERS_H
#define HELPERS_H

#include <cstdint>
#include <string>
#include <vector>
#include "board.h"

// prints a U64 in an 8x8 format
void printBitboard(uint64_t bitboard);

// returns index of lsb and sets said bit to 0
int lsb(uint64_t &b);

std::vector<std::string> split(const std::string &s, char delim);

// https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
Board generateBoardFromFen(std::string fen);

#endif