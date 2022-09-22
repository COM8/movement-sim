#include "Simulator.hpp"
#include "fall.hpp"
#include "kompute/Manager.hpp"
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
#include <filesystem>
#include <fstream>
#include <iostream>
#include <kompute/operations/OpTensorSyncDevice.hpp>
#include <kompute/operations/OpTensorSyncLocal.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <bits/chrono.h>

#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
#include <renderdoc_app.h>
#endif

namespace sim {
Simulator::Simulator() {
    prepare_log_csv_file();
}

Simulator::~Simulator() {
    assert(logFile);
    logFile->close();
    logFile = nullptr;
}

void Simulator::init() {
    assert(!initialized);

#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
    // Init RenderDoc:
    init_renderdoc();
#endif

    mgr = std::make_shared<kp::Manager>();

    // Load map:
    map = Map::load_from_file("/home/sauter/Documents/Repos/movement-sim/munich.json");

    shader = std::vector(RANDOM_MOVE_COMP_SPV.begin(), RANDOM_MOVE_COMP_SPV.end());

    // Entities:
    add_entities();
    tensorEntities = mgr->tensor(entities->data(), entities->size(), sizeof(Entity), kp::Tensor::TensorDataTypes::eUnsignedInt);

    // Uniform data:
    tensorRoads = mgr->tensor(map->roads.data(), map->roads.size(), sizeof(Road), kp::Tensor::TensorDataTypes::eUnsignedInt);
    tensorConnections = mgr->tensor(map->connections.data(), map->connections.size(), sizeof(unsigned int), kp::Tensor::TensorDataTypes::eUnsignedInt);

    // Quad Tree:
    static_assert(sizeof(gpu_quad_tree::Entity) == sizeof(uint32_t) * 5, "Quad Tree entity size does not match. Expected to be constructed out of 5 uint32_t.");
    quadTreeEntities.resize(MAX_ENTITIES);
    tensorQuadTreeEntities = mgr->tensor(quadTreeEntities.data(), quadTreeEntities.size(), sizeof(gpu_quad_tree::Entity), kp::Tensor::TensorDataTypes::eUnsignedInt);

    assert(gpu_quad_tree::calc_level_count(1) == 1);
    assert(gpu_quad_tree::calc_level_count(2) == 5);
    assert(gpu_quad_tree::calc_level_count(3) == 21);
    assert(gpu_quad_tree::calc_level_count(4) == 85);
    assert(gpu_quad_tree::calc_level_count(8) == 21845);

    quadTreeLevels->resize(gpu_quad_tree::calc_level_count(QUAD_TREE_MAX_DEPTH));
    gpu_quad_tree::init_level_zero((*quadTreeLevels)[0], map->width, map->height);
    tensorQuadTreeLevels = mgr->tensor(quadTreeLevels->data(), quadTreeLevels->size(), sizeof(gpu_quad_tree::Level), kp::Tensor::TensorDataTypes::eUnsignedInt);

    quadTreeLevelUsedStatus.resize(quadTreeLevels->size() + 2);  // +2 since one is used as lock and one as next pointer
    quadTreeLevelUsedStatus[1] = 2;  // Pointer to the first free level index;
    tensorQuadTreeLevelUsedStatus = mgr->tensor(quadTreeLevelUsedStatus.data(), quadTreeLevelUsedStatus.size(), sizeof(uint32_t), kp::Tensor::TensorDataTypes::eUnsignedInt);

    // Debug data:
    std::vector<uint32_t> debugData;
    debugData.resize(10);
    tensorDebugData = mgr->tensor(debugData.data(), debugData.size(), sizeof(uint32_t), kp::Tensor::TensorDataTypes::eUnsignedInt);

    params = {tensorEntities, tensorConnections, tensorRoads, tensorQuadTreeLevels, tensorQuadTreeEntities, tensorQuadTreeLevelUsedStatus, tensorDebugData};

    // Push constants:
    pushConsts.emplace_back();
    pushConsts[0].worldSizeX = map->width;
    pushConsts[0].worldSizeY = map->height;
    pushConsts[0].levelCount = static_cast<uint32_t>(quadTreeLevels->size());
    pushConsts[0].maxDepth = QUAD_TREE_MAX_DEPTH;
    pushConsts[0].entityLevelCap = QUAD_TREE_ENTITY_LEVEL_CAP;
    pushConsts[0].collisionRadius = COLLISION_RADIUS;
    pushConsts[0].tick = 1;

    algo = mgr->algorithm<float, PushConsts>(params, shader, {}, {}, {pushConsts});

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
        std::shared_ptr<kp::Sequence> sendSeq = mgr->sequence()->record<kp::OpTensorSyncDevice>(params);
        sendSeq->eval();
    }

