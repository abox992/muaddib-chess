#include "move.h"
#include <sstream>

std::string toString(Move move) {
    std::stringstream s; 
    s << move;
    return s.str();
}

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
            toFile = 6;
        } else { // queenside
            toFile = 2;
        }

    }

    o << file[fromFile] << (fromRank + 1) << file[toFile] << (toRank + 1);

    if (move.moveType() == MoveType::PROMOTION) {
        switch (move.promotionPiece()) {
            case 0:
                o << "n";
                break;
            case 1:
                o << "b";
                break;
            case 2:
                o << "r";
                break;
            case 3:
                o << "q";
                break;
        }

        assert(move.moveType() == MoveType::PROMOTION);
    }

    return o;
}
