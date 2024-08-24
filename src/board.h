#ifndef BOARD_H
#define BOARD_H

#include "bit_manip.h"
#include "move.h"
#include "types.h"
#include <cstdint>
#include <iostream>
#include <memory>

class BoardState {
public:
  BoardState() = default;
  BoardState(const BoardState &);
  BoardState &operator=(const BoardState &) = delete;

  // 0 = pawns, 2 = knights, 4 = bishops, 6 = rooks, 8 = queens, 10 = kings
  uint64_t pieces[12];

  uint64_t empty;

  // 0 = white 1 = black
  uint64_t allPieces[2];

  // legal square a pawn can move to to take with enpassant
  int enpassantPos;

  // wk, bk, wq, bq
  bool canCastle[4];

  // 0 = white 1 = black
  bool blackToMove;

  int halfMoves;
  int fullMoves;

  // helpful for making/unmaking moves
  std::unique_ptr<BoardState> prevState;
  uint64_t hash;
  int highestRepeat;
};

class Board {
private:
  std::unique_ptr<BoardState> curState;

public:
  Board();
  explicit Board(std::string fen);
  Board(const Board &) = delete;
  Board &operator=(const Board &) = delete;

  // https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
  void set(const std::string fen);

  void setPieceSet(int i, uint64_t num);

  void setStartPos();

  void updateAllPieces();

  void makeMove(const class Move &move);
  void unmakeMove();

  bool inCheck() const;

  // curState getters
  uint64_t getBB(const int i) const { return curState->pieces[i]; }

  template <Color color> uint64_t getAll() const {
    return curState->allPieces[color];
  }

  uint64_t getAll(Color color) const { return curState->allPieces[color]; }

  int getHalfMoves() const { return curState->halfMoves; }

  int getFullMoves() const { return curState->fullMoves; }

  int getHighestRepeat() const { return curState->highestRepeat; }

  uint64_t getEmpty() const { return curState->empty; }

  uint64_t getOccupied() const { return ~curState->empty; }

  int enpassantPos() const { return curState->enpassantPos; }

  bool blackToMove() const { return curState->blackToMove; }

  bool canCastle(const int i) const { return curState->canCastle[i]; }

  template <Color color> int kingPos() const {
    return tz_count(curState->pieces[Piece::KINGS + static_cast<int>(color)]);
  }

  friend std::ostream &operator<<(std::ostream &o, Board &board);
};

#endif
