# Provider Registry 与 Snapshot Pipeline

## 目标

本阶段把系统采集从 Dear ImGui 主循环中移出，同时保持现有页面、指标与交互行为不变。

新的数据流为：

```text
Dear ImGui UI
    │
    │ request_refresh()
    ▼
TelemetryPipeline
    │
    │ background worker
    ▼
ProviderRegistry
    │
    ▼
IDataProvider
    │
    ▼
SystemSnapshot
    │
    ▼
SnapshotStore ──► shared_ptr<const SystemSnapshot>
```

## 核心组件

### IDataProvider

统一 Provider 的基本契约：

- 描述信息
- 刷新类型
- 权限要求
- 静态系统信息
- 动态数据采集

首个版本注册 `LegacySystemProvider`，它封装原有 `SystemMonitor`。这样可以先建立线程和快照边界，再逐步把 CPU、内存、GPU、磁盘、网络和进程拆成独立 Provider。

### ProviderRegistry

负责：

- 注册 Provider
- 执行采集
- 捕获 Provider 异常
- 记录最近耗时、完成次数、状态与错误

任何 Provider 异常都不会越过 Registry 终止 UI。

### SystemSnapshot

每份快照包含：

- 单调递增序号
- 墙钟采集时间
- 单调时钟发布时间
- 完整动态系统数据
- Provider 运行状态

快照发布后不可修改。

### SnapshotStore

`SnapshotStore` 使用互斥锁保护一个 `shared_ptr<const SystemSnapshot>`。后台线程只发布完整快照，UI 线程只取得当前指针，因此不会看到半更新状态。

### TelemetryPipeline

`TelemetryPipeline` 是现有 UI 的兼容外观：

- `static_info()` 保持原接口
- `sample()` 不再执行 Win32 查询
- `sample()` 只提交刷新请求并返回最新不可变快照的数据副本
- Worker 合并重复请求
- 慢速刷新请求不会阻塞渲染线程
- 析构时确定性停止并 `join` Worker

## 当前阶段的兼容策略

现有 `App.cpp` 不需要一次性重写。`App.hpp` 中的 `monitor_` 已从 `SystemMonitor` 替换为 `TelemetryPipeline`，因此原来的：

```cpp
dynamic_info_ = monitor_.sample(slow_due);
```

现在成为非阻塞快照读取。首次后台快照完成前会返回空的默认数据；之后始终保留最近一份有效快照。

## 下一步

1. 在 Diagnostics 页面显示快照序号、年龄、Worker 状态和 Provider 状态。
2. 将 UI 直接切换为 `shared_ptr<const SystemSnapshot>`，删除兼容数据副本。
3. 将 `LegacySystemProvider` 拆为 CPU、Memory、GPU、Disk、Network 和 Process Provider。
4. 为 Provider Registry、Snapshot Store 和 Pipeline 增加 Mock 与单元测试。
5. 增加可配置采样调度器，让 Worker 独立管理快速和慢速周期。
