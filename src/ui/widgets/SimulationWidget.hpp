#pragma once

#include "opengl/EntityGlObject.hpp"
#include "opengl/MapGlObject.hpp"
#include "opengl/ScreenSquareGlObject.hpp"
#include "opengl/fb/EntitiesFrameBuffer.hpp"
#include "opengl/fb/MapFrameBuffer.hpp"
#include "sim/Entity.hpp"
#include "sim/Simulator.hpp"
#include "utils/TickDurationHistory.hpp"
#include "utils/TickRate.hpp"
#include <memory>
#include <epoxy/gl.h>
#include <gtkmm.h>
#include <gtkmm/glarea.h>
#include <gtkmm/scrolledwindow.h>

namespace ui::widgets {
class SimulationWidget : public Gtk::ScrolledWindow {
 private:
    std::shared_ptr<sim::Simulator> simulator{nullptr};
    std::shared_ptr<std::vector<sim::Entity>> entities{nullptr};

    utils::TickDurationHistory fpsHistory{};
    utils::TickRate fps{};

    // OpenGL:
    GLint defaultFb{0};

    opengl::EntityGlObject entityObj{};
    opengl::MapGlObject mapObj{};
    opengl::ScreenSquareGlObject screenSquareObj{};

    opengl::fb::MapFrameBuffer mapFrameBuffer;
    opengl::fb::EntitiesFrameBuffer entitiesFrameBuffer;
    bool mapRendered{false};

    Gtk::GLArea glArea;
    float zoomFactor{1};

 public:
    bool enableUiUpdates{true};

    SimulationWidget();

    [[nodiscard]] const utils::TickRate& get_fps() const;
    [[nodiscard]] const utils::TickDurationHistory& get_fps_history() const;

    void set_zoom_factor(float zoomFactor);
    [[nodiscard]] float get_zoom_factor() const;

 private:
    void prep_widget();

    //-----------------------------Events:-----------------------------
    bool on_render_handler(const Glib::RefPtr<Gdk::GLContext>& ctx);
    bool on_tick(const Glib::RefPtr<Gdk::FrameClock>& frameClock);
    void on_realized();
    void on_unrealized();
};
}  // namespace ui::widgets
