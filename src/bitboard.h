#ifndef BITBOARD_H
#define BITBOARD_H

#include "bit_manip.h"
#include "board.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>

namespace {
template <typename T>
constexpr T constexprAbs(const T& x) {
    return x >= 0 ? x : -x;
}
} // namespace

namespace Bitboard {
constexpr inline int fileOf(const int square) {
    assert(square >= 0 && square < 64);
    return (square & 7);
}

constexpr inline int rankOf(const int square) {
    assert(square >= 0 && square < 64);
    return (square >> 3);
}

constexpr inline bool isOk(const int square) {
    return square < 64 && square >= 0;
}

constexpr inline int fileDistance(const int from, const int to) {
    assert(isOk(from) && isOk(to));
    return constexprAbs(fileOf(from) - fileOf(to));
}

constexpr inline int rankDistance(const int from, const int to) {
    assert(isOk(from) && isOk(to));
    return constexprAbs(rankOf(from) - rankOf(to));
}

constexpr inline int distance(const int from, const int to) {
    assert(isOk(from) && isOk(to));
    return std::max(fileDistance(from, to), rankDistance(from, to));
}

constexpr inline bool safeDestination(const int from, const int to) {
    assert(isOk(from) && isOk(to));
    return distance(from, to) < 2;
}

/* INIT COMPILETIME PRECOMPUTED BITBOARDS */

using Arr_2x64 = std::array<std::array<uint64_t, 64>, 2>;

constinit inline const Arr_2x64 pawnMoveMasks = [] {
    Arr_2x64 moveMasks {};

    for (Color color : { WHITE, BLACK }) {

        // moves
        for (int i = 0; i < 64; i++) {
            uint64_t mask = 0;
            int offset = 8;

            int dest = color ? i - offset : i + offset;
            int doubleDest = color ? i - 16 : i + 16;

            if (!isOk(dest))
                continue;

            if (safeDestination(i, dest)) {
                mask |= maskForPos(dest);
            }
            if (rankOf(i) == (color ? 6 : 1)) {
                mask |= maskForPos(doubleDest);
            }

            moveMasks[color][i] = mask;
        }
    }

    return moveMasks;
}();

constinit inline const Arr_2x64 pawnAttackMasks = [] {
    Arr_2x64 attackMasks {};
    for (Color color : { WHITE, BLACK }) {
        // attacks
        for (int i = 0; i < 64; i++) {
            uint64_t mask = 0;
            for (int offset : { 9, 7 }) {
                int dest = color ? i - offset : i + offset;

                if (!isOk(dest))
                    continue;

                if (safeDestination(i, dest)) {
                    mask |= maskForPos(dest);
                }
            }

            attackMasks[color][i] = mask;
        }
    }

    return attackMasks;
}();

using Arr_64 = std::array<uint64_t, 64>;

constinit inline const Arr_64 knightMasks = [] {
    Arr_64 knightMasks {};
    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0;
        for (int offset : { 17, 15, 10, 6, -6, -10, -15, -17 }) {

            int dest = i + offset;

            if (!isOk(dest))
                continue;

            if (distance(i, dest) == 2) {
                mask |= maskForPos(dest);
            }
        }

        knightMasks[i] = mask;
    }

    return knightMasks;
}();

constinit inline const Arr_64 kingMasks = [] {
    Arr_64 kingMasks {};
    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0;
        for (int offset : { 9, 8, 7, 1, -1, -7, -8, -9 }) {
            int dest = i + offset;

            if (!isOk(dest))
                continue;

            if (safeDestination(i, dest)) {
                mask |= maskForPos(i + offset);
            }
        }

        kingMasks[i] = mask;
    }

    return kingMasks;
}();

constexpr inline const Arr_64 bishopMasks = [] {
    Arr_64 bishopMasks {};
    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0;
        for (int offset : { 9, 7, -7, -9 }) {
            int dest = i + offset;
            while (isOk(dest) && safeDestination(dest - offset, dest)) {
                mask |= maskForPos(dest);
                dest += offset;
            }
        }

        bishopMasks[i] = mask;
    }

    return bishopMasks;
}();

constinit inline const Arr_64 rookMasks = [] {
    Arr_64 rookMasks {};
    for (int i = 0; i < 64; i++) {
        uint64_t mask = 0;
        for (int offset : { 8, 1, -1, -8 }) {
            int dest = i + offset;
            while (isOk(dest) && safeDestination(dest - offset, dest)) {
                mask |= maskForPos(dest);
                dest += offset;
            }
        }

        rookMasks[i] = mask;
    }

    return rookMasks;
}();

