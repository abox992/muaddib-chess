#ifndef PRECOMPUTE_MASKS_H
#define PRECOMPUTE_MASKS_H

#include <cstdint>
#include <cassert>
#include "board.h"



// returns file of square (col - a,b,c etc)
inline int fileOf(int square) {
    assert(square >= 0 && square < 64);
    return (square & 7);
}

// returns the rank of a square (row - 1,2,3 etc)
inline int rankOf(const int square) {
    return (square >> 3);
}

inline bool moreThanOne(uint64_t bb) {
    return bb & (bb - 1);
}

class Bitboard {
private:
    static uint64_t pawnMoveMasks[2][64];
    static uint64_t pawnAttackMasks[2][64];
    static uint64_t knightMasks[64];
    static uint64_t kingMasks[64];
    static uint64_t bishopMasks[64];
    static uint64_t rookMasks[64];

    static uint64_t rookLegalMoves[64][16384];
    static uint64_t bishopLegalMoves[64][16384];

    static uint64_t checkMasksHV[64][16384];
    static uint64_t checkMasksDiag[64][16384];

    static uint64_t castleMasks[4]; // wk, bk, wq, bq 
    static uint64_t castleSquares[4]; // wk, bk, wq, bq 
    static uint64_t castleRookSquares[4]; // wk, bk, wq, bq 
    static uint64_t originalRookSquares[4]; // wk, bk, wq, bq 

    static uint64_t rowMasks[64];
    static uint64_t colMasks[64];

    static uint64_t directionMasks[8][64];

    static bool promoSquare[64];
public:


    // returns psuedo-moves of a piece - sliding pieces stop at blockers 
    // this function does not handle removal of own pieces
    template <PieceType pt>
    inline static uint64_t getMovesBB(const Board& board, const int square) {
        switch (pt) {
        case KNIGHTS: {
            return knightMasks[square];
        } case BISHOPS: {
            uint64_t blockers = (board.getOccupied()) & bishopMasks[square];
            uint64_t compressedBlockers = extract_bits(blockers, bishopMasks[square]);

            return bishopLegalMoves[square][compressedBlockers];
        } case ROOKS: {
            uint64_t blockers = (board.getOccupied()) & rookMasks[square];
            uint64_t compressedBlockers = extract_bits(blockers, rookMasks[square]);

            return rookLegalMoves[square][compressedBlockers];
        } case QUEENS: {
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
