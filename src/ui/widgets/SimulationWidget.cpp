#include "SimulationWidget.hpp"
#include "sim/Entity.hpp"
#include "sim/Simulator.hpp"
#include "spdlog/fmt/bundled/core.h"
#include <cassert>
#include <string>
#include <fmt/core.h>

namespace ui::widgets {
SimulationWidget::SimulationWidget() : simulator(sim::Simulator::get_instance()) {
    prep_widget();
    set_draw_func(sigc::mem_fun(*this, &SimulationWidget::on_draw_handler));
    add_tick_callback(sigc::mem_fun(*this, &SimulationWidget::on_tick));
}

void SimulationWidget::prep_widget() {}

void SimulationWidget::draw_text(const std::string& text, const Cairo::RefPtr<Cairo::Context>& ctx, double x, double y) {
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
void SimulationWidget::on_draw_handler(const Cairo::RefPtr<Cairo::Context>& ctx, int width, int height) {
    assert(simulator);

    ctx->save();

    // Entities:
    for (const sim::Entity& e : simulator->get_entities()) {
        if (e.isValid) {
            double x = (e.pos.x / sim::WORLD_SIZE_X) * static_cast<double>(width);
            double y = (e.pos.y / sim::WORLD_SIZE_Y) * static_cast<double>(height);
            ctx->rectangle(x - 0.5, y - 0.5, 1, 1);
            ctx->set_source_rgb(1.0, 0, 0);
            ctx->fill();
        }
    }

    ctx->restore();

    // Stats:
    std::string stats = fmt::format("TPS: {:.2f}\nTick Time: {}ns", simulator->get_tps(), simulator->get_avg_tick_time().count());
    draw_text(stats, ctx, 5, 20);
}

bool SimulationWidget::on_tick(const Glib::RefPtr<Gdk::FrameClock>& /*frameClock*/) {
    assert(simulator);

    if (simulator->is_simulating()) {
        queue_draw();
    }
    return true;
}
}  // namespace ui::widgets
