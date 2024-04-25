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

    // make sure movelist is not resizing
    assert(moveList.capacity() == 256);

    int attackersCount = std::popcount(attacksToKing<color, false>(board));

    const uint64_t checkMask = generateCheckMask<color>(board);
    const uint64_t pinMask = generatePinMask<color>(board);
    const int kingPos = tz_count(board.curState->pieces[Piece::KINGS + color]);

    // if king is in double-check, only need to enumerate king moves
    if (attackersCount < 2) {

        generatePawnMoves<moveFilter, color>(board, moveList, checkMask, pinMask, kingPos);
        generateKnightMoves<moveFilter, color>(board, moveList, checkMask, pinMask, kingPos);
        generateBishopMoves<moveFilter, color>(board, moveList, checkMask, pinMask, kingPos);
        generateRookMoves<moveFilter, color>(board, moveList, checkMask, pinMask, kingPos);
        generateQueenMoves<moveFilter, color>(board, moveList, checkMask, pinMask, kingPos);
    }
    generateKingMoves<moveFilter, color>(board, moveList, checkMask, kingPos);

    // make sure movelist is not resizing
    assert(moveList.capacity() == 256 && moveList.size() < 256);

}

template<MoveFilter moveFilter, Color color>
inline void generatePawnMoves(const Board& board, std::vector<Move>& moveList, const uint64_t& checkMask, const uint64_t& pinMask, const int& kingPos) {

    // const uint64_t checkMask = generateCheckMask<color>(board);
    // const uint64_t pinMask = generatePinMask<color>(board);
    // const int kingPos = tz_count(board.curState->pieces[Piece::KINGS + color]);

    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t moveTypeMask; // we generate all moves, then filter by this mask
    if constexpr (moveFilter == MoveFilter::ALL) {
        moveTypeMask = ~uint64_t(0);
    }
    else if constexpr (moveFilter == MoveFilter::CAPTURES) {
        moveTypeMask = board.curState->allPieces[enemyColor];
    }
    else if constexpr (moveFilter == MoveFilter::QUIET) {
        moveTypeMask = board.curState->empty;
    }

    uint64_t bitboard = board.curState->pieces[Piece::PAWNS + color];
    while (bitboard) { // bitloop
        const int currentSquare = tz_count(bitboard);
        pop_lsb(bitboard); // we only need to get current square, can do this now dont have to wait for end of loop

        const uint64_t currentSquareMask = maskForPos(currentSquare);
        
        uint64_t initialMoveMask = pawnMoveMasks[color][currentSquare];
        uint64_t initialAttackMask = pawnAttackMasks[color][currentSquare];

        uint64_t pLegalMoves = initialMoveMask & board.curState->empty;
        uint64_t pLegalAttacks = initialAttackMask & board.curState->allPieces[enemyColor];

        // make sure were not jumping over a piece on initial double move
        if (std::popcount(initialMoveMask) == 2) {
            constexpr int offset = color == Color::WHITE ? 8 : -8;
            if ((maskForPos(currentSquare + offset) & ~board.curState->empty) != 0) {
                pLegalMoves = 0;
            }
        }

        pLegalMoves |= pLegalAttacks;

        // adjust for checks
        pLegalMoves &= checkMask;
        // adjust for move type
        pLegalMoves &= moveTypeMask;

        // en passant is special - have to play it out and check if we are still in check manually, does not get pruned away by masks
        if (board.curState->enpassantPos > 0) {
            pLegalMoves |= (initialAttackMask & maskForPos(board.curState->enpassantPos)); // a

            constexpr int offset = color == Color::WHITE ? -8 : 8;

            /* Borrowed from attacksToSquare(), we are just augmenting the position initially */
            uint64_t adjustedPieces = (~board.curState->empty);
            adjustedPieces &= ~currentSquareMask;
            adjustedPieces &= ~maskForPos(board.curState->enpassantPos + offset);
            adjustedPieces |= maskForPos(board.curState->enpassantPos);

            uint64_t opPawns, opKnights, opRQ, opBQ;
            opPawns = board.curState->pieces[0 + enemyColor];
            opKnights = board.curState->pieces[2 + enemyColor];
            opRQ = opBQ = board.curState->pieces[8 + enemyColor];
            opRQ |= board.curState->pieces[6 + enemyColor];
            opBQ |= board.curState->pieces[4 + enemyColor];

            opPawns &= ~maskForPos(board.curState->enpassantPos + offset);

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
                pLegalMoves &= ~(initialAttackMask & maskForPos(board.curState->enpassantPos));
            }

        }

        // adjust from pins pins
        if ((pinMask & currentSquareMask) != 0) { // might be pinned
            for (int i = 0; i < 8; i++) { // loop through all directions
                uint64_t axis = pinMask & directionMasks[i][kingPos];
                if ((axis & currentSquareMask) == 0) {
                    continue; // not on this pin axis
                }

                if (std::popcount(axis & board.curState->allPieces[color]) == 1) { // more than 1 piece means were not actually pinned
                    pLegalMoves &= axis;
                    break; // can only be pinned on a single axis, stop checking if we find one
                }
            }
        }

        // moves are now all legal, add them to the list
        while(pLegalMoves) { //for (int index = tz_count(pLegalMoves); pLegalMoves; pop_lsb(pLegalMoves), index = tz_count(pLegalMoves))
            const int index = tz_count(pLegalMoves);
            pop_lsb(pLegalMoves);

            Move curMove = Move::make<MoveType::NORMAL>(currentSquare, index);

            if (board.curState->enpassantPos > 0 && index == board.curState->enpassantPos) {
                //temp.enpessant = true;

                curMove = Move::make<MoveType::EN_PASSANT>(currentSquare, index);
                moveList.push_back(curMove);
                continue;
            }

            // promotion
            if (promoSquare[index]) {
                for (int i = 0; i < 4; i++) {

                    curMove = Move::make<MoveType::PROMOTION>(currentSquare, index, static_cast<PromoPiece>(i));
                    moveList.push_back(curMove); 
                }

                continue;
            }

            moveList.push_back(curMove);
        }

    } // bitloop

}

