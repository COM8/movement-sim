#pragma once

#include "AbstractGlObject.hpp"
#include <array>
#include <epoxy/gl.h>
#include <epoxy/gl_generated.h>
#include <gtkmm/glarea.h>

namespace ui::widgets::opengl {
class ScreenSquareGlObject : public AbstractGlObject {
 private:
    GLuint vertShader{0};
    GLuint geomShader{0};
    GLuint fragShader{0};

    GLint screenSizeConst{0};
    GLint textureSizeConst{0};
    GLint quadTreeGridVisibleConst{0};

    /**
     * All textures that should be passed to the shader.
     * frameBufferTextures[0] is the map texture.
     * frameBufferTextures[1] is the entities texture.
     * frameBufferTextures[2] is the quad tree grid texture.
     **/
    std::array<GLuint, 3> frameBufferTextures{};

    Gtk::GLArea* glArea{nullptr};

 public:
    ScreenSquareGlObject() = default;
    ScreenSquareGlObject(ScreenSquareGlObject& other) = delete;
    ScreenSquareGlObject(ScreenSquareGlObject&& old) = delete;

    ~ScreenSquareGlObject() override = default;

    ScreenSquareGlObject& operator=(ScreenSquareGlObject& other) = delete;
    ScreenSquareGlObject& operator=(ScreenSquareGlObject&& old) = delete;

    void set_glArea(Gtk::GLArea* glArea);
    void bind_texture(GLuint mapFrameBufferTexture, GLuint entitiesFrameBufferTexture, GLuint quadTreeGridFrameBufferTexture);
    void set_quad_tree_grid_visibility(bool quadTreeGridVisible) const;

 protected:
    void init_internal() override;
    void render_internal() override;
    void cleanup_internal() override;
};
}  // namespace ui::widgets::opengl
