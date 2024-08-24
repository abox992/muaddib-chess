#ifndef CHECK_PIN_MASKS_H
#define CHECK_PIN_MASKS_H

#include <cstdint>
//#include <tuple>
#include "board.h"
#include "types.h"
#include "precompute_masks.h"

template <Color color, bool ignoreKing>
uint64_t attacksOnSquare(const Board& board, int pos) {
    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t opPawns, opKnights, opRQ, opBQ, opKing;
    opPawns = board.getBB(PAWNS + static_cast<int>(enemyColor));//board.curState->pieces[0 + static_cast<int>(enemyColor)];
    opKnights = board.getBB(KNIGHTS + static_cast<int>(enemyColor));//board.curState->pieces[2 + enemyColor];
    opRQ = opBQ = board.getBB(QUEENS + static_cast<int>(enemyColor));//board.curState->pieces[8 + enemyColor];
    opRQ |= board.getBB(ROOKS + static_cast<int>(enemyColor));//board.curState->pieces[6 + enemyColor];
    opBQ |= board.getBB(BISHOPS + static_cast<int>(enemyColor));//board.curState->pieces[4 + enemyColor];
    opKing = board.getBB(KINGS + static_cast<int>(enemyColor));//board.curState->pieces[10 + enemyColor];

    uint64_t blockers;

    if constexpr (!ignoreKing) {
        blockers = (board.getOccupied()) & rookMasks[pos];
    } else {
        blockers = ((board.getOccupied()) & ~board.getBB(KINGS + static_cast<int>(color))) & rookMasks[pos];
    }

    uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[pos]);

    if constexpr (!ignoreKing) {
        blockers = (board.getOccupied()) & bishopMasks[pos];
    } else {
        blockers = ((board.getOccupied()) & ~board.getBB(KINGS + static_cast<int>(color))) & bishopMasks[pos];
    }

    uint64_t bishopCompressedBlockers = extract_bits(blockers, bishopMasks[pos]);

    uint64_t kingAttackers = (pawnAttackMasks[color][pos] & opPawns)
        | (knightMasks[pos] & opKnights)
        | (bishopLegalMoves[pos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[pos][rookCompressedBlockers] & opRQ)
        | (kingMasks[pos] & opKing)
        ;

    return kingAttackers;
}

template<Color color, bool xray>
uint64_t attacksToKing(const Board& board) {
    constexpr Color enemyColor = static_cast<Color>(!color);

    //int kingPos = tz_count(board.curState->pieces[Piece::KINGS + color]); //board.getPiecePos(10 + color);
    int kingPos = board.kingPos<color>();

    uint64_t opPawns, opKnights, opRQ, opBQ;
    opPawns = board.getBB(PAWNS + static_cast<int>(enemyColor));//board.curState->pieces[0 + enemyColor];
    opKnights = board.getBB(KNIGHTS + static_cast<int>(enemyColor));//board.curState->pieces[2 + enemyColor];
    opRQ = opBQ = board.getBB(QUEENS + static_cast<int>(enemyColor));//board.curState->pieces[8 + enemyColor];
    opRQ |= board.getBB(ROOKS + static_cast<int>(enemyColor));//board.curState->pieces[6 + enemyColor];
    opBQ |= board.getBB(BISHOPS + static_cast<int>(enemyColor));//board.curState->pieces[4 + enemyColor];
    //opKing = board.getBB(KINGS + static_cast<int>(enemyColor));//board.curState->pieces[10 + enemyColor];

    uint64_t blockers;

    if constexpr (!xray) {
        blockers = (board.getOccupied()) & rookMasks[kingPos];
    } else {
        blockers = (board.getAll<enemyColor>()) & rookMasks[kingPos];
    }

    uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[kingPos]);

    if constexpr (!xray) {
        blockers = (board.getOccupied()) & bishopMasks[kingPos];
    } else {
        blockers = (board.getAll<enemyColor>()) & bishopMasks[kingPos];
    }

    uint64_t bishopCompressedBlockers = extract_bits(blockers, bishopMasks[kingPos]);

    uint64_t kingAttackers = (pawnAttackMasks[color][kingPos] & opPawns)
        | (knightMasks[kingPos] & opKnights)
        | (bishopLegalMoves[kingPos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[kingPos][rookCompressedBlockers] & opRQ)
        ;

    return kingAttackers;
}

// path from all attacks to king, including the attacking piece (note knights do not have a path as they cannot be blocked)
template<Color color>
uint64_t generateCheckMask(const Board& board) {
    int kingPos = board.kingPos<color>();//tz_count(board.curState->pieces[Piece::KINGS + color]);//board.getPiecePos(10 + color);

    uint64_t kingAttackers = attacksToKing<color, false>(board);

    // mask should be all 1s if king is not in check
    if (kingAttackers == 0) {
        return ~kingAttackers;
    }

    uint64_t blockersCompressed = extract_bits(kingAttackers, rookMasks[kingPos]);
    uint64_t checkMask = checkMasksHV[kingPos][blockersCompressed];

    blockersCompressed = extract_bits(kingAttackers, bishopMasks[kingPos]);
    checkMask |= checkMasksDiag[kingPos][blockersCompressed];

    checkMask |= kingAttackers; // make sure we include knights

    return checkMask;
}

// path from all attacks to king, including the attacking piece *AFTER REMOVING FRIENDLY PIECES - XRAY* (note knights do not have a path as they cannot be blocked)
template<Color color>
uint64_t generatePinMask(const Board& board) {
    int kingPos = board.kingPos<color>();//tz_count(board.curState->pieces[Piece::KINGS + color]);//board.getPiecePos(10 + color);

    uint64_t kingAttackers = attacksToKing<color, true>(board);

    uint64_t blockersCompressed = extract_bits(kingAttackers, rookMasks[kingPos]);
    uint64_t pinMask = checkMasksHV[kingPos][blockersCompressed];

    blockersCompressed = extract_bits(kingAttackers, bishopMasks[kingPos]);
    pinMask |= checkMasksDiag[kingPos][blockersCompressed];

    // for (int i = 0; i < 8; i++) { // loop through all directions/axis'
    //     uint64_t axis = pinMask & directionMasks[i][kingPos];

    //     if (std::popcount(axis & board.curState->allPieces[color]) == 1) { // more than 1 piece would mean were not actually pinned
    //         pinMask &= ~axis;
    //     }
    // }

    // if (pinMask == 0) {
    //     return ~pinMask;
    // }

    return pinMask;
}

#endif
