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
SimulationOverlayWidget::SimulationOverlayWidget() : simulator(sim::Simulator::get_instance()) {
    prep_widget();
    set_draw_func(sigc::mem_fun(*this, &SimulationOverlayWidget::on_draw_handler));
    add_tick_callback(sigc::mem_fun(*this, &SimulationOverlayWidget::on_tick));
}

void SimulationOverlayWidget::prep_widget() {}

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

//-----------------------------Events:-----------------------------
void SimulationOverlayWidget::on_draw_handler(const Cairo::RefPtr<Cairo::Context>& ctx, int /*width*/, int /*height*/) {
    assert(simulator);

    // Stats:
    double tick_time = static_cast<double>(simulator->get_avg_tick_time().count());
    std::string unit = "ns";
    if (tick_time >= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::microseconds(1)).count()) {
        if (tick_time >= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(1)).count()) {
            if (tick_time >= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)).count()) {
                tick_time /= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)).count();
                unit = "s";
            } else {
                tick_time /= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(1)).count();
                unit = "ms";
            }
        } else {
            tick_time /= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::microseconds(1)).count();
            unit = "us";
        }
    }
    std::string stats = fmt::format("TPS: {:.2f}\nTick Time: {:.2f}{}\nEntities: {}", simulator->get_tps(), tick_time, unit, simulator->get_entities().size());
    draw_text(stats, ctx, 5, 20);
}

bool SimulationOverlayWidget::on_tick(const Glib::RefPtr<Gdk::FrameClock>& /*frameClock*/) {
    assert(simulator);

    if (simulator->is_simulating()) {
        queue_draw();
    }
    return true;
}
}  // namespace ui::widgets
