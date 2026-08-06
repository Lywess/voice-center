#pragma once
#include "IAudioPlayer.hpp"

namespace vc {

class DryRunPlayer : public IAudioPlayer {
 public:
  bool play(const std::string& path) override;
};

}  // namespace vc
