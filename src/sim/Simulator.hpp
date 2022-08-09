#pragma once

#include "GpuQuadTree.hpp"
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

constexpr size_t MAX_ENTITIES = 10000;
constexpr float MAX_RENDER_RESOLUTION_X = 8192;  // Larger values result in errors when creating frame buffers
constexpr float MAX_RENDER_RESOLUTION_Y = 8192;

constexpr size_t QUAD_TREE_MAX_DEPTH = 8;

class Simulator {
 private:
    bool initialized{false};

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

    std::shared_ptr<std::vector<Entity>> entities{std::make_shared<std::vector<Entity>>()};
    std::shared_ptr<kp::Tensor> tensorEntities{nullptr};
    std::shared_ptr<kp::Tensor> tensorConnections{nullptr};
    std::shared_ptr<kp::Tensor> tensorRoads{nullptr};
    std::shared_ptr<kp::Tensor> tensorDebugData{nullptr};

    std::shared_ptr<Map> map{nullptr};

    // -----------------QuadTree-----------------
    std::vector<gpu_quad_tree::Entity> quadTreeEntities;
    std::shared_ptr<std::vector<gpu_quad_tree::Level>> quadTreeLevels{std::make_shared<std::vector<gpu_quad_tree::Level>>()};
    std::vector<uint32_t> quadTreeLevelUsedStatus;

    std::shared_ptr<kp::Tensor> tensorQuadTreeEntities{nullptr};
    std::shared_ptr<kp::Tensor> tensorQuadTreeLevels{nullptr};
    std::shared_ptr<kp::Tensor> tensorQuadTreeLevelUsedStatus{nullptr};
    // ------------------------------------------

#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
    RENDERDOC_API_1_4_2* rdocApi{nullptr};
#endif

 public:
    Simulator() = default;
    ~Simulator() = default;

    Simulator(Simulator&&) = delete;
    Simulator(const Simulator&) = delete;
    Simulator& operator=(Simulator&&) = delete;
    Simulator& operator=(const Simulator&) = delete;

    void init();

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
    std::shared_ptr<std::vector<gpu_quad_tree::Level>> get_quad_tree_levels();
    [[nodiscard]] const std::shared_ptr<Map> get_map() const;

    [[nodiscard]] bool is_initialized() const;

 private:
    void sim_worker();
    void sim_tick(std::shared_ptr<kp::Sequence>& calcSeq, std::shared_ptr<kp::Sequence>& retrieveEntitiesSeq, std::shared_ptr<kp::Sequence>& retrieveQuadTreeLevelsSeq, std::shared_ptr<kp::Sequence>& retrieveMiscSeq);
    void add_entities();
    void check_device_queues();

#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
    void init_renderdoc();
    void start_frame_capture();
    void end_frame_capture();
#endif
};
}  // namespace sim