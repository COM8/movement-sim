#pragma once

namespace sim {
struct Vec2 {
    float x{0};
    float y{0};

    static Vec2 random_vec(float x_min, float x_max, float y_min, float y_max);
} __attribute__((aligned(8))) __attribute__((__packed__));

struct Entity {
    Vec2 pos{};
    Vec2 direction{};
    int rand_seed = {0};
    bool is_valid{false};

    static int random_int();
} __attribute__((aligned(32))) __attribute__((__packed__));
}  // namespace sim