#pragma once

#include "sim/Entity.hpp"
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <kompute/Manager.hpp>
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

constexpr size_t MAX_ENTITIES = 10000;
constexpr float WORLD_SIZE_X = 8192;
constexpr float WORLD_SIZE_Y = 8192;

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

    kp::Manager mgr{};
    std::vector<uint32_t> shader{};
    std::shared_ptr<kp::Algorithm> algo{nullptr};
    std::vector<std::shared_ptr<kp::Tensor>> params{};

    std::shared_ptr<std::vector<Entity>> entities;
    std::shared_ptr<kp::Tensor> tensorEntities{nullptr};
    std::shared_ptr<kp::TensorT<float>> tensorEntityVertices{nullptr};
    std::shared_ptr<kp::TensorT<float>> tensorEntityVerticElements{nullptr};

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
    std::shared_ptr<std::vector<Entity>> get_entities();

 private:
    void sim_worker();
    void sim_tick(std::shared_ptr<kp::Sequence>& sendSeq, std::shared_ptr<kp::Sequence>& calcSeq, std::shared_ptr<kp::Sequence>& retriveSeq);
    void add_tick_time(const std::chrono::nanoseconds& tickTime);
    void add_entities();
};
}  // namespace sim