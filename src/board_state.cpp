#include "board_state.h"
#include <cstring>

// NOTE: THIS DOES NOT COPY PREVSTATE
BoardState::BoardState(const BoardState& copy) {
    memcpy(this->pieces, copy.pieces, sizeof(uint64_t) * (std::end(copy.pieces) - std::begin(copy.pieces)));
    memcpy(this->allPieces, copy.allPieces, sizeof(uint64_t) * (std::end(copy.allPieces) - std::begin(copy.allPieces)));
    memcpy(this->canCastle, copy.canCastle, sizeof(bool) * (std::end(copy.canCastle) - std::begin(copy.canCastle)));
    this->enpassantPos = copy.enpassantPos;
    this->empty = copy.empty;
    this->blackToMove = copy.blackToMove;
    this->halfMoves = copy.halfMoves;
    this->fullMoves = copy.fullMoves;

    // this->prevState = copy.prevState;
    this->hash = copy.hash;
    //this->highestRepeat = copy.highestRepeat;
}
