#include "SimulationWidget.hpp"
#include "logger/Logger.hpp"
#include "sim/Entity.hpp"
#include "sim/Map.hpp"
#include "sim/Simulator.hpp"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/spdlog.h"
#include "ui/widgets/opengl/fb/MapFrameBuffer.hpp"
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <string>
#include <bits/chrono.h>
#include <epoxy/gl.h>
#include <epoxy/gl_generated.h>
#include <fmt/core.h>
#include <glibconfig.h>
#include <gtkmm/gesturezoom.h>

namespace ui::widgets {
SimulationWidget::SimulationWidget() : simulator(sim::Simulator::get_instance()),
                                       mapFrameBuffer(sim::MAX_RENDER_RESOLUTION_X, sim::MAX_RENDER_RESOLUTION_Y),
                                       entitiesFrameBuffer(sim::MAX_RENDER_RESOLUTION_X, sim::MAX_RENDER_RESOLUTION_Y) {
    prep_widget();
}

void SimulationWidget::set_zoom_factor(float zoomFactor) {
    assert(zoomFactor > 0);

    assert(simulator);
    const std::shared_ptr<sim::Map> map = simulator->get_map();
    assert(map);

    this->zoomFactor = zoomFactor;
    // float widthF = static_cast<float>(map->widthPowerTwo) * this->zoomFactor;
    float widthF = static_cast<float>(sim::MAX_RENDER_RESOLUTION_X) * this->zoomFactor;
    int width = static_cast<int>(widthF);
    // float heightF = static_cast<float>(map->heightPowerTwo) * this->zoomFactor;
    float heightF = static_cast<float>(sim::MAX_RENDER_RESOLUTION_Y) * this->zoomFactor;
    int height = static_cast<int>(heightF);
    glArea.set_size_request(width, height);
    glArea.queue_draw();
}

void SimulationWidget::prep_widget() {
    set_expand();

    glArea.signal_render().connect(sigc::mem_fun(*this, &SimulationWidget::on_render_handler), true);
    glArea.signal_realize().connect(sigc::mem_fun(*this, &SimulationWidget::on_realized));
    glArea.signal_unrealize().connect(sigc::mem_fun(*this, &SimulationWidget::on_unrealized));
    glArea.add_tick_callback(sigc::mem_fun(*this, &SimulationWidget::on_tick));

    assert(simulator);
    const std::shared_ptr<sim::Map> map = simulator->get_map();
    assert(map);

    glArea.set_auto_render();
    // glArea.set_size_request(static_cast<int>(map->widthPowerTwo), static_cast<int>(map->heightPowerTwo));
    glArea.set_size_request(sim::MAX_RENDER_RESOLUTION_X, sim::MAX_RENDER_RESOLUTION_Y);
    set_child(glArea);
    screenSquareObj.set_glArea(&glArea);
}

const utils::TickRate& SimulationWidget::get_fps() const {
    return fps;
}

const utils::TickDurationHistory& SimulationWidget::get_fps_history() const {
    return fpsHistory;
}

float SimulationWidget::get_zoom_factor() const {
    return zoomFactor;
}

void SimulationWidget::load_map() {
}

//-----------------------------Events:-----------------------------
bool SimulationWidget::on_render_handler(const Glib::RefPtr<Gdk::GLContext>& /*ctx*/) {
    assert(simulator);

    std::chrono::high_resolution_clock::time_point frameStart = std::chrono::high_resolution_clock::now();

    try {
        glArea.throw_if_error();

        // Update the data on the GPU:
        bool entitiesChanged = false;
        if (enableUiUpdates) {
            std::shared_ptr<std::vector<sim::Entity>> entities = simulator->get_entities();
            if (entities) {
                entitiesChanged = true;
                this->entities = std::move(entities);
                const sim::Entity& e = (*this->entities)[0];
                SPDLOG_TRACE("Pos: {}/{} Direction: {}/{} Target: {}/{}", e.pos.x, e.pos.y, e.direction.x, e.direction.y, e.target.x, e.target.y);
            }
        }

        // Get default frame buffer since in GTK it is not always 0:
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFb);

        // Draw:
        glDisable(GL_DEPTH_TEST);

        // 1.0 Draw map if needed:
        if (!mapRendered) {
            mapRendered = true;

            mapFrameBuffer.bind();
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            GLERR;

            mapObj.render();
        }

        // 2.0 Draw to buffer:
        if (entitiesChanged) {
            entitiesFrameBuffer.bind();
            // 2.1 Blur old entities:

            // For now, we just clear them instead of blurring them:
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            GLERR;

            // 2.2 Draw entities:
            entityObj.set_entities(this->entities);
            entityObj.render();
        }

        // 3.0 Draw to screen:
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFb);
        GLERR;

        // 3.1 Clear the old screen:
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        GLERR;

        // 3.2 Draw texture from frame buffer:
        screenSquareObj.render();
    } catch (const Gdk::GLError& gle) {
        SPDLOG_ERROR("An error occurred in the render callback of the GLArea: {} - {} - {}", gle.domain(), gle.code(), gle.what());
    }

    std::chrono::high_resolution_clock::time_point frameEnd = std::chrono::high_resolution_clock::now();
    fpsHistory.add_time(frameEnd - frameStart);

    // FPS counter:
    fps.tick();
    return false;
}

bool SimulationWidget::on_tick(const Glib::RefPtr<Gdk::FrameClock>& /*frameClock*/) {
    assert(simulator);

    if (this->enableUiUpdates) {
        glArea.queue_draw();
    }
    return true;
}

void SimulationWidget::on_realized() {
    glArea.make_current();
    try {
        glArea.throw_if_error();

        mapFrameBuffer.init();
        entitiesFrameBuffer.init();

        // Get default frame buffer since in GTK it is not always 0:
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFb);
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFb);

        mapObj.init();
        entityObj.init();
        screenSquareObj.bind_texture(mapFrameBuffer.get_texture(), entitiesFrameBuffer.get_texture());
        screenSquareObj.init();
    } catch (const Gdk::GLError& gle) {
        SPDLOG_ERROR("An error occurred making the context current during realize: {} - {} - {}", gle.domain(), gle.code(), gle.what());
    }
}

void SimulationWidget::on_unrealized() {
    glArea.make_current();
    try {
        glArea.throw_if_error();

        mapObj.cleanup();
        entityObj.cleanup();
        screenSquareObj.cleanup();

        entitiesFrameBuffer.cleanup();
        mapFrameBuffer.cleanup();
    } catch (const Gdk::GLError& gle) {
        SPDLOG_ERROR("An error occurred deleting the context current during unrealize: {} - {} - {}", gle.domain(), gle.code(), gle.what());
    }
}
}  // namespace ui::widgets
