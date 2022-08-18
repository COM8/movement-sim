#include <array>
#include <atomic>
#include <barrier>
#include <cassert>
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

float distance(vec2 v1, vec2 v2) {
    return static_cast<float>(std::sqrt(std::pow(v2.x - v1.x, 2) + std::pow(v2.y - v1.y, 2)));
}

float length(vec2 v1) {
    return static_cast<float>(std::sqrt(std::pow(v1.x, 2) + std::pow(v1.y, 2)));
}

vec2 clamp(vec2 v, vec2 minVec, vec2 maxVec) {
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

    uint levelCount{0};
    uint maxDepth{0};

    float collisionRadius{0};
} __attribute__((aligned(32)));

const size_t MAX_DEPTH = 8;
const size_t NUM_LEVELS = 21845;
const size_t NUM_ENTITIES = 1000000;
const float COLLISION_RADIUS = 10;

static std::array<EntityDescriptor, NUM_ENTITIES> entities{};
static PushConstantsDescriptor pushConsts{29007.4609, 16463.7656, NUM_LEVELS, MAX_DEPTH, COLLISION_RADIUS};

// ------------------------------------------------------------------------------------
// Quad Tree
// ------------------------------------------------------------------------------------
uint TYPE_INVALID = 0;
uint TYPE_LEVEL = 1;
uint TYPE_ENTITY = 2;

// NOLINTNEXTLINE (hicpp-special-member-functions)
struct QuadTreeLevelDescriptor {
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

    uint prevLevelIndex{0};

    uint nextTL{0};
    uint nextTR{0};
    uint nextBL{0};
    uint nextBR{0};

    uint padding{0};

    QuadTreeLevelDescriptor& operator=(const QuadTreeLevelDescriptor& other) {
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

        prevLevelIndex = other.prevLevelIndex;

        nextTL = other.nextTL;
        nextTR = other.nextTR;
        nextBL = other.nextBL;
        nextBR = other.nextBR;

        padding = other.padding;

        return *this;
    }
} __attribute__((aligned(64)));

struct QuadTreeEntityDescriptor {
    uint levelIndex{0};

    uint typeNext{0};
    uint next{0};

    uint typePrev{0};
    uint prev{0};
} __attribute__((aligned(32)));

// TODO add memory qualifiers: https://www.khronos.org/opengl/wiki/Shader_Storage_Buffer_Object
static std::array<QuadTreeLevelDescriptor, NUM_LEVELS> quadTreeLevels{};
static std::array<QuadTreeEntityDescriptor, NUM_ENTITIES> quadTreeEntities{};
/**
 * [0]: Lock
 * [1]: Next free hint
 * [2... (levelCount + 2)]: Level locks
 **/
static std::array<std::atomic<uint>, NUM_LEVELS + 2> quadTreeLevelUsedStatus{};
static std::array<std::atomic<uint>, 10> debugData{};

uint LEVEL_ENTITY_CAP = 1;

void to_dot_graph_rec(std::string& result, uint levelIndex);
std::string to_dot_graph();

size_t count_entities_rec(uint levelIndex) {
    if (quadTreeLevels[levelIndex].contentType == TYPE_LEVEL) {
        assert(quadTreeLevels[levelIndex].entityCount == 0);
        return count_entities_rec(quadTreeLevels[levelIndex].nextTL) + count_entities_rec(quadTreeLevels[levelIndex].nextTR) + count_entities_rec(quadTreeLevels[levelIndex].nextBL) + count_entities_rec(quadTreeLevels[levelIndex].nextBR);
    }
    assert(quadTreeLevels[levelIndex].contentType == TYPE_ENTITY);

    uint expectedCount = 0;
    if (quadTreeLevels[levelIndex].entityCount > 0) {
        uint index = quadTreeLevels[levelIndex].first;
        while (quadTreeEntities[index].typeNext == TYPE_ENTITY) {
            expectedCount++;
            index = quadTreeEntities[index].next;
        }
        expectedCount++;  // Also count the last one
    }

    uint actualCount = quadTreeLevels[levelIndex].entityCount;
    if (actualCount != expectedCount) {
        // std::cout << to_dot_graph() << '\n';
    }
    assert(actualCount == expectedCount);
    return quadTreeLevels[levelIndex].entityCount;
}

void validate_entity_count(size_t expected) {
    size_t count = count_entities_rec(0);
    if (count != expected) {
        std::cout << to_dot_graph() << '\n';
    }
    assert(count == expected);
}

