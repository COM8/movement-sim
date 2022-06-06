#pragma once

#include "AbstractGlObject.hpp"
#include <epoxy/gl.h>
#include <gtkmm/glarea.h>

namespace ui::widgets::opengl {
class ScreenSquareGlObject : public AbstractGlObject {
 private:
    GLuint vertShader{0};
    GLuint geomShader{0};
    GLuint fragShader{0};

    GLint screenSizeConst{0};
    GLint textureSizeConst{0};

    GLuint fbufTexture{0};

    Gtk::GLArea* glArea{nullptr};

 public:
    ScreenSquareGlObject() = default;
    ScreenSquareGlObject(ScreenSquareGlObject& other) = delete;
    ScreenSquareGlObject(ScreenSquareGlObject&& old) = delete;

    ~ScreenSquareGlObject() override = default;

    ScreenSquareGlObject& operator=(ScreenSquareGlObject& other) = delete;
    ScreenSquareGlObject& operator=(ScreenSquareGlObject&& old) = delete;

    void set_glArea(Gtk::GLArea* glArea);
    void set_fb_texture(GLuint fbufTexture);

 protected:
    void init_internal() override;
    void render_internal() override;
    void cleanup_internal() override;
};
}  // namespace ui::widgets::opengl
