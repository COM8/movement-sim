#include "GpuQuadTree.hpp"
#include <cmath>
#include <cstddef>

namespace sim::gpu_quad_tree {
void init_node_zero(Node& node, float worldSizeX, float worldSizeY) {
    node.width = worldSizeX;
    node.height = worldSizeY;
    node.contentType = NextType::ENTITY;
}

size_t calc_node_count(size_t maxDepth) {
    size_t result = 0;
    for (size_t i = 0; i < maxDepth; i++) {
        result += static_cast<size_t>(std::pow(4, i));
    }
    return result;
}
}  // namespace sim::gpu_quad_tree