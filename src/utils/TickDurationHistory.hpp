#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace utils {
class TickDurationHistory {
 private:
    static constexpr size_t MAX_TIMES = 100;
    size_t index{0};
    std::vector<std::chrono::nanoseconds> times{};

 public:
    TickDurationHistory();

    void add_time(const std::chrono::nanoseconds& time);
    [[nodiscard]] std::chrono::nanoseconds get_avg_time() const;
    [[nodiscard]] std::string get_avg_time_str() const;
};
}  // namespace utils