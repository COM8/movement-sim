#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace shaders::utils {
std::vector<uint32_t> load_shader(const std::filesystem::path& path);
}  // namespace shaders::utils