#include "Map.hpp"
#include "logger/Logger.hpp"
#include "sim/Entity.hpp"
#include "spdlog/spdlog.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>

namespace sim {
Coordinate::Coordinate(Vec2 pos) : pos(pos) {}

Map::Map(float width, float height, std::vector<Line>&& lines, std::vector<LineCompact>&& linesCompact) : width(width),
                                                                                                          height(height),
                                                                                                          lines(std::move(lines)),
                                                                                                          linesCompact(std::move(linesCompact)) {}

std::shared_ptr<Map> Map::load_from_file(const std::filesystem::path& path) {
    SPDLOG_INFO("Loading map from '{}'...", path.string());
    if (!std::filesystem::exists(path)) {
        SPDLOG_ERROR("Failed to open map from '{}'. File does not exist.", path.string());
        return nullptr;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        SPDLOG_ERROR("Failed to open map from '{}'.", path.string());
        return nullptr;
    }
    nlohmann::json json = nlohmann::json::parse(file);

    if (!json.contains("maxLat")) {
        throw std::runtime_error("Failed to parse corrections. 'maxLat' field missing.");
    }
    float width = 0;
    json.at("maxLat").get_to(width);

    if (!json.contains("maxLong")) {
        throw std::runtime_error("Failed to parse corrections. 'maxLong' field missing.");
    }
    float height = 0;
    json.at("maxLong").get_to(height);

    std::vector<Line> lines;
    std::vector<LineCompact> linesCompact;

    if (!json.contains("points")) {
        throw std::runtime_error("Failed to parse corrections. 'points' field missing.");
    }
    nlohmann::json::array_t linesArray;
    json.at("points").get_to(linesArray);
    for (const nlohmann::json& jLine : linesArray) {
        nlohmann::json::array_t pointsArray = jLine;
        assert(pointsArray.size() == 2);
        nlohmann::json::array_t startArray = pointsArray[0];
        nlohmann::json::array_t endArray = pointsArray[1];

        float xStart = startArray[0];
        float yStart = startArray[1];
        Vec2 start{xStart, yStart};
        float xEnd = endArray[0];
        float yEnd = endArray[1];
        Vec2 end{xEnd, yEnd};
        lines.emplace_back(Line{Coordinate{start}, Coordinate{end}});
        linesCompact.emplace_back(LineCompact{start, end});
    }

    SPDLOG_INFO("Map loaded from '{}'.", path.string());
    return std::make_shared<Map>(width, height, std::move(lines), std::move(linesCompact));
}
}  // namespace sim