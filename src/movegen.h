#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <cstdint>

#include "bit_manip.h"
#include "bitboard.h"
#include "board.h"
#include "move.h"
#include "types.h"

// move list to add moves to, color to gen moves for (0 for white 1 for black),
// returns movecount
template<GenType gt, Color color>
int generateAllMoves(const Board& board, Move* moveList) {
    // make sure movelist is not resizing
    // assert(moveList.capacity() == 256);

    int attackersCount = std::popcount(Bitboard::attacksToKing<color, false>(board));

    const uint64_t checkMask = Bitboard::generateCheckMask<color>(board);
    const uint64_t pinMask   = Bitboard::generatePinMask<color>(board);
    const int      kingPos   = board.kingPos<color>();

    constexpr Color enemyColor = static_cast<Color>(!color);

    int count = 0;

    uint64_t target;  // set the target according to gentype - this filters the moves
    if constexpr (gt == ALL) {
        target = (board.getEmpty() | board.getAll<enemyColor>());
    } else if constexpr (gt == CAPTURES) {
        target = board.getAll<enemyColor>();
    } else if constexpr (gt == QUIET) {
        target = board.getEmpty();
    }

    uint64_t kingTarget = target;

    target &= checkMask;

    // if king is in double-check, only need to enumerate king moves
    if (attackersCount < 2) [[likely]] {
        count += generatePawnMoves<color>(board, moveList + count, target, pinMask, kingPos);
        count += generateMoves<KNIGHTS, color>(board, moveList + count, target, pinMask, kingPos);
        count += generateMoves<BISHOPS, color>(board, moveList + count, target, pinMask, kingPos);
        count += generateMoves<ROOKS, color>(board, moveList + count, target, pinMask, kingPos);
        count += generateMoves<QUEENS, color>(board, moveList + count, target, pinMask, kingPos);
    }
    count += generateKingMoves<gt, color>(board, moveList + count, kingTarget, checkMask, kingPos);

    return count;
}

template<Color color>
inline int generatePawnMoves(const Board& board, Move* moveList, const uint64_t target, const uint64_t pinMask,
                             const int kingPos) {
    int count = 0;

    constexpr Color enemyColor = static_cast<Color>(!color);

    // uint64_t bitboard = board.getBB(PAWNS + static_cast<int>(color));
    uint64_t bitboard = board.getBB(color, PAWNS);
    while (bitboard) {  // bitloop
        const int currentSquare = tz_count(bitboard);
        pop_lsb(bitboard);  // we only need to get current square, can do this now
                            // dont have to wait for end of loop

        const uint64_t currentSquareMask = maskForPos(currentSquare);

        uint64_t initialMoveMask   = Bitboard::getPawnMovesBB<color>(currentSquare);
        uint64_t initialAttackMask = Bitboard::getPawnAttacksBB<color>(currentSquare);

        uint64_t pLegalMoves   = initialMoveMask & board.getEmpty();
        uint64_t pLegalAttacks = initialAttackMask & board.getAll<enemyColor>();

        // make sure were not jumping over a piece on initial double move
        if (std::popcount(initialMoveMask) == 2) {
            /*constexpr int offset = color == Color::WHITE ? 8 : -8;*/
            constexpr int offset = Bitboard::pawnPush<color>();
            if ((maskForPos(currentSquare + offset) & board.getOccupied()) != 0) {
                pLegalMoves = 0;
            }
        }

        pLegalMoves |= pLegalAttacks;

        pLegalMoves &= target;

        // resolve enpassant
        uint64_t enpassantSquareMask = maskForPos(board.enpassantPos()) & initialAttackMask;
        if (board.enpassantPos() && enpassantSquareMask) [[unlikely]] {
            uint64_t occupied = (board.getOccupied() & ~currentSquareMask
                                 & ~maskForPos(board.enpassantPos() - Bitboard::pawnPush<color>()))
                              | enpassantSquareMask;

            if (!(Bitboard::rookLegalMoves[kingPos][extract_bits(occupied, Bitboard::rookMasks[kingPos])]
                  & (board.getBB(enemyColor, ROOKS, QUEENS)))
                && !(Bitboard::bishopLegalMoves[kingPos][extract_bits(occupied, Bitboard::bishopMasks[kingPos])]
                     & (board.getBB(enemyColor, BISHOPS, QUEENS)))) {
                pLegalMoves |= enpassantSquareMask;
            }
        }

        uint64_t pinHV   = pinMask & Bitboard::rookMasks[kingPos];
        uint64_t pinDiag = pinMask & Bitboard::bishopMasks[kingPos];

        if (currentSquareMask & pinHV) {
            pLegalMoves &= Bitboard::rookMasks[currentSquare];
            pLegalMoves &= pinHV;
        } else if (currentSquareMask & pinDiag) {
            pLegalMoves &= Bitboard::bishopMasks[currentSquare];
            pLegalMoves &= pinDiag;
        }

        // save the number of moves
        count += std::popcount(pLegalMoves);

        // moves are now all legal, add them to the list
        while (pLegalMoves) {
            const int index = tz_count(pLegalMoves);
            pop_lsb(pLegalMoves);

            Move curMove = Move::make<NORMAL>(currentSquare, index);

            if (board.enpassantPos() > 0 && index == board.enpassantPos()) {  // enpassant
                curMove = Move::make<EN_PASSANT>(currentSquare, index);
            } else if (Bitboard::rankOf(index) == 0 || Bitboard::rankOf(index) == 7) {  // promotion
                count += 3;  // adjust count for promotions accordingly

                for (int i = 0; i < 4; i++) {  // add all 4 promotion options
                    curMove     = Move::make<PROMOTION>(currentSquare, index, static_cast<PromoPiece>(i));
                    *moveList++ = curMove;
                }

                continue;
            }

            *moveList++ = curMove;  // moveList.push_back(curMove);
        }

    }  // bitloop

    return count;
}

