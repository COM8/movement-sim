#pragma once

namespace sim {
struct Vec2 {
    float x{0};
    float y{0};

    static Vec2 random_vec(float x_min, float x_max, float y_min, float y_max);
} __attribute__((aligned(8))) __attribute__((__packed__));

struct Rgb {
    float r{0};
    float g{0};
    float b{0};
} __attribute__((aligned(16))) __attribute__((__packed__));

struct Entity {
    Rgb color{1.0, 0.0, 0.0};
    Vec2 pos_cur{};
    Vec2 pos_prev{};
    Vec2 direction{};
    int rand_seed = {0};
    bool is_valid{false};

    static int random_int();
} __attribute__((aligned(64))) __attribute__((__packed__));
}  // namespace sim