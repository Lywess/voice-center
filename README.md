# voice-center

Windows C++17 工业语音播报 Demo（UDP 监听 + WAV 播放）。

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

## 运行

```powershell
.\build\Release\voice-center.exe
```

配置文件：`config/voice.json`。音频文件放在 `assets/audio/`。
