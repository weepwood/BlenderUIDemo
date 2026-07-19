# BlenderUIDemo

一个受 Blender 编辑器布局启发的原生系统信息应用。项目使用 C++、SDL3、OpenGL 和 Dear ImGui 构建，不包含 WebView、Electron 或前端网页运行时。

> 本项目是独立的界面研究 Demo，不属于 Blender Foundation，也未复制 Blender 源码、Logo、图标或官方资源。

## 功能

- Blender 风格的深色、高信息密度工作区
- 顶部菜单栏、左侧导航、中央编辑器、右侧属性面板和底部状态栏
- 可拖动调整三栏区域宽度
- CPU 型号、逻辑处理器数量和实时使用率
- 内存总量、已用、可用和实时使用率
- 最近 60 秒 CPU、内存趋势
- Windows 版本、计算机名、当前用户、系统架构和运行时长
- 本地固定盘与可移动盘容量、文件系统和使用率
- 500ms 自动采样与手动刷新
- Windows GitHub Actions 构建和 ZIP artifact

## 技术栈

| 模块 | 技术 |
|---|---|
| 应用核心 | C++20 |
| 窗口与输入 | SDL 3.4.8 |
| UI | Dear ImGui 1.91.9 |
| 图形后端 | OpenGL 3.3 |
| Windows 数据 | Win32 API |
| 构建系统 | CMake 3.24+ |

第三方依赖由 CMake `FetchContent` 自动下载，并固定版本以提高构建可复现性。

## Windows 构建

### 环境

- Windows 10/11 x64
- Visual Studio 2022 C++ 工具链
- CMake 3.24+
- Ninja
- Git

### 使用 Preset

```powershell
cmake --preset windows-release
cmake --build --preset windows-release --parallel
```

生成文件通常位于：

```text
build/windows-release/BlenderUIDemo.exe
```

同一目录会自动复制运行所需的 `SDL3.dll`。

### 通用命令

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

## 项目结构

```text
BlenderUIDemo/
├── .github/workflows/build-windows.yml
├── docs/
│   ├── ARCHITECTURE.md
│   └── PROMPT.md
├── src/
│   ├── App.cpp
│   ├── App.hpp
│   ├── SystemInfo.cpp
│   ├── SystemInfo.hpp
│   └── main.cpp
├── CMakeLists.txt
├── CMakePresets.json
└── README.md
```

## 架构

```text
Operating System
      │
      ▼
SDL3 window / events / OpenGL context
      │
      ▼
App event loop
      │
      ├───────────────┐
      ▼               ▼
Dear ImGui UI      SystemMonitor
      │               │
      ▼               ▼
OpenGL renderer    Win32 / POSIX APIs
```

系统采集层与 UI 层分离：`SystemMonitor` 不依赖 ImGui，`App` 不直接散落 Win32 调用。详细说明见 [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)。

## 优化后的生成提示词

完整需求、技术约束、设计规范和验收标准已经整理在 [`docs/PROMPT.md`](docs/PROMPT.md)，可用于继续扩展 GPU、网络、进程、传感器和日志页面。

## GitHub Actions

工作流会在 Pull Request 和 `main` 分支提交时：

1. 使用 Windows Runner 配置 CMake。
2. 通过 Ninja 构建 Release 版本。
3. 打包 EXE、SDL3 DLL、README 和 License。
4. 上传 `BlenderUIDemo-windows-x64.zip` artifact。

## 后续规划

- DXGI GPU 信息与显存状态
- 网络适配器和实时吞吐量
- 进程管理视图
- Windows 服务和启动项
- 可保存的布局与主题
- Operator/Command 命令注册机制
- 可选 LibreHardwareMonitor 传感器桥接

## License

MIT
