#include "SimulationWidget.hpp"

namespace ui::widgets {
SimulationWidget::SimulationWidget() {
    prep_widget();
    set_draw_func(sigc::mem_fun(*this, &SimulationWidget::on_draw_handler));
}

void SimulationWidget::prep_widget() {}

//-----------------------------Events:-----------------------------
void SimulationWidget::on_draw_handler(const Cairo::RefPtr<Cairo::Context>& ctx, int /*width*/, int /*height*/) {
    ctx->save();
    ctx->restore();
}
}  // namespace ui::widgets
