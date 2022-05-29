#include "SimulationWidget.hpp"
#include "logger/Logger.hpp"
#include "sim/Entity.hpp"
#include "sim/Simulator.hpp"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/spdlog.h"
#include <cassert>
#include <chrono>
#include <cmath>
#include <string>
#include <bits/chrono.h>
#include <epoxy/gl.h>
#include <epoxy/gl_generated.h>
#include <fmt/core.h>
#include <glibconfig.h>
#include <gtkmm/gesturezoom.h>

namespace ui::widgets {
SimulationWidget::SimulationWidget() : simulator(sim::Simulator::get_instance()) {
    prep_widget();
}

void SimulationWidget::set_zoom_factor(float zoomFactor) {
    assert(zoomFactor > 0);
    this->zoomFactor = zoomFactor;
    float widthF = static_cast<float>(sim::WORLD_SIZE_X) * this->zoomFactor;
    int width = static_cast<int>(widthF);
    float heightF = static_cast<float>(sim::WORLD_SIZE_X) * this->zoomFactor;
    int height = static_cast<int>(heightF);
    glUniform1f(zoomFactorConst, this->zoomFactor);
    glArea.set_size_request(width, height);
    glArea.queue_draw();
}

void SimulationWidget::prep_widget() {
    set_expand();

    get_vadjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &SimulationWidget::on_adjustment_changed));
    get_hadjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &SimulationWidget::on_adjustment_changed));

    glArea.signal_render().connect(sigc::mem_fun(*this, &SimulationWidget::on_render_handler), true);
    glArea.signal_realize().connect(sigc::mem_fun(*this, &SimulationWidget::on_realized));
    glArea.signal_unrealize().connect(sigc::mem_fun(*this, &SimulationWidget::on_unrealized));
    glArea.add_tick_callback(sigc::mem_fun(*this, &SimulationWidget::on_tick));

    glArea.set_auto_render();
    glArea.set_size_request(sim::WORLD_SIZE_X, sim::WORLD_SIZE_Y);
    set_child(glArea);
}

void SimulationWidget::prepare_shader() {
    // Load shader:
    vertShader = compile_shader("/ui/person.vert", GL_VERTEX_SHADER);
    assert(vertShader > 0);
    geomShader = compile_shader("/ui/person.geom", GL_GEOMETRY_SHADER);
    assert(geomShader > 0);
    fragShader = compile_shader("/ui/person.frag", GL_FRAGMENT_SHADER);
    assert(fragShader > 0);

    // Prepare program:
    prog = glCreateProgram();
    glAttachShader(prog, vertShader);
    glAttachShader(prog, geomShader);
    glAttachShader(prog, fragShader);
    glBindFragDataLocation(prog, 0, "outColor");
    glLinkProgram(prog);

    // Check for errors during linking:
    GLint status = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);

        std::string log_msg;
        log_msg.resize(log_len);
        glGetProgramInfoLog(prog, log_len, nullptr, static_cast<GLchar*>(log_msg.data()));
        SPDLOG_ERROR("Linking failed: {}", log_msg);
        glDeleteProgram(prog);
        prog = 0;
    } else {
        glDetachShader(prog, fragShader);
        glDetachShader(prog, vertShader);
    }
}

void SimulationWidget::prepare_buffers() {
    // Store Vertex array object settings:
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Vertex buffer:
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sim::Entity) * sim::MAX_ENTITIES, nullptr, GL_DYNAMIC_DRAW);
}

GLuint SimulationWidget::compile_shader(const std::string& resourcePath, GLenum type) {
    // Load shader:
    const Glib::RefPtr<const Glib::Bytes> shaderbytes = Gio::Resource::lookup_data_global(resourcePath);
    assert(shaderbytes);
    gsize shaderSize = shaderbytes->get_size();
    const char* shaderData = static_cast<const char*>(shaderbytes->get_data(shaderSize));

    // Compile:
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderData, nullptr);
    glCompileShader(shader);

    // Check for errors during compiling:
    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

        std::string log_msg;
        log_msg.resize(log_len);
        glGetShaderInfoLog(shader, log_len, nullptr, static_cast<GLchar*>(log_msg.data()));

        SPDLOG_ERROR("Compiling shader '{}' failed with: {}", resourcePath, log_msg);
        glDeleteShader(shader);
        return -1;
    }
    SPDLOG_DEBUG("Compiling shader '{}' was successful.", resourcePath);
    return shader;
}

