#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <cstdint>

#include "bit_manip.h"
#include "board.h"
// #include "check_pin_masks.h"
#include "move.h"
#include "bitboard.h"
#include "types.h"

// move list to add moves to, color to gen moves for (0 for white 1 for black),
// returns movecount
template <GenType gt, Color color>
int generateAllMoves(const Board& board, Move* moveList) {
    // make sure movelist is not resizing
    // assert(moveList.capacity() == 256);

    int attackersCount = std::popcount(Bitboard::attacksToKing<color, false>(board));

    const uint64_t checkMask = Bitboard::generateCheckMask<color>(board);
    const uint64_t pinMask = Bitboard::generatePinMask<color>(board);
    const int kingPos = tz_count(board.getBB(PieceType::KINGS + static_cast<int>(color)));


    constexpr Color enemyColor = static_cast<Color>(!color);

    int count = 0;

    uint64_t target; // set the target according to gentype - this filters the moves
    if constexpr (gt == ALL) {
        target = (board.getEmpty() | board.getAll<enemyColor>());
    } else if constexpr (gt == CAPTURES) {
        target = board.getAll<enemyColor>();
    } else if constexpr (gt == QUIET) {
        target = board.getEmpty();
    }

    target &= checkMask;

    // if king is in double-check, only need to enumerate king moves
    if (attackersCount < 2) {
        /*count += generatePawnMoves<moveFilter, color>(board, moveList + count,*/
        /*    checkMask, pinMask, kingPos);*/
        /*count += generateKnightMoves<moveFilter, color>(*/
        /*    board, moveList + count, checkMask, pinMask, kingPos);*/
        /*count += generateBishopMoves<moveFilter, color>(*/
        /*    board, moveList + count, checkMask, pinMask, kingPos);*/
        /*count += generateRookMoves<moveFilter, color>(board, moveList + count,*/
        /*    checkMask, pinMask, kingPos);*/
        /*count += generateQueenMoves<moveFilter, color>(board, moveList + count,*/
        /*    checkMask, pinMask, kingPos);*/
        count += generatePawnMoves<color>(board, moveList, target, pinMask, kingPos);
        count += generateMoves<KNIGHTS, color>(board, moveList + count, target, pinMask, kingPos);
        count += generateMoves<BISHOPS, color>(board, moveList + count, target, pinMask, kingPos);
        count += generateMoves<ROOKS, color>(board, moveList + count, target, pinMask, kingPos);
        count += generateMoves<QUEENS, color>(board, moveList + count, target, pinMask, kingPos);
    }
    count += generateKingMoves<gt, color>(board, moveList + count, checkMask, kingPos);

    // make sure movelist is not resizing
    // assert(moveList.capacity() == 256 && moveList.size() < 256);
    return count;
}

