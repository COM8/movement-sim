#include "SimulationWidget.hpp"
#include "logger/Logger.hpp"
#include "sim/Entity.hpp"
#include "sim/Simulator.hpp"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/spdlog.h"
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

// Error codes: https://www.khronos.org/opengl/wiki/OpenGL_Error
#define GLERR                                             \
    {                                                     \
        GLuint glErr;                                     \
        while ((glErr = glGetError()) != GL_NO_ERROR) {   \
            SPDLOG_ERROR("glGetError() = 0x{:X}", glErr); \
        }                                                 \
    }

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
    // Person shader:

    // Load shader:
    personVertShader = compile_shader("/ui/person.vert", GL_VERTEX_SHADER);
    assert(personVertShader > 0);
    personGeomShader = compile_shader("/ui/person.geom", GL_GEOMETRY_SHADER);
    assert(personGeomShader > 0);
    personFragShader = compile_shader("/ui/person.frag", GL_FRAGMENT_SHADER);
    assert(personFragShader > 0);

    // Prepare program:
    personShaderProg = glCreateProgram();
    glAttachShader(personShaderProg, personVertShader);
    glAttachShader(personShaderProg, personGeomShader);
    glAttachShader(personShaderProg, personFragShader);
    glBindFragDataLocation(personShaderProg, 0, "outColor");
    glLinkProgram(personShaderProg);
    GLERR;

    // Check for errors during linking:
    GLint status = GL_FALSE;
    glGetProgramiv(personShaderProg, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len = 0;
        glGetProgramiv(personShaderProg, GL_INFO_LOG_LENGTH, &log_len);

        std::string log_msg;
        log_msg.resize(log_len);
        glGetProgramInfoLog(personShaderProg, log_len, nullptr, static_cast<GLchar*>(log_msg.data()));
        SPDLOG_ERROR("Linking person shader program failed: {}", log_msg);
        glDeleteProgram(personShaderProg);
        personShaderProg = 0;
    } else {
        glDetachShader(personShaderProg, personFragShader);
        glDetachShader(personShaderProg, personVertShader);
        glDetachShader(personShaderProg, personGeomShader);
    }
    GLERR;

    // Screen quad shader:

    // Load shader:
    screenSquareVertShader = compile_shader("/ui/screen_square.vert", GL_VERTEX_SHADER);
    assert(screenSquareVertShader > 0);
    screenSquareGeomShader = compile_shader("/ui/screen_square.geom", GL_GEOMETRY_SHADER);
    assert(screenSquareGeomShader > 0);
    screenSquareFragShader = compile_shader("/ui/screen_square.frag", GL_FRAGMENT_SHADER);
    assert(screenSquareFragShader > 0);
    GLERR;

    // Prepare program:
    screenSquareShaderProg = glCreateProgram();
    glAttachShader(screenSquareShaderProg, screenSquareVertShader);
    glAttachShader(screenSquareShaderProg, screenSquareGeomShader);
    glAttachShader(screenSquareShaderProg, screenSquareFragShader);
    glLinkProgram(screenSquareShaderProg);
    GLERR;

    // Check for errors during linking:
    status = GL_FALSE;
    glGetProgramiv(screenSquareShaderProg, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len = 0;
        glGetProgramiv(screenSquareShaderProg, GL_INFO_LOG_LENGTH, &log_len);

        std::string log_msg;
        log_msg.resize(log_len);
        glGetProgramInfoLog(screenSquareShaderProg, log_len, nullptr, static_cast<GLchar*>(log_msg.data()));
        SPDLOG_ERROR("Linking screen square shader program failed: {}", log_msg);
        glDeleteProgram(screenSquareShaderProg);
        screenSquareShaderProg = 0;
    } else {
        glDetachShader(screenSquareShaderProg, screenSquareVertShader);
        glDetachShader(screenSquareShaderProg, screenSquareGeomShader);
        glDetachShader(screenSquareShaderProg, screenSquareFragShader);
    }
    GLERR;
}

