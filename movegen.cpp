#include "movegen.h"
#include "board.h"
#include "precompute_masks.h"
#include "move.h"
#include "helpers.h"
#include <chrono>
#include <cstdint>
#include <bit>

using namespace std;

uint64_t generateKingLosMask(Board board, int color) {

    int myColorOffset = color == 0 ? 0 : 6;

    int kingPos = lsb(board.pieces[5 + myColorOffset]);

    int rookCompressedBlockers = 0;
    uint64_t blockers = (~board.empty) & rookMasks[kingPos];
    uint64_t tempRookMask = rookMasks[kingPos];
    int count = 0;
    while (tempRookMask != 0) {
        int index = lsb(tempRookMask);

        if ((blockers & (uint64_t(1) << index)) != 0) {
            rookCompressedBlockers |= (1 << count);
        }

        count++;
    }

    int bishopCompressedBlockers = 0;
    blockers = (~board.empty) & bishopMasks[kingPos];
    uint64_t tempBishopMask = bishopMasks[kingPos];
    count = 0;
    while (tempBishopMask != 0) {
        int index = lsb(tempBishopMask);

        if ((blockers & (uint64_t(1) << index)) != 0) {
            bishopCompressedBlockers |= (1 << count);
        }

        count++;
    }

    uint64_t kingLosMask = rookLegalMoves[kingPos][rookCompressedBlockers] | bishopLegalMoves[kingPos][bishopCompressedBlockers];
    kingLosMask &= ~board.allPieces[color];

    int directions[] = {9, 8, 7, 1, -1, -7, -8, -9};
    for (int offset : directions) {
        int currentPos = kingPos;
        while ((currentPos + offset) < 64 && (currentPos + offset) > -1) {

            int currentY = (currentPos + offset) / 8;
            int currentX = (currentPos + offset) % 8;
            int prevY = (currentPos) / 8;
            int prevX = (currentPos) % 8;

            int maxDif = max(abs(currentX - prevX), abs(currentY - prevY));

            if (maxDif != 1) {
                break;
            }

            currentPos += offset;
        }

        if ((kingLosMask & (uint64_t(1) << currentPos)) != 0) {
            kingLosMask &= ~(uint64_t(1) << currentPos);
        }
    }

    printBitboard(kingLosMask);
    cout << endl;

    return kingLosMask;
}

uint64_t generateKingLosMaskXray(Board board, int color) {
    int enemyColor = color == 0 ? 1 : 0;
    int myColorOffset = color == 0 ? 0 : 6;

    int kingPos = lsb(board.pieces[5 + myColorOffset]);

    int rookCompressedBlockers = 0;
    uint64_t blockers = (board.allPieces[enemyColor]) & rookMasks[kingPos];
    uint64_t tempRookMask = rookMasks[kingPos];
    int count = 0;
    while (tempRookMask != 0) {
        int index = lsb(tempRookMask);

        if ((blockers & (uint64_t(1) << index)) != 0) {
            rookCompressedBlockers |= (1 << count);
        }

        count++;
    }

    int bishopCompressedBlockers = 0;
    blockers = (board.allPieces[enemyColor]) & bishopMasks[kingPos];
    uint64_t tempBishopMask = bishopMasks[kingPos];
    count = 0;
    while (tempBishopMask != 0) {
        int index = lsb(tempBishopMask);

        if ((blockers & (uint64_t(1) << index)) != 0) {
            bishopCompressedBlockers |= (1 << count);
        }

        count++;
    }

    uint64_t kingLosMask = rookLegalMoves[kingPos][rookCompressedBlockers] | bishopLegalMoves[kingPos][bishopCompressedBlockers];
    //kingLosMask &= ~board.allPieces[color];

    int directions[] = {9, 8, 7, 1, -1, -7, -8, -9};
    for (int offset : directions) {
        int currentPos = kingPos;
        while ((currentPos + offset) < 64 && (currentPos + offset) > -1) {

            int currentY = (currentPos + offset) / 8;
            int currentX = (currentPos + offset) % 8;
            int prevY = (currentPos) / 8;
            int prevX = (currentPos) % 8;

            int maxDif = max(abs(currentX - prevX), abs(currentY - prevY));

            if (maxDif != 1) {
                break;
            }

            currentPos += offset;
        }

        if ((kingLosMask & (uint64_t(1) << currentPos)) != 0) {
            kingLosMask &= ~(uint64_t(1) << currentPos);
        }
    }

    printBitboard(kingLosMask);
    cout << endl;

    return kingLosMask;
}

