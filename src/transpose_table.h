#ifndef TRANSPOSE_TABLE_H
#define TRANSPOSE_TABLE_H

#include <cstdint>
#include <unordered_map>
#include "move.h"
#include "zobrist.h"

struct TpEntry {
    int depth;
    int eval;
    Move bestMove;
};

class TranspositionTable {
private:
    std::unordered_map<uint64_t, TpEntry> hashTable;
public:
    TranspositionTable() = default;

    bool contains(uint64_t key) {
        return hashTable.contains(key);
    }

    TpEntry get(uint64_t key) {
        return hashTable[key];
    }

    void put(const Board& board, int depth, int eval, Move bestMove) {
        TpEntry e;
        e.depth = depth;
        e.eval = eval;
        e.bestMove = bestMove;
        hashTable[zhash(*board.curState)] = e;
    }
};

#endif