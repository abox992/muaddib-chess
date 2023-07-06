#include <iostream>
#include <cstdint>
#include "board.h"
#include "precompute_masks.h"
#include "movegen.h"
#include "move.h"

using namespace std;

void printBitboard(uint64_t bitboard) {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int i = ((7 - file) + (8 * rank));
            uint64_t mask = uint64_t(1) << i;
            int bit = ((bitboard & mask) >> i);
            cout << bit << " ";
        }

        cout << endl;
    }
}

int main() {
    Board board;
    board.setStartPos();
    initMasks();

    //board.setPieceSet(bpawns, 1);
    cout << board << endl;

    struct Move moveList[256];
    generateMoves(board, moveList, white);
    cout << endl;

    printBitboard(knightMasks[5]);
    // printBitboard(bishopLegalMoves[1][12415]);

    // cout << endl;
    // printBitboard(pawnMoveMasks[0][9]);
    // cout << endl;
    // printBitboard(pawnAttackMasks[0][9]);

    // cout << endl;
    // printBitboard(pawnMoveMasks[1][50]);
    // cout << endl;
    // printBitboard(pawnAttackMasks[1][50]);

    return 0;
}

// g++ -Wall -g -std=c++20 -o chess main.cpp board.cpp movegen.cpp precompute_masks.cpp