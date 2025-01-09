#ifndef HELPERS_H
#define HELPERS_H

#include <cstdint>
#include <string>
#include <vector>
#include "board.h"

// prints a U64 in an 8x8 format
void printBitboard(uint64_t bitboard);

std::vector<std::string> split(const std::string& s, char delim);

std::string movePretty(const Board& board, const Move& move);

class PRNG {
private:
    uint64_t seed;

public:
    PRNG(uint64_t seed) :
        seed(seed) {
        /* initial seed must be nonzero, don't use a static variable for the state if multithreaded */
        assert(seed);
    }

    // https://en.wikipedia.org/wiki/Xorshift#xorshift*
    uint64_t xorshift64star() {
        seed ^= seed >> 12;
        seed ^= seed << 25;
        seed ^= seed >> 27;
        return seed * 0x2545F4914F6CDD1DULL;
    }

    template<typename T>
    T rand() {
        return T(xorshift64star());
    }
};

#endif
