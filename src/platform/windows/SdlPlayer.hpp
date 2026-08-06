#pragma once
#include "core/IAudioPlayer.hpp"

namespace vc {

class SdlPlayer : public IAudioPlayer {
 public:
  SdlPlayer();
  ~SdlPlayer() override;
  SdlPlayer(const SdlPlayer&) = delete;
  SdlPlayer& operator=(const SdlPlayer&) = delete;

  bool play(const std::string& path) override;

 private:
  bool initialized_ = false;
};

}  // namespace vc
