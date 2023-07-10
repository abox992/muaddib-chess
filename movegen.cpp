#include <chrono>
#include <cstdint>
#include <bit>
#include <tuple>

#include "movegen.h"
#include "board.h"
#include "precompute_masks.h"
#include "move.h"
#include "helpers.h"
#include "check_pin_masks.h"

using namespace std;

// move list to add moves to, color to gen moves for (0 for white 1 for black)
void generateMoves(Board& board, struct Move moveList[], int color) {
    int moveCount = 0;
    int myColorOffset = color == 0 ? 0 : 6;
    int enemyColor = color == 0 ? 1 : 0;
    int enemyColorOffset = color == 0 ? 6 : 0;

    uint64_t currentSquareMask;

    // check mask
    chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

    tuple<uint64_t, uint64_t, uint64_t> checkAndPinMasks = generateCheckAndPinMask(board, color);
    uint64_t checkMask = get<0>(checkAndPinMasks);
    uint64_t pinHV     = get<1>(checkAndPinMasks);
    uint64_t pinDiag   = get<2>(checkAndPinMasks);

    chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
    auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout << "checkmask: " << time_span.count() << "nanoseconds" << std::endl;

    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int currentSquare = (7 - file) + (8 * rank);
            currentSquareMask = uint64_t(1) << currentSquare;

            if ((board.empty & currentSquareMask) != 0) { // skip if empty
                continue;
            }

            for (int piece = 0; piece < 6; piece++) {
                uint64_t currentBB = board.getPieceSet(piece + myColorOffset);

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

                            // en passant
                            if (board.enpassantPos > 0) {

                                int offset = color == 0 ? -8 : 8;

                                uint64_t temp = board.pieces[enemyColor];
                                board.pieces[enemyColorOffset] &= ~(uint64_t(1) << (board.enpassantPos + offset));
                                board.updateAllPieces();

                                if (generateCheckMask(board, color) == 0){
                                    pLegalAttacks |= (initialAttackMask & (uint64_t(1) << board.enpassantPos));
                                }

                                board.pieces[enemyColorOffset] = temp;
                                board.updateAllPieces();

                            }

                            // adjust for checks
                            pLegalMoves &= checkMask & pinHV;
                            pLegalAttacks &= checkMask & pinDiag;

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

                            // adjust for checks
                            pLegalMoves &= checkMask & (pinHV | pinDiag);

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

                            // adjust for checks
                            pLegalMoves &= checkMask & pinDiag;

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

                            // adjust for checks
                            pLegalMoves &= checkMask & pinHV;

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

                            // adjust for checks
                            pLegalMoves &= checkMask & pinHV & pinDiag;

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

                            uint64_t attackedSquares = 0;
                            int kingPos = currentSquare;
                            int currentY = kingPos / 8;
                            int currentX = kingPos % 8;
                            int directions[] = {9, 8, 7, 1, -1, -7, -8, -9};
                            for (int offset : directions) {

                                if ((kingPos + offset) > 63 || (kingPos + offset) < 0) {
                                    continue;
                                }

                                int prevY = (kingPos + offset) / 8;
                                int prevX = (kingPos + offset) % 8;

                                int maxDif = max(abs(currentX - prevX), abs(currentY - prevY));

                                if (maxDif != 1) {
                                    break;
                                }

                                if (attacksOnSquare(board, enemyColor, kingPos + offset) != 0) {
                                    attackedSquares |= uint64_t(1) << (kingPos + offset);
                                }

                            }

                            // adjust for checks
                            pLegalMoves &= ~attackedSquares;

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

                }

            }


        }

    }

    for (int i = 0; i < moveCount; i++) {
        cout << "move: " << moveList[i] << endl;
    }

}