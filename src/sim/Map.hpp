#pragma once

#include "Entity.hpp"
#include <filesystem>
#include <memory>
#include <vector>

namespace sim {
struct Coordinate {
    Vec2 pos;
    std::vector<size_t> connected{};

    explicit Coordinate(Vec2 pos);
} __attribute__((aligned(32)));

struct Line {
    Coordinate start;
    Coordinate end;
} __attribute__((aligned(64)));

struct LineCompact {
    Vec2 start;
    Vec2 end;
} __attribute__((aligned(16)));

class Map {
 public:
    float width;
    float height;
    std::vector<uint> connections;
    std::vector<Line> lines;
    std::vector<LineCompact> linesCompact;

    Map(float width, float height, std::vector<Line>&& lines, std::vector<LineCompact>&& linesCompact);

    static std::shared_ptr<Map> load_from_file(const std::filesystem::path& path);

    [[nodiscard]] LineCompact get_random_line() const;
};
}  // namespace sim