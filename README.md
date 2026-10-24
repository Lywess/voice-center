# voice-center

Windows C++17 工业语音播报 Demo（UDP 监听 + WAV 播放）。

监听设备 JSON UDP，检测 `run` 从 `0 → 1` 的启动边沿，按 FIFO 排队播报对应 WAV；控制台只追加日志（无状态面板 / 不清屏）。

## 协作约束（必读 · 人类与 AI 协作者均适用）

> 本节的优先级高于任何 agent 的默认行为。**违反分支纪律的改动会被直接拒绝合并。**
>
> 规则按角色分级：仓库主人的助理 agent 继承其部分权限，外部协作者与陌生 agent 则受限。

### 角色与权限

| 角色 | 权限范围 |
|---|---|
| 仓库主人 `Lywess` | 全部权限，包括 `main` |
| **主人的助理 agent**（由主人直接指派的协作 agent） | 可推送 `develop`、可将已完成的分支合入 `develop`、可管理自己创建的分支。**`main` 仍须主人逐次明确指令** |
| 外部协作者 / 其他 agent | 只能推送自己的工作分支并提交 PR，合并权不在自己手上 |

用户身份以会话上下文为准。若无法确认自己是主人的助理，按「外部协作者」对待 —— 权限不足时可以问，不要假定自己有。

### 分支模型

| 分支 | 角色 | 谁可以动 |
|---|---|---|
| `main` | 主分支，稳定发布线 | **仅仓库主人明确指令**，助理 agent 也不得自行操作 |
| `develop` | 集成分支，日常开发成果汇入这里 | 主人与其助理 agent 可推送合并；外部贡献经 PR 由主人合并 |
| `<type>/<slug>` | 个人工作分支 | 创建者自己 |

### 工作流

1. **从 `develop` 开始**，不要在 `main` 上开工，也不要在别人的分支上接着改：

   ```bash
   git fetch origin
   git switch develop
   git pull --ff-only
   git switch -c feat/your-feature
   ```

2. 分支命名：`feat/<slug>`、`fix/<slug>`、`docs/<slug>`、`chore/<slug>`
3. 只在自己的分支上提交，提交信息用 `feat:` / `fix:` / `docs:` / `chore:` 前缀
4. 完成后推送自己的分支
5. 合入 `develop`：助理 agent 可直接合并自己的分支；外部贡献发 PR，由主人审核合并

### 绝对禁止（所有角色，含助理 agent）

- ❌ 未经主人逐次明确指令，对 `main` 做任何推送或合并
- ❌ 对共享分支执行 `git push --force`
- ❌ 删除 `main`、`develop` 或他人创建的分支
- ❌ 改写已推送的历史（rebase / amend 已推送的提交）
- ❌ 把未经验证的改动合入 `develop`，然后声称「应该没问题」

### 给 AI 协作者

- 开始任何代码工作前，先确认自己在正确的基础分支上：`git branch --show-current`
- 删除、强推、动 `main` 这类不可逆或高风险操作，**先问仓库主人**
- 声称「完成」「通过」之前，必须实际跑验证命令并贴出真实输出
- 本项目是 Windows/MSVC 工程，构建与运行只能发生在 Windows 上；Linux 容器里只能做静态阅读和代码走查，不得据此断言构建通过

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

构建成功后，`build\Release\` 会自动带上：

```text
build\Release\
  voice-center.exe
  SDL2.dll
  config\voice.json
  assets\audio\...
```

双击 exe 或从任意目录运行都可以；程序优先读 **exe 同级** 的 `config/voice.json`，并把工作目录切到 exe 所在目录（音频相对路径才能对上）。

把整个 `Release` 文件夹（或至少以上文件）打包给用户即可。

若中文日志乱码：请用 **Windows Terminal** 或先执行 `chcp 65001`。

```powershell
# 真实播放（需 assets/audio/*.wav + SDL2）
.\build\Release\voice-center.exe

# 无音箱验证
.\build\Release\voice-center.exe --dry-run

# 指定配置
.\build\Release\voice-center.exe --config config\voice.json --dry-run
```

Ctrl+C 停止；启动失败会暂停等待回车，避免双击闪退。

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
