#include "StateTracker.hpp"

namespace vc {

std::optional<VoiceEvent> StateTracker::on_snapshot(const DeviceSnapshot& snap) {
  if (snap.ip.empty()) return std::nullopt;
  if (!seen_[snap.ip]) {
    seen_[snap.ip] = true;
    last_run_[snap.ip] = snap.run;
    return std::nullopt;
  }
  const int prev = last_run_[snap.ip];
  last_run_[snap.ip] = snap.run;
  if (prev == 0 && snap.run == 1) {
    return VoiceEvent{VoiceEventType::DeviceStart, snap.ip, snap.name};
  }
  return std::nullopt;
}

}  // namespace vc
