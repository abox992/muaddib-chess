#include <chrono>
#include <cstdint>
#include <bit>
#include <tuple>
#include <immintrin.h>

#include "movegen.h"
#include "board.h"
#include "precompute_masks.h"
#include "move.h"
#include "helpers.h"
#include "check_pin_masks.h"

using namespace std;

// Bitloop(bishops) {
//      const Square sq = SquareOf(bishops);
//      ...
// }

// move list to add moves to, color to gen moves for (0 for white 1 for black), returns movecount
int generateMoves(uint64_t moveMask, Board& board, struct Move moveList[], int color) {
    int moveCount = 0;
    //int myColorOffset = color == 0 ? 0 : 6;
    int enemyColor = color == 0 ? 1 : 0;
    //int enemyColorOffset = color == 0 ? 6 : 0;

    uint64_t currentSquareMask;

    // check mask
    // chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

    tuple<uint64_t, uint64_t, uint64_t> checkAndPinMasks = generateCheckAndPinMask(board, color);
    uint64_t checkMask = get<0>(checkAndPinMasks);
    uint64_t pinHV     = get<1>(checkAndPinMasks);
    uint64_t pinDiag   = get<2>(checkAndPinMasks);

    // chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
    // auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    // std::cout << "checkmask: " << time_span.count() << "nanoseconds" << std::endl;

    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int currentSquare = (7 - file) + (8 * rank);
            currentSquareMask = MaskForPos(currentSquare);

            if ((board.empty & currentSquareMask) != 0) { // skip if empty
                continue;
            }

            for (int piece = 0; piece < 12; piece += 2) {
                uint64_t currentBB = board.getPieceSet(piece + color);

                if ((currentBB & currentSquareMask) != 0) { // found piece

                    /*  
                    *  9  8  7
                    *  1  0 -1
                    * -7 -8 -9
                    */
                    switch (piece) {
                        case 0: { // pawn 

                            // chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t initialMoveMask = pawnMoveMasks[color][currentSquare];
                            uint64_t initialAttackMask = pawnAttackMasks[color][currentSquare];

                            uint64_t pLegalMoves = initialMoveMask & board.empty;
                            uint64_t pLegalAttacks = initialAttackMask & board.allPieces[enemyColor];

                            // en passant
                            if (board.enpassantPos > 0) {

                                //int offset = color == 0 ? -8 : 8;

                                // uint64_t temp = board.pieces[enemyColor];
                                // board.pieces[enemyColor] &= ~(uint64_t(1) << (board.enpassantPos + offset));
                                // board.pieces[color] &= ~currentSquareMask;
                                // board.pieces[color] |= MaskForPos(board.enpassantPos);
                                // board.updateAllPieces();

                                //if (generateCheckMask(board, color) == 0) {
                                    pLegalAttacks |= (initialAttackMask & MaskForPos(board.enpassantPos));
                                //}

                                // board.pieces[enemyColor] = temp;
                                // board.pieces[color] |= currentSquareMask;
                                // board.pieces[color] &= ~MaskForPos(board.enpassantPos);
                                // board.updateAllPieces();

                            }

                            // adjust for checks
                            pLegalMoves &= checkMask;
                            pLegalAttacks &= checkMask;

                            if (board.enpassantPos > 0) {
                                int offset = color == 0 ? -8 : 8;
                                board.pieces[enemyColor] &= ~(uint64_t(1) << (board.enpassantPos + offset));

                                if (generateCheckMask(board, color) == 0) {
                                    pLegalAttacks |= (initialAttackMask & MaskForPos(board.enpassantPos));
                                }

                                board.pieces[enemyColor] |= (uint64_t(1) << (board.enpassantPos + offset));

                            }

                            pLegalMoves &= moveMask;

                            Bitloop(pLegalMoves) {
                                const int index = SquareOf(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                int currentY = currentSquare / 8;
                                int currentX = currentSquare % 8;
                                int newY = index / 8;
                                int newX = index % 8;

                                // make sure were not jumping over a piece on pawn double move
                                int maxDif = max(abs(currentX - newX), abs(currentY - newY));
                                if (maxDif > 1) {
                                    if ((pawnMoveMasks[color][currentSquare] & ~board.empty) != 0) { // check if square is empty
                                        continue;
                                    }
                                }

                                // promotion
                                if ((index / 8) == 0 || (index / 8) == 7) {
                                    for (int i = 0; i < 4; i++) {
                                        struct Move Promo;
                                        Promo = Temp;
                                        Promo.promotion = MaskForPos(i);

                                        // check pin
                                        Board pin = board;
                                        pin.makeMove(Temp);
                                        uint64_t checkAfterMove = attacksToKing(pin, color);
                                        if (checkAfterMove != 0) {
                                            continue;
                                        }

                                        moveList[moveCount++] = Promo;
                                    }

                                    continue;
                                }

                                // check pin
                                Board pin = board;
                                pin.makeMove(Temp);
                                uint64_t checkAfterMove = attacksToKing(pin, color);
                                if (checkAfterMove != 0) {
                                    continue;
                                }

                                moveList[moveCount++] = Temp;
                                  
                            }

                            pLegalAttacks &= moveMask;

                            Bitloop(pLegalAttacks) {
                                const int index = SquareOf(pLegalAttacks);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                if (board.enpassantPos > 0 && index == board.enpassantPos) {
                                    Temp.enpessant = true;
                                }

                                // promotion
                                if ((index / 8) == 0 || (index / 8) == 7) {
                                    for (int i = 0; i < 4; i++) {
                                        struct Move Promo;
                                        Promo = Temp;
                                        Promo.promotion = MaskForPos(i);

                                        // check pin
                                        Board pin = board;
                                        pin.makeMove(Temp);
                                        uint64_t checkAfterMove = attacksToKing(pin, color);
                                        if (checkAfterMove != 0) {
                                            continue;
                                        }

                                        moveList[moveCount++] = Promo;
                                    }

                                    continue;
                                }

                                // check pin
                                Board pin = board;
                                pin.makeMove(Temp);
                                uint64_t checkAfterMove = attacksToKing(pin, color);
                                if (checkAfterMove != 0) {
                                    continue;
                                }

                                moveList[moveCount++] = Temp;
                                  
                            }
                            

                            // chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            // auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            // std::cout << "pawn: " << time_span.count() << "nanoseconds" << std::endl;

                            break;
                        } case 2: { // knight 

                            // chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t pLegalMoves = knightMasks[currentSquare] & (board.empty | board.allPieces[enemyColor]);

                            // adjust for checks
                            //pLegalMoves &= checkMask & (pinHV | pinDiag);
                            pLegalMoves &= checkMask;

                            pLegalMoves &= moveMask;

                            Bitloop(pLegalMoves) {
                                const int index = SquareOf(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                // check pin
                                Board pin = board;
                                pin.makeMove(Temp);
                                uint64_t checkAfterMove = attacksToKing(pin, color);
                                if (checkAfterMove != 0) {
                                    continue;
                                }

                                moveList[moveCount++] = Temp;
                                  
                            }

                            // chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            // auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            // std::cout << "knight: " << time_span.count() << "nanoseconds" << std::endl;
                                
                            break;
                        }
                        case 4: { // bishop
                            // chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t blockers = (~board.empty) & bishopMasks[currentSquare];
                            uint64_t compressedBlockers = _pext_u64(blockers, bishopMasks[currentSquare]);

                            uint64_t pLegalMoves = bishopLegalMoves[currentSquare][compressedBlockers];
                            pLegalMoves &= ~board.allPieces[color];

                            // adjust for checks
                            //pLegalMoves &= checkMask & pinDiag;
                            pLegalMoves &= checkMask;

                            if ((currentSquareMask & pinHV) != 0) { // can not move if pinned vert
                                continue;
                            }

                            pLegalMoves &= moveMask;

                            Bitloop(pLegalMoves) {
                                const int index = SquareOf(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                // check pin
                                Board pin = board;
                                pin.makeMove(Temp);
                                uint64_t checkAfterMove = attacksToKing(pin, color);
                                if (checkAfterMove != 0) {
                                    continue;
                                }

                                moveList[moveCount++] = Temp;
                                  
                            }

                            // chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            // auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            // std::cout << "bishop: " << time_span.count() << "nanoseconds" << std::endl;

                            break;
                        } case 6: { // rook

                            // chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t blockers = (~board.empty) & rookMasks[currentSquare];
                            uint64_t compressedBlockers = _pext_u64(blockers, rookMasks[currentSquare]);

                            uint64_t pLegalMoves = rookLegalMoves[currentSquare][compressedBlockers];
                            pLegalMoves &= ~board.allPieces[color];

                            // adjust for checks
                            //pLegalMoves &= checkMask & pinHV;
                            pLegalMoves &= checkMask;

                            if ((currentSquareMask & pinDiag) != 0) { // can not move if pinned diag
                                continue;
                            }

                            pLegalMoves &= moveMask;

                            Bitloop(pLegalMoves) {
                                const int index = SquareOf(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                // check pin
                                Board pin = board;
                                pin.makeMove(Temp);
                                uint64_t checkAfterMove = attacksToKing(pin, color);
                                if (checkAfterMove != 0) {
                                    continue;
                                }

                                moveList[moveCount++] = Temp;
                                  
                            }

                            // chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            // auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            // std::cout << "rook: " << time_span.count() << "nanoseconds" << std::endl;

                            break;
                        } case 8: { // queen

                            // chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t blockers = (~board.empty) & rookMasks[currentSquare];
                            uint64_t rookCompressedBlockers = _pext_u64(blockers, rookMasks[currentSquare]);

                            blockers = (~board.empty) & bishopMasks[currentSquare];
                            uint64_t bishopCompressedBlockers = _pext_u64(blockers, bishopMasks[currentSquare]);

                            uint64_t pLegalMoves = rookLegalMoves[currentSquare][rookCompressedBlockers] | bishopLegalMoves[currentSquare][bishopCompressedBlockers];
                            pLegalMoves &= ~board.allPieces[color];

                            // adjust for checks
                            //pLegalMoves &= checkMask & pinHV & pinDiag;
                            // printBitboard(pinDiag);
                            // printBitboard(pinHV);
                            pLegalMoves &= checkMask;

                            pLegalMoves &= moveMask;

                            Bitloop(pLegalMoves) {
                                const int index = SquareOf(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                // check pin
                                Board pin = board;
                                pin.makeMove(Temp);
                                uint64_t checkAfterMove = attacksToKing(pin, color);
                                if (checkAfterMove != 0) {
                                    continue;
                                }

                                moveList[moveCount++] = Temp;
                                  
                            }

                            // chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            // auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            // std::cout << "queen: " << time_span.count() << "nanoseconds" << std::endl;

                            break;
                        } case 10: { // king 

                            // chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t pLegalMoves = kingMasks[currentSquare] & (board.empty | board.allPieces[enemyColor]);

                            int kingPos = currentSquare;

                            // castles
                            if (~checkMask == 0) { // can only castle if not in check
                                if (board.canCastle[color]) { // king side
                                    if ((~board.empty & castleMasks[color]) == 0) {
                                        if (attacksOnSquare(board, color, currentSquare - 1) == 0) { // cannot pass through check
                                            pLegalMoves |= castleSquares[color];
                                        }
                                    }
                                }

                                if (board.canCastle[color + 2]) { // queen side
                                    if ((~board.empty & castleMasks[color + 2]) == 0) {
                                        if (attacksOnSquare(board, color, currentSquare + 1) == 0) { // cannot pass through check
                                            pLegalMoves |= castleSquares[color + 2];
                                        }
                                    }
                                }
                            }

                            pLegalMoves &= moveMask;

                            Bitloop(pLegalMoves) {
                                const int index = SquareOf(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                int currentY = kingPos / 8;
                                int currentX = kingPos % 8;
                                int newY = index / 8;
                                int newX = index % 8;

                                int maxDif = max(abs(currentX - newX), abs(currentY - newY));

                                if (maxDif > 1) {
                                    if ((MaskForPos(index) & castleSquares[color]) != 0) {
                                        Temp.castle = MaskForPos(color);
                                    }else if ((MaskForPos(index) & castleSquares[color + 2]) != 0) {
                                        Temp.castle = MaskForPos(color + 2);
                                    }
                                }

                                // filter out moves that leave the king in check
                                if (attacksOnSquareIgnoreKing(board, color, index) != 0) {
                                    continue;
                                }

                                moveList[moveCount++] = Temp;
                                  
                            }

                            // chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            // auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            // std::cout << "king: " << time_span.count() << "nanoseconds" << std::endl;
                                
                            break;
                        }
                    }

                }

            }


        }

    }

    // for (int i = 0; i < moveCount; i++) {
    //     cout << "move: " << moveList[i] << endl;
    // }

    return moveCount;

}