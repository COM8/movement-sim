#include "Utils.hpp"
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace shaders::utils {
std::vector<uint32_t> load_shader(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        throw std::filesystem::filesystem_error("Path does not exist.", path, std::error_code());
    }

    if (!std::filesystem::is_regular_file(path)) {
        throw std::filesystem::filesystem_error("Path does not point to a regular file.", path, std::error_code());
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::filesystem::filesystem_error("Failed to open file..", path, std::error_code());
    }

    // Size:
    size_t size = file.tellg();
    file.seekg(0);

    if ((size % 4) != 0) {
        throw std::runtime_error("Invalid shader file size. Expected multiple of " + std::to_string(4) + "B but the file is of size " + std::to_string(size) + ".");
    }

    // Read:
    std::vector<uint32_t> result;
    result.resize(size / 4);

    // Ensure the reinterpret_cast does what it is supposed to do:
    static_assert(sizeof(uint32_t) == 4 * sizeof(char));

    for (size_t i = 0; i < size / 4; i++) {
        // NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
        file.read(reinterpret_cast<char*>(&(result[i])), sizeof(uint32_t));
    }

    return result;
}
}  // namespace shaders::utils