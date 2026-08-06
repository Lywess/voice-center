#include "Logger.hpp"
#include <chrono>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace vc {
namespace {
std::string now_stamp() {
  using clock = std::chrono::system_clock;
  const auto t = clock::to_time_t(clock::now());
  std::tm tm{};
  localtime_s(&tm, &t);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S");
  return oss.str();
}
void write(const char* level, const std::string& msg) {
  std::cout << '[' << now_stamp() << "] [" << level << "] " << msg << std::endl;
}
}  // namespace

void log_info(const std::string& msg) { write("INFO", msg); }
void log_warn(const std::string& msg) { write("WARN", msg); }
void log_debug(const std::string& msg) { write("DEBUG", msg); }
}
