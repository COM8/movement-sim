#include "Simulator.hpp"
#include "kompute/Tensor.hpp"
#include "logger/Logger.hpp"
#include "shader/utils/Utils.hpp"
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
    tickTimes.reserve(MAX_TICK_TIMES);
    shader = shaders::utils::load_shader("sim/shader/random_move_struct.spv");

    add_entities();
    tensorEntities = mgr.tensor(entities.data(), entities.size(), sizeof(Entity), kp::Tensor::TensorDataTypes::eInt);

    params = {tensorEntities};
    algo = mgr.algorithm(params, shader, {}, {}, std::vector<float>{WORLD_SIZE_X, WORLD_SIZE_Y});
}

void Simulator::add_entities() {
    for (size_t i = 1; i <= MAX_ENTITIES; i++) {
        entities.push_back(Entity{
            {static_cast<float>(i), static_cast<float>(i)},
            {static_cast<float>(i), static_cast<float>(i)},
            true});
    }
}

std::shared_ptr<Simulator>& Simulator::get_instance() {
    static std::shared_ptr<Simulator> instance = std::make_shared<Simulator>();
    return instance;
}

SimulatorState Simulator::get_state() const {
    return state;
}

const std::vector<Entity>& Simulator::get_entities() const {
    return entities;
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
    std::unique_lock<std::mutex> lk(waitMutex);
    while (state == SimulatorState::RUNNING) {
        if (!simulating) {
            waitCondVar.wait(lk);
        }
        if (!simulating) {
            continue;
        }
        sim_tick();
    }
}

void Simulator::sim_tick() {
    std::chrono::high_resolution_clock::time_point tickStart = std::chrono::high_resolution_clock::now();
    mgr.sequence()
        ->record<kp::OpTensorSyncDevice>(params)
        ->record<kp::OpAlgoDispatch>(algo)
        ->record<kp::OpTensorSyncLocal>(params)
        ->eval();
    std::chrono::nanoseconds sinceLastTick = tickStart - lastTick;
    lastTick = tickStart;

    std::chrono::high_resolution_clock::time_point tickEnd = std::chrono::high_resolution_clock::now();
    add_tick_time(tickEnd - tickStart);

    // TPS counter:
    tpsCounterReset += sinceLastTick;
    tpsCount++;
    if (tpsCounterReset >= std::chrono::seconds(1)) {
        // Ensure we calculate the TPS for exactly on second:
        double tpsCorrectionFactor = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)).count()) / static_cast<double>(tpsCounterReset.count());
        tps = static_cast<double>(tpsCount) * tpsCorrectionFactor;
        tpsCount -= tps;
        tpsCounterReset -= std::chrono::seconds(1);
    }
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

double Simulator::get_tps() const {
    return tps;
}
std::chrono::nanoseconds Simulator::get_avg_tick_time() const {
    if (tickTimes.empty()) {
        return std::chrono::nanoseconds(0);
    }

    // Create a local copy:
    std::vector<std::chrono::nanoseconds> tickTimes = this->tickTimes;
    std::chrono::nanoseconds sum(0);
    for (const std::chrono::nanoseconds& tickTime : tickTimes) {
        sum += tickTime;
    }
    return std::chrono::nanoseconds(sum.count() / tickTimes.size());
}

void Simulator::add_tick_time(const std::chrono::nanoseconds& tickTime) {
    if (tickTimes.size() >= MAX_TICK_TIMES) {
        tickTimes[tickTimesIndex++] = tickTime;
        if (tickTimesIndex >= MAX_TICK_TIMES) {
            tickTimesIndex = 0;
        }
    } else {
        tickTimes.push_back(tickTime);
    }
}
}  // namespace sim