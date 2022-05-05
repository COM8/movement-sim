#include "logger/Logger.hpp"
#include "sim/Simulator.hpp"
#include "ui/UiContext.hpp"
#include <memory>

int main(int argc, char** argv) {
    logger::setup_logger(spdlog::level::debug);
    SPDLOG_INFO("Launching Version: {} {}", MOVEMENT_SIMULATOR_VERSION, MOVEMENT_SIMULATOR_VERSION_NAME);

    std::shared_ptr<sim::Simulator> simulator = sim::Simulator::get_instance();
    simulator->start_worker();

    // The UI context manages everything that is UI related.
    // It will return once all windows have been terminated.
    ui::UiContext ui;
    int result = ui.run(argc, argv);

    simulator->stop_worker();
    return result;
}