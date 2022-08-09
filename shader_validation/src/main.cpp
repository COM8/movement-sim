#include <array>
#include <cstdint>
#include <cstdlib>

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
} __attribute__((packed)) __attribute__((aligned(8)));

using uint = uint32_t;

using vec4 = vec4T<float>;
using vec2 = vec2T<float>;
using uvec4 = vec4T<uint>;

void atomicAdd(uint& data, uint val) {
    data += val;
}

void atomicAdd(int& data, int val) {
    data += val;
}

void memoryBarrierBuffer() {}

uint atomicCompSwap(uint& data, uint comp, uint val) {
    uint original = data;
    if (original == comp) {
        data = val;
    }
    return original;
}

int atomicCompSwap(int& data, int comp, int val) {
    int original = data;
    if (original == comp) {
        data = val;
    }
    return original;
}

uint atomicExchange(uint& data, uint val) {
    uint original = data;
    data = val;
    return original;
}

int atomicExchange(int& data, int val) {
    int original = data;
    data = val;
    return original;
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
} __attribute__((aligned(16)));

std::array<EntityDescriptor, 10000> entities{};
PushConstantsDescriptor pushConsts{29007.4609, 16463.7656, 21845, 8};

// ------------------------------------------------------------------------------------
// Quad Tree
// ------------------------------------------------------------------------------------
uint TYPE_INVALID = 0;
uint TYPE_LEVEL = 1;
uint TYPE_ENTITY = 2;

struct QuadTreeLevelDescriptor {
    int acquireLock{0};
    int writeLock{0};
    int readerLock{0};

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
} __attribute__((aligned(64)));

struct QuadTreeEntityDescriptor {
    uint levelIndex{0};

    uint typeNext{0};
    uint next{0};

    uint typePrev{0};
    uint prev{0};
} __attribute__((aligned(32)));

// TODO add memory qualifiers: https://www.khronos.org/opengl/wiki/Shader_Storage_Buffer_Object
std::array<QuadTreeLevelDescriptor, 21845> quadTreeLevels{};
std::array<QuadTreeEntityDescriptor, 10000> quadTreeEntities{};
/**
 * [0]: Lock
 * [1]: Next free hint
 * [2... (levelCount + 2)]: Level locks
 **/
std::array<uint, 21847> quadTreeLevelUsedStatus{};
std::array<uint, 10> debugData{};

uint LEVEL_ENTITY_CAP = 1;

void quad_tree_lock_level_read(uint levelIndex) {
    while (atomicCompSwap(quadTreeLevels[levelIndex].acquireLock, 0, 1) != 0) {}

    // Prevent from reading, when we are currently writing:
    while (quadTreeLevels[levelIndex].writeLock != 0) {}
    atomicAdd(quadTreeLevels[levelIndex].readerLock, 1);

    atomicExchange(quadTreeLevels[levelIndex].acquireLock, 0);
    memoryBarrierBuffer();
}

void quad_tree_unlock_level_read(uint levelIndex) {
    atomicAdd(quadTreeLevels[levelIndex].readerLock, -1);
    memoryBarrierBuffer();
}

/**
 * Locks read and write for the given levelIndex.
 **/
void quad_tree_lock_level_read_write(uint levelIndex) {
    while (atomicCompSwap(quadTreeLevels[levelIndex].acquireLock, 0, 1) != 0) {}

    // Wait until all others stopped reading:
    while (atomicCompSwap(quadTreeLevels[levelIndex].readerLock, 0, 1) != 0) {}
    while (atomicCompSwap(quadTreeLevels[levelIndex].writeLock, 0, 1) != 0) {}

    atomicExchange(quadTreeLevels[levelIndex].acquireLock, 0);
    memoryBarrierBuffer();
}

void quad_tree_unlock_level_write(uint levelIndex) {
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
    while (atomicCompSwap(quadTreeLevelUsedStatus[0], 0, 1) != 0) {}

    indices.x = quad_tree_get_free_level_index();
    indices.y = quad_tree_get_free_level_index();
    indices.z = quad_tree_get_free_level_index();
    indices.w = quad_tree_get_free_level_index();

    memoryBarrierBuffer();
    atomicExchange(quadTreeLevelUsedStatus[0], 0);

    return indices;
}