    // Prepare retrieve sequences:
    std::shared_ptr<kp::Sequence> calcSeq = mgr->sequence()->record<kp::OpAlgoDispatch>(algo);
    std::shared_ptr<kp::Sequence> retrieveEntitiesSeq = mgr->sequence()->record<kp::OpTensorSyncLocal>({tensorEntities});
    std::shared_ptr<kp::Sequence> retrieveQuadTreeLevelsSeq = mgr->sequence()->record<kp::OpTensorSyncLocal>({tensorQuadTreeLevels});
    std::shared_ptr<kp::Sequence> retrieveMiscSeq = mgr->sequence()->record<kp::OpTensorSyncLocal>({tensorQuadTreeLevelUsedStatus, tensorQuadTreeEntities, tensorDebugData});

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
    // Update quad tree and move:
    pushConsts[0].tick++;
    uint32_t tick = pushConsts[0].tick;
    SPDLOG_DEBUG("Update tick {} started.", tick);
    std::chrono::high_resolution_clock::time_point updateTickStart = std::chrono::high_resolution_clock::now();
    calcSeq->eval<kp::OpAlgoDispatch>(algo, pushConsts);
    std::chrono::nanoseconds durationUpdate = std::chrono::high_resolution_clock::now() - updateTickStart;
    updateTickHistory.add_time(durationUpdate);
    tick = pushConsts[0].tick;
    SPDLOG_DEBUG("Update tick {} ended.", tick);

    // Update collision detection:
    pushConsts[0].tick++;
    tick = pushConsts[0].tick;
    SPDLOG_DEBUG("Collision detection tick {} started.", tick);
    std::chrono::high_resolution_clock::time_point collisionDetectionTickStart = std::chrono::high_resolution_clock::now();
    calcSeq->eval<kp::OpAlgoDispatch>(algo, pushConsts);
    std::chrono::nanoseconds durationCollisionDetection = std::chrono::high_resolution_clock::now() - collisionDetectionTickStart;
    collisionDetectionTickHistory.add_time(durationCollisionDetection);

    write_log_csv_file(tick, durationUpdate, durationCollisionDetection, durationUpdate + durationCollisionDetection);
    tick = pushConsts[0].tick;
    SPDLOG_DEBUG("Collision detection tick {} ended.", tick);

    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
    end_frame_capture();
#endif

    /*
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
    }

    if (retrievingQuadTreeLevels) {
        retrieveQuadTreeLevelsSeq->evalAwait();
        quadTreeLevels = std::make_shared<std::vector<gpu_quad_tree::Level>>(tensorQuadTreeLevels->vector<gpu_quad_tree::Level>());
    }

    retrieveMiscSeq->evalAwait();
    quadTreeLevelUsedStatus = tensorQuadTreeLevelUsedStatus->vector<uint32_t>();
    quadTreeEntities = tensorQuadTreeEntities->vector<gpu_quad_tree::Entity>();
    std::vector<uint32_t> debugData = tensorDebugData->vector<uint32_t>();*/

