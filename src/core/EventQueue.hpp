#pragma once
#include "Types.hpp"
#include <chrono>
#include <cstddef>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>

namespace vc {

class EventQueue {
 public:
  explicit EventQueue(int cooldown_ms);

  bool try_enqueue(const VoiceEvent& ev);
  std::optional<VoiceEvent> try_pop();
  std::size_t size() const;

 private:
  int cooldown_ms_;
  std::queue<VoiceEvent> q_;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_accept_;
};

}  // namespace vc
