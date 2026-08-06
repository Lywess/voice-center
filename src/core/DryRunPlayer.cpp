#include "DryRunPlayer.hpp"
#include <filesystem>

namespace vc {

bool DryRunPlayer::play(const std::string& path) {
  // Chinese user-facing messages are owned by VoiceManager (dry_run_ path).
  return std::filesystem::exists(path);
}

}  // namespace vc
