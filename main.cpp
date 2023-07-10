#include <iostream>
#include <cstdint>
#include "board.h"
#include "precompute_masks.h"
#include "movegen.h"
#include "move.h"
#include "helpers.h"
#include "check_pin_masks.h"

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

    Board test = generateBoardFromFen("8/8/8/q7/4K3/8/8/1b3r1k w - - 0 1"); // 8/8/8/2pP4/8/4K3/8/7k w - c6 0 1 "" 8/q7/8/2pP4/8/4K3/8/7k w - c6 0 1
    cout << test << endl;
    //printBitboard(generatePinMask(test, white));
    generateMoves(test, moveList, white);

    return 0;
}

// g++ -Wall -g -std=c++20 -o chess main.cpp board.cpp movegen.cpp precompute_masks.cpp