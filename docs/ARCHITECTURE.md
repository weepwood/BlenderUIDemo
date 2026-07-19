# Architecture

## 1. Design Positioning

BlenderUIDemo is an independent native desktop demo inspired by Blender's editor-oriented information architecture. It does not embed Blender, link against Blender, or reuse Blender source code and assets.

The project intentionally adopts a smaller equivalent stack:

```text
Operating System
      │
      ▼
SDL3 window / input / OpenGL context
      │
      ▼
Application event loop
      │
      ├───────────────┐
      ▼               ▼
Dear ImGui UI      SystemMonitor
      │               │
      ▼               ▼
OpenGL renderer    Win32 / POSIX APIs
```

## 2. Layer Responsibilities

### Application layer

`App` owns:

- SDL initialization and shutdown
- OpenGL context lifetime
- Dear ImGui initialization and shutdown
- Event polling
- Sampling cadence
- Workspace composition
- Theme configuration

### System data layer

`SystemMonitor` owns:

- Static host information
- Dynamic CPU and memory metrics
- Uptime
- Local disk discovery
- Unit and duration formatting
- Platform-specific conditional compilation

The data layer contains no UI calls. This keeps future collectors usable from tests, command-line tools or another renderer.

## 3. UI Composition

The main window is composed as one editor workspace:

```text
┌─────────────────────────────────────────────────────────┐
│ Menu Bar                                                │
├─────────────┬───────────────────────────┬───────────────┤
│ Navigation  │ Main Editor               │ Properties    │
│             │                           │ Inspector     │
│ Overview    │ Metrics / Graphs / Tables │ Live values   │
│ Hardware    │                           │ Identity      │
│ Storage     │                           │ Display info  │
├─────────────┴───────────────────────────┴───────────────┤
│ Status Bar                                              │
└─────────────────────────────────────────────────────────┘
```

The three workspace columns use an ImGui table with resizable separators. This approximates Blender's split editor regions without implementing a full docking graph.

## 4. Sampling Model

Rendering runs every frame. System sampling runs at most once every 500 milliseconds.

```text
Frame loop
  ├─ poll SDL events
  ├─ sample system metrics when interval elapsed
  ├─ build UI
  ├─ render ImGui draw data with OpenGL
  └─ swap buffers
```

CPU usage is calculated from the delta between two `GetSystemTimes` samples on Windows. The interface never invents random telemetry.

## 5. Windows Data Sources

| Data | API |
|---|---|
| Computer name | `GetComputerNameW` |
| Current user | `GetUserNameW` |
| CPU model | Windows Registry |
| Architecture / thread count | `GetNativeSystemInfo` |
| Windows version | `RtlGetVersion` |
| CPU usage | `GetSystemTimes` |
| Memory | `GlobalMemoryStatusEx` |
| Uptime | `GetTickCount64` |
| Local drives | `GetLogicalDrives`, `GetDiskFreeSpaceExW` |
| File system | `GetVolumeInformationW` |

## 6. Future Extension Points

Recommended additions:

1. GPU adapter information through DXGI.
2. Network adapters and throughput through IP Helper API.
3. Process list and per-process CPU/memory usage.
4. Windows service and startup item inspection.
5. Hardware sensors through an optional LibreHardwareMonitor bridge.
6. Persistent layout and user theme preferences.
7. Docking branch support for detachable editor regions.
8. A command/operator registry inspired by Blender operators.

Sensor data should remain optional because Windows does not expose all temperature and fan metrics through one stable universal API.
