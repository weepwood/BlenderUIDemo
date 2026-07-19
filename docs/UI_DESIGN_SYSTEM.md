# BlenderUIDemo UI 设计系统

## 目标

该设计系统用于统一 BlenderUIDemo 的暗色工作区、卡片、表格、导航和交互状态。它不替代现有页面逻辑，而是在 Dear ImGui 帧开始时集中应用 Token，并对少量通用控件增加装饰。

## 设计原则

1. **高信息密度但不拥挤**：继续保留 Blender 风格的紧凑布局，但增加明确的层级和留白。
2. **暗色背景不等于纯黑**：通过 Workspace、Panel、Card、Input 四级灰阶区分容器。
3. **强调色只用于方向性信息**：橙色用于当前工作区、关键操作和资源进度，不大面积铺色。
4. **状态颜色有稳定语义**：绿色正常、黄色关注、红色异常、蓝色信息。
5. **DPI 由字体尺度驱动**：尺寸 Token 根据当前字体高度推导，避免系统 DPI 与手动缩放重复放大。
6. **低额外开销**：完整 Style 仅在上下文或字体尺度发生变化时重建；普通帧只增加少量 DrawList 命令。

## 颜色层级

| Token | 用途 |
|---|---|
| Workspace | 应用根背景 |
| Panel | 导航、编辑器和属性栏基础面板 |
| Card | 指标卡、趋势卡和详情容器 |
| Card Raised | 选中标签或强调容器 |
| Input | 输入框、按钮和滑块轨道 |
| Hover | 鼠标悬停状态 |
| Active | 按下或编辑状态 |
| Border | 普通分隔线和卡片边框 |
| Border Strong | 表头和重要分隔线 |
| Accent | 当前工作区、进度和焦点 |

## 尺寸体系

参考字体为 16.5px。实际比例：

```text
scale = current_font_size / 16.5
```

以下尺寸均由 `scale` 推导：

- Window / Frame / Cell Padding
- ItemSpacing / ItemInnerSpacing
- Child / Frame / Popup / Tab Rounding
- ScrollbarSize / GrabMinSize
- Border Width
- 导航指示条和卡片内高光

样式不会直接读取 Windows DPI；字体层已经完成系统 DPI 与用户缩放的合成，因此可以避免二次缩放。

## 控件增强

### Navigation Selectable

- 选中项左侧显示细橙色指示条。
- 悬停项显示低透明度底边提示。
- 键盘焦点显示细描边。
- 不改变原有点击范围和页面切换逻辑。

### Child Card

带边框的 Child 会增加：

- 柔和顶部渐变；
- 1px 内高光；
- 与 DPI 一致的圆角和边框；
- Brand 与 Page Header 使用更弱的橙色方向提示。

### Progress Bar

资源进度条改为：

- 紧凑高度；
- 胶囊圆角；
- 状态色填充；
- 保留可选的居中文字。

## 工程实现

```text
src/ui/
├── UiStyleOverride.hpp
└── UiStyleOverride.cpp
```

`UiStyleOverride.hpp` 只对 `App.cpp` 强制包含，将下列公共 Dear ImGui 调用重定向到轻量 Wrapper：

- `NewFrame`
- `BeginChild`
- `Selectable`
- `ProgressBar`

页面代码继续使用标准 ImGui 调用方式，不需要在每个页面重复添加主题代码。

## 性能约束

- Style Token 仅在 ImGui Context、字体大小或样式重置后重新应用。
- 不使用模糊、离屏纹理、图片背景或着色器特效。
- 卡片渐变和导航指示均为少量即时 DrawList 图元。
- UI Composition 平均耗时应继续通过 Diagnostics 观察。

## 后续扩展

- 紧凑 / 标准 / 宽松三种密度预设；
- Blender Dark、Slate、High Contrast 主题预设；
- 色盲安全状态色；
- 可保存的主题配置；
- 统一图标字体与命令面板样式。
