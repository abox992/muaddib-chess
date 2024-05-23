#include "precompute_masks.h"
#include <iostream>
#include <cstdint>
#include "bit_manip.h"

// 0 for white 1 for black
uint64_t pawnMoveMasks[2][64];
uint64_t pawnAttackMasks[2][64];

uint64_t knightMasks[64];
uint64_t kingMasks[64];
uint64_t bishopMasks[64];
uint64_t rookMasks[64];

// [piece pos, blockers (compressed)]
uint64_t rookLegalMoves[64][16384];
uint64_t bishopLegalMoves[64][16384];

uint64_t checkMasksHV[64][16384];
uint64_t checkMasksDiag[64][16384];

uint64_t castleMasks[4];
uint64_t castleSquares[4];
uint64_t castleRookSquares[4];
uint64_t originalRookSquares[4];

uint64_t rowMasks[64];
uint64_t colMasks[64];

// up, down, left, right, tl, br, tr, bl
uint64_t directionMasks[8][64];

bool promoSquare[64];

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

            int maxDif = std::max(std::abs(currentX - jumpX), std::abs(currentY - jumpY));

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

        int maxDif = std::max(std::abs(currentX - jumpX), std::abs(currentY - jumpY));

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

            int jumpY = (i - offset) / 8;
            int jumpX = (i - offset) % 8;

            int maxDif = std::max(std::abs(currentX - jumpX), std::abs(currentY - jumpY));

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

        int jumpY = (i - offset) / 8;
        int jumpX = (i - offset) % 8;

        int maxDif = std::max(std::abs(currentX - jumpX), std::abs(currentY - jumpY));

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

            int maxDif = std::max(std::abs(currentX - jumpX), std::abs(currentY - jumpY));

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

            int maxDif = std::max(std::abs(currentX - jumpX), std::abs(currentY - jumpY));

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

                int maxDif = std::max(std::abs(currentX - prevX), std::abs(currentY - prevY));

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

                int maxDif = std::max(std::abs(currentX - prevX), std::abs(currentY - prevY));

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
            while (tempRookMask) {
                const int index = tz_count(tempRookMask);
                pop_lsb(tempRookMask);

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

void initCheckMaskTable() {
    // for each square
    for (int currentSquare = 0; currentSquare < 64; currentSquare++) {

        // for each possible block combination
        for (int i = 0; i < 16384; i++) {
            uint64_t blockMask = 0x0;

            uint64_t tempRookMask = rookMasks[currentSquare];
            int compressed = i;
            int count = 0;
            while (tempRookMask) {
                const int index = tz_count(tempRookMask);
                pop_lsb(tempRookMask);

                blockMask |= (uint64_t((compressed >> count) & 1) << index);
                count++;
                    
            }

            //rookLegalMoves[currentSquare][i] = blockMask;
            uint64_t checkMaskHV = rookMasks[currentSquare];
            int directions[] = {8, 1, -1, -8};
            for (int offset : directions) {

                uint64_t directionMask = 0;
                bool hitBlocker = false;
                int index = currentSquare + offset;
                int count = 0; // do it a max of 7 times so we dont wrap
                while (index < 64 && index > -1 && count < 7) {
                    // if weve already hit a blocker, set bit to 0
                    if (hitBlocker) {
                        checkMaskHV &= ~maskForPos(index);
                    } else if ((blockMask & maskForPos(index)) != 0) { // if current square is a blocker, flag
                        hitBlocker = true;
                    }

                    directionMask |= maskForPos(index);
                    index += offset;
                    count++;
                }

                // if there is no blocker on the row, remove bits
                if (!hitBlocker) {
                    checkMaskHV &= ~directionMask;
                }

            }

            checkMasksHV[currentSquare][i] = checkMaskHV;

        }

    }

    // for each square
    for (int currentSquare = 0; currentSquare < 64; currentSquare++) {

        // for each possible block combination
        for (int i = 0; i < 16384; i++) {

            // uncompress bits, create the blockers mask
            uint64_t blockMask = 0x0;

            uint64_t tempBishopMask = bishopMasks[currentSquare];
            int compressed = i;
            int count = 0;
            while (tempBishopMask) {
                const int index = tz_count(tempBishopMask);
                pop_lsb(tempBishopMask);

                blockMask |= (uint64_t((compressed >> count) & 1) << index);
                count++;
                    
            }

            //rookLegalMoves[currentSquare][i] = blockMask;
            uint64_t checkMaskDiag = bishopMasks[currentSquare];
            int directions[] = {9, 7, -7, -9};
            for (int offset : directions) {

                uint64_t directionMask = 0;
                bool hitBlocker = false;
                int index = currentSquare + offset;
                int count = 0; // do it a max of 7 times so we dont wrap
                while (index < 64 && index > -1 && count < 7) {
                    // if weve already hit a blocker, set bit to 0
                    if (hitBlocker) {
                        checkMaskDiag &= ~maskForPos(index);
                    } else if ((blockMask & maskForPos(index)) != 0) { // if current square is a blocker, flag
                        hitBlocker = true;
                    }

                    directionMask |= maskForPos(index);
                    index += offset;
                    count++;
                }

                // if there is no blocker on the row, remove bits
                if (!hitBlocker) {
                    checkMaskDiag &= ~directionMask;
                }

            }

            checkMasksDiag[currentSquare][i] = checkMaskDiag;

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
            while (tempBishopMask) {
                const int index = tz_count(tempBishopMask);
                pop_lsb(tempBishopMask);

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

void initCastleMasks() {
    uint64_t castle = 0;
    castle |= uint64_t(1) << 1;
    castle |= uint64_t(1) << 2;

    castleMasks[0] = castle;
    castleMasks[1] = castle << 56;

    castleSquares[0] = uint64_t(1) << 1;
    castleSquares[1] = castleSquares[0] << 56;

    castle = 0;
    castle |= uint64_t(1) << 4;
    castle |= uint64_t(1) << 5;
    castle |= uint64_t(1) << 6;

    castleMasks[2] = castle;
    castleMasks[3] = castle << 56;

    castleSquares[2] = uint64_t(1) << 5;
    castleSquares[3] = castleSquares[2] << 56;

    // rooks
    castleRookSquares[0] = uint64_t(1) << 2;
    castleRookSquares[1] = uint64_t(1) << 58;
    castleRookSquares[2] = uint64_t(1) << 4;
    castleRookSquares[3] = uint64_t(1) << 60;

    originalRookSquares[0] = uint64_t(1);
    originalRookSquares[1] = uint64_t(1) << 56;
    originalRookSquares[2] = uint64_t(1) << 7;
    originalRookSquares[3] = uint64_t(1) << 63;

}

void initRowColMasks() {
    uint64_t rowMask = 0xFF;
    uint64_t colMask = 0x101010101010101;
    for (int i = 0; i < 64; i++) {
        int row = i / 8;
        int col = i % 8;

        rowMasks[i] = rowMask << (row * 8);
        colMasks[i] = colMask << col;
    }

    // direction masks
    /*  
    *  9  8  7
    *  1  0 -1
    * -7 -8 -9
    */
    int directions[] = {8, -8, 1, -1, 9, -9, 7, -7};
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 8; j++) {
            uint64_t mask = 0x0;
            int temp = directions[j];
            while ((i + temp) < 64 && (i + temp) > -1) {

                int currentY = (i + temp) / 8;
                int currentX = (i + temp) % 8;
                int prevY = (i + temp - directions[j]) / 8;
                int prevX = (i + temp - directions[j]) % 8;

                int maxDif = std::max(std::abs(currentX - prevX), std::abs(currentY - prevY));

                if (maxDif != 1) {
                    break;
                }

                mask |= (uint64_t(1) << (i + temp));
                temp += directions[j];
            }

            directionMasks[j][i] = mask;
        }
    }
}

void initPromoSquareTable() {
    for (int i = 0; i < 64; i++) {
        if ((i / 8) == 0 || (i / 8) == 7) {
            promoSquare[i] = true;
        } else {
            promoSquare[i] = false;
        }
    }
}

void initMasks() {
    initPawnMasks();
    initKnightMasks();
    initKingMasks();
    initBishopMasks();
    initRookMasks();

    initBishopMovesTable();
    initRookMovesTable();

    initCastleMasks();

    initRowColMasks();

    initCheckMaskTable();
    //initKingCheckMaskTable();

    initPromoSquareTable();
}