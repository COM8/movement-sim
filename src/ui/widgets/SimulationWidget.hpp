#pragma once

#include <gtkmm.h>

namespace ui::widgets {
class SimulationWidget : public Gtk::DrawingArea {
 public:
    SimulationWidget();

 private:
    static void prep_widget();

    //-----------------------------Events:-----------------------------
    void on_draw_handler(const Cairo::RefPtr<Cairo::Context>& ctx, int width, int height);
};
}  // namespace ui::widgets
