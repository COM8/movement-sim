#include <array>
#include <atomic>
#include <barrier>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <thread>

template <class T>
struct vec4T {
    T x;
    T y;
    T z;
    T w;

    explicit vec4T(T val) : x(val), y(val), z(val), w(val) {}
    vec4T(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

    bool operator==(const vec4T<T>& other) const {
        return other.x == x && other.y == y && other.z == z && other.w == w;
    }
} __attribute__((packed)) __attribute__((aligned(16)));

template <class T>
struct vec2T {
    T x;
    T y;

    explicit vec2T(T val) : x(val), y(val) {}
    vec2T(T x, T y) : x(x), y(y) {}

    bool operator==(const vec2T<T>& other) const {
        return other.x == x && other.y == y;
    }

    vec2T<T> operator-(const vec2T<T>& other) {
        return {x - other.x, y - other.y};
    }

    vec2T<T>& operator-=(const vec2T<T>& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    vec2T<T> operator+(const vec2T<T>& other) {
        return {x + other.x, y + other.y};
    }

    vec2T<T>& operator+=(const vec2T<T>& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    vec2T<T>& operator*=(const vec2T<T>& other) {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    vec2T<T> operator*(const vec2T<T>& other) {
        return {x * other.x, y * other.y};
    }

    vec2T<T>& operator/=(const vec2T<T>& other) {
        x /= other.x;
        y /= other.y;
        return *this;
    }

    vec2T<T> operator/(const vec2T<T>& other) {
        return {x / other.x, y / other.y};
    }

} __attribute__((packed)) __attribute__((aligned(8)));

using uint = uint32_t;

using vec4 = vec4T<float>;
using vec2 = vec2T<float>;
using uvec4 = vec4T<uint>;

template <class T>
void atomicAdd(std::atomic<T>& data, T val) {
    data += val;
    std::atomic_thread_fence(std::memory_order_release);
}

void memoryBarrierBuffer() {
    std::atomic_thread_fence(std::memory_order_release);
}

template <class T>
T atomicCompSwap(std::atomic<T>& data, T comp, T val) {
    T expected = comp;
    if (data.compare_exchange_strong(expected, val)) {
        return comp;
    }
    return expected;
}

template <class T>
T atomicExchange(std::atomic<T>& data, T val) {
    return data.exchange(val);
}

float distance(const vec2& v1, const vec2& v2) {
    return static_cast<float>(std::sqrt(std::pow(v2.x - v1.x, 2) + std::pow(v2.y - v1.y, 2)));
}

float length(const vec2& v1) {
    return static_cast<float>(std::sqrt(std::pow(v1.x, 2) + std::pow(v1.y, 2)));
}

vec2 clamp(const vec2& v, const vec2& minVec, const vec2& maxVec) {
    return {std::max(minVec.x, std::min(maxVec.x, v.x)), std::max(minVec.y, std::min(maxVec.y, v.y))};
}

struct EntityDescriptor {
    vec4 color{0};  // Offset: 0-15
    uvec4 randState{0};  // Offset 16-31
    vec2 pos{0};  // Offset: 32-39
    vec2 target{0};  // Offset: 40-47
    vec2 direction{0};  // Offset: 48-55
    uint roadIndex{0};  // Offset: 56-59
    uint initialized{0};  // Offset: 60-63
} __attribute__((packed)) __attribute__((aligned(64)));  // Size will be rounded up to the next multiple of the largest member (vec4) -> 64 Bytes

struct PushConstantsDescriptor {
    float worldSizeX{0};
    float worldSizeY{0};

    uint nodeCount{0};
    uint maxDepth{0};

    float collisionRadius{0};

    uint tick{0};
} __attribute__((aligned(32)));

const size_t MAX_DEPTH = 8;
const size_t NUM_NODES = 21845;
const size_t NUM_ENTITIES = 1000;  // 246400;
const float COLLISION_RADIUS = 10;

static std::array<EntityDescriptor, NUM_ENTITIES> entities{};
static PushConstantsDescriptor pushConsts{29007.4609, 16463.7656, NUM_NODES, MAX_DEPTH, COLLISION_RADIUS};

// ------------------------------------------------------------------------------------
// Quad Tree
// ------------------------------------------------------------------------------------
uint TYPE_INVALID = 0;
uint TYPE_NODE = 1;
uint TYPE_ENTITY = 2;

// NOLINTNEXTLINE (hicpp-special-member-functions)
struct QuadTreeNodeDescriptor {
    std::atomic_int acquireLock{0};
    std::atomic_int writeLock{0};
    std::atomic_int readerLock{0};

    float offsetX{0};
    float offsetY{0};
    float width{0};
    float height{0};

    uint contentType{0};
    uint entityCount{0};
    uint first{0};

    uint prevNodeIndex{0};

    uint nextTL{0};
    uint nextTR{0};
    uint nextBL{0};
    uint nextBR{0};

    uint padding{0};

    QuadTreeNodeDescriptor& operator=(const QuadTreeNodeDescriptor& other) {
        if (this == &other) {
            return *this;
        }

        acquireLock = static_cast<int>(other.acquireLock);
        writeLock = static_cast<int>(other.writeLock);
        readerLock = static_cast<int>(other.readerLock);

        offsetX = other.offsetX;
        offsetY = other.offsetY;
        width = other.width;
        height = other.height;

        contentType = other.contentType;
        entityCount = other.entityCount;
        first = other.first;

        prevNodeIndex = other.prevNodeIndex;

        nextTL = other.nextTL;
        nextTR = other.nextTR;
        nextBL = other.nextBL;
        nextBR = other.nextBR;

        padding = other.padding;

        return *this;
    }
} __attribute__((aligned(64)));

struct QuadTreeEntityDescriptor {
    uint nodeIndex{0};

    uint typeNext{0};
    uint next{0};

    uint typePrev{0};
    uint prev{0};
} __attribute__((aligned(32)));

// TODO add memory qualifiers: https://www.khronos.org/opengl/wiki/Shader_Storage_Buffer_Object
static std::array<QuadTreeNodeDescriptor, NUM_NODES> quadTreeNodes{};
static std::array<QuadTreeEntityDescriptor, NUM_ENTITIES> quadTreeEntities{};
/**
 * [0]: Lock
 * [1]: Next free hint
 * [2... (nodeCount + 2)]: Node locks
 **/
static std::array<std::atomic<uint>, NUM_NODES + 2> quadTreeNodeUsedStatus{};
static std::array<std::atomic<uint>, 10> debugData{};

uint NODE_ENTITY_CAP = 100;

void to_dot_graph_rec(std::string& result, uint nodeIndex);
std::string to_dot_graph();

size_t count_entities_rec(uint nodeIndex) {
    if (quadTreeNodes[nodeIndex].contentType == TYPE_NODE) {
        assert(quadTreeNodes[nodeIndex].entityCount == 0);
        return count_entities_rec(quadTreeNodes[nodeIndex].nextTL) + count_entities_rec(quadTreeNodes[nodeIndex].nextTR) + count_entities_rec(quadTreeNodes[nodeIndex].nextBL) + count_entities_rec(quadTreeNodes[nodeIndex].nextBR);
    }
    assert(quadTreeNodes[nodeIndex].contentType == TYPE_ENTITY);

    uint expectedCount = 0;
    if (quadTreeNodes[nodeIndex].entityCount > 0) {
        uint index = quadTreeNodes[nodeIndex].first;
        while (quadTreeEntities[index].typeNext == TYPE_ENTITY) {
            expectedCount++;
            index = quadTreeEntities[index].next;
        }
        expectedCount++;  // Also count the last one
    }

    uint actualCount = quadTreeNodes[nodeIndex].entityCount;
    if (actualCount != expectedCount) {
        // std::cout << to_dot_graph() << '\n';
    }
    assert(actualCount == expectedCount);
    return quadTreeNodes[nodeIndex].entityCount;
}

void validate_entity_count(size_t expected) {
    size_t count = count_entities_rec(0);
    if (count != expected) {
        std::cout << to_dot_graph() << '\n';
        std::cerr << "Count != Expected " << expected << " != " << count << '\n';
    }
    assert(count == expected);
}

void quad_tree_lock_node_read(uint nodeIndex) {
    assert(quadTreeNodes[nodeIndex].acquireLock >= 0);
    while (atomicCompSwap(quadTreeNodes[nodeIndex].acquireLock, 0, 1) != 0) {}

    // Prevent from reading, when we are currently writing:
    assert(quadTreeNodes[nodeIndex].writeLock >= 0);
    while (quadTreeNodes[nodeIndex].writeLock != 0) {}
    atomicAdd(quadTreeNodes[nodeIndex].readerLock, 1);

    atomicExchange(quadTreeNodes[nodeIndex].acquireLock, 0);
    memoryBarrierBuffer();
}

void quad_tree_unlock_node_read(uint nodeIndex) {
    assert(quadTreeNodes[nodeIndex].readerLock > 0);
    atomicAdd(quadTreeNodes[nodeIndex].readerLock, -1);
    memoryBarrierBuffer();
}

/**
 * Locks read and write for the given nodeIndex.
 **/
void quad_tree_lock_node_read_write(uint nodeIndex) {
    assert(quadTreeNodes[nodeIndex].acquireLock >= 0);
    while (atomicCompSwap(quadTreeNodes[nodeIndex].acquireLock, 0, 1) != 0) {}

    // Wait until all others stopped reading:
    assert(quadTreeNodes[nodeIndex].readerLock >= 0);
    while (atomicCompSwap(quadTreeNodes[nodeIndex].readerLock, 0, 1) != 0) {}
    assert(quadTreeNodes[nodeIndex].writeLock >= 0);
    while (atomicCompSwap(quadTreeNodes[nodeIndex].writeLock, 0, 1) != 0) {}

    atomicExchange(quadTreeNodes[nodeIndex].acquireLock, 0);
    memoryBarrierBuffer();
}

void quad_tree_unlock_node_write(uint nodeIndex) {
    assert(quadTreeNodes[nodeIndex].writeLock == 1);
    atomicExchange(quadTreeNodes[nodeIndex].writeLock, 0);
    memoryBarrierBuffer();
}

void quad_tree_init_entity(uint index, uint typeNext, uint next, uint nodeIndex) {
    quadTreeEntities[index].typeNext = typeNext;
    quadTreeEntities[index].next = next;
    quadTreeEntities[index].typePrev = TYPE_INVALID;
    quadTreeEntities[index].prev = 0;
    quadTreeEntities[index].nodeIndex = nodeIndex;
}

void quad_tree_append_entity(uint nodeIndex, uint index) {
    if (quadTreeNodes[nodeIndex].entityCount <= 0) {
        quadTreeNodes[nodeIndex].first = index;
        quad_tree_init_entity(index, TYPE_INVALID, 0, nodeIndex);
    } else {
        // Add in front:
        uint oldFirstIndex = quadTreeNodes[nodeIndex].first;
        quad_tree_init_entity(index, TYPE_ENTITY, oldFirstIndex, nodeIndex);

        quadTreeEntities[oldFirstIndex].typePrev = TYPE_ENTITY;
        quadTreeEntities[oldFirstIndex].prev = index;

        quadTreeNodes[nodeIndex].first = index;
    }
    quadTreeNodes[nodeIndex].entityCount++;
    memoryBarrierBuffer();
}

/**
 * Moves up the quad tree and unlocks all nodes from reading again.
 **/
void quad_tree_unlock_nodes_read(uint nodeIndex) {
    while (quadTreeNodes[nodeIndex].prevNodeIndex != nodeIndex) {
        uint oldNodeIndex = nodeIndex;
        nodeIndex = quadTreeNodes[nodeIndex].prevNodeIndex;
        quad_tree_unlock_node_read(oldNodeIndex);
    }
    // Unlock the first node:
    quad_tree_unlock_node_read(nodeIndex);
}

uint quad_tree_get_free_node_index() {
    uint index = 0;
    uint i = quadTreeNodeUsedStatus[1];
    while (index <= 0) {
        if (quadTreeNodeUsedStatus[i] == 0) {
            quadTreeNodeUsedStatus[i] = 1;
            index = i - 2;
        }

        i++;
        if (i >= (pushConsts.nodeCount + 2)) {
            i = 2;
        }
    }
    quadTreeNodeUsedStatus[1] = i;
    return index;
}

uvec4 quad_tree_get_free_node_indices() {
    uvec4 indices = uvec4(0);
    while (atomicCompSwap(quadTreeNodeUsedStatus[0], static_cast<uint>(0), static_cast<uint>(1)) != 0) {}

    indices.x = quad_tree_get_free_node_index();
    indices.y = quad_tree_get_free_node_index();
    indices.z = quad_tree_get_free_node_index();
    indices.w = quad_tree_get_free_node_index();

    memoryBarrierBuffer();
    atomicExchange(quadTreeNodeUsedStatus[0], static_cast<uint>(0));

    return indices;
}

void quad_tree_free_node_indices(uvec4 indices) {
    while (atomicCompSwap(quadTreeNodeUsedStatus[0], static_cast<uint>(0), static_cast<uint>(1)) != 0) {}

    quadTreeNodeUsedStatus[indices.x + 2] = 0;
    quadTreeNodeUsedStatus[indices.y + 2] = 0;
    quadTreeNodeUsedStatus[indices.z + 2] = 0;
    quadTreeNodeUsedStatus[indices.w + 2] = 0;

    quadTreeNodeUsedStatus[1] = indices.x + 2;

    memoryBarrierBuffer();
    atomicExchange(quadTreeNodeUsedStatus[0], static_cast<uint>(0));
}

void quad_tree_init_node(uint nodeIndex, uint prevNodeIndex, float offsetX, float offsetY, float width, float height) {
    quadTreeNodes[nodeIndex].acquireLock = 0;
    quadTreeNodes[nodeIndex].writeLock = 0;
    quadTreeNodes[nodeIndex].readerLock = 0;

    quadTreeNodes[nodeIndex].offsetX = offsetX;
    quadTreeNodes[nodeIndex].offsetY = offsetY;
    quadTreeNodes[nodeIndex].width = width;
    quadTreeNodes[nodeIndex].height = height;

    quadTreeNodes[nodeIndex].prevNodeIndex = prevNodeIndex;
    quadTreeNodes[nodeIndex].contentType = TYPE_ENTITY;
    quadTreeNodes[nodeIndex].entityCount = 0;
}

void quad_tree_move_entities(uint nodeIndex) {
    float offsetXNext = quadTreeNodes[nodeIndex].offsetX + (quadTreeNodes[nodeIndex].width / 2);
    float offsetYNext = quadTreeNodes[nodeIndex].offsetY + (quadTreeNodes[nodeIndex].height / 2);

    uint index = quadTreeNodes[nodeIndex].first;
    quadTreeNodes[nodeIndex].first = 0;
    quadTreeNodes[nodeIndex].entityCount = 0;

    bool hasNext = false;
    do {
        uint nextIndex = quadTreeEntities[index].next;
        hasNext = quadTreeEntities[index].typeNext != TYPE_INVALID;
        quadTreeEntities[index].next = 0;
        quadTreeEntities[index].typeNext = TYPE_INVALID;

        vec2 ePos = entities[index].pos;

        uint newNodeIndex = 0;
        // Left:
        if (ePos.x < offsetXNext) {
            // Top:
            if (ePos.y < offsetYNext) {
                newNodeIndex = quadTreeNodes[nodeIndex].nextTL;
            } else {
                newNodeIndex = quadTreeNodes[nodeIndex].nextBL;
            }
        }
        // Right:
        else {
            // Top:
            if (ePos.y < offsetYNext) {
                newNodeIndex = quadTreeNodes[nodeIndex].nextTR;
            } else {
                newNodeIndex = quadTreeNodes[nodeIndex].nextBR;
            }
        }
        quad_tree_append_entity(newNodeIndex, index);
        index = nextIndex;
    } while (hasNext);
}

void quad_tree_split_up_node(uint nodeIndex) {
    quadTreeNodes[nodeIndex].contentType = TYPE_NODE;

    uvec4 newNodeIndices = quad_tree_get_free_node_indices();

    float newWidth = quadTreeNodes[nodeIndex].width / 2;
    float newHeight = quadTreeNodes[nodeIndex].height / 2;

    quadTreeNodes[nodeIndex].nextTL = newNodeIndices.x;
    quad_tree_init_node(quadTreeNodes[nodeIndex].nextTL, nodeIndex, quadTreeNodes[nodeIndex].offsetX, quadTreeNodes[nodeIndex].offsetY, newWidth, newHeight);
    quadTreeNodes[nodeIndex].nextTR = newNodeIndices.y;
    quad_tree_init_node(quadTreeNodes[nodeIndex].nextTR, nodeIndex, quadTreeNodes[nodeIndex].offsetX + newWidth, quadTreeNodes[nodeIndex].offsetY, newWidth, newHeight);
    quadTreeNodes[nodeIndex].nextBL = newNodeIndices.z;
    quad_tree_init_node(quadTreeNodes[nodeIndex].nextBL, nodeIndex, quadTreeNodes[nodeIndex].offsetX, quadTreeNodes[nodeIndex].offsetY + newHeight, newWidth, newHeight);
    quadTreeNodes[nodeIndex].nextBR = newNodeIndices.w;
    quad_tree_init_node(quadTreeNodes[nodeIndex].nextBR, nodeIndex, quadTreeNodes[nodeIndex].offsetX + newWidth, quadTreeNodes[nodeIndex].offsetY + newHeight, newWidth, newHeight);

    quad_tree_move_entities(nodeIndex);
    memoryBarrierBuffer();
}

bool quad_tree_same_pos_as_fist(uint nodeIndex, vec2 ePos) {
    if (quadTreeNodes[nodeIndex].entityCount > 0) {
        uint index = quadTreeNodes[nodeIndex].first;
        return entities[index].pos == ePos;
    }
    return false;
}

void quad_tree_insert(uint index, uint startNodeIndex, uint startNodeDepth) {
    // Count the number of inserted items:
    atomicAdd(debugData[0], static_cast<uint>(1));

    vec2 ePos = entities[index].pos;
    uint curDepth = startNodeDepth;

    uint nodeIndex = startNodeIndex;
    while (true) {
        quad_tree_lock_node_read(nodeIndex);
        float offsetXNext = quadTreeNodes[nodeIndex].offsetX + (quadTreeNodes[nodeIndex].width / 2);
        float offsetYNext = quadTreeNodes[nodeIndex].offsetY + (quadTreeNodes[nodeIndex].height / 2);

        // Go one node deeper:
        if (quadTreeNodes[nodeIndex].contentType == TYPE_NODE) {
            // Left:
            if (ePos.x < offsetXNext) {
                // Top:
                if (ePos.y < offsetYNext) {
                    nodeIndex = quadTreeNodes[nodeIndex].nextTL;
                } else {
                    nodeIndex = quadTreeNodes[nodeIndex].nextBL;
                }
            }
            // Right:
            else {
                // Top:
                if (ePos.y < offsetYNext) {
                    nodeIndex = quadTreeNodes[nodeIndex].nextTR;
                } else {
                    nodeIndex = quadTreeNodes[nodeIndex].nextBR;
                }
            }
            curDepth += 1;
        } else {
            // Prevent a deadlock:
            quad_tree_unlock_node_read(nodeIndex);
            quad_tree_lock_node_read_write(nodeIndex);

            // Check if something has changed in the meantime with the node. Retry...
            if (quadTreeNodes[nodeIndex].contentType == TYPE_NODE) {
                quad_tree_unlock_node_write(nodeIndex);
                quad_tree_unlock_node_read(nodeIndex);
            } else {
                // Insert the entity in case there is space left, we can't go deeper or the entity has the same pos as the last:
                if (quadTreeNodes[nodeIndex].entityCount < NODE_ENTITY_CAP || curDepth >= pushConsts.maxDepth || quad_tree_same_pos_as_fist(nodeIndex, ePos)) {
                    quad_tree_append_entity(nodeIndex, index);
                    memoryBarrierBuffer();
                    quad_tree_unlock_node_write(nodeIndex);
                    break;
                }
                // Split up
                memoryBarrierBuffer();
                quad_tree_split_up_node(nodeIndex);
                quad_tree_unlock_node_write(nodeIndex);
                quad_tree_unlock_node_read(nodeIndex);
            }
        }
        memoryBarrierBuffer();  // Ensure everything is in sync
    }
    // Unlock all nodes again:
    quad_tree_unlock_nodes_read(nodeIndex);
}

bool quad_tree_is_entity_on_node(uint nodeIndex, vec2 ePos) {
    return ePos.x >= quadTreeNodes[nodeIndex].offsetX && ePos.x < (quadTreeNodes[nodeIndex].offsetX + quadTreeNodes[nodeIndex].width) && ePos.y >= quadTreeNodes[nodeIndex].offsetY && ePos.y < (quadTreeNodes[nodeIndex].offsetY + quadTreeNodes[nodeIndex].height);
}

/**
 * Moves down the quad tree and locks all nodes as read, except the last node, which gets locked as write so we can edit it.
 **/
uint quad_tree_lock_for_entity_edit(uint index) {
    vec2 ePos = entities[index].pos;
    uint nodeIndex = 0;
    while (true) {
        quad_tree_lock_node_read(nodeIndex);
        float offsetXNext = quadTreeNodes[nodeIndex].offsetX + (quadTreeNodes[nodeIndex].width / 2);
        float offsetYNext = quadTreeNodes[nodeIndex].offsetY + (quadTreeNodes[nodeIndex].height / 2);

        // Go one node deeper:
        if (quadTreeNodes[nodeIndex].contentType == TYPE_NODE) {
            // Left:
            if (ePos.x < offsetXNext) {
                // Top:
                if (ePos.y < offsetYNext) {
                    nodeIndex = quadTreeNodes[nodeIndex].nextTL;
                } else {
                    nodeIndex = quadTreeNodes[nodeIndex].nextBL;
                }
            }
            // Right:
            else {
                // Top:
                if (ePos.y < offsetYNext) {
                    nodeIndex = quadTreeNodes[nodeIndex].nextTR;
                } else {
                    nodeIndex = quadTreeNodes[nodeIndex].nextBR;
                }
            }
            assert(nodeIndex != 0);
        } else {
            // Prevent a deadlock:
            quad_tree_unlock_node_read(nodeIndex);
            quad_tree_lock_node_read_write(nodeIndex);

            // Check if something has changed in the meantime with the node. Retry...
            if (quadTreeNodes[nodeIndex].contentType == TYPE_NODE) {
                quad_tree_unlock_node_write(nodeIndex);
                quad_tree_unlock_node_read(nodeIndex);
            } else {
                assert(quadTreeNodes[nodeIndex].contentType == TYPE_ENTITY);
                assert(nodeIndex != 0);
                assert(nodeIndex == quadTreeEntities[index].nodeIndex);
                return nodeIndex;
            }
        }
    }
}

/**
 * Removes the given entity from its node.
 * Returns true in case it was the last entity on this node.
 **/
bool quad_tree_remove_entity(uint index) {
    uint nodeIndex = quadTreeEntities[index].nodeIndex;
    // uint count = count_entities_rec(0);
    if (quadTreeNodes[nodeIndex].entityCount <= 1) {
        quadTreeNodes[nodeIndex].first = 0;
        quadTreeNodes[nodeIndex].entityCount = 0;
        // validate_entity_count(count - 1);
        return true;
    }

    if (quadTreeNodes[nodeIndex].first == index) {
        quadTreeNodes[nodeIndex].first = quadTreeEntities[index].next;
    }

    if (quadTreeEntities[index].typePrev == TYPE_ENTITY) {
        uint prevIndex = quadTreeEntities[index].prev;
        quadTreeEntities[prevIndex].next = quadTreeEntities[index].next;
        quadTreeEntities[prevIndex].typeNext = quadTreeEntities[index].typeNext;
    }

    if (quadTreeEntities[index].typeNext == TYPE_ENTITY) {
        uint nextIndex = quadTreeEntities[index].next;
        quadTreeEntities[nextIndex].prev = quadTreeEntities[index].prev;
        quadTreeEntities[nextIndex].typePrev = quadTreeEntities[index].typePrev;
    }
    quadTreeEntities[index].typePrev = TYPE_INVALID;
    quadTreeEntities[index].typeNext = TYPE_INVALID;
    quadTreeNodes[nodeIndex].entityCount -= 1;

    // validate_entity_count(count - 1);
    return false;
}

bool quad_tree_is_node_empty(uint nodeIndex) {
    return quadTreeNodes[nodeIndex].entityCount <= 0 && quadTreeNodes[nodeIndex].contentType == TYPE_ENTITY;
}

bool quad_tree_try_merging_subnode(uint nodeIndex) {
    if (quadTreeNodes[nodeIndex].contentType != TYPE_NODE) {
        return false;
    }

    if (quad_tree_is_node_empty(quadTreeNodes[nodeIndex].nextTL) && quad_tree_is_node_empty(quadTreeNodes[nodeIndex].nextTR) && quad_tree_is_node_empty(quadTreeNodes[nodeIndex].nextBL) && quad_tree_is_node_empty(quadTreeNodes[nodeIndex].nextBR)) {
        quad_tree_free_node_indices(uvec4(quadTreeNodes[nodeIndex].nextTL, quadTreeNodes[nodeIndex].nextTR, quadTreeNodes[nodeIndex].nextBL, quadTreeNodes[nodeIndex].nextBR));
        assert(quadTreeNodes[nodeIndex].entityCount <= 0);
        quadTreeNodes[nodeIndex].contentType = TYPE_ENTITY;
        // std::cout << "Merged nodes of " << nodeIndex << '\n';
        return true;
    }
    return false;
}

uint quad_tree_get_cur_depth(uint nodeIndex) {
    uint depth = 1;
    while (quadTreeNodes[nodeIndex].prevNodeIndex != nodeIndex) {
        nodeIndex = quadTreeNodes[nodeIndex].prevNodeIndex;
        depth++;
    }
    return depth;
}

void quad_tree_update(uint index, vec2 newPos) {
    // Lock for read:
    while (true) {
        uint oldNodeIndex = quadTreeEntities[index].nodeIndex;
        quad_tree_lock_node_read(oldNodeIndex);
        // Make sure our entity is still on the same node when we finally get the lock.
        // Could happen in case the current node got split up in the meantime.
        if (oldNodeIndex == quadTreeEntities[index].nodeIndex) {
            break;
        }
        quad_tree_unlock_node_read(oldNodeIndex);
    }

    // Still on the same node, so we do not need to do anything:
    if (quad_tree_is_entity_on_node(quadTreeEntities[index].nodeIndex, newPos)) {
        entities[index].pos = newPos;
        quad_tree_unlock_node_read(quadTreeEntities[index].nodeIndex);
        return;
    }

    uint nodeIndex = quadTreeEntities[index].nodeIndex;
    quad_tree_unlock_node_read(nodeIndex);
    nodeIndex = quad_tree_lock_for_entity_edit(index);
    if (nodeIndex != quadTreeEntities[index].nodeIndex) {
        // std::cout << to_dot_graph() << '\n';
        assert(nodeIndex == quadTreeEntities[index].nodeIndex);
    }
    if (quad_tree_remove_entity(index)) {
        // Try merging nodes:
        while (nodeIndex != quadTreeNodes[nodeIndex].prevNodeIndex) {
            quad_tree_unlock_node_write(nodeIndex);
            quad_tree_unlock_node_read(nodeIndex);
            nodeIndex = quadTreeNodes[nodeIndex].prevNodeIndex;

            // Prevent deadlocks:
            quad_tree_unlock_node_read(nodeIndex);
            quad_tree_lock_node_read_write(nodeIndex);

            if (!quad_tree_try_merging_subnode(nodeIndex)) {
                break;
            }
        }
    }
    quad_tree_unlock_node_write(nodeIndex);

    // Update the removed entity position:
    entities[index].pos = newPos;

    // Move up until we reach a node where our entity is on:
    while (!quad_tree_is_entity_on_node(nodeIndex, entities[index].pos)) {
        uint oldNodeIndex = nodeIndex;
        nodeIndex = quadTreeNodes[nodeIndex].prevNodeIndex;
        quad_tree_unlock_node_read(oldNodeIndex);
    }

    // Insert the entity again:
    quad_tree_unlock_node_read(nodeIndex);
    quad_tree_insert(index, nodeIndex, quad_tree_get_cur_depth(nodeIndex));
    // std::cout << "Updated " << index << '\n';
}

/**
 * Collision routine that gets called each time we notice a collision.
 * Called only once per collision pair.
 **/
void quad_tree_collision(uint index0, uint index1) {
    entities[index0].color = vec4(1, 0, 0, 1);
    entities[index1].color = vec4(1, 0, 0, 1);
    atomicAdd(debugData[1], static_cast<uint>(1));
}

bool quad_tree_in_range(const vec2& v1, const vec2& v2, float maxDistance) {
    // atomicAdd(debugData[6], static_cast<uint>(1));
    float dx = std::abs(v2.x - v1.x);
    if (dx > maxDistance) {
        return false;
    }

    float dy = std::abs(v2.y - v1.y);
    if (dy > maxDistance) {
        return false;
    }
    // atomicAdd(debugData[4], static_cast<uint>(1));
    return distance(v1, v2) < maxDistance;
}

void quad_tree_check_entity_collisions_on_node(uint index, uint nodeIndex) {
    // atomicAdd(debugData[5], static_cast<uint>(1));
    if (quadTreeNodes[nodeIndex].entityCount <= 0) {
        return;
    }

    vec2 ePos = entities[index].pos;

    uint curEntityIndex = quadTreeNodes[nodeIndex].first;
    while (true) {
        // atomicAdd(debugData[7], static_cast<uint>(1));
        // Prevent checking collision with our self and prevent duplicate entries by checking only for ones where the ID is smaller than ours:
        if (curEntityIndex < index && quad_tree_in_range(entities[curEntityIndex].pos, ePos, pushConsts.collisionRadius)) {
            quad_tree_collision(index, curEntityIndex);
        }

        if (quadTreeEntities[curEntityIndex].typeNext != TYPE_ENTITY) {
            break;
        }
        curEntityIndex = quadTreeEntities[curEntityIndex].next;
    }
}

/**
 * Returns the next (TL -> TR -> BL -> BR -> 0) node index of the parent node.
 * Returns 0 in case the given nodeIndex is BR of the parent node.
 **/
uint quad_tree_get_next_node_index(uint nodeIndex) {
    uint prevNodeIndex = quadTreeNodes[nodeIndex].prevNodeIndex;
    if (quadTreeNodes[prevNodeIndex].nextTL == nodeIndex) {
        return quadTreeNodes[prevNodeIndex].nextTR;
    }

    if (quadTreeNodes[prevNodeIndex].nextTR == nodeIndex) {
        return quadTreeNodes[prevNodeIndex].nextBL;
    }

    if (quadTreeNodes[prevNodeIndex].nextBL == nodeIndex) {
        return quadTreeNodes[prevNodeIndex].nextBR;
    }

    return 0;
}

bool quad_tree_collision_on_node(uint index, uint nodeIndex) {
    float nodeOffsetX = quadTreeNodes[nodeIndex].offsetX;
    float nodeOffsetY = quadTreeNodes[nodeIndex].offsetY;

    vec2 ePos = entities[index].pos;
    vec2 aabbHalfExtents = vec2((quadTreeNodes[nodeIndex].width / 2), (quadTreeNodes[nodeIndex].height / 2));
    vec2 nodeCenter = vec2(nodeOffsetX, nodeOffsetY) + aabbHalfExtents;
    vec2 diff = ePos - nodeCenter;
    vec2 clamped = clamp(diff, vec2(-aabbHalfExtents.x, -aabbHalfExtents.y), aabbHalfExtents);
    vec2 closest = nodeCenter + clamped;
    diff = closest - ePos;
    return length(diff) < pushConsts.collisionRadius;
}

void quad_tree_check_collisions_on_node(uint index, uint nodeIndex) {
    if (quadTreeNodes[nodeIndex].contentType == TYPE_ENTITY) {
        if (quadTreeEntities[index].nodeIndex != nodeIndex || quadTreeNodes[nodeIndex].entityCount > 1) {
            quad_tree_check_entity_collisions_on_node(index, nodeIndex);
        }
        return;
    }

    uint curNodeIndex = quadTreeNodes[nodeIndex].nextTL;
    uint sourceNodeIndex = nodeIndex;
    while (true) {
        if (quadTreeNodes[curNodeIndex].contentType == TYPE_ENTITY) {
            if (quad_tree_collision_on_node(index, curNodeIndex)) {
                quad_tree_check_entity_collisions_on_node(index, curNodeIndex);
            }

            curNodeIndex = quad_tree_get_next_node_index(curNodeIndex);
            while (true) {
                if (curNodeIndex == 0) {
                    if (quadTreeNodes[sourceNodeIndex].prevNodeIndex == sourceNodeIndex) {
                        return;
                    }
                    curNodeIndex = quad_tree_get_next_node_index(sourceNodeIndex);
                    sourceNodeIndex = quadTreeNodes[sourceNodeIndex].prevNodeIndex;
                } else {
                    break;
                }
            }
        } else if (!quad_tree_collision_on_node(index, curNodeIndex)) {
            curNodeIndex = quad_tree_get_next_node_index(curNodeIndex);
            while (true) {
                if (curNodeIndex == 0) {
                    if (quadTreeNodes[sourceNodeIndex].prevNodeIndex == sourceNodeIndex) {
                        return;
                    }
                    curNodeIndex = quad_tree_get_next_node_index(sourceNodeIndex);
                    sourceNodeIndex = quadTreeNodes[sourceNodeIndex].prevNodeIndex;
                } else {
                    break;
                }
            }
        } else {
            sourceNodeIndex = curNodeIndex;
            curNodeIndex = quadTreeNodes[curNodeIndex].nextTL;
        }
    }
}

bool quad_tree_collisions_only_on_same_node(uint index, uint nodeIndex) {
    float nodeOffsetX = quadTreeNodes[nodeIndex].offsetX;
    float nodeOffsetY = quadTreeNodes[nodeIndex].offsetY;

    vec2 ePos = entities[index].pos;
    vec2 minEPos = ePos - vec2(pushConsts.collisionRadius);
    minEPos = vec2(std::max(minEPos.x, 0.0F), std::max(minEPos.y, 0.0F));
    vec2 maxEPos = ePos + vec2(pushConsts.collisionRadius);
    maxEPos = vec2(std::min(maxEPos.x, pushConsts.worldSizeX), std::min(maxEPos.y, pushConsts.worldSizeY));

    return (minEPos.x >= nodeOffsetX) && (maxEPos.x < quadTreeNodes[nodeIndex].width) && (minEPos.y >= nodeOffsetY) && (maxEPos.y < quadTreeNodes[nodeIndex].height);
}

/**
 * Checks for collisions inside the pushConsts.collisionRadius with other entities.
 * Will invoke quad_tree_collision(index, otherIndex) only in case index < otherIndex
 * to prevent duplicate invocations.
 **/
void quad_tree_check_collisions(uint index) {
    uint nodeIndex = quadTreeEntities[index].nodeIndex;
    quad_tree_check_collisions_on_node(index, nodeIndex);

    if (quad_tree_collisions_only_on_same_node(index, nodeIndex)) {
        return;
    }

    uint prevNodeIndex = quadTreeNodes[nodeIndex].prevNodeIndex;
    while (nodeIndex != prevNodeIndex) {
        if (nodeIndex != quadTreeNodes[prevNodeIndex].nextTL && quad_tree_collision_on_node(index, quadTreeNodes[prevNodeIndex].nextTL)) {
            quad_tree_check_collisions_on_node(index, quadTreeNodes[prevNodeIndex].nextTL);
        }

        if (nodeIndex != quadTreeNodes[prevNodeIndex].nextTR && quad_tree_collision_on_node(index, quadTreeNodes[prevNodeIndex].nextTR)) {
            quad_tree_check_collisions_on_node(index, quadTreeNodes[prevNodeIndex].nextTR);
        }

        if (nodeIndex != quadTreeNodes[prevNodeIndex].nextBL && quad_tree_collision_on_node(index, quadTreeNodes[prevNodeIndex].nextBL)) {
            quad_tree_check_collisions_on_node(index, quadTreeNodes[prevNodeIndex].nextBL);
        }

        if (nodeIndex != quadTreeNodes[prevNodeIndex].nextBR && quad_tree_collision_on_node(index, quadTreeNodes[prevNodeIndex].nextBR)) {
            quad_tree_check_collisions_on_node(index, quadTreeNodes[prevNodeIndex].nextBR);
        }

        if (quad_tree_collisions_only_on_same_node(index, nodeIndex)) {
            return;
        }

        nodeIndex = prevNodeIndex;
        prevNodeIndex = quadTreeNodes[nodeIndex].prevNodeIndex;
    }
}

// ------------------------------------------------------------------------------------

vec2 random_Target() {
    static std::random_device device;
    static std::mt19937 gen(device());
    static std::uniform_real_distribution<float> distr_x(0, pushConsts.worldSizeX);
    static std::uniform_real_distribution<float> distr_y(0, pushConsts.worldSizeY);

    return {distr_x(gen), distr_y(gen)};
}

// 1.4 meters per second:
const float SPEED = 1.4;

void update_direction(uint index, vec2 pos) {
    vec2 dist = entities[index].target - pos;
    float len = length(dist);
    if (len == 0) {
        entities[index].direction = vec2(0);
        return;
    }
    vec2 normVec = dist / vec2(len);
    entities[index].direction = normVec * vec2(SPEED);
}

vec2 move(uint index) {
    float dist = distance(entities[index].pos, entities[index].target);

    if (dist > SPEED) {
        return entities[index].pos + entities[index].direction;
    }

    vec2 newPos = entities[index].target;
    entities[index].target = random_Target();
    update_direction(index, newPos);
    return newPos;
}

void shader_main(uint index) {
    if (entities[index].initialized == 0) {
        entities[index].pos = random_Target();
        quad_tree_insert(index, 0, 1);
        entities[index].initialized = 1;
        return;
    }

    if ((pushConsts.tick % 2) == 0) {
        update_direction(index, entities[index].pos);
        vec2 newPos = move(index);
        // vec2 newPos = random_Target();
        quad_tree_update(index, newPos);
    } else {
        entities[index].color = vec4(0, 1, 0, 1);
        quad_tree_check_collisions(index);
    }
}

void shader_main_move_only(uint index) {
    if (entities[index].initialized == 0) {
        quad_tree_insert(index, 0, 1);
        entities[index].initialized = 1;
        return;
    }

    vec2 newPos = move(index);
    quad_tree_update(index, newPos);
}

void reset() {
    for (std::atomic<uint>& i : quadTreeNodeUsedStatus) {
        i = 0;
    }

    for (QuadTreeNodeDescriptor& quadTreeNode : quadTreeNodes) {
        quadTreeNode = QuadTreeNodeDescriptor();
    }

    for (QuadTreeEntityDescriptor& quadTreeEntity : quadTreeEntities) {
        quadTreeEntity = QuadTreeEntityDescriptor();
    }

    for (std::atomic<uint>& i : debugData) {
        i = 0;
    }

    pushConsts.tick = 0;
}

void init() {
    // Entities:
    for (size_t index = 0; index < entities.size(); index++) {
        entities[index].pos = move(index);
    }

    // Quad Tree:
    quadTreeNodes[0].width = pushConsts.worldSizeX;
    quadTreeNodes[0].height = pushConsts.worldSizeY;
    quadTreeNodes[0].contentType = TYPE_ENTITY;
    quadTreeNodeUsedStatus[1] = 2;  // Pointer to the first free node index;
}

void to_dot_graph_rec(std::string& result, uint nodeIndex) {
    if (quadTreeNodes[nodeIndex].contentType == TYPE_NODE) {
        result += "n" + std::to_string(nodeIndex) + " [shape=box];";
        to_dot_graph_rec(result, quadTreeNodes[nodeIndex].nextTL);
        to_dot_graph_rec(result, quadTreeNodes[nodeIndex].nextTR);
        to_dot_graph_rec(result, quadTreeNodes[nodeIndex].nextBL);
        to_dot_graph_rec(result, quadTreeNodes[nodeIndex].nextBR);

        result += "n" + std::to_string(nodeIndex) + " -> " + "n" + std::to_string(quadTreeNodes[nodeIndex].nextTL) + ";";
        result += "n" + std::to_string(nodeIndex) + " -> " + "n" + std::to_string(quadTreeNodes[nodeIndex].nextTR) + ";";
        result += "n" + std::to_string(nodeIndex) + " -> " + "n" + std::to_string(quadTreeNodes[nodeIndex].nextBL) + ";";
        result += "n" + std::to_string(nodeIndex) + " -> " + "n" + std::to_string(quadTreeNodes[nodeIndex].nextBR) + ";";
    } else {
        result += "n" + std::to_string(nodeIndex) + " [shape=box, label=\"n" + std::to_string(nodeIndex) + ": " + std::to_string(quadTreeNodes[nodeIndex].entityCount) + "\"];";
        if (quadTreeNodes[nodeIndex].entityCount > 0) {
            uint index = quadTreeNodes[nodeIndex].first;
            std::string prevName = "n" + std::to_string(nodeIndex);
            while (quadTreeEntities[index].typeNext == TYPE_ENTITY) {
                std::string curName = "n" + std::to_string(nodeIndex) + "e" + std::to_string(index);
                result += curName + ";";
                result += (prevName + " -> " + curName + ";");
                if (quadTreeEntities[index].typePrev == TYPE_ENTITY) {
                    result += curName + " -> " + ("n" + std::to_string(nodeIndex) + "e" + std::to_string(quadTreeEntities[index].prev) + ";");
                }
                prevName = curName;

                index = quadTreeEntities[index].next;
            }

            std::string curName = "n" + std::to_string(nodeIndex) + "e" + std::to_string(index);
            result += curName + ";";
            result += (prevName + " -> " + curName + ";");
            if (quadTreeEntities[index].typePrev == TYPE_ENTITY) {
                result += (curName + " -> " + "n" + std::to_string(nodeIndex) + "e" + std::to_string(quadTreeEntities[index].prev) + ";");
            }
        }
    }
}

std::string to_dot_graph() {
    std::string result = "digraph quad_tree {";
    to_dot_graph_rec(result, 0);
    return result + '}';
}

void run_default() {
    init();
    pushConsts.tick = 0;
    std::vector<std::thread> threads;

    std::cout << "tick; seconds move; seconds collision; seconds all\n";

    const size_t THREAD_COUNT = 64;
    std::barrier syncPoint(THREAD_COUNT);
    std::barrier incSyncPoint(THREAD_COUNT);
    for (size_t i = 0; i < THREAD_COUNT; i++) {
        size_t start = (entities.size() / THREAD_COUNT) * i;
        size_t end = (entities.size() / THREAD_COUNT) * (i + 1);
        threads.emplace_back([start, end, i, &syncPoint, &incSyncPoint]() {
            std::chrono::high_resolution_clock::time_point lastStartTp = std::chrono::high_resolution_clock::now();
            std::chrono::high_resolution_clock::time_point lastStartMoveTp = std::chrono::high_resolution_clock::now();
            std::chrono::high_resolution_clock::time_point lastEndMoveTp = std::chrono::high_resolution_clock::now();
            std::chrono::high_resolution_clock::time_point lastStartCollisionTp = std::chrono::high_resolution_clock::now();
            std::chrono::high_resolution_clock::time_point lastEndCollisionTp = std::chrono::high_resolution_clock::now();

            // while (pushConsts.tick < 20000) {
            while (true) {
                lastStartTp = std::chrono::high_resolution_clock::now();
                lastStartMoveTp = std::chrono::high_resolution_clock::now();
                for (size_t index = start; index < end; index++) {
                    shader_main(index);
                    // shader_main_move_only(index);
                }

                syncPoint.arrive_and_wait();
                lastEndMoveTp = std::chrono::high_resolution_clock::now();
                if (i == 0) {
                    // validate_entity_count(NUM_ENTITIES);

                    pushConsts.tick++;
                }

                syncPoint.arrive_and_wait();
                lastStartCollisionTp = std::chrono::high_resolution_clock::now();
                for (size_t index = start; index < end; index++) {
                    shader_main(index);
                }

                syncPoint.arrive_and_wait();
                lastEndCollisionTp = std::chrono::high_resolution_clock::now();
                if (i == 0) {
                    // validate_entity_count(NUM_ENTITIES);

                    pushConsts.tick++;
                    std::chrono::nanoseconds durationMove = lastEndMoveTp - lastStartMoveTp;
                    std::chrono::nanoseconds durationCollision = lastEndCollisionTp - lastStartCollisionTp;
                    std::chrono::nanoseconds durationAll = std::chrono::high_resolution_clock::now() - lastStartTp;
                    double secMove = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(durationMove).count()) / 1000;
                    double secCollision = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(durationCollision).count()) / 1000;
                    double secAll = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(durationAll).count()) / 1000;

                    std::cout << (pushConsts.tick / 2) << "; " << secMove << "; " << secCollision << "; " << secAll << "\n";
                    std::cerr << (pushConsts.tick / 2) << "; " << secMove << "; " << secCollision << "; " << secAll << "\n";
                    // std::cout << "Shader ticked: " << pushConsts.tick++ << " took " << sec << " seconds\n";
                }
                incSyncPoint.arrive_and_wait();
            }
        });
    }

