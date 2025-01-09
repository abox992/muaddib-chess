#ifndef MOVE_LIST_H
#define MOVE_LIST_H

#include "board.h"
#include "move.h"
#include "movegen.h"
#include "transpose_table.h"
#include "types.h"
#include <array>
#include <cstdint>

#define MAX_MOVES 256

template<GenType gt>
class MoveList {
private:
    std::array<Move, MAX_MOVES> moveList;

    std::size_t count;

public:
    MoveList(const Board& board) {
        if (board.blackToMove()) {
            count = generateAllMoves<gt, BLACK>(board, moveList.data());
        } else {
            count = generateAllMoves<gt, WHITE>(board, moveList.data());
        }
    }

    MoveList(const Board& board, const Color color) {
        if (color == Color::BLACK) {
            count = generateAllMoves<gt, BLACK>(board, moveList.data());
        } else {
            count = generateAllMoves<gt, WHITE>(board, moveList.data());
        }
    }

    void sort(Board& board, const std::vector<Move>& pv, const TranspositionTable& ttable) {
        auto swapIter = moveList.begin();

        // pv moves
        for (auto pvIter = pv.cbegin(); pvIter != pv.cend(); pvIter++) {
            auto iter = std::find(moveList.begin(), moveList.end(), *pvIter);
            if (iter != moveList.end()) {
                std::iter_swap(iter, swapIter);
                swapIter++;
            }
        }

        // hash moves
        /*for (auto iter = swapIter; iter != moveList.end(); iter++) {*/
        /*    board.makeMove(*iter);*/
        /*    uint64_t hash = board.hash();*/
        /*    board.undoMove();*/
        /*    if (ttable.contains(hash)) {*/
        /*        std::iter_swap(iter, swapIter);*/
        /*        swapIter++;*/
        /*    }*/
        /*}*/
    }

    MoveList(const MoveList&)            = delete;
    MoveList& operator=(const MoveList&) = delete;

    Move get(const int i) const { return moveList[i]; }

    size_t size() const { return count; }

    Move*       begin() { return &moveList[0]; }
    const Move* cbegin() const { return &moveList[0]; }
    Move*       end() { return &moveList[count]; }
    const Move* cend() const { return &moveList[count]; }
};

#endif
