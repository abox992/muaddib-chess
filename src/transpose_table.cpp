#include "transpose_table.h"
#include <cstdint>

TranspositionTable::TranspositionTable(size_t mbSize) {
    this->clusterCount = mbSize * 1024 * 1024 / sizeof(Cluster);
    this->size         = 0;
    this->curGen       = 0;

    table = std::unique_ptr<Cluster[], Deleter>(
      static_cast<Cluster*>(std::aligned_alloc(4096, clusterCount * sizeof(Cluster))));
    std::memset(table.get(), 0, clusterCount * sizeof(Cluster));
    assert(std::popcount(clusterCount) == 1);
}

bool TranspositionTable::contains(uint64_t key) const {
    const TTEntry* cluster = getCluster(key);

    for (int i = 0; i < clusterSize; i++) {
        if (cluster[i].key16 == getKeyTag(key) && cluster[i].isOccupied()) {
            return true;
        }
    }

    return false;
}

TTData TranspositionTable::get(uint64_t key) const {
    assert(this->contains(key));

    const TTEntry* cluster = getCluster(key);
    TTData         data;

    for (int i = 0; i < clusterSize; i++) {
        if (cluster[i].key16 == getKeyTag(key) && cluster[i].isOccupied()) {
            data.eval  = cluster[i].eval;
            data.depth = cluster[i].depth;
            data.flag  = cluster[i].flag;
            data.move  = cluster[i].move;
            return data;
        }
    }

    assert(false);
    return data;  // should never hit
}

void TranspositionTable::save(uint64_t key, TTEntry entry) {

    TTEntry* cluster = getCluster(key);

    entry.key16      = getKeyTag(key);
    entry.generation = curGen;

    // replace existing entry if depth is greater
    for (int i = 0; i < clusterSize; i++) {
        if (cluster[i].key16 == getKeyTag(key) && cluster[i].isOccupied()) {
            if (entry.depth >= cluster[i].depth) {
                cluster[i] = entry;
            }
            return;
        }
    }

    // find empty slot
    for (int i = 0; i < clusterSize; i++) {
        if (!cluster[i].isOccupied()) {
            cluster[i] = entry;
            size++;
            return;
        }
    }

    // all are full, replace one
    size_t replaceIndex = 0;
    for (int i = 0; i < clusterSize; i++) {
        if (cluster[i].depth <= cluster[replaceIndex].depth) {
            if (cluster[i].depth == cluster[replaceIndex].depth) {
                if (cluster[i].generation < cluster[replaceIndex].generation) {
                    replaceIndex = i;
                }
            } else {
                replaceIndex = i;
            }
        }
    }
    cluster[replaceIndex] = entry;
}

uint16_t TranspositionTable::getKeyTag(uint64_t key) const { return static_cast<uint16_t>(key >> 48); }

TTEntry* TranspositionTable::getCluster(uint64_t key) { return &table[key & (clusterCount - 1)].entry[0]; }

const TTEntry* TranspositionTable::getCluster(uint64_t key) const { return &table[key & (clusterCount - 1)].entry[0]; }