template <Color color>
inline int generatePawnMoves(const Board& board, Move* moveList, const uint64_t& target, const uint64_t& pinMask, const int& kingPos) {
    int count = 0;

    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t bitboard = board.getBB(PAWNS + static_cast<int>(color));
    while (bitboard) { // bitloop
        const int currentSquare = tz_count(bitboard);
        pop_lsb(bitboard); // we only need to get current square, can do this now
                           // dont have to wait for end of loop

        const uint64_t currentSquareMask = maskForPos(currentSquare);

        uint64_t initialMoveMask = Bitboard::getPawnMovesBB<color>(currentSquare);
        uint64_t initialAttackMask = Bitboard::getPawnAttacksBB<color>(currentSquare);

        uint64_t pLegalMoves = initialMoveMask & board.getEmpty();
        uint64_t pLegalAttacks = initialAttackMask & board.getAll<enemyColor>();

        // make sure were not jumping over a piece on initial double move
        if (std::popcount(initialMoveMask) == 2) {
            constexpr int offset = color == Color::WHITE ? 8 : -8;
            if ((maskForPos(currentSquare + offset) & board.getOccupied()) != 0) {
                pLegalMoves = 0;
            }
        }

        pLegalMoves |= pLegalAttacks;

        pLegalMoves &= target;

        uint64_t enpassantSquareMask = maskForPos(board.enpassantPos());
        // en passant is special - have to play it out and check if we are still in
        // check manually, does not get pruned away by masks
        if ((initialAttackMask & enpassantSquareMask) && board.enpassantPos() > 0) {
            pLegalMoves |= (initialAttackMask & enpassantSquareMask); // a

            constexpr int offset = color == Color::WHITE ? -8 : 8;

            // Borrowed from attacksToSquare(), we are just augmenting the position initially
            uint64_t adjustedPieces = (board.getOccupied());
            adjustedPieces &= ~currentSquareMask;
            adjustedPieces &= ~maskForPos(board.enpassantPos() + offset);
            adjustedPieces |= enpassantSquareMask;

            uint64_t opPawns, opKnights, opRQ, opBQ;
            opPawns = board.getBB(0 + enemyColor);
            opKnights = board.getBB(2 + enemyColor);
            opRQ = opBQ = board.getBB(8 + enemyColor);
            opRQ |= board.getBB(6 + enemyColor);
            opBQ |= board.getBB(4 + enemyColor);

            opPawns &= ~maskForPos(board.enpassantPos() + offset);

            uint64_t blockers = adjustedPieces & Bitboard::rookMasks[kingPos];
            uint64_t rookCompressedBlockers = extract_bits(blockers, Bitboard::rookMasks[kingPos]);

            blockers = adjustedPieces & Bitboard::bishopMasks[kingPos];
            uint64_t bishopCompressedBlockers = extract_bits(blockers, Bitboard::bishopMasks[kingPos]);

            uint64_t kingAttackers = (Bitboard::pawnAttackMasks[color][kingPos] & opPawns) | (Bitboard::knightMasks[kingPos] & opKnights) | (Bitboard::bishopLegalMoves[kingPos][bishopCompressedBlockers] & opBQ) | (Bitboard::rookLegalMoves[kingPos][rookCompressedBlockers] & opRQ);

            // remove if still in check
            if (kingAttackers != 0) {
                pLegalMoves &= ~(initialAttackMask & enpassantSquareMask);
            }
        }

        // adjust from pins pins
        if ((pinMask & currentSquareMask) != 0) { // might be pinned
            for (int i = 0; i < 8; i++) { // loop through all directions
                uint64_t axis = pinMask & Bitboard::directionMasks[i][kingPos];
                if ((axis & currentSquareMask) == 0) {
                    continue; // not on this pin axis
                }

                if (std::popcount(axis & board.getAll<color>()) == 1) { // more than 1 piece means were not actually pinned
                    pLegalMoves &= axis;
                    break; // can only be pinned on a single axis, stop checking if we
                           // find one
                }
            }
        }

        // save the number of moves
        count += std::popcount(pLegalMoves);

        // moves are now all legal, add them to the list
        while (
            pLegalMoves) { // for (int index = tz_count(pLegalMoves); pLegalMoves;
                           // pop_lsb(pLegalMoves), index = tz_count(pLegalMoves))
            const int index = tz_count(pLegalMoves);
            pop_lsb(pLegalMoves);

            Move curMove = Move::make<MoveType::NORMAL>(currentSquare, index);

            if (board.enpassantPos() > 0 && index == board.enpassantPos()) { // enpassant
                curMove = Move::make<MoveType::EN_PASSANT>(currentSquare, index);
            } else if (Bitboard::promoSquare[index]) { // promotion
                count += 3; // adjust count for promotions accordingly

                for (int i = 0; i < 4; i++) { // add all 4 promotion options
                    curMove = Move::make<MoveType::PROMOTION>(currentSquare, index,
                        static_cast<PromoPiece>(i));
                    *moveList++ = curMove;
                }

                continue;
            }

            *moveList++ = curMove; // moveList.push_back(curMove);
        }

    } // bitloop

    return count;
}

