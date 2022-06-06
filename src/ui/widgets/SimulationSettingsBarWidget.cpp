#include "SimulationSettingsBarWidget.hpp"
#include "sim/Simulator.hpp"
#include "ui/widgets/SimulationWidget.hpp"
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

    zoomInBtn.signal_clicked().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_zoom_in_clicked));
    zoomInBtn.set_tooltip_text("Zoom in");
    zoomInBtn.set_image_from_icon_name("zoom-in");
    zoomInBtn.set_margin_start(10);
    zoomInBtn.set_sensitive(false);
    append(zoomInBtn);

    zoomOutBtn.signal_clicked().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_zoom_out_clicked));
    zoomOutBtn.set_tooltip_text("Zoom out");
    zoomOutBtn.set_image_from_icon_name("zoom-out");
    zoomOutBtn.set_margin_start(10);
    append(zoomOutBtn);

    zoomFitBtn.signal_clicked().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_zoom_fit_clicked));
    zoomFitBtn.set_tooltip_text("Zoom fit");
    zoomFitBtn.set_image_from_icon_name("zoom-fit-best");
    zoomFitBtn.set_margin_start(10);
    append(zoomFitBtn);

    zoomResetBtn.signal_clicked().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_zoom_reset_clicked));
    zoomResetBtn.set_tooltip_text("Zoom reset");
    zoomResetBtn.set_image_from_icon_name("zoom-original");
    zoomResetBtn.set_margin_start(10);
    append(zoomResetBtn);
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

void SimulationSettingsBarWidget::on_zoom_in_clicked() {
    assert(simWidget);
    float zoomFactor = simWidget->get_zoom_factor();
    zoomFactor *= 1.25;
    if (zoomFactor >= 1) {
        zoomFactor = 1;
        zoomInBtn.set_sensitive(false);
    }
    simWidget->set_zoom_factor(zoomFactor);
}

void SimulationSettingsBarWidget::on_zoom_out_clicked() {
    assert(simWidget);
    float zoomFactor = simWidget->get_zoom_factor();
    zoomFactor *= 0.75;
    zoomInBtn.set_sensitive(true);
    simWidget->set_zoom_factor(zoomFactor);
}

void SimulationSettingsBarWidget::on_zoom_reset_clicked() {
    assert(simWidget);
    simWidget->set_zoom_factor(1.0);
    zoomInBtn.set_sensitive(false);
}

void SimulationSettingsBarWidget::on_zoom_fit_clicked() {
    assert(simWidget);
    float zoomFactor = simWidget->get_zoom_factor();
    float zoomX = static_cast<float>(simWidget->get_width()) / sim::MAX_RENDER_RESOLUTION_X;
    float zoomY = static_cast<float>(simWidget->get_height()) / sim::MAX_RENDER_RESOLUTION_Y;
    zoomFactor = zoomX > zoomY ? zoomY : zoomX;
    simWidget->set_zoom_factor(zoomFactor);
    zoomInBtn.set_sensitive(true);
}
}  // namespace ui::widgets
