#pragma once

#include <cstdint>
#include <vector>

namespace sim::gpu_quad_tree {
enum class NextType : uint32_t {
    INVALID = 0,
    LEVEL = 1,
    ENTITY = 2
};

// NOLINTNEXTLINE (altera-struct-pack-align) Ignore alignment since we only need it for size calculation.
struct Level {
    NextType typeTL{NextType::INVALID};
    uint32_t firstTL{0};
    uint32_t countTL{0};

    NextType typeTR{NextType::INVALID};
    uint32_t firstTR{0};
    uint32_t countTR{0};

    NextType typeBL{NextType::INVALID};
    uint32_t firstBL{0};
    uint32_t countBL{0};

    NextType typeBR{NextType::INVALID};
    uint32_t firstBR{0};
    uint32_t countBR{0};

    float offsetX{0};
    float offsetY{0};
    float width{0};
    float height{0};

    uint32_t locked{0};
} __attribute__((aligned(4)));

// NOLINTNEXTLINE (altera-struct-pack-align) Ignore alignment since we only need it for size calculation.
struct Entity {
    uint32_t index{0};
    uint32_t locked{0};

    NextType typeNext{NextType::INVALID};
    uint32_t next{0};

    NextType typePrev{NextType::INVALID};
    uint32_t prev{0};
} __attribute__((aligned(4)));

void init_level_zero(Level& level, float worldSizeX, float worldSizeY);
}  // namespace sim::gpu_quad_tree