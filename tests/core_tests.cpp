#include "core/Config.hpp"
#include "core/EventQueue.hpp"
#include "core/IAudioPlayer.hpp"
#include "core/StateTracker.hpp"
#include "core/VoiceManager.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

class FakePlayer : public vc::IAudioPlayer {
 public:
  int play_calls = 0;
  bool play(const std::string& /*path*/) override {
    ++play_calls;
    return true;
  }
};

}  // namespace

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

  {
    vc::EventQueue q(5000);
    vc::VoiceEvent e{vc::VoiceEventType::DeviceStart, "192.168.1.10", "灌装机"};
    assert(q.try_enqueue(e));
    assert(!q.try_enqueue(e));  // cooldown
    auto out = q.try_pop();
    assert(out.has_value());
    assert(out->ip == "192.168.1.10");
    assert(!q.try_pop().has_value());
  }
  {
    vc::EventQueue q(0);  // zero cooldown for multi-device order
    assert(q.try_enqueue({vc::VoiceEventType::DeviceStart, "a", "A"}));
    assert(q.try_enqueue({vc::VoiceEventType::DeviceStart, "b", "B"}));
    assert(q.try_pop()->ip == "a");
    assert(q.try_pop()->ip == "b");
  }
  std::cout << "event queue tests passed\n";

  {
    vc::VoiceEvent e{vc::VoiceEventType::DeviceStart, "192.168.1.10", "灌装机"};
    // Config path assets/audio/filling_start.wav may or may not exist in CWD;
    // missing-file branch must not call player; dry_run also skips player.
    const fs::path missing_cfg =
        fs::temp_directory_path() / "voice_center_test_missing_audio.json";
    {
      std::ofstream o(missing_cfg);
      o << R"({
        "udp": {"bind": "0.0.0.0", "port": 9999},
        "queue": {"cooldown_ms": 5000},
        "devices": [
          {"ip": "192.168.1.10", "name": "8线灌装机",
           "start_audio": "definitely_missing_audio_file.wav"}
        ]
      })";
    }
    auto cfg_miss = vc::load_config(missing_cfg);
    FakePlayer miss_player;
    vc::VoiceManager vm_miss(cfg_miss, miss_player, /*dry_run=*/false);
    vm_miss.handle(e);  // start log + 未找到; no play
    assert(miss_player.play_calls == 0);

    // Unknown IP: still start-signal path with empty audio warn; no play.
    FakePlayer unk_player;
    vc::VoiceManager vm_unk(cfg, unk_player, /*dry_run=*/false);
    vm_unk.handle({vc::VoiceEventType::DeviceStart, "9.9.9.9", "unknown"});
    assert(unk_player.play_calls == 0);

    // dry_run with existing file: log 将播放, do not call player.
    const fs::path exist_wav =
        fs::temp_directory_path() / "voice_center_test_exist.wav";
    {
      std::ofstream o(exist_wav, std::ios::binary);
      o << "RIFF";  // placeholder; VoiceManager only checks exists()
    }
    const fs::path exist_cfg =
        fs::temp_directory_path() / "voice_center_test_exist_audio.json";
    {
      std::ofstream o(exist_cfg);
      o << "{\n"
        << "  \"udp\": {\"bind\": \"0.0.0.0\", \"port\": 9999},\n"
        << "  \"queue\": {\"cooldown_ms\": 5000},\n"
        << "  \"devices\": [{\n"
        << "    \"ip\": \"192.168.1.10\", \"name\": \"8线灌装机\",\n"
        << "    \"start_audio\": \"" << exist_wav.generic_string() << "\"\n"
        << "  }]\n"
        << "}\n";
    }
    auto cfg_exist = vc::load_config(exist_cfg);
    FakePlayer dry_player;
    vc::VoiceManager vm_dry(cfg_exist, dry_player, /*dry_run=*/true);
    vm_dry.handle(e);
    assert(dry_player.play_calls == 0);

    // Non-dry with existing file: must call player.
    FakePlayer real_player;
    vc::VoiceManager vm_real(cfg_exist, real_player, /*dry_run=*/false);
    vm_real.handle(e);
    assert(real_player.play_calls == 1);

    std::error_code ec;
    fs::remove(missing_cfg, ec);
    fs::remove(exist_cfg, ec);
    fs::remove(exist_wav, ec);
  }
  std::cout << "voice manager tests passed\n";

  return 0;
}
