#pragma once

#include <cfloat>

#include <imgui.h>

namespace blender_ui_demo::ui_style {

void apply_frame_style();
void decorate_child(const char* id, ImGuiChildFlags child_flags);
void decorate_selectable(bool selected);
void draw_progress_bar(float fraction, const ImVec2& size_arg, const char* overlay);

}  // namespace blender_ui_demo::ui_style

#ifndef BLENDER_UI_STYLE_IMPLEMENTATION
namespace ImGui {

inline void BlenderWorkspaceNewFrame() {
  NewFrame();
  blender_ui_demo::ui_style::apply_frame_style();
}

inline bool BlenderWorkspaceBeginChild(const char* str_id,
                                       const ImVec2& size_arg = ImVec2{0.0F, 0.0F},
                                       ImGuiChildFlags child_flags = 0,
                                       ImGuiWindowFlags window_flags = 0) {
  const bool visible = BeginChild(str_id, size_arg, child_flags, window_flags);
  if (visible) {
    blender_ui_demo::ui_style::decorate_child(str_id, child_flags);
  }
  return visible;
}

inline bool BlenderWorkspaceBeginChild(ImGuiID id,
                                       const ImVec2& size_arg = ImVec2{0.0F, 0.0F},
                                       ImGuiChildFlags child_flags = 0,
                                       ImGuiWindowFlags window_flags = 0) {
  const bool visible = BeginChild(id, size_arg, child_flags, window_flags);
  if (visible) {
    blender_ui_demo::ui_style::decorate_child(nullptr, child_flags);
  }
  return visible;
}

inline bool BlenderWorkspaceSelectable(const char* label,
                                        bool selected = false,
                                        ImGuiSelectableFlags flags = 0,
                                        const ImVec2& size_arg = ImVec2{0.0F, 0.0F}) {
  const bool activated = Selectable(label, selected, flags, size_arg);
  blender_ui_demo::ui_style::decorate_selectable(selected);
  return activated;
}

inline bool BlenderWorkspaceSelectable(const char* label,
                                        bool* selected,
                                        ImGuiSelectableFlags flags = 0,
                                        const ImVec2& size_arg = ImVec2{0.0F, 0.0F}) {
  const bool activated = Selectable(label, selected, flags, size_arg);
  blender_ui_demo::ui_style::decorate_selectable(selected != nullptr && *selected);
  return activated;
}

inline void BlenderWorkspaceProgressBar(float fraction,
                                        const ImVec2& size_arg = ImVec2{-FLT_MIN, 0.0F},
                                        const char* overlay = nullptr) {
  blender_ui_demo::ui_style::draw_progress_bar(fraction, size_arg, overlay);
}

}  // namespace ImGui

#define NewFrame BlenderWorkspaceNewFrame
#define BeginChild BlenderWorkspaceBeginChild
#define Selectable BlenderWorkspaceSelectable
#define ProgressBar BlenderWorkspaceProgressBar
#endif
