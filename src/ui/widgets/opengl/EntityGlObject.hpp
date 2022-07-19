#pragma once

#include "AbstractGlObject.hpp"
#include "sim/Entity.hpp"
#include <memory>
#include <vector>
#include <epoxy/gl.h>

namespace ui::widgets::opengl {
class EntityGlObject : public AbstractGlObject {
 private:
    GLuint vertShader{0};
    GLuint geomShader{0};
    GLuint fragShader{0};
    GLuint computeShader{0};

    GLint worldSizeConst{0};
    GLint rectSizeConst{0};

    GLsizei entityCount{0};

 public:
    EntityGlObject() = default;
    EntityGlObject(EntityGlObject& other) = delete;
    EntityGlObject(EntityGlObject&& old) = delete;

    ~EntityGlObject() override = default;

    EntityGlObject& operator=(EntityGlObject& other) = delete;
    EntityGlObject& operator=(EntityGlObject&& old) = delete;

    void set_entities(const std::shared_ptr<std::vector<sim::Entity>>& entities);

 protected:
    void init_internal() override;
    void render_internal() override;
    void cleanup_internal() override;

    void init_compute_shader();
};
}  // namespace ui::widgets::opengl
