#include "transpose_table.h"
#include <memory>

TranspositionTable::TranspositionTable(size_t mbSize) {
    maxSize = mbSize * 1024 * 1024 / sizeof(Cluster);

    table = std::make_unique<Cluster[]>(maxSize * sizeof(Cluster));
    //table = std::unique_ptr<Cluster, decltype(&std::free)>(static_cast<Cluster*>(std::aligned_alloc(64, maxSize * sizeof(Cluster))), &std::free);
}
