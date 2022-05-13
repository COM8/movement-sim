#pragma once

#include "ui/widgets/SimulationOverlayWidget.hpp"
#include "ui/widgets/SimulationSettingsBarWidget.hpp"
#include "ui/widgets/SimulationWidget.hpp"
#include <gtkmm.h>
#include <gtkmm/box.h>
#include <gtkmm/enums.h>
#include <gtkmm/overlay.h>

namespace ui::windows {
class MainWindow : public Gtk::Window {
 private:
    Gtk::Button inspectorBtn;
    widgets::SimulationSettingsBarWidget simulationSettingsBarWidget;
    widgets::SimulationWidget simulationWidget;
    widgets::SimulationOverlayWidget simulationOverlayWidget;
    Gtk::Box mainBox{Gtk::Orientation::VERTICAL};
    Gtk::Overlay simulationOverlay;

 public:
    MainWindow();

 private:
    void prep_window();

    //-----------------------------Events:-----------------------------
    static void on_inspector_btn_clicked();
    bool on_key_pressed(guint keyVal, guint keyCode, Gdk::ModifierType modifier);
};
}  // namespace ui::windows
