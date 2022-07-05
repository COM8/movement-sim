#pragma once

#include "Map.hpp"
#include <array>

namespace sim {
struct PushConsts {
    float worldSizeX{0};
    float worldSizeY{0};
    std::array<unsigned int, 289347> connections{};
    std::array<Road, 96449> roads{};
} __attribute__((aligned(128)));
}  // namespace sim