# BlenderUIDemo

一个受 Blender 编辑器布局启发的原生 Windows 系统信息与运行时诊断应用。项目使用 C++、SDL3、OpenGL 和 Dear ImGui 构建，不包含 WebView、Electron 或前端网页运行时。

> 本项目是独立的界面研究 Demo，不属于 Blender Foundation，也未复制 Blender 源码、Logo、图标或官方资源。

## 当前功能

### Blender 风格工作区

- 顶部菜单栏、分组式左侧导航、中央 Editor、右侧 Properties 和底部状态栏
- 深灰中性色、橙色强调线、高信息密度表格与紧凑指标卡
- 三栏宽度可拖动调整
- Overview、Performance、Hardware、Storage、Network、Processes、Diagnostics 七个工作区
- 性能策略可直接在右侧 Properties 中调整

### 系统与硬件

- CPU 型号、架构、逻辑处理器数量和实时系统使用率
- 物理内存总量、已用、可用和实时使用率
- Windows 版本、计算机名、当前用户和运行时长
- DXGI 显卡名称和专用显存
- 本地固定盘、可移动盘、文件系统、容量与使用率

### 网络与进程

- 网络适配器连接状态、描述和 IPv4 地址
- 聚合与单适配器实时上下行速率
- 进程总数
- 按工作集内存排序的前 15 个进程
- PID、线程数量与工作集内存

### 应用自诊断

- BlenderUIDemo 自身 CPU 使用率
- 工作集内存与私有内存
- 当前 FPS 与帧时间
- SDL 事件、系统采集、UI 组合、OpenGL 绘制、缓冲交换、限帧等待等模块耗时
- CPU 高占用原因的动态判断
- 快速采集与慢速采集的耗时拆分
- 在 Diagnostics 页面展示已经采用的优化措施

## CPU 占用优化

旧版本的主要问题不是单个 Win32 API，而是渲染与采集策略叠加：

1. 主循环持续重建并绘制完整 Dear ImGui 界面。
2. 显卡驱动拒绝 VSync 时，循环可能以不受限制的帧率运行。
3. 每 500ms 重新枚举全部磁盘。
4. 图表每一帧都从 `deque` 复制到临时 `vector`。
5. 窗口失焦或最小化后没有主动降频。

当前版本已经调整为：

- 默认前台 30 FPS，可选 15、30、60 FPS
- 窗口失焦后自动降至 10 FPS
- 窗口最小化后每轮休眠 120ms
- 检测 VSync 是否实际启用，并始终保留软件限帧兜底
- CPU、内存、网络和自身指标默认每 1000ms 采集
- 磁盘与进程清单默认每 5000ms 采集并缓存
- 图表改用固定大小环形缓冲区，避免逐帧分配与复制
- 在 Diagnostics 中显示各模块真实耗时，而不是凭经验猜测

详细说明见 [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md)。

## 技术栈

| 模块 | 技术 |
|---|---|
| 应用核心 | C++20 |
| 窗口、输入、帧控制 | SDL 3.4.8 |
| UI | Dear ImGui 1.91.9 Docking |
| 图形后端 | OpenGL 3.3 |
| 系统信息 | Win32 API |
| GPU 信息 | DXGI |
| 网络信息 | IP Helper API |
| 进程信息 | Tool Help + PSAPI |
| Windows 编译器 | MSVC x64 |
| 构建系统 | CMake 3.24+ |

第三方依赖由 CMake `FetchContent` 自动下载并固定版本。Windows Release 使用静态 MSVC C/C++ 运行库，不依赖 MinGW 的 `libgcc_s_seh-1.dll`、`libstdc++-6.dll`，也不要求单独安装 Visual C++ Redistributable。

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

从 GitHub Actions 下载 `BlenderUIDemo-windows-x64`，解压后应直接得到：

```text
BlenderUIDemo.exe
SDL3.dll
README.md
LICENSE
```

保持 `BlenderUIDemo.exe` 与 `SDL3.dll` 位于同一目录，再双击 EXE。

若系统仍提示缺少 `libgcc_s_seh-1.dll`、`libstdc++-6.dll` 或 `libwinpthread-1.dll`，说明运行的是旧版 MinGW 产物。请删除旧目录并重新下载最新 Artifact，不要从第三方网站单独下载 DLL 放入系统目录。

## 项目结构

```text
BlenderUIDemo/
├── .github/workflows/build-windows.yml
├── docs/
│   ├── ARCHITECTURE.md
│   ├── PERFORMANCE.md
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
Windows
  │
  ├── Win32 / DXGI / IP Helper / PSAPI
  │                    │
  │                    ▼
  │              SystemMonitor
  │                    │
  ▼                    ▼
SDL3 event loop ──► Runtime telemetry
  │                    │
  ├── frame policy     │
  ├── module profiler  │
  ▼                    ▼
Dear ImGui UI ─────► Diagnostics workspace
  │
  ▼
OpenGL renderer
```

系统采集层与 UI 层保持分离：`SystemMonitor` 不依赖 ImGui，`App` 不直接散落 Win32 采集调用。

## GitHub Actions

工作流会在 Pull Request 和 `main` 分支提交时：

1. 初始化 x64 MSVC 开发环境。
2. 通过 Ninja 构建 Release 并静态链接 MSVC 运行库。
3. 使用 `dumpbin /dependents` 审计 EXE 和 SDL3 DLL。
4. 若发现 MinGW 或动态 MSVC 运行库依赖则中止发布。
5. 上传包含 EXE、SDL3 DLL、README 和 License 的可直接解压 Artifact。

## 后续规划

- Windows 服务与启动项视图
- GPU 实时使用率与显存使用量
- 可保存的布局、主题和性能策略
- Operator/Command 命令注册机制
- 可选 LibreHardwareMonitor 传感器桥接
- ETW 高精度进程 CPU、磁盘和网络分析

## License

MIT
