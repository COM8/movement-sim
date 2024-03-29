#pragma once

#include "AbstractGlObject.hpp"
#include "sim/Entity.hpp"
#include "sim/GpuQuadTree.hpp"
#include <cstddef>
#include <memory>
#include <vector>
#include <epoxy/gl.h>

namespace ui::widgets::opengl {
class QuadTreeGridGlObject : public AbstractGlObject {
 private:
    GLuint vertShader{0};
    GLuint fragShader{0};

    GLint worldSizeConst{0};

    std::vector<sim::Vec2> vertices;
    GLsizei verticesCount{0};

 public:
    QuadTreeGridGlObject() = default;
    QuadTreeGridGlObject(QuadTreeGridGlObject& other) = delete;
    QuadTreeGridGlObject(QuadTreeGridGlObject&& old) = delete;

    ~QuadTreeGridGlObject() override = default;

    QuadTreeGridGlObject& operator=(QuadTreeGridGlObject& other) = delete;
    QuadTreeGridGlObject& operator=(QuadTreeGridGlObject&& old) = delete;

    void set_quad_tree_nodes(const std::shared_ptr<std::vector<sim::gpu_quad_tree::Node>>& nodes);

 private:
    void add_node_rec(const std::shared_ptr<std::vector<sim::gpu_quad_tree::Node>>& nodes, const sim::gpu_quad_tree::Node& node, GLsizei& newVerticesCount);

 protected:
    void init_internal() override;
    void render_internal() override;
    void cleanup_internal() override;
};
}  // namespace ui::widgets::opengl