void quad_tree_lock_level_read(uint levelIndex) {
    assert(quadTreeLevels[levelIndex].acquireLock >= 0);
    while (atomicCompSwap(quadTreeLevels[levelIndex].acquireLock, 0, 1) != 0) {}

    // Prevent from reading, when we are currently writing:
    assert(quadTreeLevels[levelIndex].writeLock >= 0);
    while (quadTreeLevels[levelIndex].writeLock != 0) {}
    atomicAdd(quadTreeLevels[levelIndex].readerLock, 1);

    atomicExchange(quadTreeLevels[levelIndex].acquireLock, 0);
    memoryBarrierBuffer();
}

void quad_tree_unlock_level_read(uint levelIndex) {
    assert(quadTreeLevels[levelIndex].readerLock > 0);
    atomicAdd(quadTreeLevels[levelIndex].readerLock, -1);
    memoryBarrierBuffer();
}

/**
 * Locks read and write for the given levelIndex.
 **/
void quad_tree_lock_level_read_write(uint levelIndex) {
    assert(quadTreeLevels[levelIndex].acquireLock >= 0);
    while (atomicCompSwap(quadTreeLevels[levelIndex].acquireLock, 0, 1) != 0) {}

    // Wait until all others stopped reading:
    assert(quadTreeLevels[levelIndex].readerLock >= 0);
    while (atomicCompSwap(quadTreeLevels[levelIndex].readerLock, 0, 1) != 0) {}
    assert(quadTreeLevels[levelIndex].writeLock >= 0);
    while (atomicCompSwap(quadTreeLevels[levelIndex].writeLock, 0, 1) != 0) {}

    atomicExchange(quadTreeLevels[levelIndex].acquireLock, 0);
    memoryBarrierBuffer();
}

void quad_tree_unlock_level_write(uint levelIndex) {
    assert(quadTreeLevels[levelIndex].writeLock == 1);
    atomicExchange(quadTreeLevels[levelIndex].writeLock, 0);
    memoryBarrierBuffer();
}

void quad_tree_init_entity(uint index, uint typeNext, uint next, uint levelIndex) {
    quadTreeEntities[index].typeNext = typeNext;
    quadTreeEntities[index].next = next;
    quadTreeEntities[index].typePrev = TYPE_INVALID;
    quadTreeEntities[index].prev = 0;
    quadTreeEntities[index].levelIndex = levelIndex;
}

void quad_tree_append_entity(uint levelIndex, uint index) {
    if (quadTreeLevels[levelIndex].entityCount <= 0) {
        quadTreeLevels[levelIndex].first = index;
        quad_tree_init_entity(index, TYPE_INVALID, 0, levelIndex);
    } else {
        // Add in front:
        uint oldFirstIndex = quadTreeLevels[levelIndex].first;
        quad_tree_init_entity(index, TYPE_ENTITY, oldFirstIndex, levelIndex);

        quadTreeEntities[oldFirstIndex].typePrev = TYPE_ENTITY;
        quadTreeEntities[oldFirstIndex].prev = index;

        quadTreeLevels[levelIndex].first = index;
    }
    quadTreeLevels[levelIndex].entityCount++;
    memoryBarrierBuffer();
}

/**
 * Moves up the quad tree and unlocks all levels from reading again.
 **/
void quad_tree_unlock_levels_read(uint levelIndex) {
    while (quadTreeLevels[levelIndex].prevLevelIndex != levelIndex) {
        uint oldLevelIndex = levelIndex;
        levelIndex = quadTreeLevels[levelIndex].prevLevelIndex;
        quad_tree_unlock_level_read(oldLevelIndex);
    }
    // Unlock the first level:
    quad_tree_unlock_level_read(levelIndex);
}

uint quad_tree_get_free_level_index() {
    uint index = 0;
    uint i = quadTreeLevelUsedStatus[1];
    while (index <= 0) {
        if (quadTreeLevelUsedStatus[i] == 0) {
            quadTreeLevelUsedStatus[i] = 1;
            index = i - 2;
        }

        i++;
        if (i >= (pushConsts.levelCount + 2)) {
            i = 2;
        }
    }
    quadTreeLevelUsedStatus[1] = i;
    return index;
}

uvec4 quad_tree_get_free_level_indices() {
    uvec4 indices = uvec4(0);
    while (atomicCompSwap(quadTreeLevelUsedStatus[0], static_cast<uint>(0), static_cast<uint>(1)) != 0) {}

    indices.x = quad_tree_get_free_level_index();
    indices.y = quad_tree_get_free_level_index();
    indices.z = quad_tree_get_free_level_index();
    indices.w = quad_tree_get_free_level_index();

    memoryBarrierBuffer();
    atomicExchange(quadTreeLevelUsedStatus[0], static_cast<uint>(0));

    return indices;
}

