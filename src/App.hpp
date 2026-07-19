#pragma once

#include "SystemInfo.hpp"

#include <chrono>
#include <deque>

namespace blender_ui_demo {

class App {
 public:
  int run();

 private:
  enum class Page {
    Overview,
    Hardware,
    Storage,
  };

  void apply_blender_theme();
  void update_metrics(bool force = false);
  void draw_workspace();
  void draw_menu_bar();
  void draw_navigation();
  void draw_overview();
  void draw_hardware();
  void draw_storage();
  void draw_inspector();
  void draw_status_bar();
  void draw_about_dialog();

  SystemMonitor monitor_;
  DynamicSystemInfo dynamic_info_;
  std::deque<float> cpu_history_;
  std::deque<float> memory_history_;
  std::chrono::steady_clock::time_point last_sample_{};
  Page page_ = Page::Overview;
  bool request_exit_ = false;
  bool show_about_ = false;
  bool show_imgui_demo_ = false;
};

}  // namespace blender_ui_demo
