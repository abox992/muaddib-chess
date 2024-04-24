#include "precompute_masks.h"
#include "board.h"
#include "helpers.h"
#include "bit_manip.h"
#include "constants.h"
#include <tuple>
#include <cassert>
#include <iostream>

// U64 CBoard::attacksToKing(enumSquare squareOfKing, enumColor colorOfKing) {
//    U64 opPawns, opKnights, opRQ, opBQ;
//    opPawns     = pieceBB[nBlackPawn   - colorOfKing];
//    opKnights   = pieceBB[nBlackKnight - colorOfKing];
//    opRQ = opBQ = pieceBB[nBlackQueen  - colorOfKing];
//    opRQ       |= pieceBB[nBlackRook   - colorOfKing];
//    opBQ       |= pieceBB[nBlackBishop - colorOfKing];
//    return (arrPawnAttacks[colorOfKing][squareOfKing] & opPawns)
//         | (arrKnightAttacks[squareOfKing]            & opKnights)
//         | (bishopAttacks (occupiedBB, squareOfKing)  & opBQ)
//         | (rookAttacks   (occupiedBB, squareOfKing)  & opRQ)
//         ;
// }

uint64_t attacksOnSquareIgnoreKing(const Board& board, int color, int pos) {
    //int enemyOffset = color == 0 ? 0 : 6;
    int enemyColor = color == 0 ? 1 : 0;

    uint64_t opPawns, opKnights, opRQ, opBQ, opKing;
    opPawns = board.curState->pieces[0 + enemyColor];
    opKnights = board.curState->pieces[2 + enemyColor];
    opRQ = opBQ = board.curState->pieces[8 + enemyColor];
    opRQ |= board.curState->pieces[6 + enemyColor];
    opBQ |= board.curState->pieces[4 + enemyColor];
    opKing = board.curState->pieces[10 + enemyColor];

    uint64_t blockers = ((~board.curState->empty) & ~board.curState->pieces[10 + color]) & rookMasks[pos];
    uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[pos]);

    blockers = ((~board.curState->empty) & ~board.curState->pieces[10 + color]) & bishopMasks[pos];
    uint64_t bishopCompressedBlockers = extract_bits(blockers, bishopMasks[pos]);

    uint64_t kingAttackers = (pawnAttackMasks[color][pos] & opPawns)
        | (knightMasks[pos] & opKnights)
        | (bishopLegalMoves[pos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[pos][rookCompressedBlockers] & opRQ)
        | (kingMasks[pos] & opKing)
        ;

    return kingAttackers;
}

uint64_t attacksToKing(const Board& board, int color) {
    //int enemyOffset = color == 0 ? 6 : 0;
    //int myColorOffset = color == 0 ? 0 : 6;
    int enemyColor = color == 0 ? 1 : 0;

    int kingPos = squareOf(board.curState->pieces[Piece::KINGS + color]); //board.getPiecePos(10 + color);

    uint64_t opPawns, opKnights, opRQ, opBQ;
    opPawns = board.curState->pieces[0 + enemyColor];
    opKnights = board.curState->pieces[2 + enemyColor];
    opRQ = opBQ = board.curState->pieces[8 + enemyColor];
    opRQ |= board.curState->pieces[6 + enemyColor];
    opBQ |= board.curState->pieces[4 + enemyColor];

    uint64_t blockers = (~board.curState->empty) & rookMasks[kingPos];
    uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[kingPos]);

    blockers = (~board.curState->empty) & bishopMasks[kingPos];
    uint64_t bishopCompressedBlockers = extract_bits(blockers, bishopMasks[kingPos]);

    uint64_t kingAttackers = (pawnAttackMasks[color][kingPos] & opPawns)
        | (knightMasks[kingPos] & opKnights)
        | (bishopLegalMoves[kingPos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[kingPos][rookCompressedBlockers] & opRQ)
        ;

    return kingAttackers;
}

uint64_t attacksToKingXray(const Board& board, int color) {
    //int enemyOffset = color == 0 ? 6 : 0;
    //int myColorOffset = color == 0 ? 0 : 6;
    int enemyColor = color == 0 ? 1 : 0;

    int kingPos = squareOf(board.curState->pieces[Piece::KINGS + color]); //board.getPiecePos(10 + color);

    uint64_t opPawns, opKnights, opRQ, opBQ;
    opPawns = board.curState->pieces[0 + enemyColor];
    opKnights = board.curState->pieces[2 + enemyColor];
    opRQ = opBQ = board.curState->pieces[8 + enemyColor];
    opRQ |= board.curState->pieces[6 + enemyColor];
    opBQ |= board.curState->pieces[4 + enemyColor];

    uint64_t blockers = (board.curState->allPieces[enemyColor]) & rookMasks[kingPos];
    uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[kingPos]);

    blockers = (board.curState->allPieces[enemyColor]) & bishopMasks[kingPos];
    uint64_t bishopCompressedBlockers = extract_bits(blockers, bishopMasks[kingPos]);

    uint64_t kingAttackers = (pawnAttackMasks[color][kingPos] & opPawns)
        | (knightMasks[kingPos] & opKnights)
        | (bishopLegalMoves[kingPos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[kingPos][rookCompressedBlockers] & opRQ)
        ;

    return kingAttackers;
}

uint64_t generateCheckMask(const Board& board, int color) {

    int kingPos = squareOf(board.curState->pieces[Piece::KINGS + color]);//board.getPiecePos(10 + color);

    uint64_t kingAttackers = attacksToKing(board, color);

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

uint64_t generateKingCheckMask(const Board& board, int color) {

    int kingPos = squareOf(board.curState->pieces[Piece::KINGS + color]);//board.getPiecePos(10 + color);

    uint64_t kingAttackers = attacksToKing(board, color);

    uint64_t blockersCompressed = extract_bits(kingAttackers, rookMasks[kingPos]);
    uint64_t checkMask = kingCheckMasksHV[kingPos][blockersCompressed];

    blockersCompressed = extract_bits(kingAttackers, bishopMasks[kingPos]);
    checkMask |= kingCheckMasksDiag[kingPos][blockersCompressed];

    checkMask |= kingAttackers; // make sure we include knights

    return checkMask;
}

uint64_t generatePinMask(const Board& board, int color) {
    int kingPos = squareOf(board.curState->pieces[Piece::KINGS + color]);//board.getPiecePos(10 + color);

    uint64_t kingAttackers = attacksToKingXray(board, color);

    uint64_t blockersCompressed = extract_bits(kingAttackers, rookMasks[kingPos]);
    uint64_t pinMask = checkMasksHV[kingPos][blockersCompressed];

    blockersCompressed = extract_bits(kingAttackers, bishopMasks[kingPos]);
    pinMask |= checkMasksDiag[kingPos][blockersCompressed];

    return pinMask;
}