void quad_tree_free_level_indices(uvec4 indices) {
    while (atomicCompSwap(quadTreeLevelUsedStatus[0], static_cast<uint>(0), static_cast<uint>(1)) != 0) {}

    quadTreeLevelUsedStatus[indices.x + 2] = 0;
    quadTreeLevelUsedStatus[indices.y + 2] = 0;
    quadTreeLevelUsedStatus[indices.z + 2] = 0;
    quadTreeLevelUsedStatus[indices.w + 2] = 0;

    quadTreeLevelUsedStatus[1] = indices.x + 2;

    memoryBarrierBuffer();
    atomicExchange(quadTreeLevelUsedStatus[0], static_cast<uint>(0));
}

void quad_tree_init_level(uint levelIndex, uint prevLevelIndex, float offsetX, float offsetY, float width, float height) {
    quadTreeLevels[levelIndex].acquireLock = 0;
    quadTreeLevels[levelIndex].writeLock = 0;
    quadTreeLevels[levelIndex].readerLock = 0;

    quadTreeLevels[levelIndex].offsetX = offsetX;
    quadTreeLevels[levelIndex].offsetY = offsetY;
    quadTreeLevels[levelIndex].width = width;
    quadTreeLevels[levelIndex].height = height;

    quadTreeLevels[levelIndex].prevLevelIndex = prevLevelIndex;
    quadTreeLevels[levelIndex].contentType = TYPE_ENTITY;
    quadTreeLevels[levelIndex].entityCount = 0;
}

void quad_tree_move_entities(uint levelIndex) {
    float offsetXNext = quadTreeLevels[levelIndex].offsetX + (quadTreeLevels[levelIndex].width / 2);
    float offsetYNext = quadTreeLevels[levelIndex].offsetY + (quadTreeLevels[levelIndex].height / 2);

    uint index = quadTreeLevels[levelIndex].first;
    quadTreeLevels[levelIndex].first = 0;
    quadTreeLevels[levelIndex].entityCount = 0;

    bool hasNext = false;
    do {
        uint nextIndex = quadTreeEntities[index].next;
        hasNext = quadTreeEntities[index].typeNext != TYPE_INVALID;
        quadTreeEntities[index].next = 0;
        quadTreeEntities[index].typeNext = TYPE_INVALID;

        vec2 ePos = entities[index].pos;

        uint newLevelIndex = 0;
        // Left:
        if (ePos.x < offsetXNext) {
            // Top:
            if (ePos.y < offsetYNext) {
                newLevelIndex = quadTreeLevels[levelIndex].nextTL;
            } else {
                newLevelIndex = quadTreeLevels[levelIndex].nextBL;
            }
        }
        // Right:
        else {
            // Top:
            if (ePos.y < offsetYNext) {
                newLevelIndex = quadTreeLevels[levelIndex].nextTR;
            } else {
                newLevelIndex = quadTreeLevels[levelIndex].nextBR;
            }
        }
        quad_tree_append_entity(newLevelIndex, index);
        index = nextIndex;
    } while (hasNext);
}

void quad_tree_split_up_level(uint levelIndex) {
    quadTreeLevels[levelIndex].contentType = TYPE_LEVEL;

    uvec4 newLevelIndices = quad_tree_get_free_level_indices();

    float newWidth = quadTreeLevels[levelIndex].width / 2;
    float newHeight = quadTreeLevels[levelIndex].height / 2;

    quadTreeLevels[levelIndex].nextTL = newLevelIndices.x;
    quad_tree_init_level(quadTreeLevels[levelIndex].nextTL, levelIndex, quadTreeLevels[levelIndex].offsetX, quadTreeLevels[levelIndex].offsetY, newWidth, newHeight);
    quadTreeLevels[levelIndex].nextTR = newLevelIndices.y;
    quad_tree_init_level(quadTreeLevels[levelIndex].nextTR, levelIndex, quadTreeLevels[levelIndex].offsetX + newWidth, quadTreeLevels[levelIndex].offsetY, newWidth, newHeight);
    quadTreeLevels[levelIndex].nextBL = newLevelIndices.z;
    quad_tree_init_level(quadTreeLevels[levelIndex].nextBL, levelIndex, quadTreeLevels[levelIndex].offsetX, quadTreeLevels[levelIndex].offsetY + newHeight, newWidth, newHeight);
    quadTreeLevels[levelIndex].nextBR = newLevelIndices.w;
    quad_tree_init_level(quadTreeLevels[levelIndex].nextBR, levelIndex, quadTreeLevels[levelIndex].offsetX + newWidth, quadTreeLevels[levelIndex].offsetY + newHeight, newWidth, newHeight);

    quad_tree_move_entities(levelIndex);
    memoryBarrierBuffer();
}

