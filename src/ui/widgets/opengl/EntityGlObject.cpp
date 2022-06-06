
#include "EntityGlObject.hpp"
#include "sim/Entity.hpp"
#include "sim/Simulator.hpp"
#include <cassert>

namespace ui::widgets::opengl {
void EntityGlObject::set_entities(std::shared_ptr<std::vector<sim::Entity>>& entities) {
    entityCount = entities ? static_cast<GLsizei>(entities->size()) : 0;
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(sim::Entity)) * entityCount, static_cast<void*>(entities->data()), GL_DYNAMIC_DRAW);
}

void EntityGlObject::init_internal() {
    assert(simulator);
    const std::shared_ptr<sim::Map> map = simulator->get_map();
    assert(map);

    // Vertex data:
    glBufferData(GL_ARRAY_BUFFER, sizeof(sim::Entity) * sim::MAX_ENTITIES, nullptr, GL_DYNAMIC_DRAW);

    // Compile shader:
    vertShader = compile_shader("/ui/shader/person/person.vert", GL_VERTEX_SHADER);
    assert(vertShader > 0);
    geomShader = compile_shader("/ui/shader/person/person.geom", GL_GEOMETRY_SHADER);
    assert(geomShader > 0);
    fragShader = compile_shader("/ui/shader/person/person.frag", GL_FRAGMENT_SHADER);
    assert(fragShader > 0);

    // Prepare program:
    glAttachShader(shaderProg, vertShader);
    glAttachShader(shaderProg, geomShader);
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
        SPDLOG_ERROR("Linking entity shader program failed: {}", log_msg);
        glDeleteProgram(shaderProg);
        shaderProg = 0;
    } else {
        glDetachShader(shaderProg, fragShader);
        glDetachShader(shaderProg, vertShader);
        glDetachShader(shaderProg, geomShader);
    }
    GLERR;

    // Bind attributes:
    glUseProgram(shaderProg);
    GLint colAttrib = glGetAttribLocation(shaderProg, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(sim::Entity), nullptr);

    GLint posAttrib = glGetAttribLocation(shaderProg, "position");
    glEnableVertexAttribArray(posAttrib);
    // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(sim::Entity), reinterpret_cast<void*>(4 * sizeof(GLfloat)));

    worldSizeConst = glGetUniformLocation(shaderProg, "worldSize");
    glUniform2f(worldSizeConst, map->width, map->height);

    rectSizeConst = glGetUniformLocation(shaderProg, "rectSize");
    glUniform2f(rectSizeConst, 1, 1);
    GLERR;
}

void EntityGlObject::render_internal() {
    glDrawArrays(GL_POINTS, 0, entityCount);
}

void EntityGlObject::cleanup_internal() {
    glDeleteShader(fragShader);
    glDeleteShader(geomShader);
    glDeleteShader(fragShader);
}
}  // namespace ui::widgets::opengl