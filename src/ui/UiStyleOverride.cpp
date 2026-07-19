#define BLENDER_UI_STYLE_IMPLEMENTATION
#include "ui/UiStyleOverride.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace blender_ui_demo::ui_style {
namespace {

constexpr float kReferenceFontSize = 16.5F;

constexpr ImVec4 kText{0.925F, 0.937F, 0.953F, 1.0F};
constexpr ImVec4 kTextMuted{0.560F, 0.596F, 0.655F, 1.0F};
constexpr ImVec4 kWorkspace{0.038F, 0.043F, 0.052F, 1.0F};
constexpr ImVec4 kPanel{0.057F, 0.064F, 0.076F, 1.0F};
constexpr ImVec4 kCard{0.072F, 0.081F, 0.096F, 1.0F};
constexpr ImVec4 kCardRaised{0.086F, 0.096F, 0.114F, 1.0F};
constexpr ImVec4 kInput{0.092F, 0.103F, 0.122F, 1.0F};
constexpr ImVec4 kHover{0.132F, 0.146F, 0.171F, 1.0F};
constexpr ImVec4 kActive{0.166F, 0.181F, 0.211F, 1.0F};
constexpr ImVec4 kBorder{0.145F, 0.160F, 0.188F, 1.0F};
constexpr ImVec4 kBorderStrong{0.204F, 0.222F, 0.255F, 1.0F};
constexpr ImVec4 kAccent{0.973F, 0.455F, 0.125F, 1.0F};
constexpr ImVec4 kAccentHover{1.0F, 0.555F, 0.205F, 1.0F};
constexpr ImVec4 kAccentSelected{0.290F, 0.145F, 0.072F, 1.0F};
constexpr ImVec4 kGood{0.300F, 0.790F, 0.535F, 1.0F};
constexpr ImVec4 kWarning{0.965F, 0.710F, 0.250F, 1.0F};
constexpr ImVec4 kDanger{0.945F, 0.350F, 0.340F, 1.0F};
constexpr ImVec4 kInfo{0.345F, 0.665F, 0.985F, 1.0F};

float current_scale() {
  if (ImGui::GetCurrentContext() == nullptr) {
    return 1.0F;
  }
  return std::clamp(ImGui::GetFontSize() / kReferenceFontSize, 0.75F, 4.0F);
}

float scaled(float value) {
  return value * current_scale();
}

ImU32 color_with_alpha(const ImVec4& color, float alpha) {
  ImVec4 adjusted = color;
  adjusted.w *= alpha;
  return ImGui::ColorConvertFloat4ToU32(adjusted);
}

bool id_equals(const char* id, const char* expected) {
  return id != nullptr && std::strcmp(id, expected) == 0;
}

bool is_card_id(const char* id) {
  return id_equals(id, "metric") || id_equals(id, "plot_card") ||
         id_equals(id, "health_summary") || id_equals(id, "frame_pacing") ||
         id_equals(id, "hardware_content") || id_equals(id, "storage_content") ||
         id_equals(id, "network_content") || id_equals(id, "process_content") ||
         id_equals(id, "diagnostics_content");
}

bool is_identity_id(const char* id) {
  return id_equals(id, "brand") || id_equals(id, "page_header");
}

}  // namespace

