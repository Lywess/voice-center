#include "EventQueue.hpp"

namespace vc {

EventQueue::EventQueue(int cooldown_ms) : cooldown_ms_(cooldown_ms) {}

bool EventQueue::try_enqueue(const VoiceEvent& ev) {
  const auto now = std::chrono::steady_clock::now();
  auto it = last_accept_.find(ev.ip);
  if (it != last_accept_.end()) {
    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
    if (ms < cooldown_ms_) return false;
  }
  q_.push(ev);
  last_accept_[ev.ip] = now;
  return true;
}

std::optional<VoiceEvent> EventQueue::try_pop() {
  if (q_.empty()) return std::nullopt;
  auto e = q_.front();
  q_.pop();
  return e;
}

std::size_t EventQueue::size() const { return q_.size(); }

}  // namespace vc