uint64_t generatePinMask(Board board, int color) {

    int myColorOffset = color == 0 ? 0 : 6;
    int enemyColorOffset = color == 0 ? 6 : 0;

    uint64_t currentSquareMask;

    uint64_t kingMask = board.pieces[5 + myColorOffset];

    uint64_t KingLosMask = generateKingLosMaskXray(board, color);

    uint64_t pinMask = 0;

    // want to flip concept of color and enemy color, we are analyzing attacks on ourself, so we must gen moves for other color
    int enemyColor = color;
    color = color == 0 ? 1 : 0;

    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int currentSquare = (7 - file) + (8 * rank);
            currentSquareMask = uint64_t(1) << currentSquare;

            if ((board.empty & currentSquareMask) != 0) { // skip if empty
                continue;
            }

            for (int piece = 0; piece < 6; piece++) {
                uint64_t currentBB = board.getPieceSet(piece + enemyColorOffset);

                if ((currentBB & currentSquareMask) != 0) { // found piece

                    /*  
                    *  9  8  7
                    *  1  0 -1
                    * -7 -8 -9
                    */
                    switch (piece) {
                        case 0: { // pawn 

                            uint64_t initialMoveMask = pawnMoveMasks[color][currentSquare];
                            uint64_t initialAttackMask = pawnAttackMasks[color][currentSquare];

                            uint64_t pLegalMoves = initialMoveMask & board.empty;
                            uint64_t pLegalAttacks = initialAttackMask & board.allPieces[enemyColor];

                            // en passant
                            if (board.enpassantPos > 0) {
                                pLegalAttacks |= (initialAttackMask & (uint64_t(1) << board.enpassantPos));
                            }

                            if ((pLegalAttacks & kingMask) != 0) { // piece can attack king (ie checking)
                                pinMask |= (KingLosMask & pLegalAttacks);
                                pinMask |= currentSquareMask; // include piece in check mask (allowing a capture to remove the check)
                            }                        

                            break;
                        } case 1: { // knight 

                            uint64_t pLegalAttacks = knightMasks[currentSquare] & (board.empty | board.allPieces[enemyColor]);
                                
                            if ((pLegalAttacks & kingMask) != 0) { // piece can attack king (ie checking)
                                pinMask |= (KingLosMask & pLegalAttacks);
                                pinMask |= currentSquareMask; // include piece in check mask (allowing a capture to remove the check)
                            }      

                            break;
                        }
                        case 2: { // bishop

                            int compressedBlockers = 0;
                            uint64_t blockers = (board.allPieces[color]) & bishopMasks[currentSquare];
                            uint64_t tempBishopMask = bishopMasks[currentSquare];
                            int count = 0;
                            while (tempBishopMask != 0) {
                                int index = lsb(tempBishopMask);

                                if ((blockers & (uint64_t(1) << index)) != 0) {
                                    compressedBlockers |= (1 << count);
                                }

                                count++;
                            }

                            uint64_t pLegalAttacks = bishopLegalMoves[currentSquare][compressedBlockers];
                            pLegalAttacks &= ~board.allPieces[color];

                            if ((pLegalAttacks & kingMask) != 0) { // piece can attack king (ie checking)
                                pinMask |= (KingLosMask & pLegalAttacks);
                                pinMask |= currentSquareMask; // include piece in check mask (allowing a capture to remove the check)
                            }      

                            break;
                        } case 3: { // rook

                            int compressedBlockers = 0;
                            uint64_t blockers = (board.allPieces[color]) & rookMasks[currentSquare];
                            uint64_t tempRookMask = rookMasks[currentSquare];
                            int count = 0;
                            while (tempRookMask != 0) {
                                int index = lsb(tempRookMask);

                                if ((blockers & (uint64_t(1) << index)) != 0) {
                                    compressedBlockers |= (1 << count);
                                }

                                count++;
                            }

                            uint64_t pLegalAttacks = rookLegalMoves[currentSquare][compressedBlockers];
                            pLegalAttacks &= ~board.allPieces[color];

                            if ((pLegalAttacks & kingMask) != 0) { // piece can attack king (ie checking)
                                pinMask |= (KingLosMask & pLegalAttacks);
                                pinMask |= currentSquareMask; // include piece in check mask (allowing a capture to remove the check)
                            }      

                            break;
                        } case 4: { // queen
                            int rookCompressedBlockers = 0;
                            uint64_t blockers = (board.allPieces[color]) & rookMasks[currentSquare];
                            uint64_t tempRookMask = rookMasks[currentSquare];

                            printBitboard(blockers);
                            cout << endl;

                            int count = 0;
                            while (tempRookMask != 0) {
                                int index = lsb(tempRookMask);

                                if ((blockers & (uint64_t(1) << index)) != 0) {
                                    rookCompressedBlockers |= (1 << count);
                                }

                                count++;
                            }

                            int bishopCompressedBlockers = 0;
                            blockers = (board.allPieces[color]) & bishopMasks[currentSquare];
                            uint64_t tempBishopMask = bishopMasks[currentSquare];
                            count = 0;
                            while (tempBishopMask != 0) {
                                int index = lsb(tempBishopMask);

                                if ((blockers & (uint64_t(1) << index)) != 0) {
                                    bishopCompressedBlockers |= (1 << count);
                                }

                                count++;
                            }

                            uint64_t pLegalAttacks = rookLegalMoves[currentSquare][rookCompressedBlockers] | bishopLegalMoves[currentSquare][bishopCompressedBlockers];
                            pLegalAttacks &= ~board.allPieces[color];

                            printBitboard(pLegalAttacks);
                            cout << endl;

                            if ((pLegalAttacks & kingMask) != 0) { // piece can attack king (ie checking)
                                pinMask |= (KingLosMask & pLegalAttacks);
                                pinMask |= currentSquareMask; // include piece in check mask (allowing a capture to remove the check)
                            }  

                            break;
                        }
                    }

                }

            }

        }

    }

    if (pinMask == 0) {
        pinMask = ~pinMask;
    }

    return pinMask;

}