void apply_frame_style() {
  if (ImGui::GetCurrentContext() == nullptr) {
    return;
  }

  static ImGuiContext* applied_context = nullptr;
  static float applied_font_size = 0.0F;

  ImGuiStyle& style = ImGui::GetStyle();
  const float font_size = ImGui::GetFontSize();
  const float scale = current_scale();
  const float desired_child_rounding = 7.0F * scale;

  const bool style_is_current =
      applied_context == ImGui::GetCurrentContext() &&
      std::abs(applied_font_size - font_size) < 0.01F &&
      std::abs(style.ChildRounding - desired_child_rounding) < 0.05F;
  if (style_is_current) {
    return;
  }

  applied_context = ImGui::GetCurrentContext();
  applied_font_size = font_size;

  style.WindowPadding = ImVec2{11.0F * scale, 10.0F * scale};
  style.FramePadding = ImVec2{10.0F * scale, 6.0F * scale};
  style.CellPadding = ImVec2{10.0F * scale, 7.0F * scale};
  style.ItemSpacing = ImVec2{9.0F * scale, 8.0F * scale};
  style.ItemInnerSpacing = ImVec2{7.0F * scale, 5.0F * scale};
  style.TouchExtraPadding = ImVec2{0.0F, 0.0F};
  style.IndentSpacing = 20.0F * scale;
  style.ScrollbarSize = 13.0F * scale;
  style.GrabMinSize = 9.0F * scale;

  style.WindowRounding = 0.0F;
  style.ChildRounding = desired_child_rounding;
  style.FrameRounding = 5.5F * scale;
  style.PopupRounding = 7.0F * scale;
  style.ScrollbarRounding = 9.0F * scale;
  style.GrabRounding = 9.0F * scale;
  style.TabRounding = 5.0F * scale;

  style.WindowBorderSize = 0.0F;
  style.ChildBorderSize = std::max(1.0F, 0.75F * scale);
  style.PopupBorderSize = std::max(1.0F, 0.75F * scale);
  style.FrameBorderSize = std::max(0.0F, 0.50F * scale);
  style.TabBorderSize = 0.0F;
  style.SeparatorTextBorderSize = std::max(1.0F, 0.65F * scale);
  style.SeparatorTextPadding = ImVec2{8.0F * scale, 4.0F * scale};

  style.DisabledAlpha = 0.55F;
  style.AntiAliasedLines = true;
  style.AntiAliasedLinesUseTex = true;
  style.AntiAliasedFill = true;
  style.CurveTessellationTol = 1.25F * scale;
  style.CircleTessellationMaxError = 0.30F * scale;

  auto& colors = style.Colors;
  colors[ImGuiCol_Text] = kText;
  colors[ImGuiCol_TextDisabled] = kTextMuted;
  colors[ImGuiCol_WindowBg] = kWorkspace;
  colors[ImGuiCol_ChildBg] = kPanel;
  colors[ImGuiCol_PopupBg] = ImVec4{kCard.x, kCard.y, kCard.z, 0.995F};
  colors[ImGuiCol_Border] = kBorder;
  colors[ImGuiCol_BorderShadow] = ImVec4{0.0F, 0.0F, 0.0F, 0.0F};

  colors[ImGuiCol_FrameBg] = kInput;
  colors[ImGuiCol_FrameBgHovered] = kHover;
  colors[ImGuiCol_FrameBgActive] = kActive;

  colors[ImGuiCol_TitleBg] = kPanel;
  colors[ImGuiCol_TitleBgActive] = kCard;
  colors[ImGuiCol_TitleBgCollapsed] = kPanel;
  colors[ImGuiCol_MenuBarBg] = ImVec4{0.050F, 0.056F, 0.067F, 1.0F};

  colors[ImGuiCol_ScrollbarBg] = ImVec4{0.035F, 0.039F, 0.047F, 0.70F};
  colors[ImGuiCol_ScrollbarGrab] = ImVec4{0.230F, 0.248F, 0.286F, 0.90F};
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{0.310F, 0.334F, 0.385F, 1.0F};
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{0.390F, 0.418F, 0.480F, 1.0F};

  colors[ImGuiCol_CheckMark] = kAccent;
  colors[ImGuiCol_SliderGrab] = kAccent;
  colors[ImGuiCol_SliderGrabActive] = kAccentHover;

  colors[ImGuiCol_Button] = kInput;
  colors[ImGuiCol_ButtonHovered] = kHover;
  colors[ImGuiCol_ButtonActive] = kActive;

  colors[ImGuiCol_Header] = ImVec4{0.105F, 0.116F, 0.136F, 1.0F};
  colors[ImGuiCol_HeaderHovered] = kHover;
  colors[ImGuiCol_HeaderActive] = kAccentSelected;

  colors[ImGuiCol_Separator] = kBorder;
  colors[ImGuiCol_SeparatorHovered] = ImVec4{kAccent.x, kAccent.y, kAccent.z, 0.72F};
  colors[ImGuiCol_SeparatorActive] = kAccent;

  colors[ImGuiCol_ResizeGrip] = ImVec4{0.30F, 0.32F, 0.37F, 0.18F};
  colors[ImGuiCol_ResizeGripHovered] = ImVec4{kAccent.x, kAccent.y, kAccent.z, 0.75F};
  colors[ImGuiCol_ResizeGripActive] = kAccent;

  colors[ImGuiCol_Tab] = ImVec4{0.075F, 0.083F, 0.099F, 1.0F};
  colors[ImGuiCol_TabHovered] = kHover;
  colors[ImGuiCol_TabSelected] = kCardRaised;
  colors[ImGuiCol_TabDimmed] = kPanel;
  colors[ImGuiCol_TabDimmedSelected] = kCard;

  colors[ImGuiCol_PlotLines] = kAccent;
  colors[ImGuiCol_PlotLinesHovered] = kAccentHover;
  colors[ImGuiCol_PlotHistogram] = kAccent;
  colors[ImGuiCol_PlotHistogramHovered] = kAccentHover;

  colors[ImGuiCol_TableHeaderBg] = ImVec4{0.085F, 0.095F, 0.113F, 1.0F};
  colors[ImGuiCol_TableBorderStrong] = kBorderStrong;
  colors[ImGuiCol_TableBorderLight] = kBorder;
  colors[ImGuiCol_TableRowBg] = ImVec4{0.0F, 0.0F, 0.0F, 0.0F};
  colors[ImGuiCol_TableRowBgAlt] = ImVec4{0.090F, 0.101F, 0.120F, 0.46F};

  colors[ImGuiCol_TextSelectedBg] = ImVec4{kAccent.x, kAccent.y, kAccent.z, 0.30F};
  colors[ImGuiCol_DragDropTarget] = kWarning;
  colors[ImGuiCol_NavCursor] = kAccent;
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4{1.0F, 1.0F, 1.0F, 0.68F};
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4{0.020F, 0.025F, 0.032F, 0.78F};
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4{0.015F, 0.018F, 0.024F, 0.76F};

  (void)kGood;
  (void)kWarning;
  (void)kDanger;
  (void)kInfo;
}

