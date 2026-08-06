#pragma once
#include "Types.hpp"
#include <cstdint>
#include <optional>
#include <string>

namespace vc {

class UdpListener {
 public:
  UdpListener(std::string bind, int port);
  ~UdpListener();

  UdpListener(const UdpListener&) = delete;
  UdpListener& operator=(const UdpListener&) = delete;

  void open();
  std::optional<DeviceSnapshot> poll();
  void close();

 private:
  std::string bind_;
  int port_;
  std::uintptr_t sock_{static_cast<std::uintptr_t>(-1)};
  bool wsa_started_{false};
};

}  // namespace vc
