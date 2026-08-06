#pragma once
#include "IAudioPlayer.hpp"
#include "Types.hpp"

namespace vc {

class VoiceManager {
 public:
  VoiceManager(const AppConfig& cfg, IAudioPlayer& player, bool dry_run = false);

  void handle(const VoiceEvent& ev);

 private:
  const AppConfig& cfg_;
  IAudioPlayer& player_;
  bool dry_run_;
};

}  // namespace vc
