#pragma once

namespace sim {
struct Vec2 {
    float x{0};
    float y{0};
} __attribute__((aligned(8))) __attribute__((__packed__));

struct Entity {
    Vec2 pos{};
    Vec2 direction{};
    bool isValid{false};
} __attribute__((aligned(32))) __attribute__((__packed__));
}  // namespace sim