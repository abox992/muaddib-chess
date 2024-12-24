#ifndef MOVE_LIST_H
#define MOVE_LIST_H

#include "board.h"
#include "move.h"
#include "movegen.h"
#include "types.h"
#include <array>

#define MAX_MOVES 256

template<GenType gt>
class MoveList {
private:
    std::array<Move, MAX_MOVES> moveList;

    std::size_t count;

public:
    MoveList(const Board& board) {

        if (board.blackToMove()) {
            count = generateAllMoves<gt, Color::BLACK>(board, moveList.data());
        } else {
            count = generateAllMoves<gt, Color::WHITE>(board, moveList.data());
        }
    }

    MoveList(const Board& board, const Color color) {

        if (color == Color::BLACK) {
            count = generateAllMoves<gt, Color::BLACK>(board, moveList.data());
        } else {
            count = generateAllMoves<gt, Color::WHITE>(board, moveList.data());
        }
    }

    void sort(const Board& board) {
        size_t next = 0;
        for (size_t i = 0; i < count; i++) {
            if (maskForPos(moveList[i].to()) && board.getOccupied()) {
                Move temp      = moveList[next];
                moveList[next] = moveList[i];
                moveList[i]    = temp;

                if (i != next) {
                    i--;
                }
                next++;
            }
        }
    }

    MoveList(const MoveList&)            = delete;
    MoveList& operator=(const MoveList&) = delete;

    // I dont think this is needed, shouldnt be altering list after movegen
    // void push_back(const Move& move) { moveList[count++] = move; }

    Move get(const int i) const { return moveList[i]; }

    size_t size() const { return count; }

    Move*       begin() { return &moveList[0]; }
    const Move* begin() const { return &moveList[0]; }
    Move*       end() { return &moveList[count]; }
    const Move* end() const { return &moveList[count]; }
};

#endif
