#pragma once
#include "Types.hpp"
#include <optional>
#include <string>
#include <unordered_map>

namespace vc {

class StateTracker {
 public:
  std::optional<VoiceEvent> on_snapshot(const DeviceSnapshot& snap);

 private:
  std::unordered_map<std::string, int> last_run_;  // key = ip
  std::unordered_map<std::string, bool> seen_;
};

}  // namespace vc
