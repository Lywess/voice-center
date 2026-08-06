#pragma once
#include <string>

namespace vc {

class IAudioPlayer {
 public:
  virtual ~IAudioPlayer() = default;
  // Returns true if playback started/finished OK.
  // Returns false if file missing or backend error (caller logs).
  virtual bool play(const std::string& path) = 0;
};

}  // namespace vc
