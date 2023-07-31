#include "precompute_masks.h"
#include "board.h"
#include "helpers.h"
#include <tuple>
#include <immintrin.h>

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


// ex if color is white -> tells you if black is attacking x square
uint64_t attacksOnSquare(Board& board, int color, int pos) {
    //int enemyOffset = color == 0 ? 0 : 6;
    int enemyColor = color == 0 ? 1 : 0;

    uint64_t opPawns, opKnights, opRQ, opBQ, opKing;
    opPawns = board.pieces[0 + enemyColor];
    opKnights = board.pieces[2 + enemyColor];
    opRQ = opBQ = board.pieces[8 + enemyColor];
    opRQ |= board.pieces[6 + enemyColor];
    opBQ |= board.pieces[4 + enemyColor];
    opKing = board.pieces[10 + enemyColor];

    uint64_t blockers = (~board.empty) & rookMasks[pos];
    uint64_t rookCompressedBlockers = _pext_u64(blockers, rookMasks[pos]);

    blockers = (~board.empty) & bishopMasks[pos];
    uint64_t bishopCompressedBlockers = _pext_u64(blockers, bishopMasks[pos]);

    uint64_t kingAttackers = (pawnAttackMasks[color][pos] & opPawns)
        | (knightMasks[pos] & opKnights)
        | (bishopLegalMoves[pos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[pos][rookCompressedBlockers] & opRQ)
        | (kingMasks[pos] & opKing)
        ;

    return kingAttackers;
}

uint64_t attacksOnSquareIgnoreKing(Board& board, int color, int pos) {
    //int enemyOffset = color == 0 ? 0 : 6;
    int enemyColor = color == 0 ? 1 : 0;

    uint64_t opPawns, opKnights, opRQ, opBQ, opKing;
    opPawns = board.pieces[0 + enemyColor];
    opKnights = board.pieces[2 + enemyColor];
    opRQ = opBQ = board.pieces[8 + enemyColor];
    opRQ |= board.pieces[6 + enemyColor];
    opBQ |= board.pieces[4 + enemyColor];
    opKing = board.pieces[10 + enemyColor];

    uint64_t blockers = ((~board.empty) & ~board.pieces[10 + color]) & rookMasks[pos];
    uint64_t rookCompressedBlockers = _pext_u64(blockers, rookMasks[pos]);

    blockers = ((~board.empty) & ~board.pieces[10 + color]) & bishopMasks[pos];
    uint64_t bishopCompressedBlockers = _pext_u64(blockers, bishopMasks[pos]);

    uint64_t kingAttackers = (pawnAttackMasks[color][pos] & opPawns)
        | (knightMasks[pos] & opKnights)
        | (bishopLegalMoves[pos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[pos][rookCompressedBlockers] & opRQ)
        | (kingMasks[pos] & opKing)
        ;

    return kingAttackers;
}

uint64_t attacksToKing(Board& board, int color) {
    //int enemyOffset = color == 0 ? 6 : 0;
    //int myColorOffset = color == 0 ? 0 : 6;
    int enemyColor = color == 0 ? 1 : 0;

    int kingPos = board.getPiecePos(10 + color);

    uint64_t opPawns, opKnights, opRQ, opBQ;
    opPawns = board.pieces[0 + enemyColor];
    opKnights = board.pieces[2 + enemyColor];
    opRQ = opBQ = board.pieces[8 + enemyColor];
    opRQ |= board.pieces[6 + enemyColor];
    opBQ |= board.pieces[4 + enemyColor];

    uint64_t blockers = (~board.empty) & rookMasks[kingPos];
    uint64_t rookCompressedBlockers = _pext_u64(blockers, rookMasks[kingPos]);

    blockers = (~board.empty) & bishopMasks[kingPos];
    uint64_t bishopCompressedBlockers = _pext_u64(blockers, bishopMasks[kingPos]);

    uint64_t kingAttackers = (pawnAttackMasks[color][kingPos] & opPawns)
        | (knightMasks[kingPos] & opKnights)
        | (bishopLegalMoves[kingPos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[kingPos][rookCompressedBlockers] & opRQ)
        ;

    return kingAttackers;
}

uint64_t attacksToKingXray(Board& board, int color) {
    //int enemyOffset = color == 0 ? 6 : 0;
    //int myColorOffset = color == 0 ? 0 : 6;
    int enemyColor = color == 0 ? 1 : 0;

    int kingPos = board.getPiecePos(10 + color);

    uint64_t opPawns, opKnights, opRQ, opBQ;
    opPawns = board.pieces[0 + enemyColor];
    opKnights = board.pieces[2 + enemyColor];
    opRQ = opBQ = board.pieces[8 + enemyColor];
    opRQ |= board.pieces[6 + enemyColor];
    opBQ |= board.pieces[4 + enemyColor];

    uint64_t blockers = (board.allPieces[enemyColor]) & rookMasks[kingPos];
    uint64_t rookCompressedBlockers = _pext_u64(blockers, rookMasks[kingPos]);

    blockers = (board.allPieces[enemyColor]) & bishopMasks[kingPos];
    uint64_t bishopCompressedBlockers = _pext_u64(blockers, bishopMasks[kingPos]);

    uint64_t kingAttackers = (pawnAttackMasks[color][kingPos] & opPawns)
        | (knightMasks[kingPos] & opKnights)
        | (bishopLegalMoves[kingPos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[kingPos][rookCompressedBlockers] & opRQ)
        ;

    return kingAttackers;
}

uint64_t generateCheckMask(Board& board, int color) {

    //int myColorOffset = color == 0 ? 0 : 6;

    int kingPos = board.getPiecePos(10 + color);

    uint64_t kingAttackers = attacksToKing(board, color);

    int directions[] = {9, 8, 7, 1, -1, -7, -8, -9};
    for (int offset : directions) {
        int currentPos = kingPos;
        uint64_t tempDirectionMask = 0;
        while ((currentPos + offset) < 64 && (currentPos + offset) > -1) {

            int currentY = (currentPos + offset) / 8;
            int currentX = (currentPos + offset) % 8;
            int prevY = (currentPos) / 8;
            int prevX = (currentPos) % 8;

            int maxDif = max(abs(currentX - prevX), abs(currentY - prevY));

            if (maxDif != 1) {
                break;
            }

            // at this point we know currentpos + offset is at least valid, now check for piece
            uint64_t currentMask = MaskForPos(currentPos + offset);
            if ((currentMask & kingAttackers) != 0) {
                kingAttackers |= tempDirectionMask;
                break;
            }

            currentPos += offset;
            tempDirectionMask |= currentMask;
        }

    }

    return kingAttackers;
}

uint64_t generatePinMask(Board& board, int color) {

    //int myColorOffset = color == 0 ? 0 : 6;

    int kingPos = board.getPiecePos(10 + color);
    uint64_t kingAttackers = attacksToKingXray(board, color);

    int directions[] = {9, 8, 7, 1, -1, -7, -8, -9};
    for (int offset : directions) {
        int currentPos = kingPos;
        uint64_t tempDirectionMask = 0;
        while ((currentPos + offset) < 64 && (currentPos + offset) > -1) {

            int currentY = (currentPos + offset) / 8;
            int currentX = (currentPos + offset) % 8;
            int prevY = (currentPos) / 8;
            int prevX = (currentPos) % 8;

            int maxDif = max(abs(currentX - prevX), abs(currentY - prevY));

            if (maxDif != 1) {
                break;
            }

            // at this point we know currentpos + offset is at least valid, now check for piece
            uint64_t currentMask = MaskForPos(currentPos + offset);
            if ((currentMask & kingAttackers) != 0) {
                kingAttackers |= tempDirectionMask;
                break;
            }

            currentPos += offset;
            tempDirectionMask |= currentMask;
        }

    }

    return kingAttackers;
}

tuple<uint64_t, uint64_t, uint64_t> generateCheckAndPinMask(Board& board, int color) {
    //int myColorOffset = color == 0 ? 0 : 6;

    int kingPos = board.getPiecePos(10 + color);

    uint64_t checkMask = generateCheckMask(board, color);
    uint64_t pinMask = generatePinMask(board, color); // inlcudes checks as well

    pinMask &= ~checkMask; // now only pins

    uint64_t pinH = pinMask & rowMasks[kingPos];
    uint64_t pinV = pinMask & colMasks[kingPos];
    uint64_t pinDiag = pinMask & bishopMasks[kingPos];

    // if there is more than one piece along the pin its not actually a pin
    uint64_t temp = board.allPieces[color] & pinH;
    int count = 0;
    Bitloop(temp) {
        count++;
    }
    if (count > 1) {
        pinH = 0;
    }

    temp = board.allPieces[color] & pinV;
    count = 0;
    Bitloop(temp) {
        count++;
    }
    if (count > 1) {
        pinV = 0;
    }

    temp = board.allPieces[color] & pinDiag;
    count = 0;
    Bitloop(temp) {
        count++;
    }
    if (count > 1) {
        pinDiag = 0;
    }

    //pinHV = pinHV == 0 ? ~pinHV : pinHV;
    //pinDiag = pinDiag == 0 ? ~pinDiag : pinDiag;
    uint64_t pinHV = pinH | pinV;

    checkMask = checkMask == 0 ? ~checkMask : checkMask;

    return make_tuple(checkMask, pinHV, pinDiag);

}