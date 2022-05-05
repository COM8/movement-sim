#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

namespace sim {
enum class SimulatorState {
    STOPPED,
    RUNNING,
    JOINING
};

class Simulator {
 private:
    std::unique_ptr<std::thread> simThread{nullptr};
    SimulatorState state{SimulatorState::STOPPED};

    std::mutex waitMutex{};
    std::condition_variable waitCondVar{};
    bool simulating{false};

    std::chrono::high_resolution_clock::time_point lastTick = std::chrono::high_resolution_clock::now();
    double tps{0};
    double tpsCount{0};
    std::chrono::nanoseconds tpsCounterReset{0};
    static constexpr size_t MAX_TICK_TIMES = 100;
    size_t tickTimesIndex{0};
    std::vector<std::chrono::nanoseconds> tickTimes{};

 public:
    Simulator();
    ~Simulator() = default;

    Simulator(Simulator&&) = delete;
    Simulator(const Simulator&) = delete;
    Simulator& operator=(Simulator&&) = delete;
    Simulator& operator=(const Simulator&) = delete;

    static std::shared_ptr<Simulator>& get_instance();
    [[nodiscard]] SimulatorState get_state() const;
    void start_worker();
    void stop_worker();

    void continue_simulation();
    void pause_simulation();
    [[nodiscard]] bool is_simulating() const;
    [[nodiscard]] double get_tps() const;
    [[nodiscard]] std::chrono::nanoseconds get_avg_tick_time() const;

 private:
    void sim_worker();
    void sim_tick();
    void add_tick_time(const std::chrono::nanoseconds& tickTime);
};
}  // namespace sim