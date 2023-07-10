#include "precompute_masks.h"
#include <iostream>
#include <cstdint>

using namespace std;

// 0 for white 1 for black
uint64_t pawnMoveMasks[2][64];
uint64_t pawnAttackMasks[2][64];
uint64_t knightMasks[64];
uint64_t kingMasks[64];
uint64_t bishopMasks[64];
uint64_t rookMasks[64];
uint64_t queenMasks[64];

uint64_t rookLegalMoves[64][16384];
uint64_t bishopLegalMoves[64][16384];

uint64_t queenLegalMoves[64][16384];

uint64_t kingHVChecks[64][16384];
uint64_t kingDiagChecks[64][16384];

void initPawnMasks() {
    // white
    int relativePos[] = {9, 7};
    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0x0;
        int currentY = i / 8;
        int currentX = i % 8;
        for (int offset : relativePos) {

            if ((i + offset) > 63 || (i + offset) < 0) {
                continue;
            }

            int jumpY = (i + offset) / 8;
            int jumpX = (i + offset) % 8;

            int maxDif = max(abs(currentX - jumpX), abs(currentY - jumpY));

            if (maxDif == 1) {
                mask |= (uint64_t(1) << (i + offset));
            }
        }

        pawnAttackMasks[0][i] = mask;
    }

    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0x0;
        int currentY = i / 8;
        int currentX = i % 8;
        int offset = 8;
        if ((i + offset) > 63 || (i + offset) < 0) {
            continue;
        }

        int jumpY = (i + offset) / 8;
        int jumpX = (i + offset) % 8;

        int maxDif = max(abs(currentX - jumpX), abs(currentY - jumpY));

        if (maxDif == 1) {
            mask |= (uint64_t(1) << (i + offset));
        }


        if (i > 7 && i < 16) {
            mask |= (uint64_t(1) << (i + 16));
        }

        pawnMoveMasks[0][i] = mask;
    }

    // black
    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0x0;
        int currentY = i / 8;
        int currentX = i % 8;
        for (int offset : relativePos) {

            if ((i - offset) > 63 || (i - offset) < 0) {
                continue;
            }

            int jumpY = (i + offset) / 8;
            int jumpX = (i + offset) % 8;

            int maxDif = max(abs(currentX - jumpX), abs(currentY - jumpY));

            if (maxDif == 1) {
                mask |= (uint64_t(1) << (i - offset));
            }
        }

        pawnAttackMasks[1][i] = mask;
    }

    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0x0;
        int currentY = i / 8;
        int currentX = i % 8;
        int offset = 8;
        if ((i - offset) > 63 || (i - offset) < 0) {
            continue;
        }

        int jumpY = (i + offset) / 8;
        int jumpX = (i + offset) % 8;

        int maxDif = max(abs(currentX - jumpX), abs(currentY - jumpY));

        if (maxDif == 1) {
            mask |= (uint64_t(1) << (i - offset));
        }


        if (i > 47 && i < 56) {
            mask |= (uint64_t(1) << (i - 16));
        }

        pawnMoveMasks[1][i] = mask;
    }
}

void initKnightMasks() {
    int relativePos[] = {17, 15, 10, 6 , -6, -10, -15, -17};
    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0x0;
        int currentY = i / 8;
        int currentX = i % 8;
        for (int offset : relativePos) {

            if ((i + offset) > 63 || (i + offset) < 0) {
                continue;
            }

            int jumpY = (i + offset) / 8;
            int jumpX = (i + offset) % 8;

            int maxDif = max(abs(currentX - jumpX), abs(currentY - jumpY));

            if (maxDif == 2) {
                mask |= (uint64_t(1) << (i + offset));
            }
        }

        knightMasks[i] = mask;
    }
}

void initKingMasks() {
    int relativePos[] = {9, 8, 7, 1 , -1, -7, -8, -9};
    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0x0;
        int currentY = i / 8;
        int currentX = i % 8;
        for (int offset : relativePos) {

            if ((i + offset) > 63 || (i + offset) < 0) {
                continue;
            }

            int jumpY = (i + offset) / 8;
            int jumpX = (i + offset) % 8;

            int maxDif = max(abs(currentX - jumpX), abs(currentY - jumpY));

            if (maxDif == 1) {
                mask |= (uint64_t(1) << (i + offset));
            }
        }

        kingMasks[i] = mask;
    }
}