template <PieceType pt, Color color>
int generateMoves(const Board& board, Move* moveList, const uint64_t& target, const uint64_t& pinMask, const int& kingPos) {
    static_assert(pt != PieceType::PAWNS && pt != PieceType::KINGS);

    int count = 0;

    uint64_t bitboard = board.getBB(pt + static_cast<int>(color));
    while (bitboard) { // bitloop
        const int currentSquare = tz_count(bitboard);
        pop_lsb(bitboard);
        const uint64_t currentSquareMask = maskForPos(currentSquare);

        uint64_t pLegalMoves = Bitboard::getMovesBB<pt>(board, currentSquare) & target;

        // adjust from pins pins
        if ((pinMask & currentSquareMask) != 0) { // might be pinned
            for (int i = 0; i < 8; i++) { // loop through all directions/axis'
                uint64_t axis = pinMask & Bitboard::directionMasks[i][kingPos];
                if ((axis & currentSquareMask) == 0) {
                    continue; // not on this pin axis
                }

                if (std::popcount(axis & board.getAll<color>()) == 1) { // more than 1 piece would mean were not actually pinned
                    pLegalMoves &= axis;
                    break; // can only be pinned on a single axis, stop checking if we
                           // find one
                }
            }
        }

        // save the number of moves
        count += std::popcount(pLegalMoves);

        while (pLegalMoves) {
            const int index = tz_count(pLegalMoves);
            pop_lsb(pLegalMoves);

            Move curMove = Move::make<NORMAL>(currentSquare, index);
            *moveList++ = curMove; // moveList.push_back(curMove);
        }
    } // bitloop

    return count;
}
/*
template <GenType gt, Color color>
inline int generateKnightMoves(const Board& board, Move* moveList,
    const uint64_t& checkMask,
    const uint64_t& pinMask, const int& kingPos) {
    int count = 0;

    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t moveTypeMask; // we generate all moves, then filter by this mask
    if constexpr (gt == ALL) {
        moveTypeMask = ~uint64_t(0);
    } else if constexpr (gt == CAPTURES) {
        moveTypeMask = board.getAll<enemyColor>();
    } else if constexpr (gt == QUIET) {
        moveTypeMask = board.getEmpty();
    }

    uint64_t bitboard = board.getBB(PieceType::KNIGHTS + static_cast<int>(color));
    while (bitboard) { // bitloop
        const int currentSquare = tz_count(bitboard);
        pop_lsb(bitboard);
        const uint64_t currentSquareMask = maskForPos(currentSquare);

        uint64_t pLegalMoves = Bitboard::knightMasks[currentSquare] & (board.getEmpty() | board.getAll<enemyColor>());

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

                if (std::popcount(axis & board.getAll<color>()) == 1) { // more than 1 piece would mean were not actually pinned
                    pLegalMoves &= axis;
                    break; // can only be pinned on a single axis, stop checking if we
                           // find one
                }
            }
        }

        // save the number of moves
        count += std::popcount(pLegalMoves);

        while (pLegalMoves) {
            const int index = tz_count(pLegalMoves);
            pop_lsb(pLegalMoves);

            Move curMove = Move::make<NORMAL>(currentSquare, index);
            *moveList++ = curMove; // moveList.push_back(curMove);
        }
    } // bitloop

    return count;
}

template <MoveFilter moveFilter, Color color>
inline int generateBishopMoves(const Board& board, Move* moveList,
    const uint64_t& checkMask,
    const uint64_t& pinMask, const int& kingPos) {
    int count = 0;

    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t moveTypeMask; // we generate all moves, then filter by this mask
    if constexpr (moveFilter == MoveFilter::ALL) {
        moveTypeMask = ~uint64_t(0);
    } else if constexpr (moveFilter == MoveFilter::CAPTURES) {
        moveTypeMask = board.getAll<enemyColor>();
    } else if constexpr (moveFilter == MoveFilter::QUIET) {
        moveTypeMask = board.getEmpty();
    }

    uint64_t bitboard = board.getBB(Piece::BISHOPS + static_cast<int>(color));
    while (bitboard) { // bitloop
        const int currentSquare = tz_count(bitboard);
        pop_lsb(bitboard);

        const uint64_t currentSquareMask = maskForPos(currentSquare);

        uint64_t blockers = (board.getOccupied()) & Bitboard::bishopMasks[currentSquare];
        uint64_t compressedBlockers = extract_bits(blockers, Bitboard::bishopMasks[currentSquare]);

        uint64_t pLegalMoves = bishopLegalMoves[currentSquare][compressedBlockers];
        pLegalMoves &= ~board.getAll<color>(); // blockers above assumes we can take any
                                               // piece, remove captures of our own pieces

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

                if (std::popcount(axis & board.getAll<color>()) == 1) { // more than 1 piece means were not actually pinned
                    pLegalMoves &= axis;
                    break; // can only be pinned on a single axis, stop checking if we
                           // find one
                }
            }
        }

        // save the number of moves
        count += std::popcount(pLegalMoves);

        while (pLegalMoves) {
            const int index = tz_count(pLegalMoves);
            pop_lsb(pLegalMoves);

            Move curMove = Move::make<NORMAL>(currentSquare, index);
            *moveList++ = curMove; // moveList.push_back(curMove);
        }

    } // bitloop

    return count;
}

template <MoveFilter moveFilter, Color color>
inline int generateRookMoves(const Board& board, Move* moveList,
    const uint64_t& checkMask, const uint64_t& pinMask,
    const int& kingPos) {
    int count = 0;

    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t moveTypeMask; // we generate all moves, then filter by this mask
    if constexpr (moveFilter == MoveFilter::ALL) {
        moveTypeMask = ~uint64_t(0);
    } else if constexpr (moveFilter == MoveFilter::CAPTURES) {
        moveTypeMask = board.getAll<enemyColor>();
    } else if constexpr (moveFilter == MoveFilter::QUIET) {
        moveTypeMask = board.getEmpty();
    }

    uint64_t bitboard = board.getBB(Piece::ROOKS + static_cast<int>(color));
    while (bitboard) { // bitloop
        const int currentSquare = tz_count(bitboard);
        pop_lsb(bitboard);

        const uint64_t currentSquareMask = maskForPos(currentSquare);

        uint64_t blockers = (board.getOccupied()) & Bitboard::rookMasks[currentSquare];
        uint64_t compressedBlockers = extract_bits(blockers, Bitboard::rookMasks[currentSquare]);

        uint64_t pLegalMoves = rookLegalMoves[currentSquare][compressedBlockers];
        pLegalMoves &= ~board.getAll<color>(); // blockers above assumes we can take any
                                               // piece, remove captures of our own pieces

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

                if (std::popcount(axis & board.getAll<color>()) == 1) { // more than 1 piece means were not actually pinned
                    pLegalMoves &= axis;
                    break; // can only be pinned on a single axis, stop checking if we
                           // find one
                }
            }
        }

        // save the number of moves
        count += std::popcount(pLegalMoves);

        while (pLegalMoves) {
            const int index = tz_count(pLegalMoves);
            pop_lsb(pLegalMoves);

            Move curMove = Move::make<NORMAL>(currentSquare, index);
            *moveList++ = curMove; // moveList.push_back(curMove);
        }

    } // bitloop

    return count;
}

template <MoveFilter moveFilter, Color color>
inline int generateQueenMoves(const Board& board, Move* moveList,
    const uint64_t& checkMask,
    const uint64_t& pinMask, const int& kingPos) {
    int count = 0;

    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t moveTypeMask; // we generate all moves, then filter by this mask
    if constexpr (moveFilter == MoveFilter::ALL) {
        moveTypeMask = ~uint64_t(0);
    } else if constexpr (moveFilter == MoveFilter::CAPTURES) {
        moveTypeMask = board.getAll<enemyColor>();
    } else if constexpr (moveFilter == MoveFilter::QUIET) {
        moveTypeMask = board.getEmpty();
    }

    uint64_t bitboard = board.getBB(Piece::QUEENS + static_cast<int>(color));
    while (bitboard) { // bitloop
        const int currentSquare = tz_count(bitboard);
        pop_lsb(bitboard);

        const uint64_t currentSquareMask = maskForPos(currentSquare);

        uint64_t blockers = (board.getOccupied()) & Bitboard::rookMasks[currentSquare];
        uint64_t rookCompressedBlockers = extract_bits(blockers, Bitboard::rookMasks[currentSquare]);

        blockers = (board.getOccupied()) & Bitboard::bishopMasks[currentSquare];
        uint64_t bishopCompressedBlockers = extract_bits(blockers, Bitboard::bishopMasks[currentSquare]);

        uint64_t pLegalMovesHV = rookLegalMoves[currentSquare][rookCompressedBlockers];
        uint64_t pLegalMovesDiag = bishopLegalMoves[currentSquare][bishopCompressedBlockers];
        uint64_t pLegalMoves = pLegalMovesHV | pLegalMovesDiag;
        pLegalMoves &= ~board.getAll<color>(); // blockers above assumes we can take any
                                               // piece, remove captures of our own pieces

        // adjust from pins pins
        if ((pinMask & currentSquareMask) != 0) { // might be pinned
            for (int i = 0; i < 8; i++) { // loop through all directions/axis'
                uint64_t axis = pinMask & directionMasks[i][kingPos];
                if ((axis & currentSquareMask) == 0) {
                    continue; // not on this pin axis
                }

                if (std::popcount(axis & board.getAll<color>()) == 1) { // more than 1 piece means were not actually pinned
                    pLegalMoves &= axis;
                    break; // can only be pinned on a single axis, stop checking if we
                           // find one
                }
            }
        }

        // adjust for checks
        pLegalMoves &= checkMask;
        pLegalMoves &= moveTypeMask;

        // save the number of moves
        count += std::popcount(pLegalMoves);

        while (pLegalMoves) {
            const int index = tz_count(pLegalMoves);
            pop_lsb(pLegalMoves);

            Move curMove = Move::make<NORMAL>(currentSquare, index);
            *moveList++ = curMove; // moveList.push_back(curMove);
        }
    } // bitloop

    return count;
}
*/

