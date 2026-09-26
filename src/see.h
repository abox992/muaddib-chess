#ifndef SEE_H
#define SEE_H

#include "board.h"
#include "move.h"
#include <cassert>

namespace See {

// Pawn, knight, bishop, rook, queen, king. King is only for static exchange.
inline constexpr int kPieceValue[6] = {100, 300, 320, 500, 900, 10000};

// Ordering bands: table move, winning captures, killers, quiets, losing captures.
inline constexpr int kTTMoveScore      = 1'000'000;
inline constexpr int kGoodCaptureScore = 20'000;
inline constexpr int kKillerScore[2]   = {9'000, 8'000};
inline constexpr int kBadCaptureScore  = -100'000;

inline int pieceValue(Piece piece) {
    if (piece == NO_PIECE) {
        return 0;
    }
    return kPieceValue[toInt(piece) / 2];
}

struct Exchange {
    int victimVal;
    int attackerVal;
};

inline Exchange exchangeMaterial(const Board& board, Move move) {
    int victimVal   = move.moveType() == MoveType::EN_PASSANT ? kPieceValue[0] : pieceValue(board.pieceOn(move.to()));
    int attackerVal = pieceValue(board.pieceOn(move.from()));

    if (move.moveType() == MoveType::PROMOTION) {
        const int promo = kPieceValue[static_cast<int>(move.promotionPiece()) + 1];
        victimVal += promo - kPieceValue[0];
        attackerVal = promo;
    }

    return {victimVal, attackerVal};
}

inline bool isCapture(const Board& board, Move move) {
    if (move.moveType() == MoveType::CASTLE) {
        return false;
    }
    if (move.moveType() == MoveType::EN_PASSANT) {
        return true;
    }

    const Piece victim = board.pieceOn(move.to());
    if (victim == NO_PIECE) {
        return false;
    }
    // Castling is the only legal move onto a friendly piece, and it returned above.
    assert((toInt(victim) & 1) != (toInt(board.pieceOn(move.from())) & 1));
    return true;
}

inline bool isTactical(const Board& board, Move move) {
    return move.moveType() == MoveType::PROMOTION || isCapture(board, move);
}

// https://chessprogramming.org/MVV-LVA
inline int mvvLva(const Board& board, Move move) {
    const Exchange exchange = exchangeMaterial(board, move);
    return exchange.victimVal * 16 - exchange.attackerVal;
}

// Material gained by playing the capture, from the mover's side.
// Positive wins material, zero is an even trade, negative loses material.
int see(const Board& board, Move move);

inline int moveOrderScore(const Board& board, Move move, Move ttMove, Move killer1, Move killer2) {
    if (!ttMove.isNull() && move == ttMove) {
        return kTTMoveScore;
    }

    if (isTactical(board, move)) {
        const int mvv = mvvLva(board, move);
        if (see(board, move) >= 0) {
            return kGoodCaptureScore + mvv;
        }
        return kBadCaptureScore + mvv;
    }

    if (!killer1.isNull() && move == killer1) {
        return kKillerScore[0];
    }
    if (!killer2.isNull() && move == killer2) {
        return kKillerScore[1];
    }
    return 0;
}

}  // namespace See

#endif
