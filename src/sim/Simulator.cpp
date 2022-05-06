#include "Simulator.hpp"
#include "logger/Logger.hpp"
#include "random_move.hpp"
#include "spdlog/spdlog.h"
#include <cassert>
#include <chrono>
#include <cstddef>
#include <kompute/operations/OpTensorSyncDevice.hpp>
#include <kompute/operations/OpTensorSyncLocal.hpp>
#include <memory>
#include <thread>

namespace sim {

Simulator::Simulator() {
    tickTimes.reserve(MAX_TICK_TIMES);
    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    shader.assign(random_move, random_move + sizeof(random_move));

    tensorInA = mgr.tensor({2.0, 4.0, 6.0});
    tensorInB = mgr.tensor({0.0, 1.0, 2.0});
    tensorOut = mgr.tensor({0.0, 0.0, 0.0});
    params = {tensorInA,
              tensorInB,
              tensorOut};
    mgr.algorithm(params, shader);
}

std::shared_ptr<Simulator>& Simulator::get_instance() {
    static std::shared_ptr<Simulator> instance = std::make_shared<Simulator>();
    return instance;
}

SimulatorState Simulator::get_state() const {
    return state;
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