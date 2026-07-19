# 优化后的项目生成提示词

下面的提示词可用于继续生成、重构或扩展 BlenderUIDemo。

## 完整提示词

请构建一个名为 **BlenderUIDemo** 的原生桌面系统信息应用。该项目用于研究 Blender 的原生桌面界面组织方式，但不得直接复制 Blender 的源代码、商标、Logo、图标或受版权保护的资源。

### 一、产品目标

应用在本地运行，读取并展示当前电脑的基础信息和实时资源使用情况。整体体验接近专业 3D/DCC 软件的工作区，而不是普通网页仪表盘。

应用必须具备：

1. 系统总览：电脑名称、当前用户、操作系统、系统架构、运行时长。
2. CPU：型号、逻辑处理器数量、实时使用率、最近 60 秒趋势。
3. 内存：物理内存总量、已使用、可用、实时使用率、最近 60 秒趋势。
4. 存储：本地磁盘、文件系统、容量、已使用、可用空间和进度条。
5. 手动刷新和 500ms 自动刷新。
6. About 页面说明技术栈及与 Blender 的关系。

### 二、技术栈

使用以下原生技术实现：

- C++20
- CMake 3.24+
- SDL3：窗口、事件、输入、高 DPI 支持和 OpenGL 上下文
- Dear ImGui：GPU 即时模式界面
- OpenGL 3.3：界面绘制后端
- Windows Win32 API：系统信息采集
- Linux/macOS：提供可编译的基础回退实现

禁止使用：

- Electron
- WebView
- HTML/CSS 页面
- React、Vue、Flutter
- Qt、GTK 等大型桌面 UI 框架
- Tailwind CSS
- 远程服务器或数据库

### 三、界面风格

参考 Blender 的信息密度和工作区概念，但保持独立设计：

1. 深灰色中性背景，橙色作为主要强调色。
2. 顶部菜单栏包含 File、View、Help 和实时状态。
3. 左侧导航区域用于切换 Overview、Hardware、Storage。
4. 中央区域作为主要 Editor，展示指标卡、趋势图和数据表。
5. 右侧 Properties 区域展示当前系统的实时属性。
6. 底部状态栏显示采样状态、刷新频率和渲染技术栈。
7. 区域之间允许拖动调整宽度。
8. 使用紧凑间距、弱圆角、细分隔线，不设计成移动端大卡片界面。
9. 不复制 Blender Logo、官方图标、字体和布局像素值。

### 四、代码架构

代码至少拆分为：

```text
src/
├── main.cpp
├── App.hpp
├── App.cpp
├── SystemInfo.hpp
└── SystemInfo.cpp
```

职责：

- `main.cpp`：程序入口。
- `App`：SDL 生命周期、ImGui 生命周期、事件循环、主题和界面布局。
- `SystemMonitor`：静态系统信息、动态采样和跨平台条件编译。
- 数据层不能依赖 ImGui。
- UI 层不能直接散落 Win32 API 调用。

### 五、质量要求

1. 使用 RAII 思维管理资源，并在所有初始化失败路径上正确清理资源。
2. 避免全局可变状态。
3. 所有字节容量统一使用 `std::uint64_t`。
4. CPU 使用率需要基于两次系统时间采样计算，而不是伪造随机数据。
5. UI 刷新和系统采样解耦，系统采样频率为 500ms。
6. Windows 下正确处理 UTF-16 到 UTF-8 的转换。
7. 磁盘列表只展示可读取的本地固定盘和可移动盘。
8. 代码使用 `/W4` 或 `-Wall -Wextra -Wpedantic`。
9. 固定第三方依赖版本，确保构建可复现。
10. 提供 Windows GitHub Actions 构建和 artifact 上传。

### 六、验收标准

执行：

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

应生成可运行的 `BlenderUIDemo`。启动后应满足：

- 窗口可缩放且最小尺寸受限。
- CPU 和内存数据持续更新。
- 页面切换正常。
- 本地磁盘容量与系统实际数据一致。
- 窗口关闭、菜单 Exit 均可安全退出。
- 不依赖浏览器、Node.js、Python 运行时或外部服务。

## 后续扩展提示

在保持现有架构的基础上，增加 GPU、网络、进程、传感器、温度和日志页面。优先调用 Windows 原生 API；无法可靠获取的数据必须明确标注为不可用，禁止使用随机数据模拟真实硬件指标。
