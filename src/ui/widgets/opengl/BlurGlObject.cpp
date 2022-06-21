
#include "BlurGlObject.hpp"
#include "sim/Entity.hpp"
#include <cassert>
#include <epoxy/gl_generated.h>

namespace ui::widgets::opengl {
void BlurGlObject::bind_texture(GLuint inputTexture) {
    this->inputTexture = inputTexture;
}

void BlurGlObject::init_internal() {
    // Compile shader:
    vertShader = compile_shader("/ui/shader/blur/blur.vert", GL_VERTEX_SHADER);
    assert(vertShader > 0);
    geomShader = compile_shader("/ui/shader/blur/blur.geom", GL_GEOMETRY_SHADER);
    assert(geomShader > 0);
    fragShader = compile_shader("/ui/shader/blur/blur.frag", GL_FRAGMENT_SHADER);
    assert(fragShader > 0);

    // Prepare program:
    glAttachShader(shaderProg, vertShader);
    glAttachShader(shaderProg, geomShader);
    glAttachShader(shaderProg, fragShader);
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
        SPDLOG_ERROR("Linking blur shader program failed: {}", log_msg);
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
    glUniform1i(glGetUniformLocation(shaderProg, "inputTexture"), 0);
    GLERR;
}

void BlurGlObject::render_internal() {
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glDrawArrays(GL_POINTS, 0, 1);
    GLERR;
}

void BlurGlObject::cleanup_internal() {
    glDeleteShader(fragShader);
    glDeleteShader(geomShader);
    glDeleteShader(fragShader);
}
}  // namespace ui::widgets::opengl