bool quad_tree_same_pos_as_fist(uint levelIndex, vec2 ePos) {
    if (quadTreeLevels[levelIndex].entityCount > 0) {
        uint index = quadTreeLevels[levelIndex].first;
        return entities[index].pos == ePos;
    }
    return false;
}

void quad_tree_insert(uint index, uint startLevelIndex, uint startLevelDepth) {
    // Count the number of inserted items:
    atomicAdd(debugData[0], static_cast<uint>(1));

    vec2 ePos = entities[index].pos;
    uint curDepth = startLevelDepth;

    uint levelIndex = startLevelIndex;
    while (true) {
        quad_tree_lock_level_read(levelIndex);
        float offsetXNext = quadTreeLevels[levelIndex].offsetX + (quadTreeLevels[levelIndex].width / 2);
        float offsetYNext = quadTreeLevels[levelIndex].offsetY + (quadTreeLevels[levelIndex].height / 2);

        // Go one level deeper:
        if (quadTreeLevels[levelIndex].contentType == TYPE_LEVEL) {
            // Left:
            if (ePos.x < offsetXNext) {
                // Top:
                if (ePos.y < offsetYNext) {
                    levelIndex = quadTreeLevels[levelIndex].nextTL;
                } else {
                    levelIndex = quadTreeLevels[levelIndex].nextBL;
                }
            }
            // Right:
            else {
                // Top:
                if (ePos.y < offsetYNext) {
                    levelIndex = quadTreeLevels[levelIndex].nextTR;
                } else {
                    levelIndex = quadTreeLevels[levelIndex].nextBR;
                }
            }
            curDepth += 1;
        } else {
            // Prevent a deadlock:
            quad_tree_unlock_level_read(levelIndex);
            quad_tree_lock_level_read_write(levelIndex);

            // Check if something has changed in the meantime with the level. Retry...
            if (quadTreeLevels[levelIndex].contentType == TYPE_LEVEL) {
                quad_tree_unlock_level_write(levelIndex);
                quad_tree_unlock_level_read(levelIndex);
            } else {
                // Insert the entity in case there is space left, we can't go deeper or the entity has the same pos as the last:
                if (quadTreeLevels[levelIndex].entityCount < LEVEL_ENTITY_CAP || curDepth >= pushConsts.maxDepth || quad_tree_same_pos_as_fist(levelIndex, ePos)) {
                    quad_tree_append_entity(levelIndex, index);
                    memoryBarrierBuffer();
                    quad_tree_unlock_level_write(levelIndex);
                    break;
                }
                // Split up
                memoryBarrierBuffer();
                quad_tree_split_up_level(levelIndex);
                quad_tree_unlock_level_write(levelIndex);
                quad_tree_unlock_level_read(levelIndex);
            }
        }
        memoryBarrierBuffer();  // Ensure everything is in sync
    }
    // Unlock all levels again:
    quad_tree_unlock_levels_read(levelIndex);
}

bool quad_tree_is_entity_on_level(uint index, uint levelIndex) {
    return entities[index].pos.x >= quadTreeLevels[levelIndex].offsetX && entities[index].pos.x < (quadTreeLevels[levelIndex].offsetX + quadTreeLevels[levelIndex].width) && entities[index].pos.y >= quadTreeLevels[levelIndex].offsetY && entities[index].pos.y < (quadTreeLevels[levelIndex].offsetY + quadTreeLevels[levelIndex].height);
}

/**
 * Moves down the quad tree and locks all levels as read, except the last level, which gets locked as write so we can edit it.
 **/
