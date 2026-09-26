#ifndef BOARD_H
#define BOARD_H

#include "bit_array.h"
#include "bit_manip.h"
#include "move.h"
#include "board_state.h"
#include "types.h"
#include <array>
#include <cstdint>
#include <iostream>

class Board {
private:
    // Board state
    std::array<uint64_t, 12> _pieces;
    std::array<uint64_t, 2>  _allPieces;
    uint64_t                 _empty;
    std::array<uint8_t, 64>  _pieceOnSquare;

    uint64_t    _hash;
    uint16_t    _halfMoves;
    uint16_t    _fullMoves;
    BitArray<4> _canCastle;     // WK, WQ, BK, BQ
    uint8_t     _enpassantPos;  // 0 = none
    uint8_t     _blackToMove;

    static constexpr int          kMaxPly = 1024;  // game length plus unbounded quiescence
    std::array<Undo, kMaxPly>     _undoStack;
    std::array<uint64_t, kMaxPly> _keyHistory;
    int                           _curPly = 0;

public:
    Board();
    explicit Board(std::string);
    explicit Board(const Board&);
    Board& operator=(const Board&) = delete;

    // https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
    void set(const std::string fen);

    void setPieceSet(int i, uint64_t num);

    void setStartPos();

    void updateAllPieces();
    void updatePieceOnSquare();

    void makeMove(const Move& move);
    void undoMove();

    bool inCheck() const;

    constexpr inline uint64_t getBB(Color color, PieceType pt) const {
        return _pieces[static_cast<int>(pt) + static_cast<int>(color)];
    }

    template<typename... PieceTypes>
    constexpr inline uint64_t getBB(Color color, PieceType pt, PieceTypes... pts) const {
        return getBB(color, pt) | getBB(color, pts...);
    }

    inline uint64_t getBB(int i) const { return _pieces[i]; }

    template<Color color>
    inline uint64_t getAll() const {
        return _allPieces[color];
    }

    inline uint64_t getAll(Color color) const { return _allPieces[color]; }

    inline int getHalfMoves() const { return _halfMoves; }

    inline int getFullMoves() const { return _fullMoves; }

    int getRepeats(uint64_t hash) const;

    inline uint64_t getEmpty() const { return _empty; }

    inline uint64_t getOccupied() const { return ~_empty; }

    inline int enpassantPos() const { return _enpassantPos; }

    inline bool blackToMove() const { return _blackToMove; }

    inline bool getCastle(const int i) const { return _canCastle[i]; }

    template<Color color>
    inline int kingPos() const {
        return tz_count(_pieces[KINGS + static_cast<int>(color)]);
    }

    inline int kingPos(Color color) { return tz_count(_pieces[KINGS + static_cast<int>(color)]); }

    inline uint64_t hash() const { return _hash; }

    friend std::ostream& operator<<(std::ostream& o, Board& board);
};

#endif
