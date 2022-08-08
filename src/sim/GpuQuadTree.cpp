#include "GpuQuadTree.hpp"
#include <cmath>
#include <cstddef>

namespace sim::gpu_quad_tree {
void init_level_zero(Level& level, float worldSizeX, float worldSizeY) {
    level.width = worldSizeX;
    level.height = worldSizeY;
}

size_t calc_level_count(size_t maxDepth) {
    size_t result = static_cast<size_t>(std::pow(4, maxDepth - 1));
    for (size_t i = 0; i < (maxDepth - 1); i++) {
        result += static_cast<size_t>(std::pow(4, i));
    }
    return result;
}
}  // namespace sim::gpu_quad_tree