void quad_tree_free_level_indices(uvec4 indices) {
    while (atomicCompSwap(quadTreeLevelUsedStatus[0], 0, 1) != 0) {}

    quadTreeLevelUsedStatus[indices.x] = 0;
    quadTreeLevelUsedStatus[indices.y] = 0;
    quadTreeLevelUsedStatus[indices.z] = 0;
    quadTreeLevelUsedStatus[indices.w] = 0;

    quadTreeLevelUsedStatus[1] = indices.x;

    memoryBarrierBuffer();
    atomicExchange(quadTreeLevelUsedStatus[0], 0);
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
    quadTreeLevels[levelIndex].contentType = TYPE_INVALID;
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

void quad_tree_insert(uint index, uint startLevelIndex) {
    // Count the number of inserted items:
    atomicAdd(debugData[0], 1);

    vec2 ePos = entities[index].pos;
    uint curDepth = 1;

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
                else {
                    memoryBarrierBuffer();
                    quad_tree_split_up_level(levelIndex);
                    quad_tree_unlock_level_write(levelIndex);
                    quad_tree_unlock_level_read(levelIndex);
                }
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
void quad_tree_lock_for_entity_edit(uint index) {
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
        } else {
            // Prevent a deadlock:
            quad_tree_unlock_level_read(levelIndex);
            quad_tree_lock_level_read_write(levelIndex);

            // Check if something has changed in the meantime with the level. Retry...
            if (quadTreeLevels[levelIndex].contentType == TYPE_LEVEL) {
                quad_tree_unlock_level_write(levelIndex);
                quad_tree_unlock_level_read(levelIndex);
            } else {
                return;
            }
        }
    }
}

/**
 * Removes the given entity from its level.
 * Returns true in case it was the last entity on this level.
 * Will not reset typePrev and typeNext of the given entity.
 **/
bool quad_tree_remove_entity(uint index) {
    uint levelIndex = quadTreeEntities[index].levelIndex;
    if (quadTreeLevels[levelIndex].entityCount <= 1) {
        quadTreeLevels[levelIndex].first = 0;
        quadTreeLevels[levelIndex].contentType = TYPE_INVALID;
        quadTreeLevels[levelIndex].entityCount = 0;
        return true;
    } else {
        if (quadTreeEntities[index].typePrev == TYPE_ENTITY) {
            uint prevIndex = quadTreeEntities[index].prev;
            quadTreeEntities[prevIndex].next = quadTreeEntities[index].next;
            quadTreeEntities[prevIndex].typeNext = quadTreeEntities[index].typeNext;
        }

        if (quadTreeEntities[index].typeNext == TYPE_ENTITY) {
            uint nextIndex = quadTreeEntities[index].prev;
            quadTreeEntities[nextIndex].prev = quadTreeEntities[index].prev;
            quadTreeEntities[nextIndex].typePrev = quadTreeEntities[index].typePrev;
        }
        quadTreeLevels[levelIndex].entityCount -= 1;
    }

    return false;
}

bool quad_tree_is_level_empty(uint levelIndex) {
    return quadTreeLevels[levelIndex].entityCount <= 0;
}

bool quad_tree_try_merging_sublevel(uint levelIndex) {
    if (quadTreeLevels[levelIndex].contentType != TYPE_LEVEL) {
        return false;
    }

    if (quad_tree_is_level_empty(quadTreeLevels[levelIndex].nextTL) && quad_tree_is_level_empty(quadTreeLevels[levelIndex].nextTR) && quad_tree_is_level_empty(quadTreeLevels[levelIndex].nextBL) && quad_tree_is_level_empty(quadTreeLevels[levelIndex].nextBR)) {
        quad_tree_free_level_indices(uvec4(quadTreeLevels[levelIndex].nextTL, quadTreeLevels[levelIndex].nextTR, quadTreeLevels[levelIndex].nextBL, quadTreeLevels[levelIndex].nextBR));
        quadTreeLevels[levelIndex].contentType = TYPE_INVALID;
    }
    return true;
}

void quad_tree_update(uint index) {
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
        quad_tree_unlock_level_read(quadTreeEntities[index].levelIndex);
        return;
    }

    quad_tree_lock_for_entity_edit(index);
    uint levelIndex = quadTreeEntities[index].levelIndex;
    if (quad_tree_remove_entity(index)) {
        // Try merging levels:
        levelIndex = quadTreeLevels[levelIndex].prevLevelIndex;
        while (quad_tree_try_merging_sublevel(levelIndex)) {
            quad_tree_unlock_level_write(levelIndex);
            levelIndex = quadTreeLevels[levelIndex].prevLevelIndex;

            // Prevent deadlocks:
            quad_tree_unlock_level_read(levelIndex);
            quad_tree_lock_level_read_write(levelIndex);
        }
    }

    // Move up until we reach a level where our entity is on:
    while (!quad_tree_is_entity_on_level(index, levelIndex)) {
        uint oldLevelIndex = levelIndex;
        levelIndex = quadTreeLevels[levelIndex].prevLevelIndex;
        quad_tree_unlock_level_read(oldLevelIndex);
    }

    // Insert the entity again:
    quad_tree_unlock_level_read(levelIndex);
    quad_tree_insert(index, levelIndex);
}

// ------------------------------------------------------------------------------------

int main() {
    return EXIT_SUCCESS;
}