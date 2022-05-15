#pragma once

#include <chrono>
#include <vector>

namespace utils {
class TickRate {
 private:
    std::chrono::high_resolution_clock::time_point lastTick = std::chrono::high_resolution_clock::now();
    double ticks{0};
    double curTickCount{0};
    std::chrono::nanoseconds tickCounterReset{0};

 public:
    TickRate() = default;

    void tick();
    [[nodiscard]] double get_ticks() const;
};
}  // namespace utils