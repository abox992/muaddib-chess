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

#define SquareOf(X) _tzcnt_u64(X)
#define Bitloop(X) for(;X; X = _blsr_u64(X))

// Bitloop(bishops) {
//      const Square sq = SquareOf(bishops);
//      ...
// }

// move list to add moves to, color to gen moves for (0 for white 1 for black)
void generateMoves(Board& board, struct Move moveList[], int color) {
    int moveCount = 0;
    //int myColorOffset = color == 0 ? 0 : 6;
    int enemyColor = color == 0 ? 1 : 0;
    //int enemyColorOffset = color == 0 ? 6 : 0;

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

                            chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t initialMoveMask = pawnMoveMasks[color][currentSquare];
                            uint64_t initialAttackMask = pawnAttackMasks[color][currentSquare];

                            uint64_t pLegalMoves = initialMoveMask & board.empty;
                            uint64_t pLegalAttacks = initialAttackMask & board.allPieces[enemyColor];

                            // en passant
                            if (board.enpassantPos > 0) {

                                int offset = color == 0 ? -8 : 8;

                                uint64_t temp = board.pieces[enemyColor];
                                board.pieces[enemyColor] &= ~(uint64_t(1) << (board.enpassantPos + offset));
                                board.updateAllPieces();

                                if (generateCheckMask(board, color) == 0){
                                    pLegalAttacks |= (initialAttackMask & MaskForPos(board.enpassantPos));
                                }

                                board.pieces[enemyColor] = temp;
                                board.updateAllPieces();

                            }

                            // adjust for checks
                            pLegalMoves &= checkMask & pinHV;
                            pLegalAttacks &= checkMask & pinDiag;

                            Bitloop(pLegalMoves) {
                                const int index = SquareOf(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                moveList[moveCount++] = Temp;
                                  
                            }

                            Bitloop(pLegalAttacks) {
                                const int index = SquareOf(pLegalAttacks);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                if (index == SquareOf(board.enpassantPos)) {
                                    Temp.enpessant = index;
                                }

                                moveList[moveCount++] = Temp;
                                  
                            }
                            

                            chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            std::cout << "pawn: " << time_span.count() << "nanoseconds" << std::endl;

                            break;
                        } case 2: { // knight 

                            chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t pLegalMoves = knightMasks[currentSquare] & (board.empty | board.allPieces[enemyColor]);

                            // adjust for checks
                            pLegalMoves &= checkMask & (pinHV | pinDiag);

                            Bitloop(pLegalMoves) {
                                const int index = SquareOf(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                moveList[moveCount++] = Temp;
                                  
                            }

                            chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            std::cout << "knight: " << time_span.count() << "nanoseconds" << std::endl;
                                
                            break;
                        }
                        case 4: { // bishop
                            chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t blockers = (~board.empty) & bishopMasks[currentSquare];
                            uint64_t compressedBlockers = _pext_u64(blockers, bishopMasks[currentSquare]);

                            uint64_t pLegalMoves = bishopLegalMoves[currentSquare][compressedBlockers];
                            pLegalMoves &= ~board.allPieces[color];

                            // adjust for checks
                            pLegalMoves &= checkMask & pinDiag;

                            Bitloop(pLegalMoves) {
                                const int index = SquareOf(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                moveList[moveCount++] = Temp;
                                  
                            }

                            chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            std::cout << "bishop: " << time_span.count() << "nanoseconds" << std::endl;

                            break;
                        } case 6: { // rook

                            chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t blockers = (~board.empty) & rookMasks[currentSquare];
                            uint64_t compressedBlockers = _pext_u64(blockers, rookMasks[currentSquare]);

                            uint64_t pLegalMoves = rookLegalMoves[currentSquare][compressedBlockers];
                            pLegalMoves &= ~board.allPieces[color];

                            // adjust for checks
                            pLegalMoves &= checkMask & pinHV;

                            Bitloop(pLegalMoves) {
                                const int index = SquareOf(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                moveList[moveCount++] = Temp;
                                  
                            }

                            chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            std::cout << "rook: " << time_span.count() << "nanoseconds" << std::endl;

                            break;
                        } case 8: { // queen

                            chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                            uint64_t blockers = (~board.empty) & rookMasks[currentSquare];
                            uint64_t rookCompressedBlockers = _pext_u64(blockers, rookMasks[currentSquare]);

                            blockers = (~board.empty) & bishopMasks[currentSquare];
                            uint64_t bishopCompressedBlockers = _pext_u64(blockers, bishopMasks[currentSquare]);

                            uint64_t pLegalMoves = rookLegalMoves[currentSquare][rookCompressedBlockers] | bishopLegalMoves[currentSquare][bishopCompressedBlockers];
                            pLegalMoves &= ~board.allPieces[color];

                            // adjust for checks
                            pLegalMoves &= checkMask & pinHV & pinDiag;

                            Bitloop(pLegalMoves) {
                                const int index = SquareOf(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                moveList[moveCount++] = Temp;
                                  
                            }

                            chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
                            auto time_span = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                            std::cout << "queen: " << time_span.count() << "nanoseconds" << std::endl;

                            break;
                        } case 10: { // king 

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
                                    attackedSquares |= MaskForPos(kingPos + offset);
                                }

                            }

                            // adjust for checks
                            pLegalMoves &= ~attackedSquares;

                            // castles
                            if (~checkMask == 0) { // can only castle if not in check
                                if (board.castle[color]) { // king side
                                if ((board.empty & castleMasks[color]) != 0) {
                                    pLegalMoves |= castleSquares[color];
                                }
                            }

                            if (board.castle[color + 2]) { // queen side
                                if ((board.empty & castleMasks[color + 2]) != 0) {
                                    pLegalMoves |= castleSquares[color + 2];
                                }
                            }
                            }

                            Bitloop(pLegalMoves) {
                                const int index = SquareOf(pLegalMoves);

                                struct Move Temp;
                                Temp.from = currentSquare;
                                Temp.to = index;
                                Temp.color = color;
                                Temp.piece = piece;

                                if ((MaskForPos(index) & castleMasks[color]) != 0) {
                                    Temp.castle = MaskForPos(color);
                                }else if ((MaskForPos(index) & castleMasks[color + 2]) != 0) {
                                    Temp.castle = MaskForPos(color + 2);
                                }

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