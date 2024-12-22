#ifndef TRANSPOSE_TABLE_H
#define TRANSPOSE_TABLE_H

#include "move.h"
#include <cassert>
#include <cstdint>
#include <memory>

struct TTEntry {
    uint8_t depth;
    uint16_t eval;
    Move move;

    uint16_t key16;

    uint8_t generation;

    enum Flags : uint8_t {
        EXACT,
        UPPER,
        LOWER
    };

    Flags flag;

    inline constexpr bool isOccupied() { return move.isNull(); }
};

struct TTData {
    int depth;
    int eval;
    Move move;

    TTEntry::Flags flag;
};

static constexpr int clusterSize = 3;

struct Cluster {
    TTEntry entry[clusterSize];

    char padding[1];
};

struct Deleter {
    template <typename T>
    void operator()(T ptr) {
        return std::free(ptr);
    }
};


class TranspositionTable {
private:
    std::unique_ptr<Cluster[], Deleter> table;
    size_t size;
    size_t clusterCount;
    uint8_t curGen;

public:
    TranspositionTable(size_t mbSize);

    bool contains(uint64_t key) {
        TTEntry* cluster = getCluster(key);

        for (int i = 0; i < clusterSize; i++) {
            if (cluster[i].key16 == uint16_t(key)) {
                return true;
            }
        }
        
        return false;
    }

    TTData get(uint64_t key) {
        assert(this->contains(key));

        TTEntry* cluster = getCluster(key);
        TTData data;

        for (int i = 0; i < clusterSize; i++) {
            if (cluster[i].key16 == uint16_t(key)) {
                data.eval = cluster[i].eval;
                data.depth = cluster[i].depth;
                data.flag = cluster[i].flag;
                data.move = cluster[i].move;
                return data;
            }
        }

        assert(false);
        return data; // should never hit 
    }

    void save(uint64_t key, TTEntry entry) {

        TTEntry* cluster = getCluster(key);

        for (int i = 0; i < clusterSize; i++) {
            if (!cluster[i].isOccupied()) {
                cluster[i] = entry;
                size++;
                return;
            }
        }

        // all are full, maybe replace one
        for (int i = 0; i < clusterSize; i++) {
            if (cluster[i].depth < entry.depth || cluster[i].generation + 3 < curGen) {
                entry.generation = curGen;
                cluster[i] = entry;
                return;
            }
        }

        /*cluster[0] = entry;*/
    }

    TTEntry* getCluster(uint64_t key) {
        assert(std::popcount(clusterCount) == 1);
        return &table[key & (clusterCount - 1)].entry[0];
    }

    inline size_t getSize() const {
        return size;
    }

    inline void NewSearch() {
        curGen++;
    }
};

#endif
