#include "SimulationSettingsBarWidget.hpp"
#include <cassert>
#include <gtkmm/box.h>
#include <gtkmm/enums.h>

namespace ui::widgets {
SimulationSettingsBarWidget::SimulationSettingsBarWidget(SimulationWidget* simWidget) : Gtk::Box(Gtk::Orientation::HORIZONTAL),
                                                                                        simWidget(simWidget),
                                                                                        simulator(sim::Simulator::get_instance()) {
    prep_widget();
}

void SimulationSettingsBarWidget::prep_widget() {
    simulateSwitch.property_active().signal_changed().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_simulate_toggled));
    simulateSwitch.set_tooltip_text("Enable simulation");
    simulateSwitch.set_margin_start(10);
    append(simulateSwitch);

    renderSwitch.property_active().signal_changed().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_render_toggled));
    renderSwitch.set_active();
    renderSwitch.set_tooltip_text("Enable UI updates");
    renderSwitch.set_margin_start(10);
    append(renderSwitch);
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

void SimulationSettingsBarWidget::on_render_toggled() {
    assert(simWidget);
    simWidget->enableUiUpdates = renderSwitch.get_active();
}
}  // namespace ui::widgets
