#include "GpuQuadTree.hpp"

namespace sim::gpu_quad_tree {
void init_level_zero(Level& level, float worldSizeX, float worldSizeY) {
    level.width = worldSizeX;
    level.height = worldSizeY;
}
}  // namespace sim::gpu_quad_tree