template<MoveFilter moveFilter, Color color>
inline void generateKnightMoves(const Board& board, std::vector<Move>& moveList, const uint64_t& checkMask, const uint64_t& pinMask, const int& kingPos) {

    // const uint64_t checkMask = generateCheckMask<color>(board);
    // const uint64_t pinMask = generatePinMask<color>(board);
    // const int kingPos = tz_count(board.curState->pieces[Piece::KINGS + color]);

    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t moveTypeMask; // we generate all moves, then filter by this mask
    if constexpr (moveFilter == MoveFilter::ALL) {
        moveTypeMask = ~uint64_t(0);
    }
    else if constexpr (moveFilter == MoveFilter::CAPTURES) {
        moveTypeMask = board.curState->allPieces[enemyColor];
    }
    else if constexpr (moveFilter == MoveFilter::QUIET) {
        moveTypeMask = board.curState->empty;
    }

    uint64_t bitboard = board.curState->pieces[Piece::KNIGHTS + color];
    while (bitboard) { // bitloop
        const int currentSquare = tz_count(bitboard);
        pop_lsb(bitboard);
        const uint64_t currentSquareMask = maskForPos(currentSquare);

        uint64_t pLegalMoves = knightMasks[currentSquare] & (board.curState->empty | board.curState->allPieces[enemyColor]);

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

                if (std::popcount(axis & board.curState->allPieces[color]) == 1) { // more than 1 piece would mean were not actually pinned
                    pLegalMoves &= axis;
                    break; // can only be pinned on a single axis, stop checking if we find one
                }
            }
        }

        while (pLegalMoves) {
            const int index = tz_count(pLegalMoves);
            pop_lsb(pLegalMoves);

            Move curMove = Move::make<NORMAL>(currentSquare, index);
            moveList.push_back(curMove);
        }
    } // bitloop
}

template<MoveFilter moveFilter, Color color>
inline void generateBishopMoves(const Board& board, std::vector<Move>& moveList, const uint64_t& checkMask, const uint64_t& pinMask, const int& kingPos) {

    // const uint64_t checkMask = generateCheckMask<color>(board);
    // const uint64_t pinMask = generatePinMask<color>(board);
    // const int kingPos = tz_count(board.curState->pieces[Piece::KINGS + color]);

    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t moveTypeMask; // we generate all moves, then filter by this mask
    if constexpr (moveFilter == MoveFilter::ALL) {
        moveTypeMask = ~uint64_t(0);
    }
    else if constexpr (moveFilter == MoveFilter::CAPTURES) {
        moveTypeMask = board.curState->allPieces[enemyColor];
    }
    else if constexpr (moveFilter == MoveFilter::QUIET) {
        moveTypeMask = board.curState->empty;
    }

    uint64_t bitboard = board.curState->pieces[Piece::BISHOPS + color];
    while (bitboard) { // bitloop
        const int currentSquare = tz_count(bitboard);
        pop_lsb(bitboard);

        const uint64_t currentSquareMask = maskForPos(currentSquare);

        uint64_t blockers = (~board.curState->empty) & bishopMasks[currentSquare];
        uint64_t compressedBlockers = extract_bits(blockers, bishopMasks[currentSquare]);

        uint64_t pLegalMoves = bishopLegalMoves[currentSquare][compressedBlockers];
        pLegalMoves &= ~board.curState->allPieces[color]; // blockers above assumes we can take any piece, remove captures of our own pieces

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

                if (std::popcount(axis & board.curState->allPieces[color]) == 1) { // more than 1 piece means were not actually pinned
                    pLegalMoves &= axis;
                    break; // can only be pinned on a single axis, stop checking if we find one
                }
            }
        }

        while (pLegalMoves) {
            const int index = tz_count(pLegalMoves);
            pop_lsb(pLegalMoves);

            Move curMove = Move::make<NORMAL>(currentSquare, index);
            moveList.push_back(curMove);
                
        }
    } // bitloop
}

