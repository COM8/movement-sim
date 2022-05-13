#pragma once

#include "sim/Entity.hpp"
#include "sim/Simulator.hpp"
#include <memory>
#include <epoxy/gl.h>
#include <gtkmm.h>

namespace ui::widgets {
class SimulationWidget : public Gtk::GLArea {
 private:
    std::shared_ptr<sim::Simulator> simulator{nullptr};
    std::shared_ptr<std::vector<sim::Entity>> entities;

    // OPenGL:
    GLuint m_Vao{0};
    GLuint m_Buffer{0};
    GLuint m_Program{0};
    GLuint m_Mvp{0};

 public:
    SimulationWidget();

 private:
    void prep_widget();
    void prepare_shader();
    void prepare_buffers();

    //-----------------------------Events:-----------------------------
    bool on_render_handler(const Glib::RefPtr<Gdk::GLContext>& ctx);
    bool on_tick(const Glib::RefPtr<Gdk::FrameClock>& frameClock);
    void on_realized();
    void on_unrealized();
};
}  // namespace ui::widgets
