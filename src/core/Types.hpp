#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace vc {

struct DeviceConfig {
  std::string ip;
  std::string name;
  std::string start_audio;
};

struct AppConfig {
  std::string udp_bind{"0.0.0.0"};
  int udp_port{9999};
  int cooldown_ms{5000};
  std::vector<DeviceConfig> devices;
};

struct DeviceSnapshot {
  std::string ip;
  std::string name;
  int run{0};
};

enum class VoiceEventType { DeviceStart };

struct VoiceEvent {
  VoiceEventType type{VoiceEventType::DeviceStart};
  std::string ip;
  std::string name;
};

}  // namespace vc
