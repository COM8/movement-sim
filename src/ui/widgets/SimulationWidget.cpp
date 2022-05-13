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
    fragShader = compile_shader("/ui/triangle.frag", GL_FRAGMENT_SHADER);
    assert(fragShader > 0);
    vertShader = compile_shader("/ui/triangle.vert", GL_VERTEX_SHADER);
    assert(vertShader > 0);

    // Prepare program:
    prog = glCreateProgram();
    glAttachShader(prog, fragShader);
    glAttachShader(prog, vertShader);
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

GLfloat vertices[] = {
    0.25, 0.75, 1.0, 0.0, 0.0,  // Top-left
    0.75, 0.75, 1.0, 0.0, 0.0,  // Top-right
    0.75, 0.25, 1.0, 0.0, 0.0,  // Bottom-right
    0.25, 0.25, 1.0, 0.0, 0.0,  // Bottom-left
    //
    -0.75, 0.75, 0.0, 1.0, 0.0,  // Top-left
    -0.25, 0.75, 0.0, 1.0, 0.0,  // Top-right
    -0.25, 0.25, 0.0, 1.0, 0.0,  // Bottom-right
    -0.75, 0.25, 0.0, 1.0, 0.0  // Bottom-left
};

static const GLuint elements[] = {
    0, 1, 2,
    2, 3, 0,
    4, 5, 6,
    6, 7, 4};

void SimulationWidget::prepare_buffers() {
    // Store Vertex array object settings:
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Vertex buffer:
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Element buffer:
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
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

void SimulationWidget::bind_attributes() const {
    glUseProgram(prog);
    GLint posAttrib = glGetAttribLocation(prog, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), nullptr);

    GLint colAttrib = glGetAttribLocation(prog, "color");
    glEnableVertexAttribArray(colAttrib);
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<void*>(2 * sizeof(GLfloat)));
}

//-----------------------------Events:-----------------------------
bool SimulationWidget::on_render_handler(const Glib::RefPtr<Gdk::GLContext>& /*ctx*/) {
    assert(simulator);

    try {
        throw_if_error();

        // Clear the screen:
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);

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
        bind_attributes();
    } catch (const Gdk::GLError& gle) {
        SPDLOG_ERROR("An error occurred making the context current during realize: {} - {} - {}", gle.domain(), gle.code(), gle.what());
    }
}

void SimulationWidget::on_unrealized() {
    make_current();
    try {
        throw_if_error();
        glDeleteProgram(prog);
        glDeleteShader(fragShader);
        glDeleteShader(vertShader);

        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteVertexArrays(1, &vao);
    } catch (const Gdk::GLError& gle) {
        SPDLOG_ERROR("An error occurred deleting the context current during unrealize: {} - {} - {}", gle.domain(), gle.code(), gle.what());
    }
}
}  // namespace ui::widgets
