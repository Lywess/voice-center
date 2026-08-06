#include "DryRunPlayer.hpp"
#include "Logger.hpp"
#include <filesystem>

namespace vc {

bool DryRunPlayer::play(const std::string& path) {
  if (!std::filesystem::exists(path)) {
    return false;
  }
  log_info("将播放语音 path=" + path);
  return true;
}

}  // namespace vc
