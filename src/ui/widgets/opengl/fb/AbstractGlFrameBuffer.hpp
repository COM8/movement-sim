#pragma once

#include <epoxy/gl.h>
#include <epoxy/gl_generated.h>

namespace ui::widgets::opengl::fb {
class AbstractGlFrameBuffer {
 protected:
    GLuint fbuf{0};
    GLuint rBuf{0};
    GLuint fbufTexture{0};

    GLsizei sizeX;
    GLsizei sizeY;

 public:
    AbstractGlFrameBuffer(GLsizei sizeX, GLsizei sizeY);
    AbstractGlFrameBuffer(AbstractGlFrameBuffer& other) = delete;
    AbstractGlFrameBuffer(AbstractGlFrameBuffer&& old) = delete;

    virtual ~AbstractGlFrameBuffer() = default;

    AbstractGlFrameBuffer& operator=(AbstractGlFrameBuffer& other) = delete;
    AbstractGlFrameBuffer& operator=(AbstractGlFrameBuffer&& old) = delete;

 protected:
    virtual void init_internal() = 0;
    virtual void bind_internal() = 0;
    virtual void cleanup_internal() = 0;

 public:
    void init();
    void bind();
    void cleanup();

    [[nodiscard]] GLuint get_texture() const;
};
}  // namespace ui::widgets::opengl::fb