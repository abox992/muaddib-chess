#include "board_state.h"

// NOTE: THIS DOES NOT COPY PREVSTATE
BoardState::BoardState(const BoardState& source) {
    this->pieces = source.pieces;
    this->allPieces = source.allPieces;
    this->canCastle = source.canCastle;
    this->enpassantPos = source.enpassantPos;
    this->empty = source.empty;
    this->blackToMove = source.blackToMove;
    this->halfMoves = source.halfMoves;
    this->fullMoves = source.fullMoves;
    this->hash = source.hash;
}
