#pragma once

#include <gtkmm.h>

namespace ui::windows {
class MainWindow : public Gtk::Window {
 private:
    Gtk::Button inspectorBtn;

 public:
    MainWindow();

 private:
    void prep_window();

    //-----------------------------Events:-----------------------------
    static void on_inspector_btn_clicked();
    bool on_key_pressed(guint keyVal, guint keyCode, Gdk::ModifierType modifier);
};
}  // namespace ui::windows
