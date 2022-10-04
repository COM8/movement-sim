#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sim::gpu_quad_tree {
enum class NextType : uint32_t {
    INVALID = 0,
    NODE = 1,
    ENTITY = 2
};

struct Node {
    int32_t acquireLock{0};
    int32_t writeLock{0};
    int32_t readerLock{0};

    float offsetX{0};
    float offsetY{0};
    float width{0};
    float height{0};

    /**
     * NextType::INVALID - Has no entities and all subnodes are not populated.
     * NextType::NODE - Has no direct entities but sub nodes are populated.
     * NextType::ENTITY - Has entities and no subnodes.
     **/
    NextType contentType{NextType::INVALID};
    uint32_t entityCount{0};
    uint32_t first{0};

    uint32_t prevNodeIndex{0};

    uint32_t nextTL{0};
    uint32_t nextTR{0};
    uint32_t nextBL{0};
    uint32_t nextBR{0};

    uint32_t padding{0};
} __attribute__((packed)) __attribute__((aligned(64)));

// NOLINTNEXTLINE (altera-struct-pack-align) Ignore alignment since we need a compact layout.
struct Entity {
    uint32_t nodeIndex{0};

    /**
     * NextType::INVALID - This is the last entity in the line.
     * NextType::NODE - Never.
     * NextType::ENTITY - Points to the next entity in this node.
     **/
    NextType typeNext{NextType::INVALID};
    uint32_t next{0};

    /**
     * NextType::INVALID - This is the first entity in the line.
     * NextType::NODE - Never.
     * NextType::ENTITY - Points to the previous entity in this node.
     **/
    NextType typePrev{NextType::INVALID};
    uint32_t prev{0};
} __attribute__((packed)) __attribute__((aligned(4)));

void init_node_zero(Node& node, float worldSizeX, float worldSizeY);

size_t calc_node_count(size_t maxDepth);
}  // namespace sim::gpu_quad_tree