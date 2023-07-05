#include "movegen.h"
#include "board.h"
#include "precompute_masks.h"
#include "move.h"
#include <chrono>
#include <cstdint>
#include <bit>

using namespace std;

uint64_t pawnMasks[64];
uint64_t knightMasks[64];
uint64_t kingMasks[64];
uint64_t bishopMasks[64];
uint64_t rookMasks[64];
uint64_t queenMasks[64];

uint64_t rookLegalMoves[64][16384];
uint64_t bishopLegalMoves[64][16384];

int lsb(uint64_t &b) {
    auto index = std::countr_zero(b);
    b &= b - 1;
    return index;
}

void initPawnMasks() {
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

        pawnMasks[i] = mask;
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

        knightMasks[i] = mask;
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
}

// move list to add moves to, color to gen moves for (0 for white 1 for black)
void generateMoves(Board board, struct Move moveList[], int color) {
    int moveCount = 0;
    int colorOffset = color == 0 ? 0 : 6;
    board.updateAllPieces();

    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int currentSquare = (7 - file) + (8 * rank);
            uint64_t mask = uint64_t(1) << currentSquare;

            for (int piece = 0; piece < 6; piece++) {
                uint64_t currentBB = board.getPieceSet(piece + colorOffset);

                if ((currentBB & mask) != 0) { // found piece
                    uint64_t directionMask;

                    /*  
                    *  9  8  7
                    *  1  0 -1
                    * -7 -8 -9
                    */
                    switch (piece) {
                        case 0: { // pawn 

                            chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            directionMask = color == 0 ? mask << 9 : mask >> 9;
                            if ((board.allPieces[color == 0 ? 1 : 0] & directionMask) != 0) { // enemy piece found top left
                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = currentSquare + (color == 0 ? 9 : -7);

                                moveList[moveCount++] = Temp;
                            }

                            directionMask = color == 0 ? mask << 7 : mask >> 7;
                            if ((board.allPieces[color == 0 ? 1 : 0] & directionMask) != 0) { // enemy piece found top right
                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = currentSquare + (color == 0 ? 7 : -9);

                                moveList[moveCount++] = Temp;
                            }

                            directionMask = color == 0 ? mask << 8 : mask >> 8;
                            if ((board.empty & directionMask) != 0) { // square above empty
                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = currentSquare + (color == 0 ? 8 : -8);

                                moveList[moveCount++] = Temp;
                            }

                            directionMask = color == 0 ? mask << 16 : mask >> 16;
                            if ((board.empty & directionMask) != 0 && 
                            (currentSquare > 7 + (40 * color) && currentSquare < 16 + (40 * color)) ) { // 2 squares above empty and on start square
                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = currentSquare + (color == 0 ? 16 : -16);

                                moveList[moveCount++] = Temp;
                            }

                            // en passant
                            directionMask = color == 0 ? mask << 1 : mask >> 1;
                            if ((board.pieces[color == 0 ? 1 : 0] & directionMask) != 0) { // enemy pawn to the left
                                uint8_t passantMask = uint8_t(1) << file;
                                if ((passantMask & board.passant[color == 0 ? 1 : 0]) != 0) {
                                    struct Move Temp;
                                    Temp.from = currentSquare;
                                    Temp.to = currentSquare + (color == 0 ? 9 : -7);

                                    moveList[moveCount++] = Temp;
                                }
                            }

                            directionMask = color == 0 ? mask >> 1 : mask << 1;
                            if ((board.pieces[color == 0 ? 1 : 0] & directionMask) != 0) { // enemy pawn to the right
                                uint8_t passantMask = uint8_t(1) << file;
                                if ((passantMask & board.passant[color == 0 ? 1 : 0]) != 0) {
                                    struct Move Temp;
                                    Temp.from = currentSquare;
                                    Temp.to = currentSquare + (color == 0 ? 7 : -9);

                                    moveList[moveCount++] = Temp;
                                }
                            }

                            chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(start - end);
                            std::cout << "pawn: " << time_span.count() << "seconds" << std::endl;

                            break;
                        } case 1: { // knight 

                            chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t pLegalMoves = knightMasks[currentSquare] & (board.empty | board.allPieces[color == 0 ? 1 : 0]);

                            while (pLegalMoves != 0) {
                                int index = lsb(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;

                                moveList[moveCount++] = Temp;
                            }

                            chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(start - end);
                            std::cout << "knight: " << time_span.count() << "seconds" << std::endl;
                                
                            break;
                        }
                        case 2: { // bishop
                            chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            int compressedBlockers = 0;
                            uint64_t blockers = (~board.empty) & bishopMasks[currentSquare];
                            uint64_t tempBishopMask = bishopMasks[currentSquare];
                            int count = 0;
                            while (tempBishopMask != 0) {
                                int index = lsb(tempBishopMask);

                                if ((blockers & (uint64_t(1) << index)) != 0) {
                                    compressedBlockers |= (1 << count);
                                }

                                count++;
                            }

                            uint64_t pLegalMoves = bishopLegalMoves[currentSquare][compressedBlockers];
                            pLegalMoves &= ~board.allPieces[color];

                            while (pLegalMoves != 0) {
                                int index = lsb(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;

                                moveList[moveCount++] = Temp;
                            }

                            chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(start - end);
                            std::cout << "bishop: " << time_span.count() << "seconds" << std::endl;

                            break;
                        } case 3: { // rook

                            chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            int compressedBlockers = 0;
                            uint64_t blockers = (~board.empty) & rookMasks[currentSquare];
                            uint64_t tempRookMask = rookMasks[currentSquare];
                            int count = 0;
                            while (tempRookMask != 0) {
                                int index = lsb(tempRookMask);

                                if ((blockers & (uint64_t(1) << index)) != 0) {
                                    compressedBlockers |= (1 << count);
                                }

                                count++;
                            }

                            uint64_t pLegalMoves = rookLegalMoves[currentSquare][compressedBlockers];
                            pLegalMoves &= ~board.allPieces[color];

                            while (pLegalMoves != 0) {
                                int index = lsb(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;

                                moveList[moveCount++] = Temp;
                            }

                            chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(start - end);
                            std::cout << "rook: " << time_span.count() << "seconds" << std::endl;

                            break;
                        } case 4: { // queen

                            chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            int rookCompressedBlockers = 0;
                            uint64_t blockers = (~board.empty) & rookMasks[currentSquare];
                            uint64_t tempRookMask = rookMasks[currentSquare];
                            int count = 0;
                            while (tempRookMask != 0) {
                                int index = lsb(tempRookMask);

                                if ((blockers & (uint64_t(1) << index)) != 0) {
                                    rookCompressedBlockers |= (1 << count);
                                }

                                count++;
                            }

                            int bishopCompressedBlockers = 0;
                            blockers = (~board.empty) & bishopMasks[currentSquare];
                            uint64_t tempBishopMask = bishopMasks[currentSquare];
                            count = 0;
                            while (tempBishopMask != 0) {
                                int index = lsb(tempBishopMask);

                                if ((blockers & (uint64_t(1) << index)) != 0) {
                                    bishopCompressedBlockers |= (1 << count);
                                }

                                count++;
                            }

                            uint64_t pLegalMoves = rookLegalMoves[currentSquare][rookCompressedBlockers] | bishopLegalMoves[currentSquare][bishopCompressedBlockers];
                            pLegalMoves &= ~board.allPieces[color];

                            while (pLegalMoves != 0) {
                                int index = lsb(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;

                                moveList[moveCount++] = Temp;
                            }

                            chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(start - end);
                            std::cout << "queen: " << time_span.count() << "seconds" << std::endl;

                            break;
                        } case 5: { // king 
                            uint64_t pLegalMoves = kingMasks[currentSquare] & (board.empty | board.allPieces[color == 0 ? 1 : 0]);

                            while (pLegalMoves != 0) {
                                int index = lsb(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;

                                moveList[moveCount++] = Temp;
                            }
                                
                            break;
                        }
                    }

                    break;
                }

            }


        }

    }

    for (int i = 0; i < moveCount; i++) {
        cout << "move: " << moveList[i] << endl;
    }

}