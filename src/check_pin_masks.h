#ifndef CHECK_PIN_MASKS_H
#define CHECK_PIN_MASKS_H

#include <cstdint>
// #include <tuple>
#include "board.h"
#include "precompute_masks.h"
#include "types.h"

template <Color color, bool ignoreKing>
uint64_t attacksOnSquare(const Board& board, int pos) {
    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t opPawns, opKnights, opRQ, opBQ, opKing;
    opPawns = board.getBB(PAWNS + static_cast<int>(enemyColor)); // board.curState->pieces[0 + static_cast<int>(enemyColor)];
    opKnights = board.getBB(KNIGHTS + static_cast<int>(enemyColor)); // board.curState->pieces[2 + enemyColor];
    opRQ = opBQ = board.getBB(QUEENS + static_cast<int>(enemyColor)); // board.curState->pieces[8 + enemyColor];
    opRQ |= board.getBB(ROOKS + static_cast<int>(enemyColor)); // board.curState->pieces[6 + enemyColor];
    opBQ |= board.getBB(BISHOPS + static_cast<int>(enemyColor)); // board.curState->pieces[4 + enemyColor];
    opKing = board.getBB(KINGS + static_cast<int>(enemyColor)); // board.curState->pieces[10 + enemyColor];

    uint64_t blockers;

    if constexpr (!ignoreKing) {
        blockers = (board.getOccupied()) & Bitboard::rookMasks[pos];
    } else {
        blockers = ((board.getOccupied()) & ~board.getBB(KINGS + static_cast<int>(color))) & Bitboard::rookMasks[pos];
    }

    uint64_t rookCompressedBlockers = extract_bits(blockers, Bitboard::rookMasks[pos]);

    if constexpr (!ignoreKing) {
        blockers = (board.getOccupied()) & Bitboard::bishopMasks[pos];
    } else {
        blockers = ((board.getOccupied()) & ~board.getBB(KINGS + static_cast<int>(color))) & Bitboard::bishopMasks[pos];
    }

    uint64_t bishopCompressedBlockers = extract_bits(blockers, Bitboard::bishopMasks[pos]);

    uint64_t kingAttackers = (Bitboard::pawnAttackMasks[color][pos] & opPawns)
        | (Bitboard::knightMasks[pos] & opKnights)
        | (Bitboard::bishopLegalMoves[pos][bishopCompressedBlockers] & opBQ)
        | (Bitboard::rookLegalMoves[pos][rookCompressedBlockers] & opRQ)
        | (Bitboard::kingMasks[pos] & opKing);

    return kingAttackers;
}

template <Color color, bool xray>
uint64_t attacksToKing(const Board& board) {
    constexpr Color enemyColor = static_cast<Color>(!color);

    // int kingPos = tz_count(board.curState->pieces[Piece::KINGS + color]); //board.getPiecePos(10 + color);
    int kingPos = board.kingPos<color>();

    uint64_t opPawns, opKnights, opRQ, opBQ;
    opPawns = board.getBB(PAWNS + static_cast<int>(enemyColor)); // board.curState->pieces[0 + enemyColor];
    opKnights = board.getBB(KNIGHTS + static_cast<int>(enemyColor)); // board.curState->pieces[2 + enemyColor];
    opRQ = opBQ = board.getBB(QUEENS + static_cast<int>(enemyColor)); // board.curState->pieces[8 + enemyColor];
    opRQ |= board.getBB(ROOKS + static_cast<int>(enemyColor)); // board.curState->pieces[6 + enemyColor];
    opBQ |= board.getBB(BISHOPS + static_cast<int>(enemyColor)); // board.curState->pieces[4 + enemyColor];
    // opKing = board.getBB(KINGS + static_cast<int>(enemyColor));//board.curState->pieces[10 + enemyColor];

    uint64_t blockers;

    if constexpr (!xray) {
        blockers = (board.getOccupied()) & Bitboard::rookMasks[kingPos];
    } else {
        blockers = (board.getAll<enemyColor>()) & Bitboard::rookMasks[kingPos];
    }

    uint64_t rookCompressedBlockers = extract_bits(blockers, Bitboard::rookMasks[kingPos]);

    if constexpr (!xray) {
        blockers = (board.getOccupied()) & Bitboard::bishopMasks[kingPos];
    } else {
        blockers = (board.getAll<enemyColor>()) & Bitboard::bishopMasks[kingPos];
    }

    uint64_t bishopCompressedBlockers = extract_bits(blockers, Bitboard::bishopMasks[kingPos]);

    uint64_t kingAttackers = (Bitboard::pawnAttackMasks[color][kingPos] & opPawns)
        | (Bitboard::knightMasks[kingPos] & opKnights)
        | (Bitboard::bishopLegalMoves[kingPos][bishopCompressedBlockers] & opBQ)
        | (Bitboard::rookLegalMoves[kingPos][rookCompressedBlockers] & opRQ);

    return kingAttackers;
}

// path from all attacks to king, including the attacking piece (note knights do not have a path as they cannot be blocked)
template <Color color>
uint64_t generateCheckMask(const Board& board) {
    int kingPos = board.kingPos<color>(); // tz_count(board.curState->pieces[Piece::KINGS + color]);//board.getPiecePos(10 + color);

    uint64_t kingAttackers = attacksToKing<color, false>(board);

    // mask should be all 1s if king is not in check
    if (kingAttackers == 0) {
        return ~kingAttackers;
    }

    uint64_t blockersCompressed = extract_bits(kingAttackers, Bitboard::rookMasks[kingPos]);
    uint64_t checkMask = Bitboard::checkMasksHV[kingPos][blockersCompressed];

    blockersCompressed = extract_bits(kingAttackers, Bitboard::bishopMasks[kingPos]);
    checkMask |= Bitboard::checkMasksDiag[kingPos][blockersCompressed];

    checkMask |= kingAttackers; // make sure we include knights

    return checkMask;
}

// path from all attacks to king, including the attacking piece *AFTER REMOVING FRIENDLY PIECES - XRAY* (note knights do not have a path as they cannot be blocked)
template <Color color>
uint64_t generatePinMask(const Board& board) {
    int kingPos = board.kingPos<color>(); // tz_count(board.curState->pieces[Piece::KINGS + color]);//board.getPiecePos(10 + color);

    uint64_t kingAttackers = attacksToKing<color, true>(board);

    uint64_t blockersCompressed = extract_bits(kingAttackers, Bitboard::rookMasks[kingPos]);
    uint64_t pinMask = Bitboard::checkMasksHV[kingPos][blockersCompressed];

    blockersCompressed = extract_bits(kingAttackers, Bitboard::bishopMasks[kingPos]);
    pinMask |= Bitboard::checkMasksDiag[kingPos][blockersCompressed];

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
