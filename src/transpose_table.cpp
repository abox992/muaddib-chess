#include "transpose_table.h"

TranspositionTable::TranspositionTable(size_t mbSize) {
    this->clusterCount = mbSize * 1024 * 1024 / sizeof(Cluster);
    this->size         = 0;

    table = std::unique_ptr<Cluster[], Deleter>(
      static_cast<Cluster*>(std::aligned_alloc(4096, clusterCount * sizeof(Cluster))));
}

bool TranspositionTable::contains(uint64_t key) const {
    const TTEntry* cluster = getCluster(key);

    for (int i = 0; i < clusterSize; i++) {
        if (cluster[i].key16 == uint16_t(key)) {
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
        if (cluster[i].key16 == uint16_t(key)) {
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
            cluster[i]       = entry;
            return;
        }
    }

    /*cluster[0] = entry;*/
}

TTEntry* TranspositionTable::getCluster(uint64_t key) {
    assert(std::popcount(clusterCount) == 1);
    return &table[key & (clusterCount - 1)].entry[0];
}

const TTEntry* TranspositionTable::getCluster(uint64_t key) const {
    assert(std::popcount(clusterCount) == 1);
    return &table[key & (clusterCount - 1)].entry[0];
}
