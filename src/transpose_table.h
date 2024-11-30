#ifndef TRANSPOSE_TABLE_H
#define TRANSPOSE_TABLE_H

#include "bit_manip.h"
#include "move.h"
#include <cassert>
#include <cstdint>
#include <list>
#include <memory>
#include <sys/types.h>
#include <unordered_map>

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

static constexpr int clusterSize = 3;

struct Cluster {
    TTEntry entry[clusterSize];
};

struct AllignedAllocFree {
    template <typename T>
    void operator() (T ptr) {
        return std::free(ptr);
    }
};

class TranspositionTable {
private:
    std::unique_ptr<Cluster[], AllignedAllocFree> table;
    std::unordered_map<uint64_t, TTEntry> hashTable;
    std::list<uint64_t> hashes; // head is oldest, tail is newest
    size_t size;
    size_t maxSize;

public:
    TranspositionTable(size_t mbSize);

    inline bool contains(uint64_t key) {
        return hashTable.contains(key);
    }

    inline TTEntry get(uint64_t key) {
        assert(hashTable.contains(key));
        return hashTable[key];
    }

    inline void removeOldest() {
        assert(hashes.size() > 0);
        uint64_t oldestKey = hashes.front();
        hashes.pop_front();
        hashTable.erase(oldestKey);
        size--;
    }


    void save(uint64_t key, TTEntry entry) {
        size++;


    }

    /*TTEntry* firstEntry(uint64_t key) {*/
    /**/
    /*}*/

    inline void put(uint64_t key, TTEntry entry) {
        size++;

        if (size > maxSize) {
            this->removeOldest();
        }

        hashTable[key] = entry;
        hashes.push_back(key);
    }

    inline size_t getSize() const {
        return size;
    }

};

#endif
