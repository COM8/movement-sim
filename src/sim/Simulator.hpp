#pragma once

#include "sim/Entity.hpp"
#include "utils/TickDurationHistory.hpp"
#include "utils/TickRate.hpp"
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <kompute/Manager.hpp>
#include <memory>
#include <mutex>
#include <sim/Map.hpp>
#include <thread>
#include <type_traits>
#include <vector>

#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
#include <renderdoc_app.h>
#endif

namespace sim {
enum class SimulatorState {
    STOPPED,
    RUNNING,
    JOINING
};

constexpr size_t MAX_ENTITIES = 1000000;
constexpr float MAX_RENDER_RESOLUTION_X = 8192;  // Larger values result in errors when creating frame buffers
constexpr float MAX_RENDER_RESOLUTION_Y = 8192;

class Simulator {
 private:
    std::unique_ptr<std::thread> simThread{nullptr};
    SimulatorState state{SimulatorState::STOPPED};

    std::mutex waitMutex{};
    std::condition_variable waitCondVar{};
    bool simulating{false};

    utils::TickDurationHistory tpsHistory{};
    utils::TickRate tps{};

    kp::Manager mgr{};
    std::vector<uint32_t> shader{};
    std::shared_ptr<kp::Algorithm> algo{nullptr};
    std::vector<std::shared_ptr<kp::Tensor>> params{};

    std::shared_ptr<std::vector<Entity>> entities;
    std::shared_ptr<kp::Tensor> tensorEntities{nullptr};
    std::shared_ptr<kp::Tensor> tensorRoads{nullptr};
    std::shared_ptr<kp::TensorT<unsigned int>> tensorConnections{nullptr};

    std::shared_ptr<Map> map{nullptr};

#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
    RENDERDOC_API_1_4_2* rdocApi{nullptr};
#endif

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
    [[nodiscard]] const utils::TickRate& get_tps() const;
    [[nodiscard]] const utils::TickDurationHistory& get_tps_history() const;
    std::shared_ptr<std::vector<Entity>> get_entities();
    [[nodiscard]] const std::shared_ptr<Map> get_map() const;

 private:
    void sim_worker();
    void sim_tick(std::shared_ptr<kp::Sequence>& sendSeq, std::shared_ptr<kp::Sequence>& calcSeq, std::shared_ptr<kp::Sequence>& retrieveSeq);
    void add_entities();

#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
    void init_renderdoc();
    void start_frame_capture();
    void end_frame_capture();
#endif
};
}  // namespace sim