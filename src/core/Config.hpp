#pragma once
#include "Types.hpp"
#include <filesystem>
namespace vc {
AppConfig load_config(const std::filesystem::path& path);
const DeviceConfig* find_device(const AppConfig& cfg, const std::string& ip);
}