void initBishopMasks() {
    int relativePos[] = {9, 7, -7, -9};
    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0x0;
        for (int offset : relativePos) {
            int temp = offset;
            while ((i + temp) < 64 && (i + temp) > -1) {

                int currentY = (i + temp) / 8;
                int currentX = (i + temp) % 8;
                int prevY = (i + temp - offset) / 8;
                int prevX = (i + temp - offset) % 8;

                int maxDif = max(abs(currentX - prevX), abs(currentY - prevY));

                if (maxDif != 1) {
                    break;
                }

                mask |= (uint64_t(1) << (i + temp));
                temp += offset;
            }
        }

        bishopMasks[i] = mask;
    }
}

void initRookMasks() {
    int relativePos[] = {8, 1, -1, -8};
    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0x0;
        for (int offset : relativePos) {
            int temp = offset;
            while ((i + temp) < 64 && (i + temp) > -1) {

                int currentY = (i + temp) / 8;
                int currentX = (i + temp) % 8;
                int prevY = (i + temp - offset) / 8;
                int prevX = (i + temp - offset) % 8;

                int maxDif = max(abs(currentX - prevX), abs(currentY - prevY));

                if (maxDif != 1) {
                    break;
                }

                mask |= (uint64_t(1) << (i + temp));
                temp += offset;
            }
        }

        rookMasks[i] = mask;
    }
}

void initRookMovesTable() {
    // for each square
    for (int currentSquare = 0; currentSquare < 64; currentSquare++) {

        // for each possible block combination
        for (int i = 0; i < 16384; i++) {
            uint64_t blockMask = 0x0;

            uint64_t tempRookMask = rookMasks[currentSquare];
            int compressed = i;
            int count = 0;
            while (tempRookMask != 0) {
                int index = lsb(tempRookMask);

                blockMask |= (uint64_t((compressed >> count) & 1) << index);
                count++;
            }

            //rookLegalMoves[currentSquare][i] = blockMask;
            uint64_t legalMask = rookMasks[currentSquare];
            int directions[] = {8, 1, -1, -8};
            for (int offset : directions) {

                bool hitBlocker = false;
                int index = currentSquare + offset;
                int count = 0; // do it a max of 7 times so we dont wrap
                while (index < 64 && index > -1 && count < 7) {

                    if (hitBlocker) {
                        legalMask &= ~(uint64_t(1) << index);
                    } else {
                        // check if index is blocker in blocker mask
                        if ((blockMask & (uint64_t(1) << index)) != 0) {
                            hitBlocker = true;
                        }
                    }

                    index += offset;
                    count++;
                }

            }

            rookLegalMoves[currentSquare][i] = legalMask;

        }

    }
}

void initBishopMovesTable() {
    // for each square
    for (int currentSquare = 0; currentSquare < 64; currentSquare++) {

        // for each possible block combination
        for (int i = 0; i < 16384; i++) {
            uint64_t blockMask = 0x0;

            uint64_t tempBishopMask = bishopMasks[currentSquare];
            int compressed = i;
            int count = 0;
            while (tempBishopMask != 0) {
                int index = lsb(tempBishopMask);

                blockMask |= (uint64_t((compressed >> count) & 1) << index);
                count++;
            }

            //rookLegalMoves[currentSquare][i] = blockMask;
            uint64_t legalMask = bishopMasks[currentSquare];
            int directions[] = {9, 7, -7, -9};
            for (int offset : directions) {

                bool hitBlocker = false;
                int index = currentSquare + offset;
                int count = 0; // do it a max of 7 times so we dont wrap
                while (index < 64 && index > -1 && count < 7) {

                    if (hitBlocker) {
                        legalMask &= ~(uint64_t(1) << index);
                    } else {
                        // check if index is blocker in blocker mask
                        if ((blockMask & (uint64_t(1) << index)) != 0) {
                            hitBlocker = true;
                        }
                    }

                    index += offset;
                    count++;
                }

            }

            bishopLegalMoves[currentSquare][i] = legalMask;

        }

    }
}

void initQueenMoveTable() {
    for (int bishopBlocker = 0; bishopBlocker < 16384; bishopBlocker++) {
        for (int rookBlocker = 0; rookBlocker < 16384; rookBlocker++) {
            for(int i = 0; i < 64; i++) {
                int queenIndex = bishopBlocker ^ rookBlocker;
                queenLegalMoves[i][queenIndex] = bishopLegalMoves[i][bishopBlocker] | rookLegalMoves[i][rookBlocker];
            }

        }

        cout << "outer " << bishopBlocker << endl;
    }
}

void initQueenMasks() {
    for (int i = 0; i < 64; i++) {
        queenMasks[i] = bishopMasks[i] | rookMasks[i];
    }
}

void initMasks() {
    initPawnMasks();
    initKnightMasks();
    initKingMasks();
    initBishopMasks();
    initRookMasks();

    // bishop and rook masks must be init first
    initQueenMasks();

    initBishopMovesTable();
    initRookMovesTable();

    //initQueenMoveTable();
}