/*
 * Given some slider piece, there are 14 positions where there may or
 * may not be a blocker that affects the possible moves of the slider.
 *
 * 0 0 0 0 x 0 0 0
 * 0 0 0 0 x 0 0 0
 * 0 0 0 0 x 0 0 0
 * 0 0 0 0 x 0 0 0
 * 0 0 0 0 x 0 0 0
 * x x x x S x x x
 * 0 0 0 0 x 0 0 0
 * 0 0 0 0 x 0 0 0
 *
 * Therfore, for all 64 squares, we precompute all possible blocker
 * combinations (2^14 = 16384) and the respective legal moves given that combination.
 *
 * Note: Queen legal moves is equal to the bitwise or of the bishop and rook legal moves
 *       for that square. Calculate HV blockers, get rook moves. Calculate Diag blockers, get
 *       bishop moves.
 * */
using Arr_64x16384 = std::array<std::array<uint64_t, 16384>, 64>;

inline const Arr_64x16384 rookLegalMoves = [] {
    Arr_64x16384 rookLegalMoves {};
    // for each square
    for (int currentSquare = 0; currentSquare < 64; currentSquare++) {

        // for each possible block combination
        for (int i = 0; i < 16384; i++) {
            uint64_t blockMask = 0x0;

            uint64_t tempRookMask = rookMasks[currentSquare];
            int compressed = i;
            int count = 0;
            while (tempRookMask) {
                const int index = tz_count(tempRookMask);
                pop_lsb(tempRookMask);

                blockMask |= (uint64_t((compressed >> count) & 1) << index);
                count++;
            }

            uint64_t legalMask = rookMasks[currentSquare];
            int directions[] = { 8, 1, -1, -8 };
            for (int offset : directions) {

                bool hitBlocker = false;
                int index = currentSquare + offset;
                int count = 0; // do it a max of 7 times so we dont wrap
                while (index < 64 && index > -1 && count < 7) {

                    if (hitBlocker) {
                        legalMask &= ~(uint64_t(1) << index);
                    } else {
                        // check if index is blocker in blocker mask
                        if ((blockMask & (uint64_t(1) << index)) != 0) {
                            hitBlocker = true;
                        }
                    }

                    index += offset;
                    count++;
                }
            }

            rookLegalMoves[currentSquare][i] = legalMask;
        }
    }

    return rookLegalMoves;
}();

inline const Arr_64x16384 bishopLegalMoves = [] {
    Arr_64x16384 bishopLegalMoves {};
    // for each square
    for (int currentSquare = 0; currentSquare < 64; currentSquare++) {

        // for each possible block combination
        for (int i = 0; i < 16384; i++) {
            uint64_t blockMask = 0x0;

            uint64_t tempBishopMask = bishopMasks[currentSquare];
            int compressed = i;
            int count = 0;
            while (tempBishopMask) {
                const int index = tz_count(tempBishopMask);
                pop_lsb(tempBishopMask);

                blockMask |= (uint64_t((compressed >> count) & 1) << index);
                count++;
            }

            uint64_t legalMask = bishopMasks[currentSquare];
            int directions[] = { 9, 7, -7, -9 };
            for (int offset : directions) {

                bool hitBlocker = false;
                int index = currentSquare + offset;
                int count = 0; // do it a max of 7 times so we dont wrap
                while (index < 64 && index > -1 && count < 7) {

                    if (hitBlocker) {
                        legalMask &= ~(uint64_t(1) << index);
                    } else {
                        // check if index is blocker in blocker mask
                        if ((blockMask & (uint64_t(1) << index)) != 0) {
                            hitBlocker = true;
                        }
                    }

                    index += offset;
                    count++;
                }
            }

            bishopLegalMoves[currentSquare][i] = legalMask;
        }
    }

    return bishopLegalMoves;
}();

