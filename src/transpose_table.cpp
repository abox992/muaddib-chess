#include "transpose_table.h"

TranspositionTable::TranspositionTable(size_t mbSize) {
    this->clusterCount = mbSize * 1024 * 1024 / sizeof(Cluster);
    this->size = 0;

    table = std::unique_ptr<Cluster[], AllignedAllocFree>(static_cast<Cluster*>(
        std::aligned_alloc(4096, clusterCount * sizeof(Cluster))));
}
