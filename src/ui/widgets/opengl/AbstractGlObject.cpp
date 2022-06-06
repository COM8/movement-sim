#include "AbstractGlObject.hpp"
#include <epoxy/gl_generated.h>
#include <gtkmm.h>

namespace ui::widgets::opengl {
AbstractGlObject::AbstractGlObject() : simulator(sim::Simulator::get_instance()) {}

void AbstractGlObject::init() {
    glGenVertexArrays(1, &vao);
    GLERR;
    glBindVertexArray(vao);
    GLERR;

    glGenBuffers(1, &vbo);
    GLERR;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLERR;

    shaderProg = glCreateProgram();
    GLERR;

    init_internal();
    GLERR;

    glBindVertexArray(0);
    GLERR;
}

void AbstractGlObject::render() {
    glBindVertexArray(vao);
    GLERR;

    glUseProgram(shaderProg);

    render_internal();
    GLERR;

    glBindVertexArray(0);
    GLERR;
}

void AbstractGlObject::cleanup() {
    cleanup_internal();

    // glDeleteProgram(shaderProg);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

GLuint AbstractGlObject::compile_shader(const std::string& resourcePath, GLenum type) {
    // Load shader:
    const Glib::RefPtr<const Glib::Bytes> shaderbytes = Gio::Resource::lookup_data_global(resourcePath);
    assert(shaderbytes);
    gsize shaderSize = shaderbytes->get_size();
    const char* shaderData = static_cast<const char*>(shaderbytes->get_data(shaderSize));
    GLERR;

    // Compile:
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderData, nullptr);
    GLERR;
    glCompileShader(shader);
    GLERR;

    // Check for errors during compiling:
    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    GLERR;
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
}  // namespace ui::widgets::opengl