    for (std::thread& t : threads) {
        t.join();
    }
}

void run_collision_detection_test_1() {
    std::cout << "Running run_collision_detection_test_1...\n";
    reset();
    init();

    entities[0].pos = vec2((pushConsts.worldSizeX / 2) - 3, 0);
    quad_tree_insert(0, 0, 1);
    validate_entity_count(1);
    quad_tree_check_collisions(0);
    assert(debugData[1] == 0);
    debugData[1] = 0;

    entities[1].pos = vec2((pushConsts.worldSizeX / 2) + 3, 0);
    quad_tree_insert(1, 0, 1);
    validate_entity_count(2);
    quad_tree_check_collisions(0);
    assert(debugData[1] == 0);
    quad_tree_check_collisions(1);
    assert(debugData[1] == 1);
    debugData[1] = 0;

    entities[2].pos = vec2(0, 0);
    quad_tree_insert(2, 0, 1);
    validate_entity_count(3);
    quad_tree_check_collisions(0);
    assert(debugData[1] == 0);
    quad_tree_check_collisions(1);
    assert(debugData[1] == 1);
    quad_tree_check_collisions(2);
    assert(debugData[1] == 1);
    debugData[1] = 0;

    entities[3].pos = vec2((pushConsts.worldSizeX / 4), 0);
    quad_tree_insert(3, 0, 1);
    validate_entity_count(4);
    quad_tree_check_collisions(0);
    assert(debugData[1] == 0);
    quad_tree_check_collisions(1);
    assert(debugData[1] == 1);
    quad_tree_check_collisions(2);
    assert(debugData[1] == 1);
    quad_tree_check_collisions(3);
    assert(debugData[1] == 1);
    debugData[1] = 0;

    entities[4].pos = vec2((pushConsts.worldSizeX / 4) - 9, 0);
    quad_tree_insert(4, 0, 1);
    validate_entity_count(5);
    quad_tree_check_collisions(0);
    assert(debugData[1] == 0);
    quad_tree_check_collisions(1);
    assert(debugData[1] == 1);
    std::cout << to_dot_graph() << '\n';
    quad_tree_check_collisions(2);
    assert(debugData[1] == 1);
    quad_tree_check_collisions(3);
    assert(debugData[1] == 1);
    quad_tree_check_collisions(4);
    assert(debugData[1] == 2);
    debugData[1] = 0;

    std::cout << "run_collision_detection_test_1 was successful.\n";
}

void run_tests() {
    run_collision_detection_test_1();
}

int main() {
    // run_tests();
    run_default();

    return EXIT_SUCCESS;
}
