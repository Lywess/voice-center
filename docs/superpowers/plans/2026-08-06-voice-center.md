# voice-center Demo Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a Windows C++17 voice-center Demo that listens for UDP device JSON, detects `run` 0→1 by IP, logs「收到启动信号」, then plays WAV (or logs「未找到音频文件」) via SDL2 with FIFO + cooldown.

**Architecture:** Single-process main loop: `UdpListener` → `StateTracker` → `EventQueue` → `VoiceManager` → `SdlPlayer` (or dry-run). Core logic is platform-agnostic; only `SdlPlayer` lives under `platform/windows`.

**Tech Stack:** C++17, CMake 3.20+, Winsock2 (UDP), nlohmann/json (header-only via FetchContent), SDL2 (audio), optional `--dry-run` without SDL playback.

## Global Constraints

- Platform for this plan: **Windows only**
- Language/standard: **C++17**
- Build: **CMake**
- Device primary key: **`ip`** (`name` is display/log only)
- Trigger: **`run` 0→1 only**; first sight of an IP never fires start
- Logging: **append-only stdout**; no status panel, no clear-screen, no GUI
- On accepted start: **must** log `收到启动信号` before play or missing-file warn
- Queue: **FIFO** + per-IP **`cooldown_ms`**
- Audio format: **WAV only**
- Spec: `docs/superpowers/specs/2026-08-06-voice-center-design.md`

---

## File Structure

| Path | Responsibility |
|------|----------------|
| `CMakeLists.txt` | Root build, FetchContent json, find SDL2, targets `voice-center` + `core_tests` |
| `config/voice.json` | UDP bind/port, cooldown, device map |
| `assets/audio/.gitkeep` | Placeholder for user-supplied WAVs |
| `src/core/Types.hpp` | `DeviceConfig`, `DeviceSnapshot`, `VoiceEvent` |
| `src/core/Logger.hpp` / `.cpp` | Timestamped INFO/WARN/DEBUG to stdout |
| `src/core/Config.hpp` / `.cpp` | Load/validate `voice.json` |
| `src/core/StateTracker.hpp` / `.cpp` | Per-IP last `run`, emit start edges |
| `src/core/EventQueue.hpp` / `.cpp` | FIFO + cooldown gate |
| `src/core/UdpListener.hpp` / `.cpp` | Winsock UDP bind/recv + JSON parse |
| `src/core/IAudioPlayer.hpp` | `virtual bool play(const std::string& path) = 0` |
| `src/core/DryRunPlayer.hpp` / `.cpp` | Logs「将播放语音」, returns true if file exists |
| `src/core/VoiceManager.hpp` / `.cpp` | Start log + resolve audio + play/missing |
| `src/platform/windows/SdlPlayer.hpp` / `.cpp` | SDL2 WAV playback (blocking until done) |
| `src/main.cpp` | CLI, wire components, loop |
| `tests/core_tests.cpp` | Assert-based unit tests for tracker/queue/config |
| `udp_mock/*` | Existing Python harness (do not rewrite unless broken) |

---

### Task 1: Scaffold + Logger + Types + Config file

**Files:**
- Create: `CMakeLists.txt`
- Create: `src/core/Types.hpp`
- Create: `src/core/Logger.hpp`
- Create: `src/core/Logger.cpp`
- Create: `config/voice.json`
- Create: `assets/audio/.gitkeep`
- Create: `README.md` (build/run notes only)

**Interfaces:**
- Produces: `vc::log_info/warn/debug(const std::string&)`; types in `vc` namespace

- [ ] **Step 1: Create types**

```cpp
// src/core/Types.hpp
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace vc {

struct DeviceConfig {
  std::string ip;
  std::string name;
  std::string start_audio;
};

struct AppConfig {
  std::string udp_bind{"0.0.0.0"};
  int udp_port{9999};
  int cooldown_ms{5000};
  std::vector<DeviceConfig> devices;
};

struct DeviceSnapshot {
  std::string ip;
  std::string name;
  int run{0};
};

enum class VoiceEventType { DeviceStart };

struct VoiceEvent {
  VoiceEventType type{VoiceEventType::DeviceStart};
  std::string ip;
  std::string name;
};

}  // namespace vc
```

- [ ] **Step 2: Create Logger**