uint quad_tree_lock_for_entity_edit(uint index) {
    vec2 ePos = entities[index].pos;
    uint levelIndex = 0;
    while (true) {
        quad_tree_lock_level_read(levelIndex);
        float offsetXNext = quadTreeLevels[levelIndex].offsetX + (quadTreeLevels[levelIndex].width / 2);
        float offsetYNext = quadTreeLevels[levelIndex].offsetY + (quadTreeLevels[levelIndex].height / 2);

        // Go one level deeper:
        if (quadTreeLevels[levelIndex].contentType == TYPE_LEVEL) {
            // Left:
            if (ePos.x < offsetXNext) {
                // Top:
                if (ePos.y < offsetYNext) {
                    levelIndex = quadTreeLevels[levelIndex].nextTL;
                } else {
                    levelIndex = quadTreeLevels[levelIndex].nextBL;
                }
            }
            // Right:
            else {
                // Top:
                if (ePos.y < offsetYNext) {
                    levelIndex = quadTreeLevels[levelIndex].nextTR;
                } else {
                    levelIndex = quadTreeLevels[levelIndex].nextBR;
                }
            }
            assert(levelIndex != 0);
        } else {
            // Prevent a deadlock:
            quad_tree_unlock_level_read(levelIndex);
            quad_tree_lock_level_read_write(levelIndex);

            // Check if something has changed in the meantime with the level. Retry...
            if (quadTreeLevels[levelIndex].contentType == TYPE_LEVEL) {
                quad_tree_unlock_level_write(levelIndex);
                quad_tree_unlock_level_read(levelIndex);
            } else {
                assert(quadTreeLevels[levelIndex].contentType == TYPE_ENTITY);
                assert(levelIndex != 0);
                return levelIndex;
            }
        }
    }
}

/**
 * Removes the given entity from its level.
 * Returns true in case it was the last entity on this level.
 **/
