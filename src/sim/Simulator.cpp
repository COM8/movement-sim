#include "Simulator.hpp"
#include "fall.hpp"
#include "kompute/Tensor.hpp"
#include "logger/Logger.hpp"
#include "random_move.hpp"
#include "sim/Entity.hpp"
#include "sim/GpuQuadTree.hpp"
#include "sim/Map.hpp"
#include "sim/PushConsts.hpp"
#include "spdlog/spdlog.h"
#include "vulkan/vulkan_enums.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <kompute/operations/OpTensorSyncDevice.hpp>
#include <kompute/operations/OpTensorSyncLocal.hpp>
#include <memory>
#include <thread>
#include <vector>

#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
#include <renderdoc_app.h>
#endif

namespace sim {
void Simulator::init() {
    assert(!initialized);

#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
    // Init RenderDoc:
    init_renderdoc();
#endif

    // Load map:
    map = Map::load_from_file("/home/fabian/Documents/Repos/movement-sim/munich.json");

    shader = std::vector(RANDOM_MOVE_COMP_SPV.begin(), RANDOM_MOVE_COMP_SPV.end());

    // Entities:
    add_entities();
    tensorEntities = mgr.tensor(entities->data(), entities->size(), sizeof(Entity), kp::Tensor::TensorDataTypes::eUnsignedInt);

    // Uniform data:
    tensorRoads = mgr.tensor(map->roads.data(), map->roads.size(), sizeof(Road), kp::Tensor::TensorDataTypes::eUnsignedInt);
    tensorConnections = mgr.tensor(map->connections.data(), map->connections.size(), sizeof(unsigned int), kp::Tensor::TensorDataTypes::eUnsignedInt);

    // Quad Tree:
    static_assert(sizeof(gpu_quad_tree::Entity) == sizeof(uint32_t) * 6, "Quad Tree entity size does not match. Expected to be constructed out of 6 uint32_t.");
    quadTreeEntities.resize(MAX_ENTITIES);
    tensorQuadTreeEntities = mgr.tensor(quadTreeEntities.data(), quadTreeEntities.size(), sizeof(gpu_quad_tree::Entity), kp::Tensor::TensorDataTypes::eUnsignedInt);

    assert(gpu_quad_tree::calc_level_count(1) == 1);
    assert(gpu_quad_tree::calc_level_count(2) == 5);
    assert(gpu_quad_tree::calc_level_count(3) == 21);
    assert(gpu_quad_tree::calc_level_count(4) == 85);

    quadTreeLevels->resize(gpu_quad_tree::calc_level_count(QUAD_TREE_MAX_DEPTH));
    gpu_quad_tree::init_level_zero((*quadTreeLevels)[0], map->width, map->height);
    tensorQuadTreeLevels = mgr.tensor(quadTreeLevels->data(), quadTreeLevels->size(), sizeof(gpu_quad_tree::Level), kp::Tensor::TensorDataTypes::eUnsignedInt);

    quadTreeLevelUsedStatus.resize(quadTreeLevels->size() + 2);  // +2 since one is used as lock and one as next pointer
    quadTreeLevelUsedStatus[1] = 2;  // Pointer to the first free level index;
    tensorQuadTreeLevelUsedStatus = mgr.tensor(quadTreeLevelUsedStatus.data(), quadTreeLevelUsedStatus.size(), sizeof(uint32_t), kp::Tensor::TensorDataTypes::eUnsignedInt);

    // Debug data:
    std::vector<uint32_t> debugData;
    debugData.resize(10);
    tensorDebugData = mgr.tensor(debugData.data(), debugData.size(), sizeof(uint32_t), kp::Tensor::TensorDataTypes::eUnsignedInt);

    params = {tensorEntities, tensorConnections, tensorRoads, tensorQuadTreeLevels, tensorQuadTreeEntities, tensorQuadTreeLevelUsedStatus, tensorDebugData};

    // Push constants:
    PushConsts pushConsts{};
    pushConsts.worldSizeX = map->width;
    pushConsts.worldSizeY = map->height;
    pushConsts.levelCount = static_cast<uint32_t>(quadTreeLevels->size());
    pushConsts.maxDepth = QUAD_TREE_MAX_DEPTH;

    algo = mgr.algorithm<float, PushConsts>(params, shader, {}, {}, {pushConsts});

    check_device_queues();

    initialized = true;
}

bool Simulator::is_initialized() const {
    return initialized;
}

void Simulator::add_entities() {
    assert(map);
    entities->reserve(MAX_ENTITIES);
    for (size_t i = 1; i <= MAX_ENTITIES; i++) {
        const unsigned int roadIndex = map->get_random_road_index();
        assert(roadIndex < map->roads.size());
        const Road road = map->roads[roadIndex];
        entities->push_back(Entity(Rgba::random_color(),
                                   Vec4U::random_vec(),
                                   Vec2(road.start.pos),
                                   Vec2(road.end.pos),
                                   {0, 0},
                                   roadIndex,
                                   false));
    }
}

std::shared_ptr<Simulator>& Simulator::get_instance() {
    static std::shared_ptr<Simulator> instance = std::make_shared<Simulator>();
    if (!instance->is_initialized()) {
        instance->init();
    }
    return instance;
}

SimulatorState Simulator::get_state() const {
    return state;
}

std::shared_ptr<std::vector<Entity>> Simulator::get_entities() {
    std::shared_ptr<std::vector<Entity>> result = std::move(entities);
    entities = nullptr;
    return result;
}

std::shared_ptr<std::vector<gpu_quad_tree::Level>> Simulator::get_quad_tree_levels() {
    std::shared_ptr<std::vector<gpu_quad_tree::Level>> result = std::move(quadTreeLevels);
    quadTreeLevels = nullptr;
    return result;
}

const std::shared_ptr<Map> Simulator::get_map() const {
    return map;
}

void Simulator::start_worker() {
    assert(initialized);
    assert(state == SimulatorState::STOPPED);
    assert(!simThread);

    SPDLOG_INFO("Starting simulation thread...");
    state = SimulatorState::RUNNING;
    simThread = std::make_unique<std::thread>(&Simulator::sim_worker, this);
}

void Simulator::stop_worker() {
    assert(initialized);
    assert(state == SimulatorState::RUNNING);
    assert(simThread);

    SPDLOG_INFO("Stopping simulation thread...");
    state = SimulatorState::JOINING;
    waitCondVar.notify_all();
    if (simThread->joinable()) {
        simThread->join();
    }
    simThread = nullptr;
    state = SimulatorState::STOPPED;
    SPDLOG_INFO("Simulation thread stopped.");
}

void Simulator::sim_worker() {
    assert(initialized);
    SPDLOG_INFO("Simulation thread started.");

    // Ensure the data is on the GPU:
    {
        std::shared_ptr<kp::Sequence> sendSeq = mgr.sequence()->record<kp::OpTensorSyncDevice>(params);
        sendSeq->eval();
    }

    // Prepare retrieve sequences:
    std::shared_ptr<kp::Sequence> calcSeq = mgr.sequence()->record<kp::OpAlgoDispatch>(algo);
    std::shared_ptr<kp::Sequence> retrieveEntitiesSeq = mgr.sequence()->record<kp::OpTensorSyncLocal>({tensorEntities});
    std::shared_ptr<kp::Sequence> retrieveQuadTreeLevelsSeq = mgr.sequence()->record<kp::OpTensorSyncLocal>({tensorQuadTreeLevels});
    std::shared_ptr<kp::Sequence> retrieveMiscSeq = mgr.sequence()->record<kp::OpTensorSyncLocal>({tensorQuadTreeLevelUsedStatus, tensorQuadTreeEntities, tensorDebugData});

    std::unique_lock<std::mutex> lk(waitMutex);
    while (state == SimulatorState::RUNNING) {
        if (!simulating) {
            waitCondVar.wait(lk);
        }
        if (!simulating) {
            continue;
        }
        sim_tick(calcSeq, retrieveEntitiesSeq, retrieveQuadTreeLevelsSeq, retrieveMiscSeq);
    }
}

void Simulator::sim_tick(std::shared_ptr<kp::Sequence>& calcSeq, std::shared_ptr<kp::Sequence>& retrieveEntitiesSeq, std::shared_ptr<kp::Sequence>& retrieveQuadTreeLevelsSeq, std::shared_ptr<kp::Sequence>& retrieveMiscSeq) {
    std::chrono::high_resolution_clock::time_point tickStart = std::chrono::high_resolution_clock::now();

#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
    start_frame_capture();
#endif
    calcSeq->eval();
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
    end_frame_capture();
#endif

    bool retrievingEntities = !entities;
    if (retrievingEntities) {
        retrieveEntitiesSeq->evalAsync();
    }

    bool retrievingQuadTreeLevels = !quadTreeLevels;
    if (retrievingQuadTreeLevels) {
        retrieveQuadTreeLevelsSeq->evalAsync();
    }

    retrieveMiscSeq->evalAsync();

    if (retrievingEntities) {
        retrieveEntitiesSeq->evalAwait();
        entities = std::make_shared<std::vector<Entity>>(tensorEntities->vector<Entity>());

        // for (const Entity& e : *entities) {
        //     assert(e.target.x >= 0 && e.target.x <= WORLD_SIZE_X);
        //     assert(e.target.y >= 0 && e.target.y <= WORLD_SIZE_Y);
        // }
        /*assert(!entities->empty());
        for (size_t i = 0; i < 5; i++) {
            float posX = (*entities)[i].pos.x;
            float posY = (*entities)[i].pos.y;
            float targetX = (*entities)[i].target.x;
            float targetY = (*entities)[i].target.y;
            float directionX = (*entities)[i].direction.x;
            float directionY = (*entities)[i].direction.y;
            unsigned int roadIndex = (*entities)[i].roadIndex;
            SPDLOG_INFO("Pos: {}/{}, Target: {}/{}, Direction: {}/{}, Road Index: {}", posX, posY, targetX, targetY, directionX, directionY, roadIndex);
        }*/
    }

    if (retrievingQuadTreeLevels) {
        retrieveQuadTreeLevelsSeq->evalAwait();
        quadTreeLevels = std::make_shared<std::vector<gpu_quad_tree::Level>>(tensorQuadTreeLevels->vector<gpu_quad_tree::Level>());
    }

    retrieveMiscSeq->evalAwait();
    quadTreeLevelUsedStatus = tensorQuadTreeLevelUsedStatus->vector<uint32_t>();
    quadTreeEntities = tensorQuadTreeEntities->vector<gpu_quad_tree::Entity>();
    std::vector<uint32_t> debugData = tensorDebugData->vector<uint32_t>();

    std::chrono::high_resolution_clock::time_point tickEnd = std::chrono::high_resolution_clock::now();
    tpsHistory.add_time(tickEnd - tickStart);

    // TPS counter:
    tps.tick();
}

void Simulator::continue_simulation() {
    if (simulating) {
        return;
    }
    simulating = true;
    waitCondVar.notify_all();
}

void Simulator::pause_simulation() {
    if (!simulating) {
        return;
    }
    simulating = false;
    waitCondVar.notify_all();
}

bool Simulator::is_simulating() const {
    return simulating;
}

const utils::TickRate& Simulator::get_tps() const {
    return tps;
}

const utils::TickDurationHistory& Simulator::get_tps_history() const {
    return tpsHistory;
}

void Simulator::check_device_queues() {
    for (const vk::PhysicalDevice& device : mgr.listDevices()) {
        std::string devInfo = device.getProperties().deviceName;
        devInfo += "\n";
        for (const vk::QueueFamilyProperties2& props : device.getQueueFamilyProperties2()) {
            if (props.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eCompute && props.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
                devInfo += "Number of graphics/compute pipelines: " + std::to_string(props.queueFamilyProperties.queueCount) + "\n";
            } else {
                if (props.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eCompute) {
                    devInfo += "Number of pure compute pipelines: " + std::to_string(props.queueFamilyProperties.queueCount) + "\n";
                }
                if (props.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
                    devInfo += "Number of pure graphics pipelines: " + std::to_string(props.queueFamilyProperties.queueCount) + "\n";
                }
            }
        }
        SPDLOG_INFO("{}", devInfo);
    }
}

#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
void Simulator::init_renderdoc() {
    SPDLOG_INFO("Initializing RenderDoc in application API...");
    void* mod = dlopen("/usr/lib64/renderdoc/librenderdoc.so", RTLD_NOW);
    if (!mod) {
        // NOLINTNEXTLINE (concurrency-mt-unsafe)
        const char* error = dlerror();
        if (error) {
            SPDLOG_ERROR("Failed to find librenderdoc.so with: {}", error);
        } else {
            SPDLOG_ERROR("Failed to find librenderdoc.so with: Unknown error");
        }
        assert(false);
    }

    // NOLINTNEXTLINE (google-readability-casting)
    pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI) dlsym(mod, "RENDERDOC_GetAPI");
    // NOLINTNEXTLINE (google-readability-casting)
    int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_2, (void**) &rdocApi);
    assert(ret == 1);
    SPDLOG_INFO("RenderDoc in application API initialized.");
}

void Simulator::start_frame_capture() {
    assert(rdocApi);
    // NOLINTNEXTLINE (google-readability-casting)
    rdocApi->StartFrameCapture(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(mgr.getVkInstance().get()), nullptr);
}

void Simulator::end_frame_capture() {
    assert(rdocApi);
    // NOLINTNEXTLINE (google-readability-casting)
    rdocApi->EndFrameCapture(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(mgr.getVkInstance().get()), nullptr);
}
#endif
}  // namespace sim