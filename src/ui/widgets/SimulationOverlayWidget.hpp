#pragma once

#include "sim/Simulator.hpp"
#include <memory>
#include <gtkmm.h>

namespace ui::widgets {
class SimulationOverlayWidget : public Gtk::DrawingArea {
 private:
    std::shared_ptr<sim::Simulator> simulator{nullptr};

 public:
    SimulationOverlayWidget();

 private:
    static void prep_widget();

    void draw_text(const std::string& text, const Cairo::RefPtr<Cairo::Context>& ctx, double x, double y);

    //-----------------------------Events:-----------------------------
    void on_draw_handler(const Cairo::RefPtr<Cairo::Context>& ctx, int width, int height);
    bool on_tick(const Glib::RefPtr<Gdk::FrameClock>& frameClock);
};
}  // namespace ui::widgets
