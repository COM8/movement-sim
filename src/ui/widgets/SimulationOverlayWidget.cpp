#include "SimulationOverlayWidget.hpp"
#include "sim/Entity.hpp"
#include "sim/Simulator.hpp"
#include "spdlog/fmt/bundled/core.h"
#include <cassert>
#include <chrono>
#include <string>
#include <bits/chrono.h>
#include <fmt/core.h>

namespace ui::widgets {
SimulationOverlayWidget::SimulationOverlayWidget(SimulationWidget* simWidget) : simulator(sim::Simulator::get_instance()),
                                                                                simWidget(simWidget) {
    prep_widget();
    set_draw_func(sigc::mem_fun(*this, &SimulationOverlayWidget::on_draw_handler));
    add_tick_callback(sigc::mem_fun(*this, &SimulationOverlayWidget::on_tick));
}

void SimulationOverlayWidget::prep_widget() {
    // Disable focus since this widget is just a overlay for the actual simulation:
    set_can_target(false);
    set_can_focus(false);
}

void SimulationOverlayWidget::draw_text(const std::string& text, const Cairo::RefPtr<Cairo::Context>& ctx, double x, double y) {
    ctx->save();
    Pango::FontDescription font;
    font.set_weight(Pango::Weight::BOLD);
    Glib::RefPtr<Pango::Layout> layout = create_pango_layout(text);
    layout->set_font_description(font);
    ctx->move_to(x, y);
    layout->add_to_cairo_context(ctx);
    Gdk::RGBA foreground = get_style_context()->get_color();
    ctx->set_source_rgba(foreground.get_red(), foreground.get_green(), foreground.get_blue(), foreground.get_alpha());
    ctx->fill_preserve();
    ctx->set_source_rgba(1 - foreground.get_red(), 1 - foreground.get_green(), 1 - foreground.get_blue(), foreground.get_alpha());
    ctx->set_line_width(0.3);
    ctx->stroke();
    ctx->restore();
}

void SimulationOverlayWidget::set_debug_overlay_enabled(bool enableDebugOverlay) {
    this->enableDebugOverlay = enableDebugOverlay;
}

//-----------------------------Events:-----------------------------
void SimulationOverlayWidget::on_draw_handler(const Cairo::RefPtr<Cairo::Context>& ctx, int /*width*/, int /*height*/) {
    if (!enableDebugOverlay) {
        return;
    }

    assert(simulator);
    assert(simWidget);

    double tps = simulator->get_tps().get_ticks();
    std::string tpsTime = simulator->get_tps_history().get_avg_time_str();

    double fps = simWidget->get_fps().get_ticks();
    std::string fpsTime = simWidget->get_fps_history().get_avg_time_str();

    std::string stats = fmt::format("TPS: {:.2f}\nTick Time: {}\n", tps, tpsTime, sim::MAX_ENTITIES);
    stats += fmt::format("FPS: {:.2f}\nFrame Time: {}\n", fps, fpsTime);
    stats += fmt::format("Entities: {}\n", sim::MAX_ENTITIES);
    stats += fmt::format("Zoom: {}\n", simWidget->get_zoom_factor());
    stats += fmt::format("\nMap Size: {}x{}\n", simulator->get_map()->width, simulator->get_map()->height);
    stats += fmt::format("Roads: {}\n", simulator->get_map()->roads.size());
    stats += fmt::format("Connections: {}\n", simulator->get_map()->connections.size());
    stats += fmt::format("Render Resolution: {}x{}\n", sim::MAX_RENDER_RESOLUTION_X, sim::MAX_RENDER_RESOLUTION_Y);
    draw_text(stats, ctx, 5, 5);
}

bool SimulationOverlayWidget::on_tick(const Glib::RefPtr<Gdk::FrameClock>& /*frameClock*/) {
    assert(simulator);
    queue_draw();
    return true;
}
}  // namespace ui::widgets
