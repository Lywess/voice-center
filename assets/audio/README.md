# 音频资源（AI 生成 WAV）

本目录用于存放设备启动播报用的 WAV 文件。仓库内**不包含**实际音频，请自行生成或录制后放入此处。

## 期望文件名

| 文件名 | 设备（`config/voice.json`） | IP |
|--------|------------------------------|-----|
| `filling_start.wav` | 8线灌装机 | 192.168.1.10 |
| `capping_start.wav` | 8线旋盖机 | 192.168.1.20 |
| `labeling_start.wav` | 8线贴标机 | 192.168.1.50 |

路径须与配置一致（相对仓库根目录）：

```
assets/audio/filling_start.wav
assets/audio/capping_start.wav
assets/audio/labeling_start.wav
```

## 格式建议

- 容器：WAV（PCM）
- 采样率 / 位深：与 SDL2 兼容即可（常见 44100 Hz、16-bit 单声道或立体声）

## 验证

1. 将三个文件放入本目录
2. 在仓库根目录运行（**不要**加 `--dry-run`）：

   ```powershell
   .\build\Release\voice-center.exe
   ```

3. 配合 `udp_mock` 联调时，日志应出现 `将播放语音 path=...`，并听到对应播报

缺文件时程序会打 `未找到音频文件 path=...`，不会崩溃。