inline const Arr_64x16384 checkMasksHV = [] {
    Arr_64x16384 checkMasksHV {};
    // for each square
    for (int currentSquare = 0; currentSquare < 64; currentSquare++) {

        // for each possible block combination
        for (int i = 0; i < 16384; i++) {
            uint64_t blockMask = 0x0;

            uint64_t tempRookMask = rookMasks[currentSquare];
            int compressed = i;
            int count = 0;
            while (tempRookMask) {
                const int index = tz_count(tempRookMask);
                pop_lsb(tempRookMask);

                blockMask |= (uint64_t((compressed >> count) & 1) << index);
                count++;
            }

            // rookLegalMoves[currentSquare][i] = blockMask;
            uint64_t checkMaskHV = rookMasks[currentSquare];
            int directions[] = { 8, 1, -1, -8 };
            for (int offset : directions) {

                uint64_t directionMask = 0;
                bool hitBlocker = false;
                int index = currentSquare + offset;
                int count = 0; // do it a max of 7 times so we dont wrap
                while (index < 64 && index > -1 && count < 7) {
                    // if weve already hit a blocker, set bit to 0
                    if (hitBlocker) {
                        checkMaskHV &= ~maskForPos(index);
                    } else if ((blockMask & maskForPos(index)) != 0) { // if current square is a blocker, flag
                        hitBlocker = true;
                    }

                    directionMask |= maskForPos(index);
                    index += offset;
                    count++;
                }

                // if there is no blocker on the row, remove bits
                if (!hitBlocker) {
                    checkMaskHV &= ~directionMask;
                }
            }

            checkMasksHV[currentSquare][i] = checkMaskHV;
        }
    }

    return checkMasksHV;
}();

inline const Arr_64x16384 checkMasksDiag = [] {
    Arr_64x16384 checkMasksDiag {};
    // for each square
    for (int currentSquare = 0; currentSquare < 64; currentSquare++) {

        // for each possible block combination
        for (int i = 0; i < 16384; i++) {

            // uncompress bits, create the blockers mask
            uint64_t blockMask = 0x0;

            uint64_t tempBishopMask = bishopMasks[currentSquare];
            int compressed = i;
            int count = 0;
            while (tempBishopMask) {
                const int index = tz_count(tempBishopMask);
                pop_lsb(tempBishopMask);

                blockMask |= (uint64_t((compressed >> count) & 1) << index);
                count++;
            }

            // rookLegalMoves[currentSquare][i] = blockMask;
            uint64_t checkMaskDiag = bishopMasks[currentSquare];
            int directions[] = { 9, 7, -7, -9 };
            for (int offset : directions) {

                uint64_t directionMask = 0;
                bool hitBlocker = false;
                int index = currentSquare + offset;
                int count = 0; // do it a max of 7 times so we dont wrap
                while (index < 64 && index > -1 && count < 7) {
                    // if weve already hit a blocker, set bit to 0
                    if (hitBlocker) {
                        checkMaskDiag &= ~maskForPos(index);
                    } else if ((blockMask & maskForPos(index)) != 0) { // if current square is a blocker, flag
                        hitBlocker = true;
                    }

                    directionMask |= maskForPos(index);
                    index += offset;
                    count++;
                }

                // if there is no blocker on the row, remove bits
                if (!hitBlocker) {
                    checkMaskDiag &= ~directionMask;
                }
            }

            checkMasksDiag[currentSquare][i] = checkMaskDiag;
        }
    }

    return checkMasksDiag;
}();

// wk, bk, wq, bq
constinit inline const std::array<uint64_t, 4> castleMasks = [] {
    std::array<uint64_t, 4> castleMasks {};

    uint64_t castle = 0;
    castle |= uint64_t(1) << 1;
    castle |= uint64_t(1) << 2;

    castleMasks[0] = castle;
    castleMasks[1] = castle << 56;

    castle = 0;
    castle |= uint64_t(1) << 4;
    castle |= uint64_t(1) << 5;
    castle |= uint64_t(1) << 6;

    castleMasks[2] = castle;
    castleMasks[3] = castle << 56;

    return castleMasks;
}();

constinit inline const std::array<uint64_t, 4> castleSquares = [] {
    std::array<uint64_t, 4> castleSquares {};

    castleSquares[0] = uint64_t(1) << 1;
    castleSquares[1] = castleSquares[0] << 56;

    castleSquares[2] = uint64_t(1) << 5;
    castleSquares[3] = castleSquares[2] << 56;

    return castleSquares;
}();

constinit inline const std::array<uint64_t, 4> castleRookSquares = [] {
    std::array<uint64_t, 4> castleRookSquares {};

    castleRookSquares[0] = uint64_t(1) << 2;
    castleRookSquares[1] = uint64_t(1) << 58;
    castleRookSquares[2] = uint64_t(1) << 4;
    castleRookSquares[3] = uint64_t(1) << 60;

    return castleRookSquares;
}();

constinit inline const std::array<uint64_t, 4> originalRookSquares = [] {
    std::array<uint64_t, 4> originalRookSquares {};

    originalRookSquares[0] = uint64_t(1);
    originalRookSquares[1] = uint64_t(1) << 56;
    originalRookSquares[2] = uint64_t(1) << 7;
    originalRookSquares[3] = uint64_t(1) << 63;

    return originalRookSquares;
}();

