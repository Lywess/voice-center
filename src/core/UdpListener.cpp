#include "UdpListener.hpp"
#include "Logger.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace vc {
namespace {

constexpr std::uintptr_t kInvalidSock = static_cast<std::uintptr_t>(-1);

SOCKET as_socket(std::uintptr_t s) { return static_cast<SOCKET>(s); }

int parse_run(const nlohmann::json& run) {
  if (run.is_number_integer()) return run.get<int>();
  if (run.is_number_unsigned()) return static_cast<int>(run.get<unsigned>());
  if (run.is_number_float()) return static_cast<int>(run.get<double>());
  if (run.is_string()) return std::stoi(run.get<std::string>());
  throw std::runtime_error("run type unsupported");
}

}  // namespace

UdpListener::UdpListener(std::string bind, int port)
    : bind_(std::move(bind)), port_(port) {}

UdpListener::~UdpListener() { close(); }

void UdpListener::open() {
  if (sock_ != kInvalidSock) return;

  WSADATA wsa{};
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    throw std::runtime_error("WSAStartup failed");
  }
  wsa_started_ = true;

  SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s == INVALID_SOCKET) {
    close();
    throw std::runtime_error("UDP socket() failed for port " + std::to_string(port_));
  }

  u_long one = 1;
  if (ioctlsocket(s, FIONBIO, &one) == SOCKET_ERROR) {
    closesocket(s);
    close();
    throw std::runtime_error("UDP ioctlsocket(FIONBIO) failed for port " +
                             std::to_string(port_));
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<u_short>(port_));
  if (InetPtonA(AF_INET, bind_.c_str(), &addr.sin_addr) != 1) {
    closesocket(s);
    close();
    throw std::runtime_error("invalid UDP bind address: " + bind_);
  }

  if (::bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
    const int err = WSAGetLastError();
    closesocket(s);
    close();
    throw std::runtime_error("UDP bind failed, port " + std::to_string(port_) +
                             " in use or unavailable (WSA " + std::to_string(err) + ")");
  }

  sock_ = static_cast<std::uintptr_t>(s);
}

std::optional<DeviceSnapshot> UdpListener::poll() {
  if (sock_ == kInvalidSock) return std::nullopt;

  char buf[65535];
  sockaddr_in from{};
  int from_len = sizeof(from);
  const int n = recvfrom(as_socket(sock_), buf, sizeof(buf), 0,
                         reinterpret_cast<sockaddr*>(&from), &from_len);
  if (n == SOCKET_ERROR) {
    const int err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK || err == WSAEINTR) return std::nullopt;
    log_debug("udp recvfrom error WSA " + std::to_string(err));
    return std::nullopt;
  }
  if (n <= 0) return std::nullopt;

  try {
    const auto j = nlohmann::json::parse(buf, buf + n);
    if (!j.contains("ip") || !j.contains("run")) {
      log_debug("udp malformed: missing ip or run");
      return std::nullopt;
    }
    DeviceSnapshot snap;
    snap.ip = j.at("ip").get<std::string>();
    snap.name = j.value("name", std::string{});
    snap.run = parse_run(j.at("run"));
    if (snap.ip.empty()) {
      log_debug("udp malformed: empty ip");
      return std::nullopt;
    }
    return snap;
  } catch (const std::exception& e) {
    log_debug(std::string("udp malformed: ") + e.what());
    return std::nullopt;
  }
}

void UdpListener::close() {
  if (sock_ != kInvalidSock) {
    closesocket(as_socket(sock_));
    sock_ = kInvalidSock;
  }
  if (wsa_started_) {
    WSACleanup();
    wsa_started_ = false;
  }
}

}  // namespace vc
