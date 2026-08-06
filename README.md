# voice-center

Windows C++17 工业语音播报 Demo（UDP 监听 + WAV 播放）。

监听设备 JSON UDP，检测 `run` 从 `0 → 1` 的启动边沿，按 FIFO 排队播报对应 WAV；控制台只追加日志（无状态面板 / 不清屏）。

## 依赖

- CMake ≥ 3.20
- C++17 编译器（MSVC / Visual Studio）
- **SDL2**（真实播放；`--dry-run` 可不播音频，但仍建议安装以便完整构建）

### 安装 SDL2（推荐 vcpkg）

```powershell
vcpkg install sdl2:x64-windows
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### 或使用官方 Windows 开发包

1. 从 [SDL2 Releases](https://github.com/libsdl-org/SDL/releases) 下载 `SDL2-devel-*-VC.zip` 并解压
2. 配置时指定前缀（指向含 `cmake/sdl2-config.cmake` 的目录）：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=D:\path\to\SDL2-2.x.y
cmake --build build --config Release
```

仅构建 dry-run 播放器（不链接 SDL2）：

```powershell
cmake -S . -B build -DVOICE_CENTER_DRY_ONLY=ON
cmake --build build --config Release
```

## 构建

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## 运行（请在仓库根目录）

默认读取 `config/voice.json`（相对当前工作目录）。

```powershell
# 真实播放（需 assets/audio/*.wav）
.\build\Release\voice-center.exe

# 无音箱验证边沿 / 冷却 / 队列与「收到启动信号」
.\build\Release\voice-center.exe --dry-run

# 指定配置
.\build\Release\voice-center.exe --config config\voice.json --dry-run
```

Ctrl+C 会停止接收新事件；若正在播 WAV，会尽量播完当前一句再退出。

## Demo 联调步骤

终端 A — UDP 模拟广播：

```powershell
cd udp_mock
python udp_broadcast.py --interval 0.2
```

终端 B — 仓库根目录启动 voice-center：

```powershell
.\build\Release\voice-center.exe --dry-run
```

当 CSV 中某配置 IP 出现 `run` `0→1`（首次见到该 IP 不会触发）时，日志应出现：

1. `收到启动信号 ip=... name=...`
2. `将播放语音 path=...`（`--dry-run`）或 `未找到音频文件 path=...`（缺 WAV 时）

同一 IP 在 `cooldown_ms`（默认 5s）内再次启动边沿会被静默丢弃。未知 IP 不会进入播报路径。

音频文件放到 `assets/audio/` 后去掉 `--dry-run` 即可有声演示。
