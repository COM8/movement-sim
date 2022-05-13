#include "MainWindow.hpp"
#include <cassert>
#include <gtkmm/window.h>

namespace ui::windows {
MainWindow::MainWindow() { prep_window(); }

void MainWindow::prep_window() {
    set_title("Movement Simulator");
    set_default_size(800, 550);

    // Keyboard events:
    Glib::RefPtr<Gtk::EventControllerKey> controller = Gtk::EventControllerKey::create();
    controller->signal_key_pressed().connect(sigc::mem_fun(*this, &MainWindow::on_key_pressed), false);
    add_controller(controller);

    // Header bar:
    Gtk::HeaderBar* headerBar = Gtk::make_managed<Gtk::HeaderBar>();
    inspectorBtn.set_label("🐞");
    inspectorBtn.set_tooltip_text("Inspector");
    inspectorBtn.signal_clicked().connect(&MainWindow::on_inspector_btn_clicked);
    headerBar->pack_end(inspectorBtn);
    set_titlebar(*headerBar);

    // Settings:
    mainBox.append(simulationSettingsBarWidget);

    // Simulator:
    simulationWidget.set_expand();
    simulationOverlay.set_child(simulationWidget);
    simulationOverlay.add_overlay(simulationOverlayWidget);
    simulationOverlay.set_expand();
    mainBox.append(simulationOverlay);

    set_child(mainBox);
}

//-----------------------------Events:-----------------------------
void MainWindow::on_inspector_btn_clicked() {
    gtk_window_set_interactive_debugging(true);
}

bool MainWindow::on_key_pressed(guint keyVal, guint /*keyCode*/, Gdk::ModifierType /*modifier*/) {
    if (keyVal == GDK_KEY_F11) {
        if (is_fullscreen()) {
            unfullscreen();
        } else {
            fullscreen();
        }
        return true;
    }
    if (keyVal == GDK_KEY_Escape && is_fullscreen()) {
        unfullscreen();
        return true;
    }
    return false;
}
}  // namespace ui::windows