bool quad_tree_remove_entity(uint index) {
    uint levelIndex = quadTreeEntities[index].levelIndex;
    // uint count = count_entities_rec(0);
    if (quadTreeLevels[levelIndex].entityCount <= 1) {
        quadTreeLevels[levelIndex].first = 0;
        quadTreeLevels[levelIndex].entityCount = 0;
        // validate_entity_count(count - 1);
        return true;
    }

    if (quadTreeLevels[levelIndex].first == index) {
        quadTreeLevels[levelIndex].first = quadTreeEntities[index].next;
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
    quadTreeLevels[levelIndex].entityCount -= 1;

    // validate_entity_count(count - 1);
    return false;
}

bool quad_tree_is_level_empty(uint levelIndex) {
    return quadTreeLevels[levelIndex].entityCount <= 0 && quadTreeLevels[levelIndex].contentType == TYPE_ENTITY;
}

bool quad_tree_try_merging_sublevel(uint levelIndex) {
    if (quadTreeLevels[levelIndex].contentType != TYPE_LEVEL) {
        return false;
    }

    if (quad_tree_is_level_empty(quadTreeLevels[levelIndex].nextTL) && quad_tree_is_level_empty(quadTreeLevels[levelIndex].nextTR) && quad_tree_is_level_empty(quadTreeLevels[levelIndex].nextBL) && quad_tree_is_level_empty(quadTreeLevels[levelIndex].nextBR)) {
        quad_tree_free_level_indices(uvec4(quadTreeLevels[levelIndex].nextTL, quadTreeLevels[levelIndex].nextTR, quadTreeLevels[levelIndex].nextBL, quadTreeLevels[levelIndex].nextBR));
        assert(quadTreeLevels[levelIndex].entityCount <= 0);
        quadTreeLevels[levelIndex].contentType = TYPE_ENTITY;
        // std::cout << "Merged levels of " << levelIndex << '\n';
        return true;
    }
    return false;
}

uint quad_tree_get_cur_depth(uint levelIndex) {
    uint depth = 1;
    while (quadTreeLevels[levelIndex].prevLevelIndex != levelIndex) {
        levelIndex = quadTreeLevels[levelIndex].prevLevelIndex;
        depth++;
    }
    return depth;
}

void quad_tree_update(uint index, vec2 newPos) {
    while (true) {
        uint oldLevelIndex = quadTreeEntities[index].levelIndex;
        quad_tree_lock_level_read(oldLevelIndex);
        // Make sure our entity is still on the same level when we finally get the lock.
        // Could happen in case the current level got split up in the meantime.
        if (oldLevelIndex == quadTreeEntities[index].levelIndex) {
            break;
        }
        quad_tree_unlock_level_read(oldLevelIndex);
    }

    // Still on the same level, so we do not need to do anything:
    if (quad_tree_is_entity_on_level(index, quadTreeEntities[index].levelIndex)) {
        entities[index].pos = newPos;
        quad_tree_unlock_level_read(quadTreeEntities[index].levelIndex);
        return;
    }

    uint levelIndex = quadTreeEntities[index].levelIndex;
    quad_tree_unlock_level_read(levelIndex);
    levelIndex = quad_tree_lock_for_entity_edit(index);
    if (levelIndex != quadTreeEntities[index].levelIndex) {
        std::cout << to_dot_graph() << '\n';
        assert(levelIndex == quadTreeEntities[index].levelIndex);
    }
    if (quad_tree_remove_entity(index)) {
        // Try merging levels:
        while (levelIndex != quadTreeLevels[levelIndex].prevLevelIndex) {
            quad_tree_unlock_level_write(levelIndex);
            quad_tree_unlock_level_read(levelIndex);
            levelIndex = quadTreeLevels[levelIndex].prevLevelIndex;

            // Prevent deadlocks:
            quad_tree_unlock_level_read(levelIndex);
            quad_tree_lock_level_read_write(levelIndex);

            if (!quad_tree_try_merging_sublevel(levelIndex)) {
                break;
            }
        }
    }
    quad_tree_unlock_level_write(levelIndex);

    // Update the removed entity position:
    entities[index].pos = newPos;

    // Move up until we reach a level where our entity is on:
    while (!quad_tree_is_entity_on_level(index, levelIndex)) {
        uint oldLevelIndex = levelIndex;
        levelIndex = quadTreeLevels[levelIndex].prevLevelIndex;
        quad_tree_unlock_level_read(oldLevelIndex);
    }

    // Insert the entity again:
    quad_tree_unlock_level_read(levelIndex);
    quad_tree_insert(index, levelIndex, quad_tree_get_cur_depth(levelIndex));
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

void quad_tree_check_entity_collisions_on_level(uint index, uint levelIndex) {
    if (quadTreeLevels[levelIndex].entityCount <= 0) {
        return;
    }

    vec2 ePos = entities[index].pos;

    uint curEntityIndex = quadTreeLevels[levelIndex].first;
    while (true) {
        // Prevent checking collision with our self and prevent duplicate entries by checking only for ones where the ID is smaller than ours:
        if (curEntityIndex < index && distance(entities[curEntityIndex].pos, ePos) <= pushConsts.collisionRadius) {
            quad_tree_collision(index, curEntityIndex);
        }

        if (quadTreeEntities[curEntityIndex].typeNext != TYPE_ENTITY) {
            break;
        }
        curEntityIndex = quadTreeEntities[curEntityIndex].next;
    }
}

/**
 * Returns the next (TL -> TR -> BL -> BR -> 0) level index of the parent level.
 * Returns 0 in case the given levelIndex is BR of the parent level.
 **/
uint quad_tree_get_next_level_index(uint levelIndex) {
    uint prevLevelIndex = quadTreeLevels[levelIndex].prevLevelIndex;
    if (quadTreeLevels[prevLevelIndex].nextTL == levelIndex) {
        return quadTreeLevels[prevLevelIndex].nextTR;
    }

    if (quadTreeLevels[prevLevelIndex].nextTR == levelIndex) {
        return quadTreeLevels[prevLevelIndex].nextBL;
    }

    if (quadTreeLevels[prevLevelIndex].nextBL == levelIndex) {
        return quadTreeLevels[prevLevelIndex].nextBR;
    }

    return 0;
}

bool quad_tree_collision_on_level(uint index, uint levelIndex) {
    float levelOffsetX = quadTreeLevels[levelIndex].offsetX;
    float levelOffsetY = quadTreeLevels[levelIndex].offsetY;

    vec2 ePos = entities[index].pos;
    vec2 aabbHalfExtents = vec2((quadTreeLevels[levelIndex].width / 2), (quadTreeLevels[levelIndex].height / 2));
    vec2 levelCenter = vec2(levelOffsetX, levelOffsetY) + aabbHalfExtents;
    vec2 diff = ePos - levelCenter;
    vec2 clamped = clamp(diff, vec2(-aabbHalfExtents.x, -aabbHalfExtents.y), aabbHalfExtents);
    vec2 closest = levelCenter + clamped;
    diff = closest - ePos;
    return length(diff) < pushConsts.collisionRadius;
}

void quad_tree_check_collisions_on_level(uint index, uint levelIndex) {
    if (quadTreeLevels[levelIndex].contentType == TYPE_ENTITY) {
        quad_tree_check_entity_collisions_on_level(index, levelIndex);
        return;
    }

    uint curLevelIndex = quadTreeLevels[levelIndex].nextTL;
    uint sourceLevelIndex = levelIndex;
    while (true) {
        if (quadTreeLevels[curLevelIndex].contentType == TYPE_ENTITY) {
            if (quad_tree_collision_on_level(index, curLevelIndex)) {
                quad_tree_check_entity_collisions_on_level(index, curLevelIndex);
            }

            curLevelIndex = quad_tree_get_next_level_index(curLevelIndex);
            while (true) {
                if (curLevelIndex == 0) {
                    if (quadTreeLevels[sourceLevelIndex].prevLevelIndex == sourceLevelIndex) {
                        return;
                    }
                    curLevelIndex = quad_tree_get_next_level_index(sourceLevelIndex);
                    sourceLevelIndex = quadTreeLevels[sourceLevelIndex].prevLevelIndex;
                } else {
                    break;
                }
            }
        } else {
            sourceLevelIndex = curLevelIndex;
            curLevelIndex = quadTreeLevels[curLevelIndex].nextTL;
        }
    }
}

bool quad_tree_collisions_only_on_same_level(uint index, uint levelIndex) {
    float levelOffsetX = quadTreeLevels[levelIndex].offsetX;
    float levelOffsetY = quadTreeLevels[levelIndex].offsetY;

    vec2 ePos = entities[index].pos;
    vec2 minEPos = ePos - vec2(pushConsts.collisionRadius);
    vec2 maxEPos = ePos + vec2(pushConsts.collisionRadius);

    return (minEPos.x >= levelOffsetX) && (maxEPos.x < quadTreeLevels[levelIndex].width) && (minEPos.y >= levelOffsetY) && (maxEPos.y < quadTreeLevels[levelIndex].height);
}

/**
 * Checks for collisions inside the pushConsts.collisionRadius with other entities.
 * Will invoke quad_tree_collision(index, otherIndex) only in case index < otherIndex
 * to prevent duplicate invocations.
 **/
void quad_tree_check_collisions(uint index) {
    uint levelIndex = quadTreeEntities[index].levelIndex;
    quad_tree_check_collisions_on_level(index, levelIndex);

    if (quad_tree_collisions_only_on_same_level(index, levelIndex)) {
        return;
    }

    uint prevLevelIndex = quadTreeLevels[levelIndex].prevLevelIndex;
    while (levelIndex != prevLevelIndex) {
        if (levelIndex != quadTreeLevels[prevLevelIndex].nextTL && quad_tree_collision_on_level(index, quadTreeLevels[prevLevelIndex].nextTL)) {
            quad_tree_check_collisions_on_level(index, quadTreeLevels[prevLevelIndex].nextTL);
        }

        if (levelIndex != quadTreeLevels[prevLevelIndex].nextTR && quad_tree_collision_on_level(index, quadTreeLevels[prevLevelIndex].nextTR)) {
            quad_tree_check_collisions_on_level(index, quadTreeLevels[prevLevelIndex].nextTR);
        }

        if (levelIndex != quadTreeLevels[prevLevelIndex].nextBL && quad_tree_collision_on_level(index, quadTreeLevels[prevLevelIndex].nextBL)) {
            quad_tree_check_collisions_on_level(index, quadTreeLevels[prevLevelIndex].nextBL);
        }

        if (levelIndex != quadTreeLevels[prevLevelIndex].nextBR && quad_tree_collision_on_level(index, quadTreeLevels[prevLevelIndex].nextBR)) {
            quad_tree_check_collisions_on_level(index, quadTreeLevels[prevLevelIndex].nextBR);
        }

        if (quad_tree_collisions_only_on_same_level(index, levelIndex)) {
            return;
        }

        levelIndex = prevLevelIndex;
        prevLevelIndex = quadTreeLevels[levelIndex].prevLevelIndex;
    }
}

// ------------------------------------------------------------------------------------

vec2 move(uint /*index*/) {
    static std::random_device device;
    static std::mt19937 gen(device());
    static std::uniform_real_distribution<float> distr_x(0, pushConsts.worldSizeX);
    static std::uniform_real_distribution<float> distr_y(0, pushConsts.worldSizeY);

    return {distr_x(gen), distr_y(gen)};
}

void shader_main(uint index) {
    if (entities[index].initialized == 0) {
        quad_tree_insert(index, 0, 1);
        entities[index].initialized = 1;
        // std::cout << "Inserted: " << index << '\n';
        return;
    }

    vec2 newPos = move(index);
    quad_tree_update(index, newPos);
}

void reset() {
    for (std::atomic<uint>& i : quadTreeLevelUsedStatus) {
        i = 0;
    }

    for (QuadTreeLevelDescriptor& quadTreeLevel : quadTreeLevels) {
        quadTreeLevel = QuadTreeLevelDescriptor();
    }

    for (QuadTreeEntityDescriptor& quadTreeEntity : quadTreeEntities) {
        quadTreeEntity = QuadTreeEntityDescriptor();
    }

    for (std::atomic<uint>& i : debugData) {
        i = 0;
    }
}

void init() {
    // Entities:
    for (size_t index = 0; index < entities.size(); index++) {
        move(index);
    }

    // Quad Tree:
    quadTreeLevels[0].width = pushConsts.worldSizeX;
    quadTreeLevels[0].height = pushConsts.worldSizeY;
    quadTreeLevels[0].contentType = TYPE_ENTITY;
    quadTreeLevelUsedStatus[1] = 2;  // Pointer to the first free level index;
}

void to_dot_graph_rec(std::string& result, uint levelIndex) {
    if (quadTreeLevels[levelIndex].contentType == TYPE_LEVEL) {
        result += "n" + std::to_string(levelIndex) + " [shape=box];";
        to_dot_graph_rec(result, quadTreeLevels[levelIndex].nextTL);
        to_dot_graph_rec(result, quadTreeLevels[levelIndex].nextTR);
        to_dot_graph_rec(result, quadTreeLevels[levelIndex].nextBL);
        to_dot_graph_rec(result, quadTreeLevels[levelIndex].nextBR);

        result += "n" + std::to_string(levelIndex) + " -> " + "n" + std::to_string(quadTreeLevels[levelIndex].nextTL) + ";";
        result += "n" + std::to_string(levelIndex) + " -> " + "n" + std::to_string(quadTreeLevels[levelIndex].nextTR) + ";";
        result += "n" + std::to_string(levelIndex) + " -> " + "n" + std::to_string(quadTreeLevels[levelIndex].nextBL) + ";";
        result += "n" + std::to_string(levelIndex) + " -> " + "n" + std::to_string(quadTreeLevels[levelIndex].nextBR) + ";";
    } else {
        result += "n" + std::to_string(levelIndex) + " [shape=box, label=\"n" + std::to_string(levelIndex) + ": " + std::to_string(quadTreeLevels[levelIndex].entityCount) + "\"];";
        if (quadTreeLevels[levelIndex].entityCount > 0) {
            uint index = quadTreeLevels[levelIndex].first;
            std::string prevName = "n" + std::to_string(levelIndex);
            while (quadTreeEntities[index].typeNext == TYPE_ENTITY) {
                std::string curName = "n" + std::to_string(levelIndex) + "e" + std::to_string(index);
                result += curName + ";";
                result += (prevName + " -> " + curName + ";");
                if (quadTreeEntities[index].typePrev == TYPE_ENTITY) {
                    result += curName + " -> " + ("n" + std::to_string(levelIndex) + "e" + std::to_string(quadTreeEntities[index].prev) + ";");
                }
                prevName = curName;

                index = quadTreeEntities[index].next;
            }

            std::string curName = "n" + std::to_string(levelIndex) + "e" + std::to_string(index);
            result += curName + ";";
            result += (prevName + " -> " + curName + ";");
            if (quadTreeEntities[index].typePrev == TYPE_ENTITY) {
                result += (curName + " -> " + "n" + std::to_string(levelIndex) + "e" + std::to_string(quadTreeEntities[index].prev) + ";");
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
    std::atomic<size_t> tick = 0;
    std::vector<std::thread> threads;

    const size_t THREAD_COUNT = 10;
    std::barrier syncPoint(THREAD_COUNT);
    std::barrier incSyncPoint(THREAD_COUNT);
    for (size_t i = 0; i < THREAD_COUNT; i++) {
        size_t start = (entities.size() / THREAD_COUNT) * i;
        size_t end = (entities.size() / THREAD_COUNT) * (i + 1);
        threads.emplace_back([start, end, i, &tick, &syncPoint, &incSyncPoint]() {
            while (true) {
                for (size_t index = start; index < end; index++) {
                    shader_main(index);
                    // if (tick == 0) {
                    //     // std::cout << to_dot_graph() << '\n';
                    //     validate_entity_count(index + 1);
                    // } else {
                    //     // std::cout << to_dot_graph() << '\n';
                    //     validate_entity_count(NUM_ENTITIES);
                    // }
                }
                syncPoint.arrive_and_wait();
                if (i == 0) {
                    validate_entity_count(NUM_ENTITIES);
                    std::cout << "Shader ticked: " << tick++ << '\n';
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
    run_tests();
    // run_default();

    return EXIT_SUCCESS;
}