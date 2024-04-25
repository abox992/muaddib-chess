
#include <iostream>
#include <random>
#include <cmath>
#include "board.h"
#include "helpers.h"
#include "bit_manip.h"

using namespace std;

uint64_t randomTable[64][12];
uint64_t randomBlackToMove;

void initZobrist() {
    std::random_device rd;

    std::mt19937_64 e2(rd());

    std::uniform_int_distribution<long long int> dist(0, std::llround(std::pow(2,64)));

    randomBlackToMove = dist(e2);

    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 12; j++) {
            randomTable[i][j] = dist(e2);
        }
    }

}

uint64_t zhash(const BoardState& state) {
    uint64_t hash = 0;

    if (state.blackToMove) {
        hash ^= randomBlackToMove;
    }

    for (int i = 0; i < 12; i++) {
        
        uint64_t curBB = state.pieces[i];
        while (curBB) {
             const int sq = tz_count(curBB);
             pop_lsb(curBB);
             hash ^= randomTable[sq][i];
        }
    }

    return hash;
}