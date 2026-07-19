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
- Windows GitHub Actions 构建和可直接解压运行的 artifact

## 技术栈

| 模块 | 技术 |
|---|---|
| 应用核心 | C++20 |
| 窗口与输入 | SDL 3.4.8 |
| UI | Dear ImGui 1.91.9 Docking |
| 图形后端 | OpenGL 3.3 |
| Windows 数据 | Win32 API |
| Windows 编译器 | MSVC x64 |
| 构建系统 | CMake 3.24+ |

第三方依赖由 CMake `FetchContent` 自动下载，并固定版本以提高构建可复现性。Windows Release 构建使用静态 MSVC C/C++ 运行库，发布包不依赖 MinGW 的 `libgcc_s_seh-1.dll`、`libstdc++-6.dll`，也不要求单独安装 Visual C++ Redistributable。

## Windows 构建

### 环境

- Windows 10/11 x64
- Visual Studio 2022 或更新版本的 C++ 工具链
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

请在 Visual Studio Developer PowerShell 中运行：

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
cmake --build build --config Release --parallel
```

## 发布包使用

从 GitHub Actions 下载 `BlenderUIDemo-windows-x64`，解压一次后应直接得到：

```text
BlenderUIDemo.exe
SDL3.dll
README.md
LICENSE
```

请保持 `BlenderUIDemo.exe` 与 `SDL3.dll` 位于同一目录，再双击 EXE。

### 缺少 libgcc 或 libstdc++ DLL

如果系统提示缺少：

```text
libgcc_s_seh-1.dll
libstdc++-6.dll
libwinpthread-1.dll
```

说明正在运行旧的 MinGW 构建产物。请删除旧目录，并重新下载最新的 `BlenderUIDemo-windows-x64` artifact。不要从互联网单独下载这些 DLL 放入系统目录。

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

1. 初始化 x64 MSVC 开发环境。
2. 通过 Ninja 构建 Release 版本并静态链接 MSVC 运行库。
3. 使用 `dumpbin /dependents` 审计 EXE 和 SDL3 DLL。
4. 若发现 MinGW 或动态 MSVC 运行库依赖则中止发布。
5. 上传包含 EXE、SDL3 DLL、README 和 License 的可直接解压 artifact。

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
