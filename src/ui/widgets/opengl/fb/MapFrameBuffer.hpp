#pragma once

#include "AbstractGlFrameBuffer.hpp"
#include "MapFrameBuffer.hpp"
#include <epoxy/gl.h>
#include <epoxy/gl_generated.h>

namespace ui::widgets::opengl::fb {
class MapFrameBuffer : public AbstractGlFrameBuffer {
 public:
    MapFrameBuffer(GLsizei sizeX, GLsizei sizeY);
    MapFrameBuffer(MapFrameBuffer& other) = delete;
    MapFrameBuffer(MapFrameBuffer&& old) = delete;

    ~MapFrameBuffer() override = default;

    MapFrameBuffer& operator=(MapFrameBuffer& other) = delete;
    MapFrameBuffer& operator=(MapFrameBuffer&& old) = delete;

 protected:
    void init_internal() override;
    void bind_internal() override;
    void cleanup_internal() override;
};
}  // namespace ui::widgets::opengl::fb