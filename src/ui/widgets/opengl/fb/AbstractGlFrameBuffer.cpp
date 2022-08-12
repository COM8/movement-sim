#include "AbstractGlFrameBuffer.hpp"
#include "ui/widgets/opengl/utils/Utils.hpp"
#include <cassert>

namespace ui::widgets::opengl::fb {
void AbstractGlFrameBuffer::init() {
    glGenFramebuffers(1, &fbuf);
    glBindFramebuffer(GL_FRAMEBUFFER, fbuf);

    // Render buffer for depth and stencil testing:
    glGenRenderbuffers(1, &rBuf);
    glBindRenderbuffer(GL_RENDERBUFFER, rBuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, sizeX, sizeY);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rBuf);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    GLERR;

    // Texture:
    glGenTextures(1, &fbufTexture);
    GLERR;
    glBindTexture(GL_TEXTURE_2D, fbufTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    std::array<float, 4> borderColor{0.5F, 0.5F, 0.0F, 1.0F};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor.data());
    GLERR;
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16, sizeX, sizeY);
    GLERR;
    std::vector<GLubyte> emptyData(sizeX * sizeX * 4, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sizeX, sizeY, GL_RGBA, GL_UNSIGNED_BYTE, emptyData.data());
    GLERR;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbufTexture, 0);
    GLERR;
    init_internal();
}

AbstractGlFrameBuffer::AbstractGlFrameBuffer(GLsizei sizeX, GLsizei sizeY) : sizeX(sizeX), sizeY(sizeY) {}

void AbstractGlFrameBuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbuf);
    glViewport(0, 0, sizeX, sizeY);
    bind_internal();
}

void AbstractGlFrameBuffer::cleanup() {
    cleanup_internal();
    glDeleteFramebuffers(1, &fbuf);
}

GLuint AbstractGlFrameBuffer::get_texture() const {
    return fbufTexture;
}

GLsizei AbstractGlFrameBuffer::get_texture_size_x() const {
    return sizeX;
}

GLsizei AbstractGlFrameBuffer::get_texture_size_y() const {
    return sizeY;
}
}  // namespace ui::widgets::opengl::fb