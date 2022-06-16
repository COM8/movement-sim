#pragma once

#include <cstdint>

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
    static Rgb random_color();
} __attribute__((aligned(16))) __attribute__((__packed__));

struct Entity {
    Rgb color{1.0, 0.0, 0.0};
    // float padding1;
    Vec2 pos{};
    Vec2 target{};
    Vec2 direction{};
    int randomSeed = {0};
    bool initialized{false};
    unsigned int roadIndex{};
    // vec3 padding2;

    Entity(Rgb&& color, Vec2&& pos, Vec2&& target, Vec2&& direction, int randSeed, bool initialized, unsigned int roadIndex);

    static int random_int();
} __attribute__((aligned(64))) __attribute__((__packed__));
}  // namespace sim