```cpp
// src/core/Logger.hpp
#pragma once
#include <string>
namespace vc {
void log_info(const std::string& msg);
void log_warn(const std::string& msg);
void log_debug(const std::string& msg);
}
```

```cpp
// src/core/Logger.cpp
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
```

- [ ] **Step 3: Write `config/voice.json` exactly as in the spec** (three devices, port 9999, cooldown 5000). Create empty `assets/audio/.gitkeep`.

- [ ] **Step 4: Minimal root CMake that builds a stub later; for now only ensure folders exist. Temporary `src/main.cpp`:**

```cpp
#include "core/Logger.hpp"
int main() {
  vc::log_info("voice-center scaffold ok");
  return 0;
}
```

```cmake
cmake_minimum_required(VERSION 3.20)
project(voice-center VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(voice-center
  src/main.cpp
  src/core/Logger.cpp
)
target_include_directories(voice-center PRIVATE src)
```

- [ ] **Step 5: Configure & build smoke**

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\voice-center.exe
```

Expected: log line containing `scaffold ok`.

- [ ] **Step 6: Commit**

```powershell
git add CMakeLists.txt src/core/Types.hpp src/core/Logger.hpp src/core/Logger.cpp src/main.cpp config/voice.json assets/audio/.gitkeep README.md
git commit -m "chore: scaffold voice-center with logger and config stub"
```

---

### Task 2: Config loader + unit test harness

**Files:**
- Create: `src/core/Config.hpp`
- Create: `src/core/Config.cpp`
- Create: `tests/core_tests.cpp`
- Modify: `CMakeLists.txt` (FetchContent nlohmann_json, `core_tests` target)

**Interfaces:**
- Produces: `vc::AppConfig vc::load_config(const std::filesystem::path& path);` throws `std::runtime_error` on failure
- Produces: `const DeviceConfig* vc::find_device(const AppConfig&, const std::string& ip);`

- [ ] **Step 1: Add FetchContent for nlohmann_json and `core_tests` target in CMakeLists.txt**

```cmake
include(FetchContent)
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_library(voice_core
  src/core/Logger.cpp
  src/core/Config.cpp
)
target_include_directories(voice_core PUBLIC src)
target_link_libraries(voice_core PUBLIC nlohmann_json::nlohmann_json)

add_executable(core_tests tests/core_tests.cpp)
target_link_libraries(core_tests PRIVATE voice_core)

# voice-center links voice_core (update from Task 1)
```

- [ ] **Step 2: Write failing tests first** in `tests/core_tests.cpp`:

```cpp
#include "core/Config.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

static void write_temp_config(const std::filesystem::path& p) {
  std::ofstream o(p);
  o << R"({
    "udp": {"bind": "0.0.0.0", "port": 9999},
    "queue": {"cooldown_ms": 5000},
    "devices": [
      {"ip": "192.168.1.10", "name": "8线灌装机", "start_audio": "assets/audio/filling_start.wav"}
    ]
  })";
}

int main() {
  namespace fs = std::filesystem;
  const fs::path tmp = fs::temp_directory_path() / "voice_center_test_config.json";
  write_temp_config(tmp);
  auto cfg = vc::load_config(tmp);
  assert(cfg.udp_port == 9999);
  assert(cfg.cooldown_ms == 5000);
  assert(cfg.devices.size() == 1);
  assert(cfg.devices[0].ip == "192.168.1.10");
  const auto* d = vc::find_device(cfg, "192.168.1.10");
  assert(d != nullptr);
  assert(vc::find_device(cfg, "9.9.9.9") == nullptr);
  std::cout << "config tests passed\n";
  return 0;
}
```

- [ ] **Step 3: Run tests — expect link/compile failure** (Config not implemented)

```powershell
cmake -S . -B build
cmake --build build --config Release --target core_tests
```

Expected: FAIL (missing Config.cpp symbols or file).

- [ ] **Step 4: Implement Config**

```cpp
// src/core/Config.hpp
#pragma once
#include "Types.hpp"
#include <filesystem>
namespace vc {
AppConfig load_config(const std::filesystem::path& path);
const DeviceConfig* find_device(const AppConfig& cfg, const std::string& ip);
}
```

```cpp
// src/core/Config.cpp
#include "Config.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace vc {