    tpsHistory.add_time(std::chrono::high_resolution_clock::now() - tickStart);

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

const utils::TickDurationHistory& Simulator::get_update_tick_history() const {
    return updateTickHistory;
}

const utils::TickDurationHistory& Simulator::get_collision_detection_tick_history() const {
    return collisionDetectionTickHistory;
}

void Simulator::check_device_queues() {
    for (const vk::PhysicalDevice& device : mgr->listDevices()) {
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

const std::filesystem::path& Simulator::get_log_csv_path() {
    static const std::filesystem::path LOG_CSV_PATH{std::to_string(MAX_ENTITIES) + ".csv"};
    return LOG_CSV_PATH;
}

void Simulator::prepare_log_csv_file() {
    assert(!logFile);
    logFile = std::make_unique<std::ofstream>(Simulator::get_log_csv_path(), std::ios::out | std::ios::app);
    assert(logFile->is_open());
    assert(logFile->good());
}

std::string Simulator::get_time_stamp() {
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    std::chrono::days d = std::chrono::duration_cast<std::chrono::days>(tp.time_since_epoch());
    tp -= d;
    std::chrono::hours h = std::chrono::duration_cast<std::chrono::hours>(tp.time_since_epoch());
    tp -= h;
    std::chrono::minutes m = std::chrono::duration_cast<std::chrono::minutes>(tp.time_since_epoch());
    tp -= m;
    std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch());
    tp -= s;
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    tp -= ms;

    std::string msStr = std::to_string(ms.count());
    while (msStr.size() < 3) {
        // NOLINTNEXTLINE (performance-inefficient-string-concatenation)
        msStr = "0" + msStr;
    }

    std::string secStr = std::to_string(s.count());
    while (secStr.size() < 2) {
        // NOLINTNEXTLINE (performance-inefficient-string-concatenation)
        secStr = "0" + secStr;
    }

    std::string minStr = std::to_string(m.count());
    while (msStr.size() < 2) {
        // NOLINTNEXTLINE (performance-inefficient-string-concatenation)
        minStr = "0" + minStr;
    }

    std::string hourStr = std::to_string(h.count());
    while (hourStr.size() < 2) {
        // NOLINTNEXTLINE (performance-inefficient-string-concatenation)
        hourStr = "0" + hourStr;
    }

    return hourStr + ":" + minStr + ":" + secStr + "." + msStr;
}

void Simulator::write_log_csv_file(uint32_t tick, std::chrono::nanoseconds durationUpdate, std::chrono::nanoseconds durationCollision, std::chrono::nanoseconds durationAll) {
    assert(logFile);
    assert(logFile->is_open());
    assert(logFile->good());
    double secUpdate = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(durationUpdate).count()) / 1000;
    double secCollision = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(durationCollision).count()) / 1000;
    double secAll = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(durationAll).count()) / 1000;

    (*logFile) << Simulator::get_time_stamp() << ";" << std::to_string(tick / 2) << ";" << secUpdate << ";" << secCollision << ";" << secAll << "\n";
    std::cerr << Simulator::get_time_stamp() << ";" << std::to_string(tick / 2) << ";" << secUpdate << ";" << secCollision << ";" << secAll << "\n";
    logFile->flush();
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
    int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, (void**) &rdocApi);
    assert(ret == 1);
    SPDLOG_INFO("RenderDoc in application API initialized.");
}

void Simulator::start_frame_capture() {
    assert(rdocApi);
    //NOLINTNEXTLINE (cppcoreguidelines-pro-type-union-access)
    // assert(rdocApi->IsTargetControlConnected() == 1);
    // NOLINTNEXTLINE (google-readability-casting)
    // rdocApi->StartFrameCapture(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(mgr->getVkInstance().get()), nullptr);
    rdocApi->StartFrameCapture(nullptr, nullptr);
    // assert(rdocApi->IsFrameCapturing());
    SPDLOG_INFO("Renderdoc frame capture started.");
}

void Simulator::end_frame_capture() {
    assert(rdocApi);
    //NOLINTNEXTLINE (cppcoreguidelines-pro-type-union-access)
    // assert(rdocApi->IsTargetControlConnected() == 1);
    if (!rdocApi->IsFrameCapturing()) {
        SPDLOG_WARN("Renderdoc no need to end frame capture, no capture running.");
        return;
    }
    // NOLINTNEXTLINE (google-readability-casting)
    // rdocApi->EndFrameCapture(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(mgr->getVkInstance().get()), nullptr);
    rdocApi->EndFrameCapture(nullptr, nullptr);
    // assert(!rdocApi->IsFrameCapturing());
    SPDLOG_INFO("Renderdoc frame capture ended.");
}
#endif
}  // namespace sim