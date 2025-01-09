#ifndef TRANSPOSE_TABLE_H
#define TRANSPOSE_TABLE_H

#include "move.h"
#include <cassert>
#include <cstdint>
#include <memory>

struct TTEntry {
    uint8_t depth;
    int16_t eval;
    Move move;

    uint16_t key16;

    uint8_t generation;

    enum Flags : uint8_t {
        EXACT,
        UPPER,
        LOWER
    };

    Flags flag;

    inline constexpr bool isOccupied() { return !move.isNull(); }
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

    bool contains(uint64_t key) const;
    TTData get(uint64_t key) const;
    void save(uint64_t key, TTEntry entry);
    const TTEntry* getCluster(uint64_t key) const;
    TTEntry* getCluster(uint64_t key);

    inline size_t getSize() const {
        return size;
    }

    inline void NewSearch() {
        curGen++;
    }
};

#endif
