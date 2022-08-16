#include "logger/Logger.hpp"
#include "sim/Simulator.hpp"
#include "ui/UiContext.hpp"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <thread>

bool should_run_headless(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (std::strcmp(argv[i], "--headless") == 0) {
            return true;
        }
    }
    return false;
}

int run_headless() {
    SPDLOG_INFO("Launching Version {} {} in headless mode.", MOVEMENT_SIMULATOR_VERSION, MOVEMENT_SIMULATOR_VERSION_NAME);
    std::shared_ptr<sim::Simulator> simulator = sim::Simulator::get_instance();
    simulator->start_worker();

    simulator->continue_simulation();
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    simulator->stop_worker();
    return EXIT_SUCCESS;
}

int run_ui(int argc, char** argv) {
    SPDLOG_INFO("Launching Version {} {} in UI mode.", MOVEMENT_SIMULATOR_VERSION, MOVEMENT_SIMULATOR_VERSION_NAME);
    std::shared_ptr<sim::Simulator> simulator = sim::Simulator::get_instance();
    simulator->start_worker();

    // The UI context manages everything that is UI related.
    // It will return once all windows have been terminated.
    ui::UiContext ui;
    int result = ui.run(argc, argv);

    simulator->stop_worker();
    return result;
}

int main(int argc, char** argv) {
    logger::setup_logger(spdlog::level::debug);
    bool headless = should_run_headless(argc, argv);

    if (headless) {
        return run_headless();
    }
    return run_ui(argc, argv);
}