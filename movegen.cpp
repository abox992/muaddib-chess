#include "movegen.h"
#include "board.h"
#include "precompute_masks.h"
#include "move.h"
#include <chrono>
#include <cstdint>
#include <bit>

using namespace std;

int lsb(uint64_t &b) {
    auto index = std::countr_zero(b);
    b &= b - 1;
    return index;
}

// move list to add moves to, color to gen moves for (0 for white 1 for black)
void generateMoves(Board board, struct Move moveList[], int color) {
    int moveCount = 0;
    int colorOffset = color == 0 ? 0 : 6;
    int enemyColor = color == 0 ? 1 : 0;
    board.updateAllPieces();

    uint64_t currentSquareMask;

    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int currentSquare = (7 - file) + (8 * rank);
            currentSquareMask = uint64_t(1) << currentSquare;

            if ((board.empty & currentSquareMask) != 0) { // skip if empty
                continue;
            }

            for (int piece = 0; piece < 6; piece++) {
                uint64_t currentBB = board.getPieceSet(piece + colorOffset);

                if ((currentBB & currentSquareMask) != 0) { // found piece

                    /*  
                    *  9  8  7
                    *  1  0 -1
                    * -7 -8 -9
                    */
                    switch (piece) {
                        case 0: { // pawn 

                            chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t initialMoveMask = pawnMoveMasks[color][currentSquare];
                            uint64_t initialAttackMask = pawnAttackMasks[color][currentSquare];

                            uint64_t pLegalMoves = initialMoveMask & board.empty;
                            uint64_t pLegalAttacks = initialAttackMask & board.allPieces[enemyColor];

                            if (file - 1 >= 0) { // check left passant
                                if ((board.pieces[(enemyColor * 6)] & (currentSquareMask << 1)) != 0) { // if enemy pawn to left
                                    if (board.passant[enemyColor][file - 1]) {
                                        uint64_t leftPassant = uint64_t(1) <<  (currentSquare + (color == 0 ? 9 : -7));
                                        pLegalAttacks |= leftPassant;
                                    }
                                }
                            }

                            if (file + 1 <= 7) { // check right passant
                                if ((board.pieces[(enemyColor * 6)] & (currentSquareMask >> 1)) != 0) { // if enemy pawn to right
                                    if (board.passant[enemyColor][file - 1]) {
                                        uint64_t leftPassant = uint64_t(1) <<  (currentSquare + (color == 0 ? 9 : -7));
                                        pLegalAttacks |= leftPassant;
                                    }
                                }
                            }

                            while (pLegalMoves != 0) {
                                int index = lsb(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;

                                moveList[moveCount++] = Temp;
                            }

                            while (pLegalAttacks != 0) {
                                int index = lsb(pLegalAttacks);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;

                                moveList[moveCount++] = Temp;
                            }
                            

                            chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            std::cout << "pawn: " << time_span.count() << "nanoseconds" << std::endl;

                            break;
                        } case 1: { // knight 

                            chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t pLegalMoves = knightMasks[currentSquare] & (board.empty | board.allPieces[enemyColor]);

                            while (pLegalMoves != 0) {
                                int index = lsb(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;

                                moveList[moveCount++] = Temp;
                            }

                            chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            std::cout << "knight: " << time_span.count() << "nanoseconds" << std::endl;
                                
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
                            auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            std::cout << "bishop: " << time_span.count() << "nanoseconds" << std::endl;

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
                            auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            std::cout << "rook: " << time_span.count() << "nanoseconds" << std::endl;

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
                            auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            std::cout << "queen: " << time_span.count() << "nanoseconds" << std::endl;

                            break;
                        } case 5: { // king 

                            chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t pLegalMoves = kingMasks[currentSquare] & (board.empty | board.allPieces[enemyColor]);

                            while (pLegalMoves != 0) {
                                int index = lsb(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;

                                moveList[moveCount++] = Temp;
                            }

                            chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            std::cout << "king: " << time_span.count() << "nanoseconds" << std::endl;
                                
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