
#include "ScreenSquareGlObject.hpp"
#include "sim/Entity.hpp"
#include <cassert>

namespace ui::widgets::opengl {
void ScreenSquareGlObject::set_glArea(Gtk::GLArea* glArea) {
    this->glArea = glArea;
}

void ScreenSquareGlObject::set_fb_texture(GLuint fbufTexture) {
    this->fbufTexture = fbufTexture;
}

void ScreenSquareGlObject::init_internal() {
    // Compile shader:
    vertShader = compile_shader("/ui/shader/screen_square/screen_square.vert", GL_VERTEX_SHADER);
    assert(vertShader > 0);
    geomShader = compile_shader("/ui/shader/screen_square/screen_square.geom", GL_GEOMETRY_SHADER);
    assert(geomShader > 0);
    fragShader = compile_shader("/ui/shader/screen_square/screen_square.frag", GL_FRAGMENT_SHADER);
    assert(fragShader > 0);

    // Prepare program:
    glAttachShader(shaderProg, vertShader);
    glAttachShader(shaderProg, geomShader);
    glAttachShader(shaderProg, fragShader);
    glBindFragDataLocation(shaderProg, 0, "outColor");
    glLinkProgram(shaderProg);
    GLERR;

    // Check for errors during linking:
    GLint status = GL_FALSE;
    glGetProgramiv(shaderProg, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len = 0;
        glGetProgramiv(shaderProg, GL_INFO_LOG_LENGTH, &log_len);

        std::string log_msg;
        log_msg.resize(log_len);
        glGetProgramInfoLog(shaderProg, log_len, nullptr, static_cast<GLchar*>(log_msg.data()));
        SPDLOG_ERROR("Linking screen square shader program failed: {}", log_msg);
        glDeleteProgram(shaderProg);
        shaderProg = 0;
    } else {
        glDetachShader(shaderProg, fragShader);
        glDetachShader(shaderProg, vertShader);
        glDetachShader(shaderProg, geomShader);
    }
    GLERR;

    // Bind attributes:
    glUseProgram(shaderProg);
    glUniform1i(glGetUniformLocation(shaderProg, "screenTexture"), 0);

    textureSizeConst = glGetUniformLocation(shaderProg, "textureSize");
    glUniform2f(textureSizeConst, sim::MAX_RENDER_RESOLUTION_X, sim::MAX_RENDER_RESOLUTION_Y);

    screenSizeConst = glGetUniformLocation(shaderProg, "screenSize");
    assert(glArea);
    glUniform2f(screenSizeConst, static_cast<float>(glArea->get_width()), static_cast<float>(glArea->get_height()));
    GLERR;
}

void ScreenSquareGlObject::render_internal() {
    assert(glArea);
    glUniform2f(screenSizeConst, static_cast<float>(glArea->get_width()), static_cast<float>(glArea->get_height()));
    glBindTexture(GL_TEXTURE_2D, fbufTexture);
    glDrawArrays(GL_POINTS, 0, 1);
    GLERR;
}

void ScreenSquareGlObject::cleanup_internal() {
    glDeleteShader(fragShader);
    glDeleteShader(geomShader);
    glDeleteShader(fragShader);
}
}  // namespace ui::widgets::opengl