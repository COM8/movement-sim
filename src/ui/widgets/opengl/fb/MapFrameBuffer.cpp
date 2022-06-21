#include "MapFrameBuffer.hpp"
#include "ui/widgets/opengl/fb/AbstractGlFrameBuffer.hpp"

namespace ui::widgets::opengl::fb {

MapFrameBuffer::MapFrameBuffer(GLsizei sizeX, GLsizei sizeY) : AbstractGlFrameBuffer(sizeX, sizeY) {}

void MapFrameBuffer::init_internal() {}

void MapFrameBuffer::bind_internal() {}

void MapFrameBuffer::cleanup_internal() {}

}  // namespace ui::widgets::opengl::fb