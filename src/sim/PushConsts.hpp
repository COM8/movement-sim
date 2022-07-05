#pragma once

#include "Map.hpp"
#include <array>

namespace sim {
struct PushConsts {
    float worldSizeX{0};
    float worldSizeY{0};
} __attribute__((aligned(8)));
}  // namespace sim