template<MoveFilter moveFilter, Color color>
inline void generateRookMoves(const Board& board, std::vector<Move>& moveList, const uint64_t& checkMask, const uint64_t& pinMask, const int& kingPos) {

    // const uint64_t checkMask = generateCheckMask<color>(board);
    // const uint64_t pinMask = generatePinMask<color>(board);
    // const int kingPos = tz_count(board.curState->pieces[Piece::KINGS + color]);

    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t moveTypeMask; // we generate all moves, then filter by this mask
    if constexpr (moveFilter == MoveFilter::ALL) {
        moveTypeMask = ~uint64_t(0);
    }
    else if constexpr (moveFilter == MoveFilter::CAPTURES) {
        moveTypeMask = board.curState->allPieces[enemyColor];
    }
    else if constexpr (moveFilter == MoveFilter::QUIET) {
        moveTypeMask = board.curState->empty;
    }

    uint64_t bitboard = board.curState->pieces[Piece::ROOKS + color];
    while (bitboard) { // bitloop
        const int currentSquare = tz_count(bitboard);
        pop_lsb(bitboard);

        const uint64_t currentSquareMask = maskForPos(currentSquare);

        uint64_t blockers = (~board.curState->empty) & rookMasks[currentSquare];
        uint64_t compressedBlockers = extract_bits(blockers, rookMasks[currentSquare]);

        uint64_t pLegalMoves = rookLegalMoves[currentSquare][compressedBlockers];
        pLegalMoves &= ~board.curState->allPieces[color]; // blockers above assumes we can take any piece, remove captures of our own pieces

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

                if (std::popcount(axis & board.curState->allPieces[color]) == 1) { // more than 1 piece means were not actually pinned
                    pLegalMoves &= axis;
                    break; // can only be pinned on a single axis, stop checking if we find one
                }
            }
        }

        while (pLegalMoves) {
            const int index = tz_count(pLegalMoves);
            pop_lsb(pLegalMoves);

            Move curMove = Move::make<NORMAL>(currentSquare, index);
            moveList.push_back(curMove);
        }
    } // bitloop
}

template<MoveFilter moveFilter, Color color>
inline void generateQueenMoves(const Board& board, std::vector<Move>& moveList, const uint64_t& checkMask, const uint64_t& pinMask, const int& kingPos) {

    // const uint64_t checkMask = generateCheckMask<color>(board);
    // const uint64_t pinMask = generatePinMask<color>(board);
    // const int kingPos = tz_count(board.curState->pieces[Piece::KINGS + color]);

    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t moveTypeMask; // we generate all moves, then filter by this mask
    if constexpr (moveFilter == MoveFilter::ALL) {
        moveTypeMask = ~uint64_t(0);
    }
    else if constexpr (moveFilter == MoveFilter::CAPTURES) {
        moveTypeMask = board.curState->allPieces[enemyColor];
    }
    else if constexpr (moveFilter == MoveFilter::QUIET) {
        moveTypeMask = board.curState->empty;
    }

    uint64_t bitboard = board.curState->pieces[Piece::QUEENS + color];
    while (bitboard) { // bitloop
        const int currentSquare = tz_count(bitboard);
        pop_lsb(bitboard);

        const uint64_t currentSquareMask = maskForPos(currentSquare);

        uint64_t blockers = (~board.curState->empty) & rookMasks[currentSquare];
        uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[currentSquare]);

        blockers = (~board.curState->empty) & bishopMasks[currentSquare];
        uint64_t bishopCompressedBlockers = extract_bits(blockers, bishopMasks[currentSquare]);

        uint64_t pLegalMovesHV = rookLegalMoves[currentSquare][rookCompressedBlockers];
        uint64_t pLegalMovesDiag = bishopLegalMoves[currentSquare][bishopCompressedBlockers];
        uint64_t pLegalMoves = pLegalMovesHV | pLegalMovesDiag;
        pLegalMoves &= ~board.curState->allPieces[color]; // blockers above assumes we can take any piece, remove captures of our own pieces

        // adjust from pins pins
        if ((pinMask & currentSquareMask) != 0) { // might be pinned
            for (int i = 0; i < 8; i++) { // loop through all directions/axis'
                uint64_t axis = pinMask & directionMasks[i][kingPos];
                if ((axis & currentSquareMask) == 0) {
                    continue; // not on this pin axis
                }

                if (std::popcount(axis & board.curState->allPieces[color]) == 1) { // more than 1 piece means were not actually pinned
                    pLegalMoves &= axis;
                    break; // can only be pinned on a single axis, stop checking if we find one
                }
            }
        }
        
        // adjust for checks
        pLegalMoves &= checkMask;
        pLegalMoves &= moveTypeMask;

        while (pLegalMoves) {
            const int index = tz_count(pLegalMoves);
            pop_lsb(pLegalMoves);

            Move curMove = Move::make<NORMAL>(currentSquare, index);
            moveList.push_back(curMove);
                
        }
    } // bitloop
}

