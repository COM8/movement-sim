#pragma once

#include <memory>
#include <thread>

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

 public:
    Simulator() = default;
    ~Simulator() = default;

    Simulator(Simulator&&) = delete;
    Simulator(const Simulator&) = delete;
    Simulator& operator=(Simulator&&) = delete;
    Simulator& operator=(const Simulator&) = delete;

    static std::shared_ptr<Simulator>& get_instance();
    [[nodiscard]] SimulatorState get_state() const;
    void start_worker();
    void stop_worker();

 private:
    void sim_worker();
};
}  // namespace sim