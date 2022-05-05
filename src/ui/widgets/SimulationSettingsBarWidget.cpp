#include "SimulationSettingsBarWidget.hpp"
#include <cassert>

namespace ui::widgets {
SimulationSettingsBarWidget::SimulationSettingsBarWidget() : simulator(sim::Simulator::get_instance()) {
    prep_widget();
}

void SimulationSettingsBarWidget::prep_widget() {
    simulateSwitch.property_active().signal_changed().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_simulate_toggled));
    append(simulateSwitch);
}

//-----------------------------Events:-----------------------------
void SimulationSettingsBarWidget::on_simulate_toggled() {
    assert(simulator);
    if (simulateSwitch.get_active()) {
        simulator->continue_simulation();
    } else {
        simulator->pause_simulation();
    }
}
}  // namespace ui::widgets
