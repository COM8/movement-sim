#pragma once

#include "Map.hpp"
#include <array>
#include <cstdint>

namespace sim {
// NOLINTNEXTLINE (altera-struct-pack-align) Ignore alignment since we need a compact layout.
struct PushConsts {
    float worldSizeX{0};
    float worldSizeY{0};

    uint32_t nodeCount{0};
    uint32_t maxDepth{0};
    uint32_t entityNodeCap{0};

    float collisionRadius{0};

    uint32_t tick{0};
} __attribute__((packed)) __attribute__((aligned(4)));
}  // namespace sim