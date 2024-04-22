#include "move.h"

std::ostream& operator<<(std::ostream& o, const Move& move) {
    char file[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

    int fromRank = unsigned(move.from) / 8;
    int fromFile = 7 - unsigned(move.from) % 8;

    int toRank = unsigned(move.to) / 8;
    int toFile = 7 - unsigned(move.to) % 8;

    o << file[fromFile] << (fromRank + 1) << file[toFile] << (toRank + 1);

    if (move.promotion != 0) {
        switch (squareOf(move.promotion)) {
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

        assert(move.promotion != 0);
    }

    return o;
}