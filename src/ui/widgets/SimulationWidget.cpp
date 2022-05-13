#include "SimulationWidget.hpp"
#include "logger/Logger.hpp"
#include "sim/Entity.hpp"
#include "sim/Simulator.hpp"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/spdlog.h"
#include <cassert>
#include <chrono>
#include <string>
#include <bits/chrono.h>
#include <epoxy/gl.h>
#include <epoxy/gl_generated.h>
#include <fmt/core.h>
#include <glibconfig.h>

namespace ui::widgets {
SimulationWidget::SimulationWidget() : simulator(sim::Simulator::get_instance()) {
    prep_widget();
    signal_render().connect(sigc::mem_fun(*this, &SimulationWidget::on_render_handler), true);
    signal_realize().connect(sigc::mem_fun(*this, &SimulationWidget::on_realized));
    signal_unrealize().connect(sigc::mem_fun(*this, &SimulationWidget::on_unrealized));
    add_tick_callback(sigc::mem_fun(*this, &SimulationWidget::on_tick));
}

void SimulationWidget::prep_widget() {
    set_auto_render();
}

void SimulationWidget::prepare_shader() {
    // Load shader:
    std::string path = "/ui/entity_draw.frag";
    const Glib::RefPtr<const Glib::Bytes> shader_bytes = Gio::Resource::lookup_data_global(path);
    assert(shader_bytes);
    gsize shader_size = shader_bytes->get_size();
    const char* shader_data = static_cast<const char*>(shader_bytes->get_data(shader_size));

    // Create shader:
    int shader_type = GL_FRAGMENT_SHADER;
    int shader = glCreateShader(shader_type);

    glShaderSource(shader, 1, &shader_data, nullptr);
    glCompileShader(shader_type);

    int status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

        std::string log_msg;
        log_msg.resize(log_len + 1);
        glGetShaderInfoLog(shader, log_len, nullptr, static_cast<GLchar*>(log_msg.data()));

        SPDLOG_ERROR("Compiling fragment shader '{}' failed with: {}", path, log_msg);
        glDeleteShader(shader);
    }

    // Prepare program:
    m_Program = glCreateProgram();
    glAttachShader(m_Program, shader);
    glLinkProgram(m_Program);

    status = GL_FALSE;
    glGetProgramiv(m_Program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len = 0;
        glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &log_len);

        std::string log_msg;
        log_msg.resize(log_len + 1);
        glGetProgramInfoLog(shader, log_len, nullptr, static_cast<GLchar*>(log_msg.data()));
        SPDLOG_ERROR("Linking failed: {}", log_msg);
        glDeleteProgram(m_Program);
        m_Program = 0;
    } else {
        /* Get the location of the "mvp" uniform */
        m_Mvp = glGetUniformLocation(m_Program, "mvp");

        glDetachShader(m_Program, shader);
    }
    glDeleteShader(shader);
}

static const GLfloat vertex_data[] = {
    0.f,
    0.5f,
    0.f,
    1.f,
    0.5f,
    -0.366f,
    0.f,
    1.f,
    -0.5f,
    -0.366f,
    0.f,
    1.f,
};

void SimulationWidget::prepare_buffers() {
    glGenVertexArrays(1, &m_Vao);
    glBindVertexArray(m_Vao);

    glGenBuffers(1, &m_Buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//-----------------------------Events:-----------------------------
bool SimulationWidget::on_render_handler(const Glib::RefPtr<Gdk::GLContext>& /*ctx*/) {
    assert(simulator);

    try {
        throw_if_error();
        glClearColor(0.5, 0.5, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        //------------------------------
        glUseProgram(m_Program);

        glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glDisableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);
        //------------------------------

        glFlush();
        return true;
    } catch (const Gdk::GLError& gle) {
        SPDLOG_ERROR("An error occurred in the render callback of the GLArea: {} - {} - {}", gle.domain(), gle.code(), gle.what());
    }
    return false;
}

bool SimulationWidget::on_tick(const Glib::RefPtr<Gdk::FrameClock>& /*frameClock*/) {
    assert(simulator);

    if (simulator->is_simulating()) {
        queue_draw();
    }
    return true;
}

void SimulationWidget::on_realized() {
    make_current();
    try {
        throw_if_error();
        prepare_buffers();
        prepare_shader();
    } catch (const Gdk::GLError& gle) {
        SPDLOG_ERROR("An error occurred making the context current during realize: {} - {} - {}", gle.domain(), gle.code(), gle.what());
    }
}

void SimulationWidget::on_unrealized() {
}
}  // namespace ui::widgets