void SimulationWidget::bind_attributes() {
    GLint colAttrib = glGetAttribLocation(prog, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(sim::Entity), nullptr);

    GLint posAttrib = glGetAttribLocation(prog, "position");
    glEnableVertexAttribArray(posAttrib);
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(sim::Entity), reinterpret_cast<void*>(4 * sizeof(GLfloat)));

    worldSizeConst = glGetUniformLocation(prog, "worldSize");
    glUniform2f(worldSizeConst, sim::WORLD_SIZE_X, sim::WORLD_SIZE_Y);

    rectSizeConst = glGetUniformLocation(prog, "rectSize");
    glUniform2f(rectSizeConst, 1, 1);

    viewPortConst = glGetUniformLocation(prog, "viewPort");

    zoomFactorConst = glGetUniformLocation(prog, "zoomFactor");
    glUniform1f(zoomFactorConst, 1);
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

//-----------------------------Events:-----------------------------
bool SimulationWidget::on_render_handler(const Glib::RefPtr<Gdk::GLContext>& /*ctx*/) {
    assert(simulator);

    std::chrono::high_resolution_clock::time_point frameStart = std::chrono::high_resolution_clock::now();

    try {
        glArea.throw_if_error();

        // Clear the screen:
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        // Update the data on the GPU:
        if (enableUiUpdates) {
            std::shared_ptr<std::vector<sim::Entity>> entities = simulator->get_entities();
            if (entities) {
                this->entities = entities;
                SPDLOG_TRACE("Pos: {}/{} Direction: {}/{} Target: {}/{}", (*this->entities)[0].pos.x, (*this->entities)[0].pos.y, (*this->entities)[0].direction.x, (*this->entities)[0].direction.y, (*this->entities)[0].target.x, (*this->entities)[0].target.y);
            }
        }
        if (this->entities) {
            size_t size = this->entities->size();
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(sim::Entity) * size), static_cast<void*>(this->entities->data()), GL_DYNAMIC_DRAW);

            // Update view port:
            GLfloat viewPortMinH = static_cast<GLfloat>(get_hadjustment()->get_value());
            GLfloat viewPortMaxH = static_cast<GLfloat>(get_hadjustment()->get_value() + get_width());
            GLfloat viewPortMinV = (static_cast<GLfloat>(get_vadjustment()->get_value()));
            GLfloat viewPortMaxV = (static_cast<GLfloat>(get_vadjustment()->get_value() + get_height()));
            glUniform4f(viewPortConst, viewPortMinH, viewPortMaxH, viewPortMaxV, viewPortMinV);

            // Draw:
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(size));
        }
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

    if (simulator->is_simulating()) {
        glArea.queue_draw();
    }
    return true;
}

void SimulationWidget::on_realized() {
    glArea.make_current();
    try {
        glArea.throw_if_error();
        prepare_buffers();
        prepare_shader();
        glUseProgram(prog);
        bind_attributes();
    } catch (const Gdk::GLError& gle) {
        SPDLOG_ERROR("An error occurred making the context current during realize: {} - {} - {}", gle.domain(), gle.code(), gle.what());
    }
}

void SimulationWidget::on_unrealized() {
    glArea.make_current();
    try {
        glArea.throw_if_error();
        // glDeleteProgram(prog);
        glDeleteShader(vertShader);
        glDeleteShader(geomShader);
        glDeleteShader(fragShader);

        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    } catch (const Gdk::GLError& gle) {
        SPDLOG_ERROR("An error occurred deleting the context current during unrealize: {} - {} - {}", gle.domain(), gle.code(), gle.what());
    }
}

void SimulationWidget::on_adjustment_changed() {
    glArea.queue_draw();
}
}  // namespace ui::widgets
