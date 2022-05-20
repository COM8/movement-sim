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

    Gtk::Button zoomInBtn;
    Gtk::Button zoomOutBtn;
    Gtk::Button zoomResetBtn;
    Gtk::Button zoomFitBtn;

    std::shared_ptr<sim::Simulator> simulator{nullptr};

 public:
    explicit SimulationSettingsBarWidget(SimulationWidget* simWidget);

 private:
    void prep_widget();

    //-----------------------------Events:-----------------------------
    void on_simulate_toggled();
    void on_render_toggled();
    void on_zoom_in_clicked();
    void on_zoom_out_clicked();
    void on_zoom_reset_clicked();
    void on_zoom_fit_clicked();
};
}  // namespace ui::widgets
