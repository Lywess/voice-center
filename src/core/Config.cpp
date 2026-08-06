// src/core/Config.cpp
#include "Config.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace vc {

AppConfig load_config(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open config: " + path.string());
  nlohmann::json j;
  in >> j;
  AppConfig cfg;
  cfg.udp_bind = j.at("udp").value("bind", std::string("0.0.0.0"));
  cfg.udp_port = j.at("udp").at("port").get<int>();
  cfg.cooldown_ms = j.at("queue").value("cooldown_ms", 5000);
  for (const auto& d : j.at("devices")) {
    DeviceConfig dc;
    dc.ip = d.at("ip").get<std::string>();
    dc.name = d.at("name").get<std::string>();
    dc.start_audio = d.at("start_audio").get<std::string>();
    if (dc.ip.empty()) throw std::runtime_error("device ip empty");
    cfg.devices.push_back(std::move(dc));
  }
  if (cfg.devices.empty()) throw std::runtime_error("no devices in config");
  return cfg;
}

const DeviceConfig* find_device(const AppConfig& cfg, const std::string& ip) {
  for (const auto& d : cfg.devices) {
    if (d.ip == ip) return &d;
  }
  return nullptr;
}

}