// note: castles are encoded as capturing our own rook
template<MoveFilter moveFilter, Color color>
inline void generateKingMoves(const Board& board, std::vector<Move>& moveList, const uint64_t& checkMask, const int& kingPos) {

    //const uint64_t checkMask = generateCheckMask<color>(board);
    //const int kingPos = tz_count(board.curState->pieces[Piece::KINGS + color]);
    
    // not an assert so we can continue to enumerate even if its wrong (for debug)
    //assert(kingPos != 64);
    if (kingPos == 64) {
        return;
    }

    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t moveTypeMask; // we generate all moves, then filter by this mask
    if constexpr (moveFilter == MoveFilter::ALL) {
        moveTypeMask = ~uint64_t(0);
    }
    else if constexpr (moveFilter == MoveFilter::CAPTURES) {
        moveTypeMask = board.curState->allPieces[enemyColor];
    }
    else if constexpr (moveFilter == MoveFilter::QUIET) {
        moveTypeMask = board.curState->empty;
    }

    // note that since we only have 1 king, no need for a bitloop, just need kingPos

    uint64_t pLegalMoves = kingMasks[kingPos] & (board.curState->empty | board.curState->allPieces[enemyColor]);

    // castles
    if (~checkMask == 0) { // can only castle if not in check
        if (board.curState->canCastle[color]) { // king side
            if ((~board.curState->empty & castleMasks[color]) == 0) { // no pieces in the way
                if (attacksOnSquare<color, false>(board, kingPos - 1) == 0) { // cannot pass through check
                    pLegalMoves |= originalRookSquares[color]; // we encode as taking rook on king side
                }
            }
        }

        if (board.curState->canCastle[color + 2]) { // queen side
            if ((~board.curState->empty & castleMasks[color + 2]) == 0) { // no pieces in the way
                if (attacksOnSquare<color, false>(board, kingPos + 1) == 0) { // cannot pass through check
                    pLegalMoves |= originalRookSquares[color + 2]; // we encode as taking rook on queen side
                }
            }
        }
    }

    pLegalMoves &= moveTypeMask;

    while (pLegalMoves) {
        int index = tz_count(pLegalMoves);
        pop_lsb(pLegalMoves);

        Move curMove = Move::make<NORMAL>(kingPos, index);

        if (maskForPos(index) & board.curState->pieces[Piece::ROOKS + color]) { // taking our own rook -> castle move
            curMove = Move::make<CASTLE>(kingPos, index);
            //std::cout << "made castle move" << std::endl;

            int pos = 0; //tz_count(maskForPos(index) & board.curState->pieces[Piece::ROOKS + color]);
            assert(index == 56 || index == 7 || index == 63 || index == 0);
            if (index == 56) {
                pos = 1;
            } else if (index == 7) {
                pos = 2;
            } else if (index == 63) {
                pos = 3;
            }

            index = tz_count(castleSquares[pos]); // get the actual square the king will be on
        }

        // filter out moves that leave the king in check
        if (attacksOnSquare<color, true>(board, index) != 0) {
            continue;
        }

        moveList.push_back(curMove);
            
    }
}

#endif