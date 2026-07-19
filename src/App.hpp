#pragma once

#include "core/TelemetryPipeline.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace blender_ui_demo {

class App {
 public:
  int run();

 private:
  enum class Page {
    Overview,
    Performance,
    Hardware,
    Storage,
    Network,
    Processes,
    Diagnostics,
  };

  enum class Language {
    SimplifiedChinese,
    English,
  };

  enum class ModuleSlot : std::size_t {
    Events,
    Sampling,
    UiComposition,
    Rendering,
    BufferSwap,
    FrameThrottle,
    Count,
  };

  struct ModuleStat {
    const char* name_zh = "";
    const char* name_en = "";
    const char* description_zh = "";
    const char* description_en = "";
    double last_ms = 0.0;
    double average_ms = 0.0;
    double budget_ms = 0.0;
  };

  static constexpr std::size_t kHistoryCapacity = 120;

  void apply_blender_theme();
  void rebuild_ui_scale(bool recreate_font_texture);
  void initialize_modules();
  void record_module(ModuleSlot slot, double milliseconds);
  void update_metrics(bool force = false);
  void push_history_sample();
  void limit_frame_rate(std::chrono::steady_clock::time_point frame_start);

  void draw_workspace();
  void draw_menu_bar();
  void draw_navigation();
  void draw_page_header(const char* eyebrow, const char* title, const char* subtitle);
  void draw_overview();
  void draw_performance();
  void draw_hardware();
  void draw_storage();
  void draw_network();
  void draw_processes();
  void draw_diagnostics();
  void draw_inspector();
  void draw_status_bar();
  void draw_about_dialog();

  [[nodiscard]] const char* text(const char* chinese, const char* english) const noexcept;
  [[nodiscard]] bool is_chinese() const noexcept;
  [[nodiscard]] std::vector<std::string> diagnose_cpu_usage() const;
  [[nodiscard]] double total_receive_rate() const;
  [[nodiscard]] double total_send_rate() const;
  [[nodiscard]] const char* current_page_name() const;

  TelemetryPipeline monitor_;
  DynamicSystemInfo dynamic_info_;

  std::array<float, kHistoryCapacity> cpu_history_{};
  std::array<float, kHistoryCapacity> memory_history_{};
  std::array<float, kHistoryCapacity> app_cpu_history_{};
  std::array<float, kHistoryCapacity> frame_time_history_{};
  int history_count_ = 0;
  int history_write_index_ = 0;
  int history_offset_ = 0;

  std::array<ModuleStat, static_cast<std::size_t>(ModuleSlot::Count)> modules_{};
  std::chrono::steady_clock::time_point last_fast_sample_{};
  std::chrono::steady_clock::time_point last_slow_sample_{};

  Page page_ = Page::Overview;
  Language language_ = Language::SimplifiedChinese;
  bool request_exit_ = false;
  bool show_about_ = false;
  bool show_imgui_demo_ = false;
  bool adaptive_throttle_ = true;
  bool vsync_active_ = false;
  bool window_focused_ = true;
  bool window_minimized_ = false;
  bool pending_ui_rebuild_ = false;
  bool cjk_font_loaded_ = false;

  int target_fps_ = 30;
  int sampling_interval_ms_ = 1000;
  int slow_refresh_interval_ms_ = 5000;
  double current_fps_ = 0.0;
  double current_frame_ms_ = 0.0;

  float system_scale_ = 1.0F;
  float user_scale_ = 1.0F;
  float requested_user_scale_ = 1.0F;
  float effective_scale_ = 1.0F;
  std::string font_source_ = "Dear ImGui default";
};

}  // namespace blender_ui_demo
