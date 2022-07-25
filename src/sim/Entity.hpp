#pragma once

#include <cstdint>

namespace sim {
struct Vec2 {
    float x{0};
    float y{0};

    [[nodiscard]] double dist(const Vec2& other) const;

    static Vec2 random_vec(float x_min, float x_max, float y_min, float y_max);
} __attribute__((aligned(8))) __attribute__((__packed__));

struct Vec4U {
    unsigned int x{0};
    unsigned int y{0};
    unsigned int z{0};
    unsigned int w{0};

    static Vec4U random_vec();
} __attribute__((aligned(16))) __attribute__((__packed__));

struct Rgba {
    float r{0};
    float g{0};
    float b{0};
    float a{0};

    static Rgba random_color();
} __attribute__((aligned(16))) __attribute__((__packed__));

struct Entity {
    Rgba color{1.0, 0.0, 0.0, 1.0};
    Vec4U randomState{};
    Vec2 pos{};
    Vec2 target{};
    Vec2 direction{};
    unsigned int roadIndex{};
    bool initialized{false};

 public:
    Entity(Rgba&& color, Vec4U&& randomState, Vec2&& pos, Vec2&& target, Vec2&& direction, unsigned int roadIndex, bool initialized);

    static int random_int();
} __attribute__((aligned(64))) __attribute__((__packed__));
}  // namespace sim