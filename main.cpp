#include <iostream>
#include <cstdint>
#include "board.h"
#include "precompute_masks.h"
#include "movegen.h"
#include "move.h"
#include "helpers.h"

using namespace std;

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

    Board test = generateBoardFromFen("8/4k3/8/K2R1q2/8/8/8/8 w - - 0 1");
    cout << test << endl;
    printBitboard(generatePinMask(test, white));

    return 0;
}

// g++ -Wall -g -std=c++20 -o chess main.cpp board.cpp movegen.cpp precompute_masks.cpp