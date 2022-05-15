#include "TickDurationHistory.hpp"
#include <fmt/core.h>

namespace utils {
TickDurationHistory::TickDurationHistory() {
    times.reserve(MAX_TIMES);
}

void TickDurationHistory::add_time(const std::chrono::nanoseconds& time) {
    if (times.size() >= MAX_TIMES) {
        times[index++] = time;
        if (index >= MAX_TIMES) {
            index = 0;
        }
    } else {
        times.push_back(time);
    }
}

std::chrono::nanoseconds TickDurationHistory::get_avg_time() const {
    if (times.empty()) {
        return std::chrono::nanoseconds(0);
    }

    // Create a local copy:
    std::vector<std::chrono::nanoseconds> times = this->times;
    std::chrono::nanoseconds sum(0);
    for (const std::chrono::nanoseconds& tickTime : times) {
        sum += tickTime;
    }
    return std::chrono::nanoseconds(sum.count() / times.size());
}

std::string TickDurationHistory::get_avg_time_str() const {
    double ticktime = static_cast<double>(get_avg_time().count());
    std::string unit = "ns";
    if (ticktime >= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::microseconds(1)).count()) {
        if (ticktime >= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(1)).count()) {
            if (ticktime >= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)).count()) {
                ticktime /= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)).count();
                unit = "s";
            } else {
                ticktime /= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(1)).count();
                unit = "ms";
            }
        } else {
            ticktime /= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::microseconds(1)).count();
            unit = "us";
        }
    }
    return fmt::format("{:.2f}{}", ticktime, unit);
}
}  // namespace utils
