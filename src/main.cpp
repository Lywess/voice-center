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
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

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

void setup_console_utf8() {
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
  // 让 MSVC CRT 把 UTF-8 窄字符串正确写到控制台
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);
}

std::filesystem::path exe_dir() {
  wchar_t buf[MAX_PATH]{};
  const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) {
    return std::filesystem::current_path();
  }
  return std::filesystem::path(buf).parent_path();
}
#endif

void pause_if_needed() {
  // 双击运行时窗口会立刻关掉，失败时停一下方便看报错
  std::cout << std::endl << "按回车键退出..." << std::endl;
  std::cin.get();
}

std::filesystem::path find_repo_root() {
  namespace fs = std::filesystem;
  std::vector<fs::path> starts;
  starts.push_back(fs::current_path());
#ifdef _WIN32
  starts.push_back(exe_dir());
#endif
  for (const auto& start : starts) {
    fs::path p = start;
    for (int i = 0; i < 6; ++i) {
      if (fs::exists(p / "config" / "voice.json")) {
        return p;
      }
      if (!p.has_parent_path() || p.parent_path() == p) {
        break;
      }
      p = p.parent_path();
    }
  }
  return {};
}

void print_usage(const char* argv0) {
  vc::log_info(std::string("用法: ") + argv0 + " [--config path] [--dry-run]");
  vc::log_info("默认配置: config/voice.json（会自动从 exe 向上查找仓库根目录）");
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
  setup_console_utf8();
  SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#endif

  std::string config_path;
  bool dry_run = false;
  bool config_explicit = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config") {
      if (i + 1 >= argc) {
        vc::log_warn("--config 需要路径参数");
        pause_if_needed();
        return 1;
      }
      config_path = argv[++i];
      config_explicit = true;
    } else if (arg == "--dry-run") {
      dry_run = true;
    } else if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return 0;
    } else {
      vc::log_warn("未知参数: " + arg);
      print_usage(argv[0]);
      pause_if_needed();
      return 1;
    }
  }

  try {
    if (!config_explicit) {
      const auto root = find_repo_root();
      if (root.empty()) {
        throw std::runtime_error(
            "找不到 config/voice.json。请在仓库根目录运行，或使用 --config 指定路径。");
      }
      std::filesystem::current_path(root);
      config_path = (root / "config" / "voice.json").string();
      vc::log_info("工作目录: " + root.string());
    }

    const vc::AppConfig cfg = vc::load_config(config_path);

    vc::UdpListener udp(cfg.udp_bind, cfg.udp_port);
    udp.open();

    vc::StateTracker tracker;
    vc::EventQueue queue(cfg.cooldown_ms);

#ifdef VOICE_CENTER_DRY_ONLY
    if (!dry_run) {
      vc::log_warn("当前为 VOICE_CENTER_DRY_ONLY 构建，请使用 --dry-run");
      pause_if_needed();
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
        } else {
          if (!snap->name.empty() && snap->name != dev->name) {
            vc::log_warn("设备名称不一致 ip=" + snap->ip + " packet_name=" + snap->name +
                         " config_name=" + dev->name);
          }
          if (auto ev = tracker.on_snapshot(*snap)) {
            (void)queue.try_enqueue(*ev);  // cooldown 拒绝时静默丢弃
          }
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
    pause_if_needed();
    return 1;
  }

  return 0;
}
