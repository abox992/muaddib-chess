#ifndef TRANSPOSE_TABLE_H
#define TRANSPOSE_TABLE_H

#include <cassert>
#include <cstdint>
#include <list>
#include <unordered_map>
#include "move.h"

struct TTEntry {
    int depth;
    int eval;
    Move move;

    enum Flags {
        EXACT,
        UPPER,
        LOWER
    };

    Flags flag; 
};

class TranspositionTable {
private:
    std::unordered_map<uint64_t, TTEntry> hashTable;
    std::list<uint64_t> hashes; // head is oldest, tail is newest
    size_t size;
    size_t maxSize;

public:
    TranspositionTable(int maxSize)
        : size(0)
        , maxSize(maxSize)
    {
    }

    inline bool contains(uint64_t key)
    {
        return hashTable.contains(key);
    }

    inline TTEntry get(uint64_t key)
    {
        assert(hashTable.contains(key));
        return hashTable[key];
    }

    inline void removeOldest()
    {
        uint64_t oldestKey = hashes.front();
        hashes.pop_front();
        hashTable.erase(oldestKey);
        size--;
    }

    inline void put(uint64_t key, TTEntry entry)
    {
        size++;

        if (size > maxSize) {
            this->removeOldest();
        }

        hashTable[key] = entry;
        hashes.push_back(key);
    }

    inline size_t getSize() const
    {
        return size;
    }
};

#endif
