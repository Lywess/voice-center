#include "core/Config.hpp"
#include "core/StateTracker.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

static void write_temp_config(const std::filesystem::path& p) {
  std::ofstream o(p);
  o << R"({
    "udp": {"bind": "0.0.0.0", "port": 9999},
    "queue": {"cooldown_ms": 5000},
    "devices": [
      {"ip": "192.168.1.10", "name": "8线灌装机", "start_audio": "assets/audio/filling_start.wav"}
    ]
  })";
}

int main() {
  namespace fs = std::filesystem;
  const fs::path tmp = fs::temp_directory_path() / "voice_center_test_config.json";
  write_temp_config(tmp);
  auto cfg = vc::load_config(tmp);
  assert(cfg.udp_port == 9999);
  assert(cfg.cooldown_ms == 5000);
  assert(cfg.devices.size() == 1);
  assert(cfg.devices[0].ip == "192.168.1.10");
  const auto* d = vc::find_device(cfg, "192.168.1.10");
  assert(d != nullptr);
  assert(vc::find_device(cfg, "9.9.9.9") == nullptr);
  std::cout << "config tests passed\n";

  vc::StateTracker st;
  vc::DeviceSnapshot a{"192.168.1.10", "灌装机", 1};
  assert(!st.on_snapshot(a).has_value());  // first sight
  a.run = 0;
  assert(!st.on_snapshot(a).has_value());
  a.run = 1;
  auto ev = st.on_snapshot(a);
  assert(ev.has_value());
  assert(ev->ip == "192.168.1.10");
  assert(ev->type == vc::VoiceEventType::DeviceStart);
  assert(!st.on_snapshot(a).has_value());  // still 1
  std::cout << "state tracker tests passed\n";

  return 0;
}
