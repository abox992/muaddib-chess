#include "precompute_masks.h"
#include "board.h"
#include "helpers.h"
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

uint64_t attacksOnSquare(Board& board, int color, int pos) {
    int enemyOffset = color == 0 ? 0 : 6;

    uint64_t opPawns, opKnights, opRQ, opBQ;
    opPawns = board.pieces[0 + enemyOffset];
    opKnights = board.pieces[1 + enemyOffset];
    opRQ = opBQ = board.pieces[4 + enemyOffset];
    opRQ |= board.pieces[3 + enemyOffset];
    opBQ |= board.pieces[2 + enemyOffset];

    int rookCompressedBlockers = 0;
    uint64_t blockers = (~board.empty) & rookMasks[pos];
    uint64_t tempRookMask = rookMasks[pos];

    int count = 0;
    while (tempRookMask != 0) {
        int index = lsb(tempRookMask);

        if ((blockers & (uint64_t(1) << index)) != 0) {
            rookCompressedBlockers |= (1 << count);
        }

        count++;
    }

    int bishopCompressedBlockers = 0;
    blockers = (~board.empty) & bishopMasks[pos];
    uint64_t tempBishopMask = bishopMasks[pos];
    count = 0;
    while (tempBishopMask != 0) {
        int index = lsb(tempBishopMask);

        if ((blockers & (uint64_t(1) << index)) != 0) {
            bishopCompressedBlockers |= (1 << count);
        }

        count++;
    }

    uint64_t kingAttackers = (pawnAttackMasks[color][pos] & opPawns)
        | (knightMasks[pos] & opKnights)
        | (bishopLegalMoves[pos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[pos][rookCompressedBlockers] & opRQ)
        ;

    return kingAttackers;
}

uint64_t attacksToKing(Board& board, int color) {
    int enemyOffset = color == 0 ? 6 : 0;
    int myColorOffset = color == 0 ? 0 : 6;

    int kingPos = board.getPiecePos(5 + myColorOffset);

    uint64_t opPawns, opKnights, opRQ, opBQ;
    opPawns = board.pieces[0 + enemyOffset];
    opKnights = board.pieces[1 + enemyOffset];
    opRQ = opBQ = board.pieces[4 + enemyOffset];
    opRQ |= board.pieces[3 + enemyOffset];
    opBQ |= board.pieces[2 + enemyOffset];

    int rookCompressedBlockers = 0;
    uint64_t blockers = (~board.empty) & rookMasks[kingPos];
    uint64_t tempRookMask = rookMasks[kingPos];

    int count = 0;
    while (tempRookMask != 0) {
        int index = lsb(tempRookMask);

        if ((blockers & (uint64_t(1) << index)) != 0) {
            rookCompressedBlockers |= (1 << count);
        }

        count++;
    }

    int bishopCompressedBlockers = 0;
    blockers = (~board.empty) & bishopMasks[kingPos];
    uint64_t tempBishopMask = bishopMasks[kingPos];
    count = 0;
    while (tempBishopMask != 0) {
        int index = lsb(tempBishopMask);

        if ((blockers & (uint64_t(1) << index)) != 0) {
            bishopCompressedBlockers |= (1 << count);
        }

        count++;
    }

    uint64_t kingAttackers = (pawnAttackMasks[color][kingPos] & opPawns)
        | (knightMasks[kingPos] & opKnights)
        | (bishopLegalMoves[kingPos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[kingPos][rookCompressedBlockers] & opRQ)
        ;

    return kingAttackers;
}

uint64_t attacksToKingXray(Board& board, int color) {
    int enemyOffset = color == 0 ? 6 : 0;
    int myColorOffset = color == 0 ? 0 : 6;

    int enemyColor = color == 0 ? 1 : 0;

    int kingPos = board.getPiecePos(5 + myColorOffset);

    uint64_t opPawns, opKnights, opRQ, opBQ;
    opPawns = board.pieces[0 + enemyOffset];
    opKnights = board.pieces[1 + enemyOffset];
    opRQ = opBQ = board.pieces[4 + enemyOffset];
    opRQ |= board.pieces[3 + enemyOffset];
    opBQ |= board.pieces[2 + enemyOffset];

    int rookCompressedBlockers = 0;
    uint64_t blockers = (board.allPieces[enemyColor]) & rookMasks[kingPos];
    uint64_t tempRookMask = rookMasks[kingPos];

    int count = 0;
    while (tempRookMask != 0) {
        int index = lsb(tempRookMask);

        if ((blockers & (uint64_t(1) << index)) != 0) {
            rookCompressedBlockers |= (1 << count);
        }

        count++;
    }

    int bishopCompressedBlockers = 0;
    blockers = (board.allPieces[enemyColor]) & bishopMasks[kingPos];
    uint64_t tempBishopMask = bishopMasks[kingPos];
    count = 0;
    while (tempBishopMask != 0) {
        int index = lsb(tempBishopMask);

        if ((blockers & (uint64_t(1) << index)) != 0) {
            bishopCompressedBlockers |= (1 << count);
        }

        count++;
    }

    uint64_t kingAttackers = (pawnAttackMasks[color][kingPos] & opPawns)
        | (knightMasks[kingPos] & opKnights)
        | (bishopLegalMoves[kingPos][bishopCompressedBlockers] & opBQ)
        | (rookLegalMoves[kingPos][rookCompressedBlockers] & opRQ)
        ;

    return kingAttackers;
}

uint64_t generateCheckMask(Board& board, int color) {

    int myColorOffset = color == 0 ? 0 : 6;

    int kingPos = board.getPiecePos(5 + myColorOffset);

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
            uint64_t currentMask = (uint64_t(1) << (currentPos + offset));
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

    int myColorOffset = color == 0 ? 0 : 6;

    int kingPos = board.getPiecePos(5 + myColorOffset);
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
            uint64_t currentMask = (uint64_t(1) << (currentPos + offset));
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
    int myColorOffset = color == 0 ? 0 : 6;

    int kingPos = board.getPiecePos(5 + myColorOffset);

    uint64_t checkMask = generateCheckMask(board, color);
    uint64_t pinMask = generatePinMask(board, color); // inlcudes checks as well

    pinMask &= ~checkMask; // now only pins

    uint64_t pinHV = pinMask & rookMasks[kingPos];
    uint64_t pinDiag = pinMask & bishopMasks[kingPos];

    pinHV = pinHV == 0 ? ~pinHV : pinHV;
    pinDiag = pinDiag == 0 ? ~pinDiag : pinDiag;

    checkMask = checkMask == 0 ? ~checkMask : checkMask;

    return make_tuple(checkMask, pinHV, pinDiag);

}