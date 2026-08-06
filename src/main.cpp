#include "core/Config.hpp"
#include "core/DryRunPlayer.hpp"
#include "core/EventQueue.hpp"
#include "core/Logger.hpp"
#include "core/StateTracker.hpp"
#include "core/UdpListener.hpp"
#include "core/VoiceManager.hpp"
#ifndef VOICE_CENTER_DRY_ONLY
#include "platform/windows/SdlPlayer.hpp"
#endif

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

std::atomic<bool> g_running{true};

#ifdef _WIN32
BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
  switch (ctrl_type) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
      g_running.store(false);
      return TRUE;
    default:
      return FALSE;
  }
}
#endif

void print_usage(const char* argv0) {
  vc::log_info(std::string("用法: ") + argv0 + " [--config path] [--dry-run]");
  vc::log_info("默认配置: config/voice.json（请在仓库根目录运行）");
}

}  // namespace

int main(int argc, char** argv) {
  std::string config_path = "config/voice.json";
  bool dry_run = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config") {
      if (i + 1 >= argc) {
        vc::log_warn("--config 需要路径参数");
        return 1;
      }
      config_path = argv[++i];
    } else if (arg == "--dry-run") {
      dry_run = true;
    } else if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return 0;
    } else {
      vc::log_warn("未知参数: " + arg);
      print_usage(argv[0]);
      return 1;
    }
  }

#ifdef _WIN32
  SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#endif

  try {
    const vc::AppConfig cfg = vc::load_config(config_path);

    vc::UdpListener udp(cfg.udp_bind, cfg.udp_port);
    udp.open();

    vc::StateTracker tracker;
    vc::EventQueue queue(cfg.cooldown_ms);

#ifdef VOICE_CENTER_DRY_ONLY
    if (!dry_run) {
      vc::log_warn("当前为 VOICE_CENTER_DRY_ONLY 构建，请使用 --dry-run");
      return 1;
    }
    vc::DryRunPlayer player;
#else
    vc::DryRunPlayer dry_player;
    vc::SdlPlayer sdl_player;
    vc::IAudioPlayer& player =
        dry_run ? static_cast<vc::IAudioPlayer&>(dry_player)
                : static_cast<vc::IAudioPlayer&>(sdl_player);
#endif

    vc::VoiceManager voice(cfg, player, dry_run);

    vc::log_info(std::string("voice-center 已启动") + (dry_run ? " (dry-run)" : "") +
                 " config=" + config_path + " udp=" + cfg.udp_bind + ":" +
                 std::to_string(cfg.udp_port));

    while (g_running.load()) {
      if (auto snap = udp.poll()) {
        const vc::DeviceConfig* dev = vc::find_device(cfg, snap->ip);
        if (dev == nullptr) {
          vc::log_debug("忽略未知设备 ip=" + snap->ip);
          continue;
        }
        if (!snap->name.empty() && snap->name != dev->name) {
          vc::log_warn("设备名称不一致 ip=" + snap->ip + " packet_name=" + snap->name +
                       " config_name=" + dev->name);
        }
        if (auto ev = tracker.on_snapshot(*snap)) {
          (void)queue.try_enqueue(*ev);  // cooldown 拒绝时静默丢弃
        }
      }

      // handle 同步阻塞播完；回到循环即空闲，可取下一条
      if (auto ev = queue.try_pop()) {
        voice.handle(*ev);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    vc::log_info("voice-center 正在退出");
    udp.close();
  } catch (const std::exception& e) {
    vc::log_warn(std::string("启动失败: ") + e.what());
    return 1;
  }

  return 0;
}