void decorate_child(const char* id, ImGuiChildFlags child_flags) {
  if (ImGui::GetCurrentContext() == nullptr) {
    return;
  }

  const bool bordered = (child_flags & ImGuiChildFlags_Borders) != 0;
  if (!bordered && !is_identity_id(id)) {
    return;
  }

  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  const ImVec2 position = ImGui::GetWindowPos();
  const ImVec2 size = ImGui::GetWindowSize();
  const float scale = current_scale();
  const float inset = std::max(1.0F, 0.75F * scale);
  const float gradient_height = std::min(size.y - inset, 34.0F * scale);

  if (size.x <= inset * 2.0F || size.y <= inset * 2.0F || gradient_height <= inset) {
    return;
  }

  const ImVec2 top_left{position.x + inset, position.y + inset};
  const ImVec2 bottom_right{position.x + size.x - inset,
                            position.y + gradient_height};

  const float top_alpha = is_identity_id(id) ? 0.18F : (is_card_id(id) ? 0.095F : 0.055F);
  const ImU32 top_left_color = color_with_alpha(is_identity_id(id) ? kAccent : kText, top_alpha);
  const ImU32 top_right_color = color_with_alpha(kText, top_alpha * 0.30F);
  const ImU32 bottom_color = color_with_alpha(kText, 0.0F);

  draw_list->AddRectFilledMultiColor(top_left,
                                     bottom_right,
                                     top_left_color,
                                     top_right_color,
                                     bottom_color,
                                     bottom_color);

  draw_list->AddLine(ImVec2{position.x + 1.5F * scale, position.y + inset},
                     ImVec2{position.x + size.x - 1.5F * scale, position.y + inset},
                     color_with_alpha(kText, is_identity_id(id) ? 0.16F : 0.075F),
                     std::max(1.0F, 0.65F * scale));

  if (is_identity_id(id)) {
    draw_list->AddRectFilled(ImVec2{position.x + inset, position.y + inset},
                             ImVec2{position.x + 3.5F * scale,
                                    position.y + size.y - inset},
                             color_with_alpha(kAccent, 0.92F),
                             1.5F * scale);
  }
}

