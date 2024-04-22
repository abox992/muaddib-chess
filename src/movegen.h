#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <cstdint>
#include <tuple>
#include "board.h"
#include "precompute_masks.h"
#include "move.h"
#include "helpers.h"
#include "check_pin_masks.h"
#include "bit_manip.h"
#include "constants.h"

enum MoveFilter {
    ALL,
    CAPTURES,
    QUIET
};

// move list to add moves to, color to gen moves for (0 for white 1 for black), returns movecount
template<MoveFilter moveFilter, Color color>
void generateMoves(const Board& board, std::vector<Move>& moveList) {

    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t moveTypeMask; // we generate all moves, then filter by this mask
    if constexpr (moveFilter == MoveFilter::ALL) {
        moveTypeMask = ~uint64_t(0);
    }
    else if constexpr (moveFilter == MoveFilter::CAPTURES) {
        moveTypeMask = board.state.allPieces[enemyColor];
    }
    else if constexpr (moveFilter == MoveFilter::QUIET) {
        moveTypeMask = board.state.empty;
    }

    // check mask
    //std::tuple<uint64_t, uint64_t, uint64_t> checkAndPinMasks = generateCheckAndPinMask(board, color);
    uint64_t checkMask = generateCheckMask(board, color);
    uint64_t pinMask = generatePinMask(board, color);
    int kingPos = squareOf(board.state.pieces[Piece::KINGS + color]);

    int attackersCount = std::popcount(attacksToKing(board, color));

    /*

        Iterate board from top left to bottom right (63 -> 0)

        63 62 61 60 59 58 57 56
        57 56   .....
        07 06 05 04 03 02 01 00

    */
    for (int currentSquare = 63; currentSquare >= 0; currentSquare--) {
        const uint64_t currentSquareMask = maskForPos(currentSquare);

        if ((board.state.empty & currentSquareMask) != 0) { // skip if empty
            continue;
        }

        // find the corresponding piece type
        int piece = 0;
        for (; piece < 12; piece += 2) {
            uint64_t currentBB = board.state.pieces[piece + color];

            if ((currentBB & currentSquareMask) != 0) { // found piece at square
                break;
            }
        }

        // if king is in double-check, only need to enumerate king moves
        if (attackersCount > 1 && piece != Piece::KINGS) {
            continue;
        }

        /*  
        *  9  8  7
        *  1  0 -1
        * -7 -8 -9
        */
        switch (piece) {
            case Piece::PAWNS: { // pawn 

                // chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

                uint64_t initialMoveMask = pawnMoveMasks[color][currentSquare];
                uint64_t initialAttackMask = pawnAttackMasks[color][currentSquare];

                uint64_t pLegalMoves = initialMoveMask & board.state.empty;
                uint64_t pLegalAttacks = initialAttackMask & board.state.allPieces[enemyColor];

                // make sure were not jumping over a piece on initial double move
                if (std::popcount(initialMoveMask) == 2) {
                    constexpr int offset = color == Color::WHITE ? 8 : -8;
                    if ((maskForPos(currentSquare + offset) & ~board.state.empty) != 0) {
                        pLegalMoves = 0;
                    }
                }

                pLegalMoves |= pLegalAttacks;

                // adjust for checks
                pLegalMoves &= checkMask;
                // adjust for move type
                pLegalMoves &= moveTypeMask;

                // en passant is special - have to play it out and check if we are still in check manually, does not get pruned away by masks
                if (board.state.enpassantPos > 0) {
                    pLegalMoves |= (initialAttackMask & maskForPos(board.state.enpassantPos)); // a

                    constexpr int offset = color == Color::WHITE ? -8 : 8;

                    /* Borrowed from attacksToSquare(), we are just augmenting the position initially */
                    uint64_t adjustedPieces = (~board.state.empty);
                    adjustedPieces &= ~currentSquareMask;
                    adjustedPieces &= ~maskForPos(board.state.enpassantPos + offset);
                    adjustedPieces |= maskForPos(board.state.enpassantPos);

                    uint64_t opPawns, opKnights, opRQ, opBQ;
                    opPawns = board.state.pieces[0 + enemyColor];
                    opKnights = board.state.pieces[2 + enemyColor];
                    opRQ = opBQ = board.state.pieces[8 + enemyColor];
                    opRQ |= board.state.pieces[6 + enemyColor];
                    opBQ |= board.state.pieces[4 + enemyColor];

                    opPawns &= ~maskForPos(board.state.enpassantPos + offset);

                    uint64_t blockers = adjustedPieces & rookMasks[kingPos];
                    uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[kingPos]);

                    blockers = adjustedPieces & bishopMasks[kingPos];
                    uint64_t bishopCompressedBlockers = extract_bits(blockers, bishopMasks[kingPos]);

                    uint64_t kingAttackers = (pawnAttackMasks[color][kingPos] & opPawns)
                        | (knightMasks[kingPos] & opKnights)
                        | (bishopLegalMoves[kingPos][bishopCompressedBlockers] & opBQ)
                        | (rookLegalMoves[kingPos][rookCompressedBlockers] & opRQ)
                        ;

                    // remove if still in check
                    if (kingAttackers != 0) {
                        pLegalMoves &= ~(initialAttackMask & maskForPos(board.state.enpassantPos)); // a
                    }

                }

                // adjust from pins pins
                if ((pinMask & currentSquareMask) != 0) { // might be pinned
                    for (int i = 0; i < 8; i++) { // loop through all directions/axis'
                        uint64_t axis = pinMask & directionMasks[i][kingPos];
                        if ((axis & currentSquareMask) == 0) {
                            continue; // not on this pin axis
                        }

                        if (std::popcount(axis & board.state.allPieces[color]) == 1) { // more than 1 piece means were not actually pinned
                            pLegalMoves &= axis;
                            break; // can only be pinned on a single axis, stop checking if we find one
                        }
                    }
                }

                // moves are now all legal, add them to the list
                for (int index = std::countr_zero(pLegalMoves); pLegalMoves; popRSB(pLegalMoves), index = std::countr_zero(pLegalMoves)) {

                    Move temp;
                    temp.from = currentSquare;
                    temp.to = index;
                    temp.color = color;
                    temp.piece = piece;

                    if (board.state.enpassantPos > 0 && index == board.state.enpassantPos) {
                        temp.enpessant = true;
                    }

                    // promotion
                    if ((index / 8) == 0 || (index / 8) == 7) {
                        for (int i = 0; i < 4; i++) {
                            Move promo;
                            promo = temp;
                            promo.promotion = maskForPos(i);

                            moveList.push_back(promo);
                        }

                        continue;
                    }

                    moveList.push_back(temp);
                        
                }

                break;
            } case Piece::KNIGHTS: { // knight 

                uint64_t pLegalMoves = knightMasks[currentSquare] & (board.state.empty | board.state.allPieces[enemyColor]);

                // adjust for checks
                pLegalMoves &= checkMask;
                // adjust for move type
                pLegalMoves &= moveTypeMask;

                // adjust from pins pins
                if ((pinMask & currentSquareMask) != 0) { // might be pinned
                    for (int i = 0; i < 8; i++) { // loop through all directions/axis'
                        uint64_t axis = pinMask & directionMasks[i][kingPos];
                        if ((axis & currentSquareMask) == 0) {
                            continue; // not on this pin axis
                        }

                        if (std::popcount(axis & board.state.allPieces[color]) == 1) { // more than 1 piece means were not actually pinned
                            pLegalMoves &= axis;
                            break; // can only be pinned on a single axis, stop checking if we find one
                        }
                    }
                }

                Bitloop(pLegalMoves) {
                    const int index = squareOf(pLegalMoves);

                    struct Move temp;
                    temp.from = currentSquare;
                    temp.to = index;
                    temp.color = color;
                    temp.piece = piece;

                    moveList.push_back(temp);
                        
                }
                    
                break;
            }
            case Piece::BISHOPS: { // bishop

                uint64_t blockers = (~board.state.empty) & bishopMasks[currentSquare];
                uint64_t compressedBlockers = extract_bits(blockers, bishopMasks[currentSquare]);

                uint64_t pLegalMoves = bishopLegalMoves[currentSquare][compressedBlockers];
                pLegalMoves &= ~board.state.allPieces[color]; // blockers above assumes we can take any piece, remove captures of our own pieces

                // adjust for checks
                pLegalMoves &= checkMask;
                // adjust for move type
                pLegalMoves &= moveTypeMask;

                // adjust from pins pins
                if ((pinMask & currentSquareMask) != 0) { // might be pinned
                    for (int i = 0; i < 8; i++) { // loop through all directions/axis'
                        uint64_t axis = pinMask & directionMasks[i][kingPos];
                        if ((axis & currentSquareMask) == 0) {
                            continue; // not on this pin axis
                        }

                        if (std::popcount(axis & board.state.allPieces[color]) == 1) { // more than 1 piece means were not actually pinned
                            pLegalMoves &= axis;
                            break; // can only be pinned on a single axis, stop checking if we find one
                        }
                    }
                }

                Bitloop(pLegalMoves) {
                    const int index = squareOf(pLegalMoves);

                    struct Move temp;
                    temp.from = currentSquare;
                    temp.to = index;
                    temp.color = color;
                    temp.piece = piece;

                    moveList.push_back(temp);
                        
                }

                break;
            } case Piece::ROOKS: { // rook
                uint64_t blockers = (~board.state.empty) & rookMasks[currentSquare];
                uint64_t compressedBlockers = extract_bits(blockers, rookMasks[currentSquare]);

                uint64_t pLegalMoves = rookLegalMoves[currentSquare][compressedBlockers];
                pLegalMoves &= ~board.state.allPieces[color]; // blockers above assumes we can take any piece, remove captures of our own pieces

                // adjust for checks
                pLegalMoves &= checkMask;
                // adjust for move type
                pLegalMoves &= moveTypeMask;

                // adjust from pins pins
                if ((pinMask & currentSquareMask) != 0) { // might be pinned
                    for (int i = 0; i < 8; i++) { // loop through all directions/axis'
                        uint64_t axis = pinMask & directionMasks[i][kingPos];
                        if ((axis & currentSquareMask) == 0) {
                            continue; // not on this pin axis
                        }

                        if (std::popcount(axis & board.state.allPieces[color]) == 1) { // more than 1 piece means were not actually pinned
                            pLegalMoves &= axis;
                            break; // can only be pinned on a single axis, stop checking if we find one
                        }
                    }
                }

                Bitloop(pLegalMoves) {
                    const int index = squareOf(pLegalMoves);

                    struct Move temp;
                    temp.from = currentSquare;
                    temp.to = index;
                    temp.color = color;
                    temp.piece = piece;

                    moveList.push_back(temp);
                        
                }

                break;
            } case Piece::QUEENS: { // queen

                uint64_t blockers = (~board.state.empty) & rookMasks[currentSquare];
                uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[currentSquare]);

                blockers = (~board.state.empty) & bishopMasks[currentSquare];
                uint64_t bishopCompressedBlockers = extract_bits(blockers, bishopMasks[currentSquare]);

                uint64_t pLegalMovesHV = rookLegalMoves[currentSquare][rookCompressedBlockers];
                uint64_t pLegalMovesDiag = bishopLegalMoves[currentSquare][bishopCompressedBlockers];
                uint64_t pLegalMoves = pLegalMovesHV | pLegalMovesDiag;
                pLegalMoves &= ~board.state.allPieces[color]; // blockers above assumes we can take any piece, remove captures of our own pieces

                // adjust from pins pins
                if ((pinMask & currentSquareMask) != 0) { // might be pinned
                    for (int i = 0; i < 8; i++) { // loop through all directions/axis'
                        uint64_t axis = pinMask & directionMasks[i][kingPos];
                        if ((axis & currentSquareMask) == 0) {
                            continue; // not on this pin axis
                        }

                        if (std::popcount(axis & board.state.allPieces[color]) == 1) { // more than 1 piece means were not actually pinned
                            pLegalMoves &= axis;
                            break; // can only be pinned on a single axis, stop checking if we find one
                        }
                    }
                }
                
                // adjust for checks
                pLegalMoves &= checkMask;
                pLegalMoves &= moveTypeMask;

                Bitloop(pLegalMoves) {
                    const int index = squareOf(pLegalMoves);

                    struct Move temp;
                    temp.from = currentSquare;
                    temp.to = index;
                    temp.color = color;
                    temp.piece = piece;

                    moveList.push_back(temp);
                        
                }

                break;
            } case Piece::KINGS: { // king 

                uint64_t pLegalMoves = kingMasks[currentSquare] & (board.state.empty | board.state.allPieces[enemyColor]);

                int kingPos = currentSquare;

                // castles
                if (~checkMask == 0) { // can only castle if not in check
                    if (board.state.canCastle[color]) { // king side
                        if ((~board.state.empty & castleMasks[color]) == 0) {
                            if (attacksOnSquare<color>(board, currentSquare - 1) == 0) { // cannot pass through check
                                pLegalMoves |= castleSquares[color];
                            }
                        }
                    }

                    if (board.state.canCastle[color + 2]) { // queen side
                        if ((~board.state.empty & castleMasks[color + 2]) == 0) {
                            if (attacksOnSquare<color>(board, currentSquare + 1) == 0) { // cannot pass through check
                                pLegalMoves |= castleSquares[color + 2];
                            }
                        }
                    }
                }

                pLegalMoves &= moveTypeMask;

                Bitloop(pLegalMoves) {
                    const int index = squareOf(pLegalMoves);

                    // filter out moves that leave the king in check
                    if (attacksOnSquareIgnoreKing(board, color, index) != 0) {
                        continue;
                    }

                    struct Move temp;
                    temp.from = currentSquare;
                    temp.to = index;
                    temp.color = color;
                    temp.piece = piece;

                    int currentY = kingPos / 8;
                    int currentX = kingPos % 8;
                    int newY = index / 8;
                    int newX = index % 8;

                    int maxDif = std::max(std::abs(currentX - newX), std::abs(currentY - newY));

                    if (maxDif > 1) {
                        if ((maskForPos(index) & castleSquares[color]) != 0) {
                            temp.castle = maskForPos(color);
                        } else if ((maskForPos(index) & castleSquares[color + 2]) != 0) {
                            temp.castle = maskForPos(color + 2);
                        }
                    }

                    moveList.push_back(temp);
                        
                }
                    
                break;
            }
        }

    }

    //assert(moveList.capacity() == 256 && moveList.size() < 256);

}

#endif