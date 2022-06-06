#pragma once

#include "AbstractGlObject.hpp"
#include <epoxy/gl.h>

namespace ui::widgets::opengl {
class MapGlObject : public AbstractGlObject {
 private:
    GLuint vertShader{0};
    GLuint fragShader{0};

    GLint worldSizeConst{0};
    GLint rectSizeConst{0};

 public:
    MapGlObject() = default;
    MapGlObject(MapGlObject& other) = delete;
    MapGlObject(MapGlObject&& old) = delete;

    ~MapGlObject() override = default;

    MapGlObject& operator=(MapGlObject& other) = delete;
    MapGlObject& operator=(MapGlObject&& old) = delete;

 protected:
    void init_internal() override;
    void render_internal() override;
    void cleanup_internal() override;
};
}  // namespace ui::widgets::opengl
