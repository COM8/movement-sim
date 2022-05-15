#include "TickRate.hpp"

namespace utils {
void TickRate::tick() {
    std::chrono::nanoseconds sinceLastTick = std::chrono::high_resolution_clock::now() - lastTick;
    lastTick = std::chrono::high_resolution_clock::now();

    tickCounterReset += sinceLastTick;
    curTickCount++;
    if (tickCounterReset >= std::chrono::seconds(1)) {
        // Ensure we calculate the TPS for exactly on second:
        double tpsCorrectionFactor = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)).count()) / static_cast<double>(tickCounterReset.count());
        ticks = static_cast<double>(curTickCount) * tpsCorrectionFactor;
        curTickCount -= ticks;
        tickCounterReset -= std::chrono::seconds(1);
    }
}

double TickRate::get_ticks() const {
    return ticks;
}
}  // namespace utils
