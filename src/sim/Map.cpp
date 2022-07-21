#include "Map.hpp"
#include "logger/Logger.hpp"
#include "sim/Entity.hpp"
#include "spdlog/spdlog.h"
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <random>

namespace sim {
Coordinate::Coordinate(Vec2 pos, unsigned int connectedIndex, unsigned int connectedCount) : pos(pos),
                                                                                             connectedIndex(connectedIndex),
                                                                                             connectedCount(connectedCount) {}

Map::Map(float width, float height, std::vector<Road>&& roads, std::vector<RoadPiece>&& roadPieces, std::vector<unsigned int>&& connections) : width(width),
                                                                                                                                               height(height),
                                                                                                                                               roads(std::move(roads)),
                                                                                                                                               roadPieces(std::move(roadPieces)),
                                                                                                                                               connections(std::move(connections)) {
    assert(roadPieces.size() == roads.size() * 2);
}

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

    std::vector<Road> roads;
    std::vector<RoadPiece> roadPieces;

    if (!json.contains("roads")) {
        throw std::runtime_error("Failed to parse map. 'roads' field missing.");
    }
    nlohmann::json::array_t roadsArray;
    json.at("roads").get_to(roadsArray);
    for (const nlohmann::json& jRoad : roadsArray) {
        if (!jRoad.contains("connIndexStart")) {
            throw std::runtime_error("Failed to parse map. 'connIndexStart' field missing.");
        }
        unsigned int connIndexStart = 0;
        jRoad.at("connIndexStart").get_to(connIndexStart);

        if (!jRoad.contains("connCountStart")) {
            throw std::runtime_error("Failed to parse map. 'connCountStart' field missing.");
        }
        unsigned int connCountStart = 0;
        jRoad.at("connCountStart").get_to(connCountStart);

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

        if (!jRoad.contains("connIndexEnd")) {
            throw std::runtime_error("Failed to parse map. 'connIndexEnd' field missing.");
        }
        unsigned int connIndexEnd = 0;
        jRoad.at("connIndexEnd").get_to(connIndexEnd);

        if (!jRoad.contains("connCountEnd")) {
            throw std::runtime_error("Failed to parse map. 'connCountEnd' field missing.");
        }
        unsigned int connCountEnd = 0;
        jRoad.at("connCountEnd").get_to(connCountEnd);

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

        // Ensure we don't have any zero long edges (points):
        if (start.x == end.x && start.y == end.y) {
            SPDLOG_WARN("Zero length road detected. Skipping...");
            continue;
        }

        roads.emplace_back(Road{Coordinate{start, connIndexStart, connCountStart}, Coordinate{end, connIndexEnd, connCountEnd}});
        roadPieces.emplace_back(RoadPiece{start, {}, sim::Rgba{1.0, 0.0, 0.0, 1.0}});  // Start
        roadPieces.emplace_back(RoadPiece{start, {}, sim::Rgba{1.0, 0.0, 0.0, 1.0}});  // End
    }

    std::vector<unsigned int> connections{};
    if (!json.contains("connectionRoadIndexList")) {
        throw std::runtime_error("Failed to parse map. 'connectionRoadIndexList' field missing.");
    }
    nlohmann::json::array_t connectionsArray;
    json.at("connectionRoadIndexList").get_to(connectionsArray);
    connections.reserve(connectionsArray.size());
    for (const nlohmann::json& jConnection : connectionsArray) {
        assert(jConnection.is_number_unsigned());
        connections.push_back(static_cast<unsigned int>(jConnection));
    }

    SPDLOG_INFO("Map loaded from '{}'. Found {} roads with {} connections.", path.string(), roadPieces.size(), connections.size());
    assert(roads.size() * 2 == roadPieces.size());
    return std::make_shared<Map>(width, height, std::move(roads), std::move(roadPieces), std::move(connections));
}

unsigned int Map::get_random_road_index() const {
    static std::random_device device;
    static std::mt19937 gen(device());
    static std::uniform_int_distribution<unsigned int> distr(0, roadPieces.size());
    return distr(gen);
}
}  // namespace sim