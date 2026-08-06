# voice-center 设计规格（Demo 第一期）

日期：2026-08-06  
状态：已确认，待实现计划

## 1. 目标

在 Windows 上实现一个最小可用的工业语音播报 Demo：

- 监听 UDP 设备状态报文（与 `udp_mock` 联调）
- 检测设备 `run` 从 `0 → 1` 的启动边沿
- 按 FIFO 排队播放对应 WAV 语音
- **只需普通日志打印**（无状态面板、无前端 UI）；启动时必打「收到启动信号」

技术选型：**C++17 + CMake + SDL2（播放）**。

## 2. 非目标（第一期不做）

- 控制台状态面板 / 清屏刷新 / 前端 GUI
- 停止 / 报警播报
- 优先级抢占、打断当前播报
- Linux / Jetson 播放后端（目录可预留，实现不做）
- MP3、数据库、HTTP 管理界面
- 直接绑定 WASAPI / ALSA 等系统 API

## 3. 架构

单进程、主循环驱动：

```
udp_mock :9999
      │  JSON UDP
      ▼
 UdpListener ──► StateTracker ──► EventQueue ──► VoiceManager ──► SdlPlayer
                      │                              │
                      └──── 日志：收到启动信号 ────────┘── 播放 / 未找到音频
```

职责边界：

| 模块 | 职责 | 不负责 |
|------|------|--------|
| UdpListener | 收 UDP、解析 JSON | 状态机、播放 |
| StateTracker | 按设备记上次 `run`，产生启动事件 | 排队、音频 |
| EventQueue | FIFO + 同设备冷却去重 | 音频 I/O |
| VoiceManager | 打启动日志；事件 → 音频路径；驱动播放 | 平台音频细节 |
| SdlPlayer | 同步播完一个 WAV | UDP / 状态 |

## 4. 目录结构

```
voice-center/
├── CMakeLists.txt
├── config/
│   └── voice.json
├── assets/
│   └── audio/                 # AI 生成的 wav，由用户后续放入
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── UdpListener.*
│   │   ├── StateTracker.*
│   │   ├── EventQueue.*
│   │   └── VoiceManager.*
│   └── platform/
│       └── windows/
│           └── SdlPlayer.*
├── udp_mock/                  # Python 广播/监听，联调用
│   ├── udp_broadcast.py
│   ├── udp_listener.py
│   └── yu_device_data.csv
└── docs/
    └── superpowers/
        └── specs/
            └── 2026-08-06-voice-center-design.md
```

## 5. 设备标识

- **主键：`ip`**（状态记忆、冷却、配置查找均用 IP）
- **`name`**：展示与日志辅助；若报文 `name` 与配置不一致，打 warning，仍按 `ip` 处理
- 当前 CSV 中三者一对一：

| IP | name |
|----|------|
| 192.168.1.10 | 8线灌装机 |
| 192.168.1.20 | 8线旋盖机 |
| 192.168.1.50 | 8线贴标机 |

## 6. 配置格式

`config/voice.json`：

```json
{
  "udp": {
    "bind": "0.0.0.0",
    "port": 9999
  },
  "queue": {
    "cooldown_ms": 5000
  },
  "devices": [
    {
      "ip": "192.168.1.10",
      "name": "8线灌装机",
      "start_audio": "assets/audio/filling_start.wav"
    },
    {
      "ip": "192.168.1.20",
      "name": "8线旋盖机",
      "start_audio": "assets/audio/capping_start.wav"
    },
    {
      "ip": "192.168.1.50",
      "name": "8线贴标机",
      "start_audio": "assets/audio/labeling_start.wav"
    }
  ]
}
```

## 7. 触发与排队规则

| 规则 | 行为 |
|------|------|
| 启动边沿 | 某 `ip` 的 `run` 从 `0` → `1` 产生 `device_start` 事件 |
| 首次见到 | 进程启动后第一次见到该 `ip`，即使 `run=1` 也不播、也不打启动信号日志（避免启动误报） |
| 未知 IP | 不在配置中 → 忽略（可 info 日志） |
| 冷却去重 | 同一 `ip` 在 `cooldown_ms` 内再次启动边沿 → 丢弃（不入队、不播报、不重复打「收到启动信号」） |
| 排队 | FIFO，一条播完再播下一条 |
| 缺音频 / 播放失败 | **必须先有「收到启动信号」日志**，再打印缺文件/失败原因，跳过该条，不卡死队列 |

### 7.1 启动事件日志顺序（硬性要求）

每次有效的 `run 0→1`（通过首次见到过滤，且通过冷却允许入队）时，**只追加打印日志，不清屏**：

1. **收到启动信号**（必有）  
   示例：`[INFO] 收到启动信号 ip=192.168.1.20 name=8线旋盖机`
2. 随后二选一：
   - 找到音频：`[INFO] 播放语音 path=assets/audio/capping_start.wav`（`--dry-run` 则改为「将播放语音」）
   - 未找到音频：`[WARN] 未找到音频文件 path=assets/audio/capping_start.wav`

说明：「收到启动信号」与是否有 wav **解耦**——没有音频也不能省略第 1 条。

## 8. 错误处理

| 情况 | 行为 |
|------|------|
| UDP 端口占用 | 启动失败，非 0 退出，日志说明端口 |
| 非 JSON / 缺 `ip`/`run` | 丢弃，debug 日志，进程不退出 |
| 配置文件缺失或无效 | 启动失败 |
| wav 缺失或 SDL 失败 | **先打「收到启动信号」**，再打「未找到音频文件」或播放失败原因，跳过该条，继续队列 |
| Ctrl+C | 尽量播完当前一句后退出 |

第一期日志：控制台 `stdout` + 时间戳 + 级别，不上重型日志库，不做状态面板。

## 9. 测试计划

1. 启动 `udp_mock/udp_broadcast.py`，再启动 `voice-center`，看日志与播报。
2. `--dry-run`：无音箱也可验证边沿、冷却、队列与「收到启动信号」。
3. 同一 IP 在冷却窗口内多次 `0→1`，应只处理一次。
4. 故意缺少 wav，确认仍有「收到启动信号」+「未找到音频文件」。
5. 用户将 AI 生成的三份 wav 放入 `assets/audio/` 后做有声演示。

## 10. 成功标准

- Windows 下可编译运行
- 能消费 `udp_mock` 的 JSON 广播
- `run 0→1` 时 **必有**「收到启动信号」日志；有 wav 则播放（或 dry-run「将播放」），无 wav 则明确「未找到音频文件」
- 同设备冷却去重与 FIFO 排队行为符合第 7 节
- 无清屏、无状态面板、无前端 UI