void decorate_selectable(bool selected) {
  if (ImGui::GetCurrentContext() == nullptr || !ImGui::IsItemVisible()) {
    return;
  }

  const ImVec2 minimum = ImGui::GetItemRectMin();
  const ImVec2 maximum = ImGui::GetItemRectMax();
  const float scale = current_scale();
  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  if (selected) {
    const float vertical_padding = std::max(3.0F * scale, (maximum.y - minimum.y) * 0.18F);
    draw_list->AddRectFilled(ImVec2{minimum.x + 1.0F * scale, minimum.y + vertical_padding},
                             ImVec2{minimum.x + 4.0F * scale, maximum.y - vertical_padding},
                             ImGui::ColorConvertFloat4ToU32(kAccent),
                             2.0F * scale);
  } else if (ImGui::IsItemHovered()) {
    draw_list->AddLine(ImVec2{minimum.x + 1.0F * scale, maximum.y - 1.0F * scale},
                       ImVec2{maximum.x - 3.0F * scale, maximum.y - 1.0F * scale},
                       color_with_alpha(kAccent, 0.24F),
                       std::max(1.0F, 0.55F * scale));
  }

  if (ImGui::IsItemFocused()) {
    draw_list->AddRect(minimum,
                       maximum,
                       color_with_alpha(kAccent, 0.72F),
                       4.0F * scale,
                       0,
                       std::max(1.0F, 0.70F * scale));
  }
}

void draw_progress_bar(float fraction, const ImVec2& size_arg, const char* overlay) {
  if (ImGui::GetCurrentContext() == nullptr) {
    return;
  }

  const float scale = current_scale();
  const ImVec2 available = ImGui::GetContentRegionAvail();
  const float width = size_arg.x < 0.0F
                          ? std::max(1.0F, available.x + size_arg.x)
                          : (size_arg.x > 0.0F ? size_arg.x : available.x);
  const float height = size_arg.y > 0.0F
                           ? size_arg.y
                           : std::max(6.0F * scale, ImGui::GetFrameHeight() * 0.34F);
  const ImVec2 size{width, height};
  const ImVec2 position = ImGui::GetCursorScreenPos();

  ImGui::Dummy(size);

  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  const float rounding = height * 0.50F;
  const float clamped = std::clamp(fraction, 0.0F, 1.0F);
  const ImU32 background = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
  const ImU32 foreground = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram));

  draw_list->AddRectFilled(position,
                           ImVec2{position.x + size.x, position.y + size.y},
                           background,
                           rounding);

  if (clamped > 0.0F) {
    const float fill_width = std::max(height, size.x * clamped);
    draw_list->AddRectFilled(position,
                             ImVec2{std::min(position.x + size.x, position.x + fill_width),
                                    position.y + size.y},
                             foreground,
                             rounding);
  }

  char generated_overlay[32]{};
  const char* display_overlay = overlay;
  if (display_overlay == nullptr) {
    std::snprintf(generated_overlay,
                  sizeof(generated_overlay),
                  "%.0f%%",
                  static_cast<double>(clamped * 100.0F));
    display_overlay = generated_overlay;
  }

  if (display_overlay[0] != '\0') {
    const ImVec2 text_size = ImGui::CalcTextSize(display_overlay);
    if (text_size.x + 8.0F * scale < size.x && text_size.y <= size.y + 2.0F * scale) {
      draw_list->AddText(ImVec2{position.x + (size.x - text_size.x) * 0.50F,
                                position.y + (size.y - text_size.y) * 0.50F},
                         ImGui::GetColorU32(ImGuiCol_Text),
                         display_overlay);
    }
  }
}

}  // namespace blender_ui_demo::ui_style
