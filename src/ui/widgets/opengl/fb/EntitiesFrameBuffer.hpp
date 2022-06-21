#pragma once

#include "AbstractGlFrameBuffer.hpp"
#include "EntitiesFrameBuffer.hpp"
#include <epoxy/gl.h>
#include <epoxy/gl_generated.h>

namespace ui::widgets::opengl::fb {
class EntitiesFrameBuffer : public AbstractGlFrameBuffer {
 public:
    EntitiesFrameBuffer(GLsizei sizeX, GLsizei sizeY);
    EntitiesFrameBuffer(EntitiesFrameBuffer& other) = delete;
    EntitiesFrameBuffer(EntitiesFrameBuffer&& old) = delete;

    ~EntitiesFrameBuffer() override = default;

    EntitiesFrameBuffer& operator=(EntitiesFrameBuffer& other) = delete;
    EntitiesFrameBuffer& operator=(EntitiesFrameBuffer&& old) = delete;

 protected:
    void init_internal() override;
    void bind_internal() override;
    void cleanup_internal() override;
};
}  // namespace ui::widgets::opengl::fb