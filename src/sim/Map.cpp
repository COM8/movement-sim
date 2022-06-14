#include "Map.hpp"
#include "logger/Logger.hpp"
#include "sim/Entity.hpp"
#include "spdlog/spdlog.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <random>

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

    if (!json.contains("maxDistLat")) {
        throw std::runtime_error("Failed to parse map. 'maxDistLat' field missing.");
    }
    float width = 0;
    json.at("maxDistLat").get_to(width);

    if (!json.contains("maxDistLong")) {
        throw std::runtime_error("Failed to parse map. 'maxDistLong' field missing.");
    }
    float height = 0;
    json.at("maxDistLong").get_to(height);

    std::vector<Line> lines;
    std::vector<LineCompact> linesCompact;

    if (!json.contains("roads")) {
        throw std::runtime_error("Failed to parse map. 'roads' field missing.");
    }
    nlohmann::json::array_t roadsArray;
    json.at("roads").get_to(roadsArray);
    for (const nlohmann::json& jRoad : roadsArray) {
        if (!jRoad.contains("start")) {
            throw std::runtime_error("Failed to parse map. 'start' field missing.");
        }
        nlohmann::json jStart = jRoad["start"];
        if (!jStart.contains("distLat")) {
            throw std::runtime_error("Failed to parse map. 'distLat' field missing.");
        }
        float latStart = 0;
        jStart.at("distLat").get_to(latStart);

        if (!jStart.contains("distLong")) {
            throw std::runtime_error("Failed to parse map. 'distLong' field missing.");
        }
        float longStart = 0;
        jStart.at("distLong").get_to(longStart);

        if (!jRoad.contains("end")) {
            throw std::runtime_error("Failed to parse map. 'end' field missing.");
        }
        nlohmann::json jEnd = jRoad["end"];
        if (!jEnd.contains("distLat")) {
            throw std::runtime_error("Failed to parse map. 'distLat' field missing.");
        }
        float latEnd = 0;
        jEnd.at("distLat").get_to(latEnd);

        if (!jEnd.contains("distLong")) {
            throw std::runtime_error("Failed to parse map. 'distLong' field missing.");
        }
        float longEnd = 0;
        jEnd.at("distLong").get_to(longEnd);

        Vec2 start{latStart, longStart};
        Vec2 end{latEnd, longEnd};
        lines.emplace_back(Line{Coordinate{start}, Coordinate{end}});
        linesCompact.emplace_back(LineCompact{start, end});
    }

    SPDLOG_INFO("Map loaded from '{}'.", path.string());
    return std::make_shared<Map>(width, height, std::move(lines), std::move(linesCompact));
}

LineCompact Map::get_random_line() const {
    static std::random_device device;
    static std::mt19937 gen(device());
    static std::uniform_int_distribution<size_t> distr(0, linesCompact.size());
    return linesCompact[distr(gen)];
}
}  // namespace sim