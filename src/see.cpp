#include "see.h"
#include "bit_manip.h"
#include "bitboard.h"
#include <algorithm>
#include <array>

namespace See {

namespace {

template<bool bishop>
uint64_t sliderAttacks(int square, uint64_t occupied) {
    const uint64_t mask       = bishop ? Bitboard::bishopMasks[square] : Bitboard::rookMasks[square];
    const uint64_t compressed = extract_bits(occupied & mask, mask);
    return bishop ? Bitboard::bishopLegalMoves[square][compressed] : Bitboard::rookLegalMoves[square][compressed];
}

// Lowest piece type of `side` that attacks `to` through `occupied`. Square, or -1.
int leastValuableAttacker(const Board& board, uint64_t occupied, int to, Color color) {
    auto pieces = [&](PieceType type) { return board.getBB(color, type) & occupied; };

    const Color defender = static_cast<Color>(static_cast<int>(color) ^ 1);

    const uint64_t pawns = Bitboard::pawnAttackMasks[defender][to] & pieces(PieceType::PAWNS);
    if (pawns) {
        return tz_count(pawns);
    }

    const uint64_t knights = Bitboard::knightMasks[to] & pieces(PieceType::KNIGHTS);
    if (knights) {
        return tz_count(knights);
    }

    const uint64_t bishopHits = sliderAttacks<true>(to, occupied);
    const uint64_t rookHits   = sliderAttacks<false>(to, occupied);

    const uint64_t bishops = bishopHits & pieces(PieceType::BISHOPS);
    if (bishops) {
        return tz_count(bishops);
    }

    const uint64_t rooks = rookHits & pieces(PieceType::ROOKS);
    if (rooks) {
        return tz_count(rooks);
    }

    const uint64_t queens = (bishopHits | rookHits) & pieces(PieceType::QUEENS);
    if (queens) {
        return tz_count(queens);
    }

    const uint64_t kings = Bitboard::kingMasks[to] & pieces(PieceType::KINGS);
    if (kings) {
        return tz_count(kings);
    }

    return -1;
}

}  // namespace

// static exchange evaluation. Determine if a capture is winning or losing.
int see(const Board& board, Move move) {
    const int   from = move.from();
    const int   to   = move.to();
    const Color us   = static_cast<Color>(toInt(board.pieceOn(from)) & 1);

    uint64_t occupied = board.getOccupied() & ~maskForPos(from);
    if (move.moveType() == MoveType::EN_PASSANT) {
        const int victimSquare = to - Bitboard::pawnPush(us);
        occupied &= ~maskForPos(victimSquare);
    }

    const Exchange exchange = exchangeMaterial(board, move);

    // victims[0] is what this move gains. Each later entry is the piece a recapture would take.
    // A side declines a recapture when it does not improve the score, so a king is not
    // stepped onto a square the opponent still attacks.
    std::array<int, 32> victims{};
    int                 count   = 0;
    int                 hanging = exchange.attackerVal;
    victims[count++]            = exchange.victimVal;

    Color side = static_cast<Color>(static_cast<int>(us) ^ 1);
    while (count < static_cast<int>(victims.size())) {
        const int attacker = leastValuableAttacker(board, occupied, to, side);
        if (attacker < 0) {
            break;
        }

        victims[count++] = hanging;
        hanging          = pieceValue(board.pieceOn(attacker));
        occupied &= ~maskForPos(attacker);
        side = static_cast<Color>(static_cast<int>(side) ^ 1);
    }

    int recapture = 0;
    for (int i = count - 1; i >= 1; --i) {
        recapture = std::max(0, victims[i] - recapture);
    }
    return victims[0] - recapture;
}

}  // namespace See