// note: castles are encoded as capturing our own rook
template <GenType gt, Color color>
inline int generateKingMoves(const Board& board, Move* moveList, const uint64_t& checkMask, const int& kingPos) {
    int count = 0;

    // not an assert so we can continue to enumerate even if its wrong (for debug)
    // assert(kingPos != 64);
    if (kingPos == 64) {
        return count;
    }

    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t moveTypeMask; // we generate all moves, then filter by this mask
    if constexpr (gt == ALL) {
        moveTypeMask = ~uint64_t(0);
    } else if constexpr (gt == CAPTURES) {
        moveTypeMask = board.getAll<enemyColor>();
    } else if constexpr (gt == QUIET) {
        moveTypeMask = board.getEmpty();
    }

    // note that since we only have 1 king, no need for a bitloop, just need
    // kingPos

    uint64_t pLegalMoves = Bitboard::kingMasks[kingPos] & (board.getEmpty() | board.getAll<enemyColor>());

    // castles
    if (~checkMask == 0) { // can only castle if not in check
        if (board.canCastle(color)) { // king side
            if ((board.getOccupied() & Bitboard::castleMasks[color]) == 0) { // no pieces in the way
                if (Bitboard::attacksOnSquare<color, false>(board, kingPos - 1) == 0) { // cannot pass through check
                    pLegalMoves |= Bitboard::originalRookSquares[color]; // we encode as taking
                                                                         // rook on king side
                }
            }
        }

        if (board.canCastle(color + 2)) { // queen side
            if ((board.getOccupied() & Bitboard::castleMasks[color + 2]) == 0) { // no pieces in the way
                if (Bitboard::attacksOnSquare<color, false>(board, kingPos + 1) == 0) { // cannot pass through check
                    pLegalMoves |= Bitboard::originalRookSquares[color + 2]; // we encode as taking rook on queen side
                }
            }
        }
    }

    pLegalMoves &= moveTypeMask;

    // save the number of moves
    count += std::popcount(pLegalMoves);

    while (pLegalMoves) {
        int index = tz_count(pLegalMoves);
        pop_lsb(pLegalMoves);

        Move curMove = Move::make<NORMAL>(kingPos, index);

        if (maskForPos(index) & board.getBB(PieceType::ROOKS + static_cast<int>(color))) { // taking our own rook -> castle move
            curMove = Move::make<CASTLE>(kingPos, index);
            // std::cout << "made castle move" << std::endl;

            int pos = 0; // tz_count(maskForPos(index) & board.getBB(Piece::ROOKS +
                         // static_cast<int>(color)));
            // assert(index == 56 || index == 7 || index == 63 || index == 0);
            if (index == 56) {
                pos = 1;
            } else if (index == 7) {
                pos = 2;
            } else if (index == 63) {
                pos = 3;
            }

            index = tz_count(Bitboard::castleSquares[pos]); // get the actual square the king will be on
        }

        // filter out moves that leave the king in check
        if (Bitboard::attacksOnSquare<color, true>(board, index) != 0) {
            count--;
            continue;
        }

        *moveList++ = curMove; // moveList.push_back(curMove);
    }

    return count;
}

#endif
