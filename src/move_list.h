#include "move.h"
#include "types.h"
#include "movegen.h"
#include "board.h"
#include <vector>

#define MAX_MOVES 256

template <MoveFilter filter>
class MoveList {
private:
    std::array<Move, MAX_MOVES> moveList;

    std::size_t count;

public:
    MoveList(const Board& board) {

        if (board.curState->blackToMove) {
            count = generateMoves<filter, Color::BLACK>(board, moveList.data());
        } else {
            count = generateMoves<filter, Color::WHITE>(board, moveList.data());
        }

    }

    MoveList(const MoveList&) = delete;
    MoveList& operator=(const MoveList&) = delete;

    // I dont think this is needed, shouldnt be altering list after movegen
    // void push_back(const Move& move) { moveList[count++] = move; }

    Move get(const int i) const { return moveList[i]; }

    size_t size() const { return count; }

    Move* begin() { return &moveList[0]; }
    const Move* begin() const { return &moveList[0]; }
    Move* end() { return &moveList[count]; }
    const Move* end() const { return &moveList[count]; }
};