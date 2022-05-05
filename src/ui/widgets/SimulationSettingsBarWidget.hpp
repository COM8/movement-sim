#pragma once

#include "sim/Simulator.hpp"
#include <memory>
#include <gtkmm/box.h>
#include <gtkmm/switch.h>

namespace ui::widgets {
class SimulationSettingsBarWidget : public Gtk::Box {
 private:
    Gtk::Switch simulateSwitch;

    std::shared_ptr<sim::Simulator> simulator{nullptr};

 public:
    SimulationSettingsBarWidget();

 private:
    void prep_widget();

    //-----------------------------Events:-----------------------------
    void on_simulate_toggled();
};
}  // namespace ui::widgets