AppConfig load_config(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open config: " + path.string());
  nlohmann::json j;
  in >> j;
  AppConfig cfg;
  cfg.udp_bind = j.at("udp").value("bind", std::string("0.0.0.0"));
  cfg.udp_port = j.at("udp").at("port").get<int>();
  cfg.cooldown_ms = j.at("queue").value("cooldown_ms", 5000);
  for (const auto& d : j.at("devices")) {
    DeviceConfig dc;
    dc.ip = d.at("ip").get<std::string>();
    dc.name = d.at("name").get<std::string>();
    dc.start_audio = d.at("start_audio").get<std::string>();
    if (dc.ip.empty()) throw std::runtime_error("device ip empty");
    cfg.devices.push_back(std::move(dc));
  }
  if (cfg.devices.empty()) throw std::runtime_error("no devices in config");
  return cfg;
}

const DeviceConfig* find_device(const AppConfig& cfg, const std::string& ip) {
  for (const auto& d : cfg.devices) {
    if (d.ip == ip) return &d;
  }
  return nullptr;
}

}
```

- [ ] **Step 5: Rebuild and run tests**

```powershell
cmake --build build --config Release --target core_tests
.\build\Release\core_tests.exe
```

Expected: `config tests passed`

- [ ] **Step 6: Commit**

```powershell
git add CMakeLists.txt src/core/Config.hpp src/core/Config.cpp tests/core_tests.cpp
git commit -m "feat: load voice.json config with unit tests"
```

---

### Task 3: StateTracker (edge detect)

**Files:**
- Create: `src/core/StateTracker.hpp`
- Create: `src/core/StateTracker.cpp`
- Modify: `tests/core_tests.cpp`
- Modify: `CMakeLists.txt` (add StateTracker.cpp to `voice_core`)

**Interfaces:**
- Consumes: `DeviceSnapshot`, `VoiceEvent`
- Produces:
  - `class StateTracker { public: std::optional<VoiceEvent> on_snapshot(const DeviceSnapshot&); };`
  - First sight → `nullopt` even if `run==1`
  - `0→1` → `VoiceEvent{DeviceStart, ip, name}`
  - `1→0` or unchanged → `nullopt`

- [ ] **Step 1: Append failing tests**

```cpp
#include "core/StateTracker.hpp"
// ...
{
  vc::StateTracker st;
  vc::DeviceSnapshot a{"192.168.1.10", "灌装机", 1};
  assert(!st.on_snapshot(a).has_value()); // first sight
  a.run = 0;
  assert(!st.on_snapshot(a).has_value());
  a.run = 1;
  auto ev = st.on_snapshot(a);
  assert(ev.has_value());
  assert(ev->ip == "192.168.1.10");
  assert(ev->type == vc::VoiceEventType::DeviceStart);
  assert(!st.on_snapshot(a).has_value()); // still 1
}
```

- [ ] **Step 2: Build — fail** then implement:

```cpp
// StateTracker.hpp
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
  std::unordered_map<std::string, int> last_run_; // key = ip
  std::unordered_map<std::string, bool> seen_;
};
}
```

```cpp
// StateTracker.cpp
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
}
```

- [ ] **Step 3: Run `core_tests.exe` — expect PASS including tracker cases**

- [ ] **Step 4: Commit**

```powershell
git add src/core/StateTracker.hpp src/core/StateTracker.cpp tests/core_tests.cpp CMakeLists.txt
git commit -m "feat: detect device start edges by IP"
```

---

### Task 4: EventQueue (FIFO + cooldown)

**Files:**
- Create: `src/core/EventQueue.hpp`
- Create: `src/core/EventQueue.cpp`
- Modify: `tests/core_tests.cpp`, `CMakeLists.txt`

**Interfaces:**
- Produces:
  - `explicit EventQueue(int cooldown_ms);`
  - `bool try_enqueue(const VoiceEvent& ev);` // false if cooling down
  - `std::optional<VoiceEvent> try_pop();`
  - `std::size_t size() const;`
- Cooldown clock: `std::chrono::steady_clock`; record time on successful enqueue

- [ ] **Step 1: Failing tests**

```cpp
#include "core/EventQueue.hpp"
{
  vc::EventQueue q(5000);
  vc::VoiceEvent e{vc::VoiceEventType::DeviceStart, "192.168.1.10", "灌装机"};
  assert(q.try_enqueue(e));
  assert(!q.try_enqueue(e)); // cooldown
  auto out = q.try_pop();
  assert(out.has_value());
  assert(out->ip == "192.168.1.10");
  assert(!q.try_pop().has_value());
}
{
  vc::EventQueue q(0); // zero cooldown for multi-device order
  assert(q.try_enqueue({vc::VoiceEventType::DeviceStart, "a", "A"}));
  assert(q.try_enqueue({vc::VoiceEventType::DeviceStart, "b", "B"}));
  assert(q.try_pop()->ip == "a");
  assert(q.try_pop()->ip == "b");
}
```

- [ ] **Step 2: Implement**

```cpp
// EventQueue.hpp / .cpp — use std::queue<VoiceEvent> + unordered_map<string, steady_clock::time_point>
// try_enqueue: if now - last[ip] < cooldown → return false; else push + update last[ip]
```

Full implementation:

```cpp
#include "EventQueue.hpp"
namespace vc {
EventQueue::EventQueue(int cooldown_ms) : cooldown_ms_(cooldown_ms) {}

bool EventQueue::try_enqueue(const VoiceEvent& ev) {
  const auto now = std::chrono::steady_clock::now();
  auto it = last_accept_.find(ev.ip);
  if (it != last_accept_.end()) {
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
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
}
```

- [ ] **Step 3: Run tests — PASS**

- [ ] **Step 4: Commit**

```powershell
git add src/core/EventQueue.hpp src/core/EventQueue.cpp tests/core_tests.cpp CMakeLists.txt
git commit -m "feat: FIFO voice event queue with per-IP cooldown"
```

---

### Task 5: UdpListener (Winsock)

**Files:**
- Create: `src/core/UdpListener.hpp`
- Create: `src/core/UdpListener.cpp`
- Modify: `CMakeLists.txt` (`ws2_32` for Windows)

**Interfaces:**
- Produces:
  - `UdpListener(std::string bind, int port);`
  - `void open();` // throws on bind failure
  - `std::optional<DeviceSnapshot> poll();` // non-blocking; nullopt if no packet / bad packet
  - `void close();`
- Parse JSON fields `ip`, `name`, `run` (run may be string or number; accept both, convert to int)
- Unknown/malformed → `log_debug` and `nullopt`
- Does **not** filter by configured device list (caller does)

- [ ] **Step 1: Implement Winsock UDP non-blocking recv**

Key points in `UdpListener.cpp`:
- `WSAStartup` / `WSACleanup`
- `socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)`
- `ioctlsocket(sock, FIONBIO, &one)`
- `bind` to `udp_bind:udp_port`
- `recvfrom` into buffer → `nlohmann::json::parse` → fill `DeviceSnapshot`
- On port in use: throw `std::runtime_error` with port number

- [ ] **Step 2: Manual smoke (optional small `tools` not required): build `voice-center` temporary main that opens listener and prints one packet — or wait for Task 8.** Prefer not leaving broken main; keep Task 1 main until Task 8.

- [ ] **Step 3: Commit**

```powershell
git add src/core/UdpListener.hpp src/core/UdpListener.cpp CMakeLists.txt
git commit -m "feat: non-blocking UDP listener for device JSON"
```

---

### Task 6: IAudioPlayer + DryRunPlayer + SdlPlayer

**Files:**
- Create: `src/core/IAudioPlayer.hpp`
- Create: `src/core/DryRunPlayer.hpp`
- Create: `src/core/DryRunPlayer.cpp`
- Create: `src/platform/windows/SdlPlayer.hpp`
- Create: `src/platform/windows/SdlPlayer.cpp`
- Modify: `CMakeLists.txt` (`find_package(SDL2)`, link when not dry-only build)
- Modify: `README.md` with SDL2 install notes (vcpkg or SDL2 Windows release)

**Interfaces:**
- Produces:
```cpp
class IAudioPlayer {
 public:
  virtual ~IAudioPlayer() = default;
  // Returns true if playback started/finished OK.
  // Returns false if file missing or backend error (caller logs).
  virtual bool play(const std::string& path) = 0;
};
```
- `DryRunPlayer::play`: if `!filesystem::exists(path)` return false; else `log_info("将播放语音 path=" + path)` and return true
- `SdlPlayer::play`: if missing return false; else SDL_LoadWAV + open device + queue + wait until done + cleanup; on SDL error return false

- [ ] **Step 1: Implement DryRunPlayer (no SDL dependency in this TU)**

- [ ] **Step 2: Implement SdlPlayer using SDL2 audio API**

Minimal pattern:
```cpp
bool SdlPlayer::play(const std::string& path) {
  if (!std::filesystem::exists(path)) return false;
  SDL_AudioSpec wav_spec{};
  Uint8* buf = nullptr;
  Uint32 len = 0;
  if (!SDL_LoadWAV(path.c_str(), &wav_spec, &buf, &len)) return false;
  SDL_AudioSpec have{};
  SDL_AudioDeviceID dev = SDL_OpenAudioDevice(nullptr, 0, &wav_spec, &have, 0);
  if (!dev) { SDL_FreeWAV(buf); return false; }
  SDL_QueueAudio(dev, buf, len);
  SDL_PauseAudioDevice(dev, 0);
  while (SDL_GetQueuedAudioSize(dev) > 0) SDL_Delay(10);
  SDL_CloseAudioDevice(dev);
  SDL_FreeWAV(buf);
  return true;
}
```

Call `SDL_Init(SDL_INIT_AUDIO)` once in `SdlPlayer` constructor; `SDL_Quit` in destructor.

- [ ] **Step 3: CMake**

```cmake
find_package(SDL2 REQUIRED)
# voice-center links SDL2::SDL2 (or SDL2::SDL2main as needed on Windows)
```

Document in README:

```powershell
vcpkg install sdl2:x64-windows
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake
```

- [ ] **Step 4: Commit**

```powershell
git add src/core/IAudioPlayer.hpp src/core/DryRunPlayer.* src/platform/windows/SdlPlayer.* CMakeLists.txt README.md
git commit -m "feat: SDL2 and dry-run audio players"
```

---

### Task 7: VoiceManager (mandatory start log)

**Files:**
- Create: `src/core/VoiceManager.hpp`
- Create: `src/core/VoiceManager.cpp`
- Modify: `tests/core_tests.cpp` (optional logic test with DryRunPlayer + temp missing path)
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `AppConfig`, `IAudioPlayer&`, `VoiceEvent`
- Produces: `void VoiceManager::handle(const VoiceEvent& ev);`

**Required log order inside `handle`:**

1. Always: `log_info("收到启动信号 ip=" + ev.ip + " name=" + name);`  
   Prefer configured `name` if device found; else event name.
2. Look up device by `ev.ip`. If no device or empty `start_audio`:  
   `log_warn("未找到音频文件 path=...")` (path may be empty string explanation) and return.
3. If `player.play(path)` false: `log_warn("未找到音频文件 path=" + path);`  
   (also covers SDL failure — message stays as spec for missing; for SDL failure can append reason in same WARN or second WARN `播放失败`)
4. If play true and not dry-run path already logged: for SdlPlayer, VoiceManager should `log_info("播放语音 path=" + path)` **before** calling `play`.  
   For DryRunPlayer, either:
   - VoiceManager logs「将播放语音」when a `dry_run` flag is set, **or**
   - DryRunPlayer logs it (as Task 6). Pick **one** place only — **prefer VoiceManager** owning all user-facing play messages so order stays: 启动信号 → 播放/将播放/未找到.

**Clarify (lock this in code):**

```cpp
void VoiceManager::handle(const VoiceEvent& ev) {
  const DeviceConfig* dev = find_device(cfg_, ev.ip);
  const std::string name = dev ? dev->name : ev.name;
  log_info("收到启动信号 ip=" + ev.ip + " name=" + name);

  if (!dev || dev->start_audio.empty()) {
    log_warn("未找到音频文件 path=");
    return;
  }
  const std::string& path = dev->start_audio;
  if (!std::filesystem::exists(path)) {
    log_warn("未找到音频文件 path=" + path);
    return;
  }
  if (dry_run_) {
    log_info("将播放语音 path=" + path);
    return;
  }
  log_info("播放语音 path=" + path);
  if (!player_.play(path)) {
    log_warn("播放失败 path=" + path);
  }
}
```

Then simplify `DryRunPlayer` to unused in favor of `dry_run_` flag **or** keep DryRunPlayer as no-op play that returns exists(). Either is fine; **VoiceManager owns the Chinese messages.**

- [ ] **Step 1: Implement VoiceManager as above**

- [ ] **Step 2: Unit test with missing file still emits start log** — capture by temporarily not needing stream redirect: call handle and assert exists() path logic via a small test that uses a FakePlayer counting calls; at minimum assert `filesystem` branch. If stdout capture is awkward on Windows, skip automated log assert and verify in Task 8 manually.

- [ ] **Step 3: Commit**

```powershell
git add src/core/VoiceManager.hpp src/core/VoiceManager.cpp tests/core_tests.cpp CMakeLists.txt
git commit -m "feat: VoiceManager start-signal logging and playback dispatch"
```

---

### Task 8: main loop + CLI + end-to-end with udp_mock

**Files:**
- Modify: `src/main.cpp`
- Modify: `CMakeLists.txt` if needed
- Modify: `README.md` with demo steps

**Interfaces:**
- CLI: `voice-center [--config path] [--dry-run]`
- Default config: `config/voice.json` relative to CWD (document: run from repo root)

**Main loop algorithm:**

```text
load config
open UdpListener
create StateTracker, EventQueue(cooldown), VoiceManager(config, player, dry_run)
while running:
  if auto snap = udp.poll():
    if find_device(config, snap.ip) == nullptr: optional debug; continue
    if name mismatch with config: log_warn once-ish or every time (spec: warning OK)
    if auto ev = tracker.on_snapshot(*snap):
      queue.try_enqueue(*ev)  # false → silent drop (cooldown)
  if not playing:
    if auto ev = queue.try_pop():
      voice.handle(*ev)   # may block during SDL play — OK for demo
  sleep 1~5ms
```

**Ctrl+C:** use `SetConsoleCtrlHandler` on Windows to set `running=false`; if currently inside `play`, finish current clip then exit (acceptable).

- [ ] **Step 1: Implement `main.cpp` wiring**

- [ ] **Step 2: Build Release**

```powershell
cmake --build build --config Release
```

- [ ] **Step 3: Dry-run integration**

Terminal A:

```powershell
cd udp_mock
python udp_broadcast.py --interval 0.2
```

Terminal B (repo root):

```powershell
.\build\Release\voice-center.exe --dry-run
```

Expected when CSV transitions create 0→1:
- Line with `收到启动信号`
- Then `将播放语音` **or** `未找到音频文件` (likely missing until user adds WAV)
- No clear-screen; logs append
- Same IP not repeatedly accepted within 5s

- [ ] **Step 4: Commit**

```powershell
git add src/main.cpp README.md CMakeLists.txt
git commit -m "feat: wire UDP pipeline and dry-run demo entrypoint"
```

---

### Task 9: README polish + sample audio placeholders

**Files:**
- Modify: `README.md`
- Create: `assets/audio/README.md` listing expected filenames:
  - `filling_start.wav`
  - `capping_start.wav`
  - `labeling_start.wav`

- [ ] **Step 1: Document build, vcpkg SDL2, run with/without dry-run, udp_mock usage, where to drop AI WAVs**

- [ ] **Step 2: Commit**

```powershell
git add README.md assets/audio/README.md
git commit -m "docs: usage for voice-center demo and audio assets"
```

---

## Spec Coverage Checklist

| Spec item | Task |
|-----------|------|
| C++17 + CMake + SDL2 Windows | 1, 6, 8 |
| UDP JSON listen port 9999 | 5, 8 |
| Primary key IP | 3, 4, 7 |
| run 0→1 only; first sight silent | 3 |
| FIFO + cooldown | 4 |
| 收到启动信号 then play/missing | 7 |
| Log-only, no UI/panel | 7, 8 |
| `--dry-run` | 7, 8 |
| udp_mock integration | 8 |
| config/voice.json mapping | 2 |
| Skip missing WAV without stalling queue | 7, 8 |

## Self-Review Notes

- No ConsoleStatusBoard (removed from spec).
- VoiceManager owns Chinese user messages to keep order strict.
- Blocking SDL play in main loop is acceptable for Demo; do not introduce audio threads in this plan.
- `nlohmann/json` via FetchContent; SDL2 via `find_package` + vcpkg documented.
