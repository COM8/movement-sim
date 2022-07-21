#pragma once

#include "Entity.hpp"
#include <filesystem>
#include <memory>
#include <vector>
#include <sys/types.h>

namespace sim {
struct Coordinate {
    Vec2 pos{};
    unsigned int connectedIndex{};
    unsigned int connectedCount{};

    Coordinate(Vec2 pos, unsigned int connectedIndex, unsigned int connectedCount);
    Coordinate() = default;
} __attribute__((aligned(16))) __attribute__((__packed__));

struct Road {
    Coordinate start;
    Coordinate end;
} __attribute__((aligned(32))) __attribute__((__packed__));

struct RoadPiece {
    Vec2 pos;
    Vec2 padding;
    sim::Rgba color;
} __attribute__((aligned(32))) __attribute__((__packed__));

class Map {
 public:
    float width;
    float height;
    std::vector<Road> roads;
    std::vector<RoadPiece> roadPieces;
    std::vector<unsigned int> connections;

    Map(float width, float height, std::vector<Road>&& roads, std::vector<RoadPiece>&& roadPieces, std::vector<unsigned int>&& connections);

    static std::shared_ptr<Map> load_from_file(const std::filesystem::path& path);

    [[nodiscard]] unsigned int get_random_road_index() const;
};
}  // namespace sim