#include "Simulator.hpp"
#include "fall.hpp"
#include "kompute/Tensor.hpp"
#include "logger/Logger.hpp"
#include "random_move.hpp"
#include "sim/Entity.hpp"
#include "sim/Map.hpp"
#include "sim/PushConsts.hpp"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
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
    tensorEntities = mgr.tensor(entities->data(), entities->size(), sizeof(Entity), kp::Tensor::TensorDataTypes::eDouble);

    // Uniform data:
    tensorRoads = mgr.tensor(map->roads.data(), map->roads.size(), sizeof(Road), kp::Tensor::TensorDataTypes::eDouble);
    tensorConnections = mgr.tensor(map->connections.data(), map->connections.size(), sizeof(unsigned int), kp::Tensor::TensorDataTypes::eUnsignedInt);

    params = {tensorEntities, tensorConnections, tensorRoads};

    // Push constants:
    PushConsts pushConsts{};
    pushConsts.worldSizeX = map->width;
    pushConsts.worldSizeY = map->height;

    algo = mgr.algorithm<float, PushConsts>(params, shader, {}, {}, {pushConsts});

    initialized = true;
}

bool Simulator::is_initialized() const {
    return initialized;
}

void Simulator::add_entities() {
    assert(map);
    entities = std::make_shared<std::vector<Entity>>();
    entities->reserve(MAX_ENTITIES);
    for (size_t i = 1; i <= MAX_ENTITIES; i++) {
        const unsigned int roadIndex = map->get_random_road_index();
        assert(roadIndex < map->roads.size());
        const Road road = map->roads[roadIndex];
        entities->push_back(Entity(Rgba::random_color(),
                                   Vec2(road.start.pos),
                                   Vec2(road.end.pos),
                                   {0, 0},
                                   Entity::random_int(),
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
    std::shared_ptr<kp::Sequence> sendSeq = mgr.sequence()->record<kp::OpTensorSyncDevice>(params);
    sendSeq->evalAsync();
    std::shared_ptr<kp::Sequence> calcSeq = mgr.sequence()->record<kp::OpAlgoDispatch>(algo);
    std::shared_ptr<kp::Sequence> retrieveSeq = mgr.sequence()->record<kp::OpTensorSyncLocal>({tensorEntities});
    sendSeq->evalAwait();

    // Make sure we have started receiving once:
    retrieveSeq->evalAsync();

    std::unique_lock<std::mutex> lk(waitMutex);
    while (state == SimulatorState::RUNNING) {
        if (!simulating) {
            waitCondVar.wait(lk);
        }
        if (!simulating) {
            continue;
        }
        sim_tick(sendSeq, calcSeq, retrieveSeq);
    }
}

void Simulator::sim_tick(std::shared_ptr<kp::Sequence>& /*sendSeq*/, std::shared_ptr<kp::Sequence>& calcSeq, std::shared_ptr<kp::Sequence>& retrieveSeq) {
    std::chrono::high_resolution_clock::time_point tickStart = std::chrono::high_resolution_clock::now();

#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
    start_frame_capture();
#endif
    calcSeq->eval();
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
    end_frame_capture();
#endif
    if (!entities) {
        retrieveSeq->evalAwait();
        entities = std::make_shared<std::vector<Entity>>(tensorEntities->vector<Entity>());
        retrieveSeq->evalAsync();
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
            int randSeed = (*entities)[i].randomSeed;
            SPDLOG_INFO("Pos: {}/{}, Target: {}/{}, Direction: {}/{}, Road Index: {}, Rand Seed: {}", posX, posY, targetX, targetY, directionX, directionY, roadIndex, randSeed);
        }*/
    }

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