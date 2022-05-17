#pragma once

#include "SimulationWidget.hpp"
#include "sim/Simulator.hpp"
#include <memory>
#include <gtkmm/box.h>
#include <gtkmm/switch.h>

namespace ui::widgets {
class SimulationSettingsBarWidget : public Gtk::Box {
 private:
    SimulationWidget* simWidget{nullptr};

    Gtk::Switch simulateSwitch;
    Gtk::Switch renderSwitch;

    std::shared_ptr<sim::Simulator> simulator{nullptr};

 public:
    explicit SimulationSettingsBarWidget(SimulationWidget* simWidget);

 private:
    void prep_widget();

    //-----------------------------Events:-----------------------------
    void on_simulate_toggled();
    void on_render_toggled();
};
}  // namespace ui::widgets
