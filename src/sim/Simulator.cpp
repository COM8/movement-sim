#include "Simulator.hpp"
#include "fall.hpp"
#include "kompute/Tensor.hpp"
#include "logger/Logger.hpp"
#include "random_move.hpp"
#include "sim/Entity.hpp"
#include "sim/Map.hpp"
#include "spdlog/spdlog.h"
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

Simulator::Simulator() {
#ifdef MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API
    // Init RenderDoc:
    init_renderdoc();
#endif

    // Load map:
    map = Map::load_from_file("/home/fabian/Documents/Repos/movement-sim/munich.json");

    shader = std::vector(RANDOM_MOVE_COMP_SPV.begin(), RANDOM_MOVE_COMP_SPV.end());

    add_entities();
    tensorEntities = mgr.tensor(entities->data(), entities->size(), sizeof(Entity), kp::Tensor::TensorDataTypes::eDouble);
    tensorRoads = mgr.tensor(map->roads.data(), map->roads.size(), sizeof(Road), kp::Tensor::TensorDataTypes::eDouble);
    tensorConnections = mgr.tensorT(map->connections);
    params = {tensorEntities, tensorRoads, tensorConnections};
    algo = mgr.algorithm(params, shader, {}, {}, std::vector<float>{map->width, map->height});
}

void Simulator::add_entities() {
    assert(map);
    entities = std::make_shared<std::vector<Entity>>();
    entities->reserve(MAX_ENTITIES);
    for (size_t i = 1; i <= MAX_ENTITIES; i++) {
        const unsigned int roadIndex = map->get_random_road_index();
        const Road road = map->roads[roadIndex];
        entities->push_back(Entity(Rgb::random_color(),
                                   Vec2(road.start.pos),
                                   Vec2(road.end.pos),
                                   {0, 0},
                                   Entity::random_int(),
                                   false,
                                   roadIndex));
    }
}

std::shared_ptr<Simulator>& Simulator::get_instance() {
    static std::shared_ptr<Simulator> instance = std::make_shared<Simulator>();
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
    assert(state == SimulatorState::STOPPED);
    assert(!simThread);

    SPDLOG_INFO("Starting simulation thread...");
    state = SimulatorState::RUNNING;
    simThread = std::make_unique<std::thread>(&Simulator::sim_worker, this);
}

void Simulator::stop_worker() {
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
    SPDLOG_INFO("Simulation thread started.");

    // Ensure the data is on the GPU:
    std::shared_ptr<kp::Sequence> sendSeq = mgr.sequence()->record<kp::OpTensorSyncDevice>(params);
    sendSeq->evalAsync();
    std::shared_ptr<kp::Sequence> calcSeq = mgr.sequence()->record<kp::OpAlgoDispatch>(algo);
    std::shared_ptr<kp::Sequence> retrieveSeq = mgr.sequence()->record<kp::OpTensorSyncLocal>(params);
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
        /*float posX = (*entities)[0].pos.x;
        float posY = (*entities)[0].pos.y;
        float targetX = (*entities)[0].target.x;
        float targetY = (*entities)[0].target.y;
        float directionX = (*entities)[0].direction.x;
        float directionY = (*entities)[0].direction.y;
        SPDLOG_INFO("Pos: {}/{}, Target: {}/{}, Direction: {}/{}", posX, posY, targetX, targetY, directionX, directionY);*/
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