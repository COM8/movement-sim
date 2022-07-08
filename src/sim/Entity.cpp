#include "Entity.hpp"
#include <limits>
#include <random>

namespace sim {
Entity::Entity(Rgba&& color, Vec2&& pos, Vec2&& target, Vec2&& direction, int randSeed, unsigned int roadIndex, bool initialized) : color(color),
                                                                                                                                    pos(pos),
                                                                                                                                    target(target),
                                                                                                                                    direction(direction),
                                                                                                                                    randomSeed(randSeed),
                                                                                                                                    roadIndex(roadIndex),
                                                                                                                                    initialized(initialized) {}

int Entity::random_int() {
    static std::random_device device;
    static std::mt19937 gen(device());
    static std::uniform_int_distribution<int> distr(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    return distr(gen);
}

Vec2 Vec2::random_vec(float x_min, float x_max, float y_min, float y_max) {
    static std::random_device device;
    static std::mt19937 gen(device());
    static std::uniform_real_distribution<float> distr_x(0, x_max);
    static std::uniform_real_distribution<float> distr_y(0, y_max);

    // Reuse existing distributions:
    if (distr_x.a() != x_min || distr_x.b() != x_max) {
        distr_x = std::uniform_real_distribution<float>(x_min, x_max);
    }
    if (distr_y.a() != y_min || distr_y.b() != y_max) {
        distr_y = std::uniform_real_distribution<float>(y_min, y_max);
    }

    return Vec2{distr_x(gen), distr_y(gen)};
}

Rgba Rgba::random_color() {
    static std::random_device device;
    static std::mt19937 gen(device());
    static std::uniform_real_distribution<float> distr(0, 1.0);
    return Rgba{
        distr(gen),
        distr(gen),
        distr(gen)};
}
}  // namespace sim