template<PieceType pt, Color color>
int generateMoves(const Board& board, Move* moveList, const uint64_t target, const uint64_t pinMask,
                  const int kingPos) {
    static_assert(pt != PieceType::PAWNS && pt != PieceType::KINGS);

    int count = 0;

    uint64_t bitboard = board.getBB(color, pt);
    while (bitboard) {  // bitloop
        const int currentSquare = tz_count(bitboard);
        pop_lsb(bitboard);
        const uint64_t currentSquareMask = maskForPos(currentSquare);

        uint64_t pLegalMoves = Bitboard::getMovesBB<pt>(board, currentSquare) & target;

        uint64_t pinHV   = pinMask & Bitboard::rookMasks[kingPos];
        uint64_t pinDiag = pinMask & Bitboard::bishopMasks[kingPos];

        if (currentSquareMask & pinHV) {
            pLegalMoves &= Bitboard::rookMasks[currentSquare];
            pLegalMoves &= pinHV;
        } else if (currentSquareMask & pinDiag) {
            pLegalMoves &= Bitboard::bishopMasks[currentSquare];
            pLegalMoves &= pinDiag;
        }

        // save the number of moves
        count += std::popcount(pLegalMoves);

        while (pLegalMoves) {
            const int index = tz_count(pLegalMoves);
            pop_lsb(pLegalMoves);

            Move curMove = Move::make<NORMAL>(currentSquare, index);
            *moveList++  = curMove;
        }
    }  // bitloop

    return count;
}

// note: castles are encoded as capturing our own rook
template<GenType gt, Color color>
inline int generateKingMoves(const Board& board, Move* moveList, const uint64_t target, const uint64_t checkMask,
                             const int kingPos) {
    int count = 0;

    // note that since we only have 1 king, no need for a bitloop, just need kingPos
    uint64_t pLegalMoves = Bitboard::kingMasks[kingPos] & target;

    // castles
    if (~checkMask == 0 && gt != CAPTURES) {  // can only castle if not in check
        for (auto side : {0 /* king side */, 2 /* queen side */}) {
            if (board.getCastle(color + side)) {
                // no pieces in the way
                if ((board.getOccupied() & Bitboard::castleMasks[color + side]) == 0) {
                    // cannot pass through check
                    if (Bitboard::attacksOnSquare<color, false>(board, kingPos - 1 + side) == 0) {
                        // we encode as taking our own rook on respective side
                        pLegalMoves |= Bitboard::originalRookSquares[color + side];
                    }
                }
            }
        }
    }

    while (pLegalMoves) {
        int index = tz_count(pLegalMoves);
        pop_lsb(pLegalMoves);

        Move curMove = Move::make<NORMAL>(kingPos, index);

        if (maskForPos(index) & board.getBB(color, ROOKS)) {  // taking our own rook -> castle move
            curMove = Move::make<CASTLE>(kingPos, index);

            int pos = Bitboard::rookPosToIndex(index);

            index = tz_count(Bitboard::castledKingSquares[pos]);  // get the actual square the king will be on
        }

        // filter out moves that leave the king in check
        if (Bitboard::attacksOnSquare<color, true>(board, index) != 0) {
            continue;
        }

        *moveList++ = curMove;
        count++;
    }

    return count;
}

#endif
