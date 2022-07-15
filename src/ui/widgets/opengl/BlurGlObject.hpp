#pragma once

#include "AbstractGlObject.hpp"
#include <array>
#include <cstddef>
#include <epoxy/gl.h>
#include <epoxy/gl_generated.h>
#include <gtkmm/glarea.h>

namespace ui::widgets::opengl {
class BlurGlObject : public AbstractGlObject {
 private:
    GLuint vertShader{0};
    GLuint geomShader{0};
    GLuint fragShader{0};

    GLuint inputTexture{0};
    GLsizei inputTextureSizeX{0};
    GLsizei inputTextureSizeY{0};
    GLuint offsetsUBO{0};

 public:
    BlurGlObject() = default;
    BlurGlObject(BlurGlObject& other) = delete;
    BlurGlObject(BlurGlObject&& old) = delete;

    ~BlurGlObject() override = default;

    BlurGlObject& operator=(BlurGlObject& other) = delete;
    BlurGlObject& operator=(BlurGlObject&& old) = delete;

    void bind_texture(GLuint inputTexture);
    void set_texture_size(GLsizei sizeX, GLsizei sizeY);

 protected:
    void init_internal() override;
    void render_internal() override;
    void cleanup_internal() override;
};
}  // namespace ui::widgets::opengl
