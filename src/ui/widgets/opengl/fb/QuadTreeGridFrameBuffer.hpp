#pragma once

#include "AbstractGlFrameBuffer.hpp"
#include "QuadTreeGridFrameBuffer.hpp"
#include <epoxy/gl.h>
#include <epoxy/gl_generated.h>

namespace ui::widgets::opengl::fb {
class QuadTreeGridFrameBuffer : public AbstractGlFrameBuffer {
 public:
    QuadTreeGridFrameBuffer(GLsizei sizeX, GLsizei sizeY);
    QuadTreeGridFrameBuffer(QuadTreeGridFrameBuffer& other) = delete;
    QuadTreeGridFrameBuffer(QuadTreeGridFrameBuffer&& old) = delete;

    ~QuadTreeGridFrameBuffer() override = default;

    QuadTreeGridFrameBuffer& operator=(QuadTreeGridFrameBuffer& other) = delete;
    QuadTreeGridFrameBuffer& operator=(QuadTreeGridFrameBuffer&& old) = delete;

 protected:
    void init_internal() override;
    void bind_internal() override;
    void cleanup_internal() override;
};
}  // namespace ui::widgets::opengl::fb