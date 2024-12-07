#include "bitboard.h"
#include "bit_manip.h"
#include "types.h"
#include <algorithm>
#include <cstdint>

void Bitboard::initPawnMasks() {
    for (Color color : { WHITE, BLACK }) {
        // attacks
        for (int i = 0; i < 64; i++) {
            uint64_t mask = 0;
            for (int offset : { 9, 7 }) {
                int dest = color ? i - offset : i + offset;

                if (!isOk(dest))
                    continue;

                if (safeDestination(i, dest)) {
                    mask |= maskForPos(dest);
                }
            }

            pawnAttackMasks[color][i] = mask;
        }

        // moves
        for (int i = 0; i < 64; i++) {
            uint64_t mask = 0;
            int offset = 8;

            int dest = color ? i - offset : i + offset;
            int doubleDest = color ? i - 16 : i + 16;

            if (!isOk(dest))
                continue;

            if (safeDestination(i, dest)) {
                mask |= maskForPos(dest);
            }
            if (rankOf(i) == (color ? 6 : 1)) {
                mask |= maskForPos(doubleDest);
            }

            pawnMoveMasks[color][i] = mask;
        }
    }
}

void Bitboard::initKnightMasks() {
    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0;
        for (int offset : { 17, 15, 10, 6, -6, -10, -15, -17 }) {

            int dest = i + offset;

            if (!isOk(dest))
                continue;

            if (distance(i, dest) == 2) {
                mask |= maskForPos(dest);
            }
        }

        knightMasks[i] = mask;
    }
}

void Bitboard::initKingMasks() {
    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0;
        for (int offset : { 9, 8, 7, 1, -1, -7, -8, -9 }) {
            int dest = i + offset;

            if (!isOk(dest))
                continue;

            if (safeDestination(i, dest)) {
                mask |= (uint64_t(1) << (i + offset));
            }
        }

        kingMasks[i] = mask;
    }
}

void Bitboard::initRookMovesTable() {
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

            uint64_t legalMask = rookMasks[currentSquare];
            int directions[] = { 8, 1, -1, -8 };
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

void Bitboard::initCheckMaskTable() {
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

            // rookLegalMoves[currentSquare][i] = blockMask;
            uint64_t checkMaskHV = rookMasks[currentSquare];
            int directions[] = { 8, 1, -1, -8 };
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

            // rookLegalMoves[currentSquare][i] = blockMask;
            uint64_t checkMaskDiag = bishopMasks[currentSquare];
            int directions[] = { 9, 7, -7, -9 };
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

void Bitboard::initBishopMovesTable() {
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

            // rookLegalMoves[currentSquare][i] = blockMask;
            uint64_t legalMask = bishopMasks[currentSquare];
            int directions[] = { 9, 7, -7, -9 };
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

void Bitboard::initCastleMasks() {
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

void Bitboard::initRowColMasks() {
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
    int directions[] = { 8, -8, 1, -1, 9, -9, 7, -7 };
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

void Bitboard::initPromoSquareTable() {
    for (int i = 0; i < 64; i++) {
        if (rankOf(i) == 0 || rankOf(i) == 7) {
            promoSquare[i] = true;
        } else {
            promoSquare[i] = false;
        }
    }
}

void Bitboard::init() {

    initPawnMasks();
    initKnightMasks();
    initKingMasks();

    initSliderMask<ROOKS>();
    initSliderMask<BISHOPS>();

    initBishopMovesTable();
    initRookMovesTable();

    initCastleMasks();

    initRowColMasks();

    initCheckMaskTable();

    initPromoSquareTable();
}
