#ifndef PRECOMPUTE_MASKS_H
#define PRECOMPUTE_MASKS_H

#include "board.h"
#include <cassert>
#include <cstdint>

// returns file of square (col - a,b,c etc)
inline int fileOf(int square) {
    assert(square >= 0 && square < 64);
    return (square & 7);
}

// returns the rank of a square (row - 1,2,3 etc)
inline int rankOf(const int square) {
    return (square >> 3);
}


class Bitboard {
public:
    inline static uint64_t pawnMoveMasks[2][64];
    inline static uint64_t pawnAttackMasks[2][64];
    inline static uint64_t knightMasks[64];
    inline static uint64_t kingMasks[64];
    inline static uint64_t bishopMasks[64];
    inline static uint64_t rookMasks[64];

    inline static uint64_t rookLegalMoves[64][16384];
    inline static uint64_t bishopLegalMoves[64][16384];

    inline static uint64_t checkMasksHV[64][16384];
    inline static uint64_t checkMasksDiag[64][16384];

    inline static uint64_t castleMasks[4]; // wk, bk, wq, bq
    inline static uint64_t castleSquares[4]; // wk, bk, wq, bq
    inline static uint64_t castleRookSquares[4]; // wk, bk, wq, bq
    inline static uint64_t originalRookSquares[4]; // wk, bk, wq, bq

    inline static uint64_t rowMasks[64];
    inline static uint64_t colMasks[64];

    inline static uint64_t directionMasks[8][64];

    inline static bool promoSquare[64];

    template <Color color, bool ignoreKing>
    static uint64_t attacksOnSquare(const Board& board, int square) {
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
            blockers = (board.getOccupied()) & Bitboard::rookMasks[square];
        } else {
            blockers = ((board.getOccupied()) & ~board.getBB(KINGS + static_cast<int>(color))) & Bitboard::rookMasks[square];
        }

        uint64_t rookCompressedBlockers = extract_bits(blockers, Bitboard::rookMasks[square]);

        if constexpr (!ignoreKing) {
            blockers = (board.getOccupied()) & Bitboard::bishopMasks[square];
        } else {
            blockers = ((board.getOccupied()) & ~board.getBB(KINGS + static_cast<int>(color))) & Bitboard::bishopMasks[square];
        }

        uint64_t bishopCompressedBlockers = extract_bits(blockers, Bitboard::bishopMasks[square]);

        uint64_t kingAttackers = (Bitboard::pawnAttackMasks[color][square] & opPawns)
            | (Bitboard::knightMasks[square] & opKnights)
            | (Bitboard::bishopLegalMoves[square][bishopCompressedBlockers] & opBQ)
            | (Bitboard::rookLegalMoves[square][rookCompressedBlockers] & opRQ)
            | (Bitboard::kingMasks[square] & opKing);

        return kingAttackers;
    }

    template <Color color, bool xray>
    static uint64_t attacksToKing(const Board& board) {
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
    static uint64_t generateCheckMask(const Board& board) {
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
    static uint64_t generatePinMask(const Board& board) {
        int kingPos = board.kingPos<color>(); // tz_count(board.curState->pieces[Piece::KINGS + color]);//board.getPiecePos(10 + color);

        uint64_t kingAttackers = attacksToKing<color, true>(board);

        uint64_t blockersCompressed = extract_bits(kingAttackers, Bitboard::rookMasks[kingPos]);
        uint64_t pinMask = Bitboard::checkMasksHV[kingPos][blockersCompressed];

        blockersCompressed = extract_bits(kingAttackers, Bitboard::bishopMasks[kingPos]);
        pinMask |= Bitboard::checkMasksDiag[kingPos][blockersCompressed];

        return pinMask;
    }

    // returns psuedo-moves of a piece - sliding pieces stop at blockers
    // this function does not handle removal of own pieces
    template <PieceType pt>
    inline static uint64_t getMovesBB(const Board& board, const int square) {
        switch (pt) {
        case KNIGHTS: {
            return knightMasks[square];
        }
        case BISHOPS: {
            uint64_t blockers = (board.getOccupied()) & bishopMasks[square];
            uint64_t compressedBlockers = extract_bits(blockers, bishopMasks[square]);

            return bishopLegalMoves[square][compressedBlockers];
        }
        case ROOKS: {
            uint64_t blockers = (board.getOccupied()) & rookMasks[square];
            uint64_t compressedBlockers = extract_bits(blockers, rookMasks[square]);

            return rookLegalMoves[square][compressedBlockers];
        }
        case QUEENS: {
            return getMovesBB<BISHOPS>(board, square) | getMovesBB<ROOKS>(board, square);
        }
        default:
            return 0;
        }
    }

    template <Color color>
    inline static uint64_t getPawnMovesBB(const int square) {
        return pawnMoveMasks[color][square];
    }

    template <Color color>
    inline static uint64_t getPawnAttacksBB(const int square) {
        return pawnAttackMasks[color][square];
    }

    static void initPawnMasks();
    static void initKnightMasks();
    static void initKingMasks();
    static void initBishopMasks();
    static void initRookMasks();

    static void initBishopMovesTable();
    static void initRookMovesTable();

    static void initCastleMasks();

    static void initRowColMasks();

    static void initCheckMaskTable();
    static void initPromoSquareTable();

    static void initMasks();
};

#endif
