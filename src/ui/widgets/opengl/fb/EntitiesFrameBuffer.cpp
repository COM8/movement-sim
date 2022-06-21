#include "EntitiesFrameBuffer.hpp"
#include "ui/widgets/opengl/fb/AbstractGlFrameBuffer.hpp"

namespace ui::widgets::opengl::fb {

EntitiesFrameBuffer::EntitiesFrameBuffer(GLsizei sizeX, GLsizei sizeY) : AbstractGlFrameBuffer(sizeX, sizeY) {}

void EntitiesFrameBuffer::init_internal() {}

void EntitiesFrameBuffer::bind_internal() {}

void EntitiesFrameBuffer::cleanup_internal() {}

}  // namespace ui::widgets::opengl::fb