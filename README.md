# voice-center

Windows C++17 工业语音播报 Demo（UDP 监听 + WAV 播放）。

## 构建

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## 运行

```powershell
.\build\Release\voice-center.exe
```

配置文件：`config/voice.json`。音频文件放入 `assets/audio/`。
