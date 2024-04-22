#include "precompute_masks.h"
#include "board.h"
#include "helpers.h"
#include "bit_manip.h"
#include "constants.h"
#include <tuple>

using namespace std;

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
    opPawns = board.state.pieces[0 + enemyColor];
    opKnights = board.state.pieces[2 + enemyColor];
    opRQ = opBQ = board.state.pieces[8 + enemyColor];
    opRQ |= board.state.pieces[6 + enemyColor];
    opBQ |= board.state.pieces[4 + enemyColor];
    opKing = board.state.pieces[10 + enemyColor];

    uint64_t blockers = ((~board.state.empty) & ~board.state.pieces[10 + color]) & rookMasks[pos];
    uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[pos]);

    blockers = ((~board.state.empty) & ~board.state.pieces[10 + color]) & bishopMasks[pos];
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

    int kingPos = squareOf(board.state.pieces[Piece::KINGS + color]); //board.getPiecePos(10 + color);

    uint64_t opPawns, opKnights, opRQ, opBQ;
    opPawns = board.state.pieces[0 + enemyColor];
    opKnights = board.state.pieces[2 + enemyColor];
    opRQ = opBQ = board.state.pieces[8 + enemyColor];
    opRQ |= board.state.pieces[6 + enemyColor];
    opBQ |= board.state.pieces[4 + enemyColor];

    uint64_t blockers = (~board.state.empty) & rookMasks[kingPos];
    uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[kingPos]);

    blockers = (~board.state.empty) & bishopMasks[kingPos];
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

    int kingPos = squareOf(board.state.pieces[Piece::KINGS + color]); //board.getPiecePos(10 + color);

    uint64_t opPawns, opKnights, opRQ, opBQ;
    opPawns = board.state.pieces[0 + enemyColor];
    opKnights = board.state.pieces[2 + enemyColor];
    opRQ = opBQ = board.state.pieces[8 + enemyColor];
    opRQ |= board.state.pieces[6 + enemyColor];
    opBQ |= board.state.pieces[4 + enemyColor];

    uint64_t blockers = (board.state.allPieces[enemyColor]) & rookMasks[kingPos];
    uint64_t rookCompressedBlockers = extract_bits(blockers, rookMasks[kingPos]);

    blockers = (board.state.allPieces[enemyColor]) & bishopMasks[kingPos];
    uint64_t bishopCompressedBlockers = extract_bits(blockers, bishopMasks[kingPos]);

    uint64_t kingAttackers = (pawnAttackMasks[color][kingPos] & opPawns)
        | (knightMasks[kingPos] & opKnights)
        | (bishopLegalMoves[kingPos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[kingPos][rookCompressedBlockers] & opRQ)
        ;

    return kingAttackers;
}

uint64_t generateCheckMask(const Board& board, int color) {

    int kingPos = squareOf(board.state.pieces[Piece::KINGS + color]);//board.getPiecePos(10 + color);

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

uint64_t generatePinMaskHV(const Board& board, int color) {

    int kingPos = squareOf(board.state.pieces[Piece::KINGS + color]);//board.getPiecePos(10 + color);

    uint64_t kingAttackers = attacksToKingXray(board, color);

    // // mask should be all 1s if nothing is pinned
    // if (kingAttackers == 0) {
    //     return ~kingAttackers;
    // }

    uint64_t blockersCompressed = extract_bits(kingAttackers, rookMasks[kingPos]);
    uint64_t pinMaskHV = checkMasksHV[kingPos][blockersCompressed];

    // if (std::popcount((board.state.allPieces[color] & (~maskForPos(kingPos))) & pinMaskHV) > 1) {
    //     pinMaskHV = 0;
    // }

    return pinMaskHV;
}

uint64_t generatePinMaskDiag(const Board& board, int color) {

    int kingPos = squareOf(board.state.pieces[Piece::KINGS + color]);//board.getPiecePos(10 + color);

    uint64_t kingAttackers = attacksToKingXray(board, color);

    // // mask should be all 1s if nothing is pinned
    // if (kingAttackers == 0) {
    //     return ~kingAttackers;
    // }

    uint64_t blockersCompressed = extract_bits(kingAttackers, bishopMasks[kingPos]);
    uint64_t pinMaskDiag = checkMasksDiag[kingPos][blockersCompressed];

    // if (std::popcount(board.state.allPieces[color] & ~maskForPos(kingPos) & pinMaskDiag) > 1) {
    //     pinMaskDiag = 0;
    // }

    return pinMaskDiag;
}

uint64_t generatePinMask(const Board& board, int color) {
    int kingPos = squareOf(board.state.pieces[Piece::KINGS + color]);//board.getPiecePos(10 + color);

    uint64_t kingAttackers = attacksToKingXray(board, color);

    uint64_t blockersCompressed = extract_bits(kingAttackers, rookMasks[kingPos]);
    uint64_t pinMask = checkMasksHV[kingPos][blockersCompressed];

    blockersCompressed = extract_bits(kingAttackers, bishopMasks[kingPos]);
    pinMask |= checkMasksDiag[kingPos][blockersCompressed];

    return pinMask;
}