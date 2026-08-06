# voice-center

Windows C++17 工业语音播报 Demo（UDP 监听 + WAV 播放）。

监听设备 JSON UDP，检测 `run` 从 `0 → 1` 的启动边沿，按 FIFO 排队播报对应 WAV；控制台只追加日志（无状态面板 / 不清屏）。

## 依赖

- **Visual Studio 2022**（含「使用 C++ 的桌面开发」工作负载）或独立 MSVC 工具链
- **CMake** ≥ 3.20
- **SDL2**（真实播放；`--dry-run` 可不播音频，但仍建议安装以便完整构建）
- **Python 3**（仅 `udp_mock` 联调脚本，可选）

## 构建（Visual Studio 2022）

在仓库根目录打开 **Developer PowerShell for VS 2022**（或已配置 `cl.exe` 的终端）。

### 方式 A：vcpkg（推荐）

```powershell
vcpkg install sdl2:x64-windows
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg根目录]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

Release 产物：`build\Release\voice-center.exe`（CMake 会尝试将 `SDL2.dll` 复制到同目录）。

### 方式 B：SDL2 官方开发包 + CMAKE_PREFIX_PATH

1. 从 [SDL2 Releases](https://github.com/libsdl-org/SDL/releases) 下载 `SDL2-devel-*-VC.zip` 并解压
2. 配置时指向前缀目录（含 `cmake/sdl2-config.cmake` 的父目录）：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=D:\path\to\SDL2-2.x.y
cmake --build build --config Release
```

### 仅构建 dry-run（不链接 SDL2）

无 SDL2 或只想验证 UDP / 队列逻辑时：

```powershell
cmake -S . -B build -DVOICE_CENTER_DRY_ONLY=ON
cmake --build build --config Release
```

此构建**必须**使用 `--dry-run` 运行。

## 音频文件

启动播报 WAV 由用户自行放入 [`assets/audio/`](assets/audio/README.md)，详见该目录下的说明。期望文件：

- `assets/audio/filling_start.wav` — 灌装机
- `assets/audio/capping_start.wav` — 旋盖机
- `assets/audio/labeling_start.wav` — 贴标机

缺文件时仍会打「收到启动信号」，但会记录 `未找到音频文件`；放入 WAV 后去掉 `--dry-run` 即可有声演示。

## 运行

程序会自动从当前目录或 exe 所在路径向上查找 `config/voice.json`，并切换到仓库根目录（双击 `build\Release\voice-center.exe` 也可以）。

若中文日志乱码：请用 **Windows Terminal** 或先执行 `chcp 65001`，再运行程序（程序启动时也会把控制台切到 UTF-8）。

```powershell
# 真实播放（需 assets/audio/*.wav + SDL2）
.\build\Release\voice-center.exe

# 无音箱：验证边沿 / 冷却 / 队列与「收到启动信号」（缺 WAV 时打「未找到音频文件」）
.\build\Release\voice-center.exe --dry-run

# 指定配置文件
.\build\Release\voice-center.exe --config config\voice.json --dry-run
```

Ctrl+C 会停止接收新事件；若正在播 WAV，会尽量播完当前一句再退出。启动失败时会暂停等待回车，避免双击窗口一闪而过。

## Demo 联调（udp_mock）

终端 A — 循环广播 CSV 中的设备 JSON（默认 `255.255.255.255:9999`，与 `config/voice.json` 中 `udp.port` 一致）：

```powershell
cd udp_mock
python udp_broadcast.py --interval 0.2
```

可选参数：`--csv`、`--host`、`--port`、`--interval`（见 `python udp_broadcast.py -h`）。

终端 B — 仓库根目录启动 voice-center：

```powershell
.\build\Release\voice-center.exe --dry-run
```

当 CSV 中某**已配置 IP** 出现 `run` **0→1**（该 IP 首次出现、无论 `run` 为何值，均不会触发）时，日志应出现：

1. `收到启动信号 ip=... name=...`
2. `--dry-run` 且文件存在：`将播放语音 path=...`；缺 WAV：`未找到音频文件 path=...`
3. 非 dry-run 且文件存在：实际播放对应 WAV

同一 IP 在 `cooldown_ms`（默认 5s）内再次启动边沿会被静默丢弃。未知 IP 不会进入播报路径（仅 DEBUG 日志）。

调试时可另开终端用 `udp_mock/udp_listener.py` 监听端口，确认报文格式。

## 配置

默认配置文件：`config/voice.json`（UDP 绑定、设备 IP → 名称 → 音频路径、队列冷却等）。
