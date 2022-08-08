#include "QuadTreeGridFrameBuffer.hpp"
#include "ui/widgets/opengl/fb/AbstractGlFrameBuffer.hpp"

namespace ui::widgets::opengl::fb {

QuadTreeGridFrameBuffer::QuadTreeGridFrameBuffer(GLsizei sizeX, GLsizei sizeY) : AbstractGlFrameBuffer(sizeX, sizeY) {}

void QuadTreeGridFrameBuffer::init_internal() {}

void QuadTreeGridFrameBuffer::bind_internal() {}

void QuadTreeGridFrameBuffer::cleanup_internal() {}

}  // namespace ui::widgets::opengl::fb