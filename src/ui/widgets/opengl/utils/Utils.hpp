#pragma once

#include "logger/Logger.hpp"
#include <epoxy/gl.h>
#include <epoxy/gl_generated.h>

// Error codes: https://www.khronos.org/opengl/wiki/OpenGL_Error
#define GLERR                                             \
    {                                                     \
        GLuint glErr;                                     \
        while ((glErr = glGetError()) != GL_NO_ERROR) {   \
            SPDLOG_ERROR("glGetError() = 0x{:X}", glErr); \
        }                                                 \
    }

namespace ui::widgets::opengl::utils {
}  // namespace ui::widgets::opengl::utils