#include "zobrist.h"
#include "bitboard.h"
#include "board.h"
#include "types.h"
#include "helpers.h"

namespace Zobrist {

void initZobrist() {
    PRNG generator(1070372);
    /*PRNG generator(23848721498);*/

    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 12; j++) {
            randomTable[i][j] = generator.rand<uint64_t>();
        }
    }

    for (int i = 0; i < 8; i++) {
        enpassantFile[i] = generator.rand<uint64_t>();
    }
    
    for (int i = 0; i < 4; i++) {
        castling[i] = generator.rand<uint64_t>();
    }

    randomBlackToMove = generator.rand<uint64_t>();
    noPawns = generator.rand<uint64_t>();
}

uint64_t zhash(const Board& board) {
    uint64_t hash = 0;

    if (board.blackToMove()) {
        hash ^= randomBlackToMove;
    }

    if ((board.getBB(WHITE, PAWNS) | board.getBB(BLACK, PAWNS)) == 0) {
        hash ^= noPawns;
    }

    if (board.enpassantPos()) {
        hash ^= enpassantFile[Bitboard::fileOf(board.enpassantPos())];
    }

    for (int i = 0; i < 4; i++) {
        if (board.getCastle(i)) {
            hash ^= castling[i]; 
        }
    }

    for (int i = 0; i < 12; i++) {
        uint64_t curBB = board.getBB(i);
        while (curBB) {
            const int sq = tz_count(curBB);
            pop_lsb(curBB);
            hash ^= randomTable[sq][i];
        }
    }

    return hash;
}

}  // namespace