void SimulationWidget::prepare_buffers() {
    // Get default frame buffer since in GTK it is not always 0:
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFb);

    // Store Vertex array object settings:
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Vertex buffer:
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sim::Entity) * sim::MAX_ENTITIES, nullptr, GL_DYNAMIC_DRAW);

    // Frame buffer:
    glGenFramebuffers(1, &fbuf);
    glBindFramebuffer(GL_FRAMEBUFFER, fbuf);

    // Frame buffer texture:
    glGenTextures(1, &fbufTexture);
    GLERR;
    glBindTexture(GL_TEXTURE_2D, fbufTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    std::array<float, 4> borderColor{0.5f, 0.5f, 0.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor.data());
    GLERR;
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16, sim::WORLD_SIZE_X, sim::WORLD_SIZE_Y);
    GLERR;
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sim::WORLD_SIZE_X, sim::WORLD_SIZE_Y, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    GLERR;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbufTexture, 0);
    GLERR;

    // Render buffer for depth and stencil testing:
    glGenRenderbuffers(1, &rBuf);
    glBindRenderbuffer(GL_RENDERBUFFER, rBuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, sim::WORLD_SIZE_X, sim::WORLD_SIZE_Y);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rBuf);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    GLERR;

    glBindFramebuffer(GL_FRAMEBUFFER, defaultFb);
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
    GLERR;
    SPDLOG_DEBUG("Compiling shader '{}' was successful.", resourcePath);
    return shader;
}

void SimulationWidget::bind_attributes() {
    // Person shader:
    glUseProgram(personShaderProg);
    GLint colAttrib = glGetAttribLocation(personShaderProg, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(sim::Entity), nullptr);

    GLint posAttrib = glGetAttribLocation(personShaderProg, "position");
    glEnableVertexAttribArray(posAttrib);
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(sim::Entity), reinterpret_cast<void*>(4 * sizeof(GLfloat)));

    worldSizeConst = glGetUniformLocation(personShaderProg, "worldSize");
    glUniform2f(worldSizeConst, sim::WORLD_SIZE_X, sim::WORLD_SIZE_Y);

    rectSizeConst = glGetUniformLocation(personShaderProg, "rectSize");
    glUniform2f(rectSizeConst, 1, 1);
    GLERR;

    // Screen quad shader:
    glUseProgram(screenSquareShaderProg);
    glUniform1i(glGetUniformLocation(screenSquareShaderProg, "screenTexture"), 0);

    textureSizeConst = glGetUniformLocation(screenSquareShaderProg, "textureSize");
    glUniform2f(textureSizeConst, sim::WORLD_SIZE_X, sim::WORLD_SIZE_Y);

    screenSizeConst = glGetUniformLocation(screenSquareShaderProg, "screenSize");
    glUniform2f(screenSizeConst, static_cast<float>(glArea.get_width()), static_cast<float>(glArea.get_height()));
    GLERR;
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

        // Update the data on the GPU:
        if (enableUiUpdates) {
            std::shared_ptr<std::vector<sim::Entity>> entities = simulator->get_entities();
            if (entities) {
                this->entities = entities;
                const sim::Entity& e = (*this->entities)[0];
                SPDLOG_TRACE("Pos: {}/{} Direction: {}/{} Target: {}/{}", e.pos.x, e.pos.y, e.direction.x, e.direction.y, e.target.x, e.target.y);
            }
        }
        if (this->entities) {
            // Get default frame buffer since in GTK it is not always 0:
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFb);

            // Draw:
            glDisable(GL_DEPTH_TEST);

            // 1.0 Draw to buffer:
            glBindFramebuffer(GL_FRAMEBUFFER, fbuf);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            GLERR;

            // 1.1 Blur:

            // 1.2 Draw people:
            size_t size = this->entities->size();
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(sim::Entity) * size), static_cast<void*>(this->entities->data()), GL_DYNAMIC_DRAW);

            glUseProgram(personShaderProg);
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(size));

            // 2.0 Draw to screen:
            glBindFramebuffer(GL_FRAMEBUFFER, defaultFb);
            GLERR;

            // 2.1 Clear the screen:
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            GLERR;

            // 2.2 Draw texture from frame buffer:
            glUseProgram(screenSquareShaderProg);
            glUniform2f(screenSizeConst, static_cast<float>(glArea.get_width()), static_cast<float>(glArea.get_height()));
            glBindTexture(GL_TEXTURE_2D, fbufTexture);
            glDrawArrays(GL_POINTS, 0, 1);
            GLERR;
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
    }
    glArea.queue_draw();
    return true;
}

void SimulationWidget::on_realized() {
    glArea.make_current();
    try {
        glArea.throw_if_error();
        prepare_buffers();
        prepare_shader();
        glUseProgram(personShaderProg);
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
        glDeleteShader(personFragShader);
        glDeleteShader(personGeomShader);
        glDeleteShader(personFragShader);

        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        glDeleteFramebuffers(1, &fbuf);
    } catch (const Gdk::GLError& gle) {
        SPDLOG_ERROR("An error occurred deleting the context current during unrealize: {} - {} - {}", gle.domain(), gle.code(), gle.what());
    }
}

void SimulationWidget::on_adjustment_changed() {
    glArea.queue_draw();
}
}  // namespace ui::widgets
