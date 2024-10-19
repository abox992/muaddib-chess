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

    uint16_t lower16;
};



struct Cluster {
    static constexpr int clusterSize = 3;
    TTEntry entries[clusterSize];

    char padding[4];
};

class TranspositionTable {
private:
    std::unique_ptr<Cluster[]> table;
    std::unordered_map<uint64_t, TTEntry> hashTable;
    std::list<uint64_t> hashes; // head is oldest, tail is newest
    size_t size;
    size_t maxSize;

public:
    TranspositionTable(size_t mbSize);

    inline bool contains(uint64_t key) {
        //return hashTable.contains(key);

        TTEntry* firstEntry = getFirstEntry(key);

        for (int i = 0; i < Cluster::clusterSize; i++) {
            if (firstEntry[i].lower16 == uint16_t(key)) {
                return true; 
            }
        }

        return false; 
    }

    inline TTEntry get(uint64_t key) {
        // assert(hashTable.contains(key));
        return hashTable[key];
    }

    inline void removeOldest() {
        uint64_t oldestKey = hashes.front();
        hashes.pop_front();
        hashTable.erase(oldestKey);
        size--;
    }

    inline void put(uint64_t key, TTEntry entry) {
        size++;

        if (size > maxSize) {
            this->removeOldest();
        }

        hashTable[key] = entry;
        hashes.push_back(key);
    }

    void save(uint64_t key, TTEntry entry) {

        TTEntry* firstEntry = getFirstEntry(key);

        for (int i = 0; i < Cluster::clusterSize; i++) {
            if (firstEntry[i].lower16 == uint16_t(key)) {
                firstEntry[i] = entry;
            }
        }

    }

    TTEntry probe(uint64_t key) {

        assert(this->contains(key));
        
        TTEntry* firstEntry = getFirstEntry(key);

        for (int i = 0; i < Cluster::clusterSize; i++) {
            if (firstEntry[i].lower16 == uint16_t(key)) {
                return firstEntry[i];
            }
        }

        return TTEntry();
    }

    inline size_t getSize() const {
        return size;
    }

    inline TTEntry* getFirstEntry(uint64_t key) {
        return &table.get()[mulhi64(key, maxSize)].entries[0];
    }
};

#endif
