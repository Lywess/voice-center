#include "VoiceManager.hpp"
#include "Config.hpp"
#include "Logger.hpp"
#include <filesystem>

namespace vc {

VoiceManager::VoiceManager(const AppConfig& cfg, IAudioPlayer& player, bool dry_run)
    : cfg_(cfg), player_(player), dry_run_(dry_run) {}

void VoiceManager::handle(const VoiceEvent& ev) {
  const DeviceConfig* dev = find_device(cfg_, ev.ip);
  const std::string name = dev ? dev->name : ev.name;
  log_info("收到启动信号 ip=" + ev.ip + " name=" + name);

  if (!dev || dev->start_audio.empty()) {
    log_warn("未找到音频文件 path=");
    return;
  }
  const std::string& path = dev->start_audio;
  if (!std::filesystem::exists(path)) {
    log_warn("未找到音频文件 path=" + path);
    return;
  }
  if (dry_run_) {
    log_info("将播放语音 path=" + path);
    return;
  }
  log_info("播放语音 path=" + path);
  if (!player_.play(path)) {
    log_warn("播放失败 path=" + path);
  }
}

}  // namespace vc
