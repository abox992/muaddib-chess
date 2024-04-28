#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <ctype.h>
#include <bit>
#include "board.h"
#include "zobrist.h"

void printBitboard(uint64_t bitboard) {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int i = ((7 - file) + (8 * rank));
            uint64_t mask = uint64_t(1) << i;
            int bit = ((bitboard & mask) >> i);
            std::cout << bit << " ";
        }

        std::cout << std::endl;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}