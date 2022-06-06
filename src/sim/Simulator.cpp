#include "Simulator.hpp"
#include "fall.hpp"
#include "kompute/Tensor.hpp"
#include "logger/Logger.hpp"
#include "random_move.hpp"
#include "sim/Entity.hpp"
#include "spdlog/spdlog.h"
#include <cassert>
#include <chrono>
#include <cstddef>
#include <kompute/operations/OpTensorSyncDevice.hpp>
#include <kompute/operations/OpTensorSyncLocal.hpp>
#include <memory>
#include <thread>
#include <vector>

namespace sim {

Simulator::Simulator() {
    // Load map:
    map = Map::load_from_file("/home/fabian/Documents/Repos/movement-sim/munich.json");

    shader = std::vector(RANDOM_MOVE_COMP_SPV.begin(), RANDOM_MOVE_COMP_SPV.end());

    add_entities();
    tensorEntities = mgr.tensor(entities->data(), entities->size(), sizeof(Entity), kp::Tensor::TensorDataTypes::eDouble);

    params = {tensorEntities};
    algo = mgr.algorithm(params, shader, {}, {}, std::vector<float>{map->width, map->height});
}

void Simulator::add_entities() {
    assert(map);
    entities = std::make_shared<std::vector<Entity>>();
    entities->reserve(MAX_ENTITIES);
    for (size_t i = 1; i <= MAX_ENTITIES; i++) {
        entities->push_back(Entity(Rgb::random_color(),
                                   Vec2::random_vec(0, map->width, 0, map->height),
                                   Vec2::random_vec(0, map->width, 0, map->height),
                                   {0, 0},
                                   Entity::random_int(),
                                   false));
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
    std::shared_ptr<kp::Sequence> retriveSeq = mgr.sequence()->record<kp::OpTensorSyncLocal>(params);
    sendSeq->evalAwait();

    // Make sure we have started receiving once:
    retriveSeq->evalAsync();

    std::unique_lock<std::mutex> lk(waitMutex);
    while (state == SimulatorState::RUNNING) {
        if (!simulating) {
            waitCondVar.wait(lk);
        }
        if (!simulating) {
            continue;
        }
        sim_tick(sendSeq, calcSeq, retriveSeq);
    }
}

void Simulator::sim_tick(std::shared_ptr<kp::Sequence>& /*sendSeq*/, std::shared_ptr<kp::Sequence>& calcSeq, std::shared_ptr<kp::Sequence>& retriveSeq) {
    std::chrono::high_resolution_clock::time_point tickStart = std::chrono::high_resolution_clock::now();

    calcSeq->eval();
    if (!entities) {
        retriveSeq->evalAwait();
        entities = std::make_shared<std::vector<Entity>>(tensorEntities->vector<Entity>());
        retriveSeq->evalAsync();
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
}  // namespace sim