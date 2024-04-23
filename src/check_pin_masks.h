#ifndef CHECK_PIN_MASKS_H
#define CHECK_PIN_MASKS_H

#include <cstdint>
#include <tuple>
#include "board.h"
#include "constants.h"

template <Color color>
uint64_t attacksOnSquare(const Board& board, int pos) {
    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t opPawns, opKnights, opRQ, opBQ, opKing;
    opPawns = board.state.pieces[0 + enemyColor];
    opKnights = board.state.pieces[2 + enemyColor];
    opRQ = opBQ = board.state.pieces[8 + enemyColor];
    opRQ |= board.state.pieces[6 + enemyColor];
    opBQ |= board.state.pieces[4 + enemyColor];
    opKing = board.state.pieces[10 + enemyColor];

    uint64_t blockers = (~board.state.empty) & rookMasks[pos];
    uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[pos]);

    blockers = (~board.state.empty) & bishopMasks[pos];
    uint64_t bishopCompressedBlockers = extract_bits(blockers, bishopMasks[pos]);

    uint64_t kingAttackers = (pawnAttackMasks[color][pos] & opPawns)
        | (knightMasks[pos] & opKnights)
        | (bishopLegalMoves[pos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[pos][rookCompressedBlockers] & opRQ)
        | (kingMasks[pos] & opKing)
        ;

    return kingAttackers;
}

uint64_t attacksOnSquareIgnoreKing(const Board& board, int color, int pos);

uint64_t attacksToKing(const Board& board, int color);
uint64_t attacksToKingXray(const Board& board, int color);

// path from all attacks to king, including the attacking piece (note knights do not have a path as they cannot be blocked)
uint64_t generateCheckMask(const Board& board, int color);

uint64_t generateKingCheckMask(const Board& board, int color);

// path from all attacks to king, including the attacking piece *AFTER REMOVING FRIENDLY PIECES - XRAY* (note knights do not have a path as they cannot be blocked)
uint64_t generatePinMask(const Board& board, int color);

#endif