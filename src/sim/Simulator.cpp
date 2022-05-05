#include "Simulator.hpp"
#include "logger/Logger.hpp"
#include "spdlog/spdlog.h"
#include <cassert>
#include <chrono>
#include <memory>
#include <thread>

namespace sim {
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
    if (simThread->joinable()) {
        simThread->join();
    }
    simThread = nullptr;
    state = SimulatorState::STOPPED;
    SPDLOG_INFO("Simulation thread stopped.");
}

void Simulator::sim_worker() {
    SPDLOG_INFO("Simulation thread started.");
    while (state == SimulatorState::RUNNING) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
}  // namespace sim