#include "transpose_table.h"


TranspositionTable::TranspositionTable(size_t mbSize) {
    this->maxSize = mbSize * 1024 * 1024 / sizeof(TTEntry);
    this->size = 0;

    this->hashTable.reserve(maxSize);
    this->hashTable.rehash(maxSize / 4);
    this->hashTable.max_load_factor(0.9);

    table = std::unique_ptr<Cluster[], AllignedAllocFree>(static_cast<Cluster*>(std::aligned_alloc(4096, maxSize * sizeof(Cluster))));
}
