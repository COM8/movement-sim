
#include "MapGlObject.hpp"
#include "sim/Entity.hpp"
#include "sim/Map.hpp"
#include "sim/Simulator.hpp"
#include <cassert>

namespace ui::widgets::opengl {
void MapGlObject::init_internal() {
    assert(simulator);
    const std::shared_ptr<sim::Map> map = simulator->get_map();
    assert(map);

    // Vertex data:
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(sim::RoadPiece) * map->roadPieces.size()), static_cast<void*>(map->roadPieces.data()), GL_DYNAMIC_DRAW);
    GLERR;

    // Compile shader:
    vertShader = compile_shader("/ui/shader/map/map.vert", GL_VERTEX_SHADER);
    assert(vertShader > 0);
    fragShader = compile_shader("/ui/shader/map/map.frag", GL_FRAGMENT_SHADER);
    assert(fragShader > 0);

    // Prepare program:
    glAttachShader(shaderProg, vertShader);
    glAttachShader(shaderProg, fragShader);
    glBindFragDataLocation(shaderProg, 0, "outColor");
    glLinkProgram(shaderProg);
    GLERR;

    // Check for errors during linking:
    GLint status = GL_FALSE;
    glGetProgramiv(shaderProg, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len = 0;
        glGetProgramiv(shaderProg, GL_INFO_LOG_LENGTH, &log_len);

        std::string log_msg;
        log_msg.resize(log_len);
        glGetProgramInfoLog(shaderProg, log_len, nullptr, static_cast<GLchar*>(log_msg.data()));
        SPDLOG_ERROR("Linking map shader program failed: {}", log_msg);
        glDeleteProgram(shaderProg);
        shaderProg = 0;
    } else {
        glDetachShader(shaderProg, fragShader);
        glDetachShader(shaderProg, vertShader);
    }
    GLERR;

    // Bind attributes:
    glUseProgram(shaderProg);
    GLint colAttrib = glGetAttribLocation(shaderProg, "color");
    GLERR;
    glEnableVertexAttribArray(colAttrib);
    GLERR;
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    glVertexAttribPointer(colAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(sim::RoadPiece), reinterpret_cast<void*>(2 * sizeof(sim::Vec2)));
    GLERR;

    GLint posAttrib = glGetAttribLocation(shaderProg, "position");
    glEnableVertexAttribArray(posAttrib);
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(sim::RoadPiece), nullptr);
    GLERR;

    GLint worldSizeConst = glGetUniformLocation(shaderProg, "worldSize");
    GLERR;
    glUniform2f(worldSizeConst, map->width, map->height);
    GLERR;
}

void MapGlObject::render_internal() {
    assert(simulator);
    const std::shared_ptr<sim::Map> map = simulator->get_map();
    assert(map);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(sizeof(sim::RoadPiece) * map->roadPieces.size()), static_cast<void*>(map->roadPieces.data()));

    glLineWidth(1);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(simulator->get_map()->roadPieces.size()));
}

void MapGlObject::cleanup_internal() {
    glDeleteShader(fragShader);
    glDeleteShader(fragShader);
}
}  // namespace ui::widgets::opengl