uint64_t generateCheckMask(Board board, int color) {
    int myColorOffset = color == 0 ? 0 : 6;
    int enemyColorOffset = color == 0 ? 6 : 0;

    uint64_t currentSquareMask;

    uint64_t kingMask = board.pieces[5 + myColorOffset];

    uint64_t KingLosMask = generateKingLosMask(board, color);

    uint64_t checkMask = 0;

    // want to flip concept of color and enemy color, we are analyzing attacks on ourself, so we must gen moves for other color
    int enemyColor = color;
    color = color == 0 ? 1 : 0;

    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int currentSquare = (7 - file) + (8 * rank);
            currentSquareMask = uint64_t(1) << currentSquare;

            if ((board.empty & currentSquareMask) != 0) { // skip if empty
                continue;
            }

            for (int piece = 0; piece < 6; piece++) {
                uint64_t currentBB = board.getPieceSet(piece + enemyColorOffset);

                if ((currentBB & currentSquareMask) != 0) { // found piece

                    /*  
                    *  9  8  7
                    *  1  0 -1
                    * -7 -8 -9
                    */
                    switch (piece) {
                        case 0: { // pawn 

                            uint64_t initialAttackMask = pawnAttackMasks[color][currentSquare];
                            uint64_t pLegalAttacks = initialAttackMask & board.allPieces[enemyColor];

                            // en passant
                            if (board.enpassantPos > 0) {
                                pLegalAttacks |= (initialAttackMask & (uint64_t(1) << board.enpassantPos));
                            }

                            if ((pLegalAttacks & kingMask) != 0) { // piece can attack king (ie checking)
                                checkMask |= (KingLosMask & pLegalAttacks);
                                checkMask |= currentSquareMask; // include piece in check mask (allowing a capture to remove the check)
                            }                        

                            break;
                        } case 1: { // knight 

                            uint64_t pLegalAttacks = knightMasks[currentSquare] & (board.empty | board.allPieces[enemyColor]);
                                
                            if ((pLegalAttacks & kingMask) != 0) { // piece can attack king (ie checking)
                                checkMask |= (KingLosMask & pLegalAttacks);
                                checkMask |= currentSquareMask; // include piece in check mask (allowing a capture to remove the check)
                            }      

                            break;
                        }
                        case 2: { // bishop

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

                            uint64_t pLegalAttacks = bishopLegalMoves[currentSquare][compressedBlockers];
                            pLegalAttacks &= ~board.allPieces[color];

                            if ((pLegalAttacks & kingMask) != 0) { // piece can attack king (ie checking)
                                checkMask |= (KingLosMask & pLegalAttacks);
                                checkMask |= currentSquareMask; // include piece in check mask (allowing a capture to remove the check)
                            }      

                            break;
                        } case 3: { // rook

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

                            uint64_t pLegalAttacks = rookLegalMoves[currentSquare][compressedBlockers];
                            pLegalAttacks &= ~board.allPieces[color];

                            if ((pLegalAttacks & kingMask) != 0) { // piece can attack king (ie checking)
                                checkMask |= (KingLosMask & pLegalAttacks);
                                checkMask |= currentSquareMask; // include piece in check mask (allowing a capture to remove the check)
                            }      

                            break;
                        } case 4: { // queen
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

                            uint64_t pLegalAttacks = rookLegalMoves[currentSquare][rookCompressedBlockers] | bishopLegalMoves[currentSquare][bishopCompressedBlockers];
                            pLegalAttacks &= ~board.allPieces[color];

                            printBitboard(pLegalAttacks);
                            cout << endl;

                            if ((pLegalAttacks & kingMask) != 0) { // piece can attack king (ie checking)
                                checkMask |= (KingLosMask & pLegalAttacks);
                                checkMask |= currentSquareMask; // include piece in check mask (allowing a capture to remove the check)
                            }  

                            break;
                        }
                    }

                }

            }

        }

    }

    if (checkMask == 0) {
        checkMask = ~checkMask;
    }

    return checkMask;
}

// move list to add moves to, color to gen moves for (0 for white 1 for black)
void generateMoves(Board board, struct Move moveList[], int color) {
    int moveCount = 0;
    int colorOffset = color == 0 ? 0 : 6;
    int enemyColor = color == 0 ? 1 : 0;

    uint64_t currentSquareMask;

    // check mask
    uint64_t checkMask = generateCheckMask(board, color);
    uint64_t pinMask = generatePinMask(board, color);


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

                            // en passant
                            if (board.enpassantPos > 0) {
                                pLegalAttacks |= (initialAttackMask & (uint64_t(1) << board.enpassantPos));
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

                }

            }


        }

    }

    for (int i = 0; i < moveCount; i++) {
        cout << "move: " << moveList[i] << endl;
    }

}