inline const std::array<uint64_t, 64> rowMasks = []{
    std::array<uint64_t, 64> rowMasks {};
    uint64_t rowMask = 0xFF;
    for (int i = 0; i < 64; i++) {
        int row = i / 8;

        rowMasks[i] = rowMask << (row * 8);
    }

    return rowMasks;

}();
inline const std::array<uint64_t, 64> colMasks = []{
    std::array<uint64_t, 64> colMasks {};

    uint64_t colMask = 0x101010101010101;
    for (int i = 0; i < 64; i++) {
        int col = i % 8;

        colMasks[i] = colMask << col;
    }

    return colMasks;

}();

inline const std::array<std::array<uint64_t, 64>, 8> directionMasks = []{

    std::array<std::array<uint64_t, 64>, 8> directionMasks {};
    /*
     *  9  8  7
     *  1  0 -1
     * -7 -8 -9
     */
    int directions[] = { 8, -8, 1, -1, 9, -9, 7, -7 };
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 8; j++) {
            uint64_t mask = 0x0;
            int temp = directions[j];
            while ((i + temp) < 64 && (i + temp) > -1) {

                int currentY = (i + temp) / 8;
                int currentX = (i + temp) % 8;
                int prevY = (i + temp - directions[j]) / 8;
                int prevX = (i + temp - directions[j]) % 8;

                int maxDif = std::max(constexprAbs(currentX - prevX), constexprAbs(currentY - prevY));

                if (maxDif != 1) {
                    break;
                }

                mask |= (uint64_t(1) << (i + temp));
                temp += directions[j];
            }

            directionMasks[j][i] = mask;
        }
    }

    return directionMasks;
}();

template <Color color, bool ignoreKing>
uint64_t attacksOnSquare(const Board& board, int square) {
    constexpr Color enemyColor = static_cast<Color>(!color);

    uint64_t opPawns, opKnights, opRQ, opBQ, opKing;
    opPawns = board.getBB(PAWNS + static_cast<int>(enemyColor)); 
    opKnights = board.getBB(KNIGHTS + static_cast<int>(enemyColor)); 
    opRQ = opBQ = board.getBB(QUEENS + static_cast<int>(enemyColor)); 
    opRQ |= board.getBB(ROOKS + static_cast<int>(enemyColor)); 
    opBQ |= board.getBB(BISHOPS + static_cast<int>(enemyColor)); 
    opKing = board.getBB(KINGS + static_cast<int>(enemyColor)); 

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
uint64_t attacksToKing(const Board& board) {
    constexpr Color enemyColor = static_cast<Color>(!color);

    int kingPos = board.kingPos<color>();

    uint64_t opPawns, opKnights, opRQ, opBQ;
    opPawns = board.getBB(PAWNS + static_cast<int>(enemyColor)); 
    opKnights = board.getBB(KNIGHTS + static_cast<int>(enemyColor)); 
    opRQ = opBQ = board.getBB(QUEENS + static_cast<int>(enemyColor)); 
    opRQ |= board.getBB(ROOKS + static_cast<int>(enemyColor)); 
    opBQ |= board.getBB(BISHOPS + static_cast<int>(enemyColor)); 

    uint64_t blockers;

    if constexpr (!xray) {
        blockers = (board.getOccupied()) & Bitboard::rookMasks[kingPos];
    } else {
        blockers = board.getOccupied() & Bitboard::rookMasks[kingPos];
        blockers &= ~rookLegalMoves[kingPos][extract_bits(blockers, rookMasks[kingPos])];
    }

    uint64_t rookCompressedBlockers = extract_bits(blockers, Bitboard::rookMasks[kingPos]);

    if constexpr (!xray) {
        blockers = (board.getOccupied()) & Bitboard::bishopMasks[kingPos];
    } else {
        blockers = board.getOccupied() & Bitboard::bishopMasks[kingPos];
        blockers &= ~bishopLegalMoves[kingPos][extract_bits(blockers, bishopMasks[kingPos])];
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

    return pinMask;
}

// returns psuedo-moves of a piece - sliding pieces stop at blockers
// this function does not handle removal of own pieces
template <PieceType pt>
constexpr inline uint64_t getMovesBB(const Board& board, const int square) {
    static_assert(pt != PAWNS);

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
constexpr inline uint64_t getPawnMovesBB(const int square) {
    return pawnMoveMasks[color][square];
}

template <Color color>
constexpr inline uint64_t getPawnAttacksBB(const int square) {
    return pawnAttackMasks[color][square];
}

};

#endif
