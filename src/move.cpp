#include "move.h"
#include "constants.h"
#include "precompute_masks.h"
#include <cstdint>

std::ostream& operator<<(std::ostream& o, const Move& move) {
    const char file[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

    int fromRank = unsigned(move.from()) / 8;
    int fromFile = 7 - unsigned(move.from()) % 8;

    int toRank = unsigned(move.to()) / 8;
    int toFile = 7 - unsigned(move.to()) % 8;

    // we encode castles as taking our rook, adjust accordingly for print
    if (move.moveType() == MoveType::CASTLE) {
        assert(toFile == 7 || toFile == 0);

        if (toFile == 7) { // kingside
            toFile = 5;
        } else { // queenside
            toFile = 2;
        }

    }

    o << file[fromFile] << (fromRank + 1) << file[toFile] << (toRank + 1);

    if (move.moveType() == MoveType::PROMOTION) {
        switch (move.promotionPiece()) {
            case 0:
                o << "->N";
                break;
            case 1:
                o << "->B";
                break;
            case 2:
                o << "->R";
                break;
            case 3:
                o << "->Q";
                break;
        }

        assert(move.moveType() == MoveType::PROMOTION);
    }

    return o;
}