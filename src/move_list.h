#ifndef MOVE_LIST_H
#define MOVE_LIST_H

#include "board.h"
#include "move.h"
#include "movegen.h"
#include "see.h"
#include "transpose_table.h"
#include "types.h"
#include <array>
#include <cstdint>

#define MAX_MOVES 256

template<GenType gt>
class MoveList {
private:
    std::array<Move, MAX_MOVES> moveList;
    std::array<int, MAX_MOVES>  scores{};

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

    void sort(const Board& board, const Move& ttableMove, const Move& killer1, const Move& killer2) {
        for (std::size_t i = 0; i < count; ++i) {
            scores[i] = See::moveOrderScore(board, moveList[i], ttableMove, killer1, killer2);
        }

        for (std::size_t i = 1; i < count; ++i) {
            const Move  moving = moveList[i];
            const int   score  = scores[i];
            std::size_t j      = i;
            while (j > 0 && scores[j - 1] < score) {
                moveList[j] = moveList[j - 1];
                scores[j]   = scores[j - 1];
                --j;
            }
            moveList[j] = moving;
            scores[j]   = score;
        }
    }

    int scoreOf(std::size_t i) const { return scores[i]; }

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
