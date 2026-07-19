#include "App.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace blender_ui_demo {
namespace {

using Clock = std::chrono::steady_clock;

constexpr ImVec4 kAccent{0.96F, 0.43F, 0.10F, 1.0F};
constexpr ImVec4 kAccentSoft{0.34F, 0.17F, 0.08F, 1.0F};
constexpr ImVec4 kGood{0.28F, 0.76F, 0.50F, 1.0F};
constexpr ImVec4 kWarning{0.96F, 0.68F, 0.22F, 1.0F};
constexpr ImVec4 kDanger{0.92F, 0.31F, 0.30F, 1.0F};
constexpr ImVec4 kInfo{0.31F, 0.62F, 0.94F, 1.0F};

[[nodiscard]] double elapsed_ms(const Clock::time_point start, const Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

std::string fixed_text(double value, int precision = 1) {
  std::ostringstream stream;
  stream.setf(std::ios::fixed);
  stream.precision(precision);
  stream << value;
  return stream.str();
}

std::string percent_text(double value) {
  return fixed_text(value) + '%';
}

std::string milliseconds_text(double value) {
  return fixed_text(value, value < 1.0 ? 2 : 1) + " ms";
}

ImVec4 usage_color(double value) {
  if (value >= 85.0) {
    return kDanger;
  }
  if (value >= 65.0) {
    return kWarning;
  }
  return kGood;
}

void property_row(const char* label, const std::string& value) {
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::TextDisabled("%s", label);
  ImGui::TableSetColumnIndex(1);
  ImGui::TextWrapped("%s", value.c_str());
}

void section_title(const char* title) {
  ImGui::Spacing();
  ImGui::TextDisabled("%s", title);
  ImGui::Separator();
  ImGui::Spacing();
}

void status_dot(const ImVec4 color, const char* label) {
  ImGui::TextColored(color, "●");
  ImGui::SameLine(0.0F, 5.0F);
  ImGui::TextUnformatted(label);
}

void metric_card(const char* id,
                 const char* title,
                 const std::string& value,
                 const std::string& detail,
                 double progress_percent = -1.0,
                 ImVec4 accent = kAccent) {
  ImGui::PushID(id);
  const ImVec2 size{ImGui::GetContentRegionAvail().x, 118.0F};
  ImGui::BeginChild("metric", size, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
  const ImVec2 window_position = ImGui::GetWindowPos();
  const ImVec2 window_size = ImGui::GetWindowSize();
  ImGui::GetWindowDrawList()->AddRectFilled(window_position,
                                            ImVec2{window_position.x + window_size.x,
                                                   window_position.y + 3.0F},
                                            ImGui::ColorConvertFloat4ToU32(accent));
  ImGui::Dummy(ImVec2{0.0F, 2.0F});
  ImGui::TextDisabled("%s", title);
  ImGui::Spacing();
  ImGui::SetWindowFontScale(1.30F);
  ImGui::TextUnformatted(value.c_str());
  ImGui::SetWindowFontScale(1.0F);
  ImGui::TextDisabled("%s", detail.c_str());
  if (progress_percent >= 0.0) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, usage_color(progress_percent));
    ImGui::ProgressBar(static_cast<float>(progress_percent / 100.0), ImVec2{-1.0F, 7.0F}, "");
    ImGui::PopStyleColor();
  }
  ImGui::EndChild();
  ImGui::PopID();
}

void plot_card(const char* id,
               const char* title,
               const char* subtitle,
               const float* values,
               int value_count,
               int offset,
               float minimum,
               float maximum,
               float height = 170.0F) {
  ImGui::PushID(id);
  ImGui::BeginChild("plot_card", ImVec2{0.0F, height}, ImGuiChildFlags_Borders);
  ImGui::TextUnformatted(title);
  ImGui::TextDisabled("%s", subtitle);
  ImGui::Spacing();
  if (value_count > 0) {
    ImGui::PlotLines("##plot",
                     values,
                     value_count,
                     offset,
                     nullptr,
                     minimum,
                     maximum,
                     ImVec2{-1.0F, height - 64.0F});
  } else {
    ImGui::TextDisabled("Waiting for samples...");
  }
  ImGui::EndChild();
  ImGui::PopID();
}

void table_value(const char* text) {
  ImGui::TableSetColumnIndex(ImGui::TableGetColumnIndex());
  ImGui::TextUnformatted(text);
}

}  // namespace

int App::run() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    std::fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }

#if defined(__APPLE__)
  const char* glsl_version = "#version 150";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
  const char* glsl_version = "#version 330";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  SDL_Window* window = SDL_CreateWindow("Blender UI Demo - System Workspace",
                                         1500,
                                         940,
                                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                             SDL_WINDOW_HIGH_PIXEL_DENSITY);
  if (window == nullptr) {
    std::fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  SDL_SetWindowMinimumSize(window, 1080, 700);

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (gl_context == nullptr) {
    std::fprintf(stderr, "OpenGL context creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_GL_MakeCurrent(window, gl_context);
  vsync_active_ = SDL_GL_SetSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.IniFilename = "blender_ui_demo.ini";

  apply_blender_theme();
  initialize_modules();
  ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  update_metrics(true);

  while (!request_exit_) {
    const auto frame_start = Clock::now();

    const auto events_start = Clock::now();
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_QUIT) {
        request_exit_ = true;
      } else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                 event.window.windowID == SDL_GetWindowID(window)) {
        request_exit_ = true;
      } else if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
        window_focused_ = true;
      } else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        window_focused_ = false;
      } else if (event.type == SDL_EVENT_WINDOW_MINIMIZED) {
        window_minimized_ = true;
      } else if (event.type == SDL_EVENT_WINDOW_RESTORED ||
                 event.type == SDL_EVENT_WINDOW_MAXIMIZED) {
        window_minimized_ = false;
      }
    }
    record_module(ModuleSlot::Events, elapsed_ms(events_start, Clock::now()));

    if (request_exit_) {
      break;
    }

    if (window_minimized_) {
      const auto sleep_start = Clock::now();
      SDL_Delay(120);
      record_module(ModuleSlot::FrameThrottle, elapsed_ms(sleep_start, Clock::now()));
      continue;
    }

    update_metrics();

    const auto ui_start = Clock::now();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    draw_workspace();
    draw_about_dialog();
    if (show_imgui_demo_) {
      ImGui::ShowDemoWindow(&show_imgui_demo_);
    }
    ImGui::Render();
    record_module(ModuleSlot::UiComposition, elapsed_ms(ui_start, Clock::now()));

    const auto render_start = Clock::now();
    int display_width = 0;
    int display_height = 0;
    SDL_GetWindowSizeInPixels(window, &display_width, &display_height);
    glViewport(0, 0, display_width, display_height);
    glClearColor(0.047F, 0.050F, 0.056F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    record_module(ModuleSlot::Rendering, elapsed_ms(render_start, Clock::now()));

    const auto swap_start = Clock::now();
    SDL_GL_SwapWindow(window);
    record_module(ModuleSlot::BufferSwap, elapsed_ms(swap_start, Clock::now()));

    limit_frame_rate(frame_start);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  SDL_GL_DestroyContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

void App::apply_blender_theme() {
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowPadding = ImVec2{10.0F, 9.0F};
  style.FramePadding = ImVec2{9.0F, 6.0F};
  style.CellPadding = ImVec2{8.0F, 7.0F};
  style.ItemSpacing = ImVec2{8.0F, 7.0F};
  style.ItemInnerSpacing = ImVec2{7.0F, 5.0F};
  style.ScrollbarSize = 13.0F;
  style.WindowRounding = 0.0F;
  style.ChildRounding = 5.0F;
  style.FrameRounding = 4.0F;
  style.PopupRounding = 5.0F;
  style.GrabRounding = 4.0F;
  style.TabRounding = 4.0F;
  style.WindowBorderSize = 0.0F;
  style.ChildBorderSize = 1.0F;
  style.FrameBorderSize = 0.0F;

  auto& colors = style.Colors;
  colors[ImGuiCol_Text] = ImVec4{0.90F, 0.91F, 0.93F, 1.0F};
  colors[ImGuiCol_TextDisabled] = ImVec4{0.53F, 0.56F, 0.61F, 1.0F};
  colors[ImGuiCol_WindowBg] = ImVec4{0.070F, 0.074F, 0.082F, 1.0F};
  colors[ImGuiCol_ChildBg] = ImVec4{0.095F, 0.101F, 0.112F, 1.0F};
  colors[ImGuiCol_PopupBg] = ImVec4{0.090F, 0.096F, 0.108F, 0.99F};
  colors[ImGuiCol_Border] = ImVec4{0.18F, 0.19F, 0.22F, 1.0F};
  colors[ImGuiCol_FrameBg] = ImVec4{0.135F, 0.145F, 0.16F, 1.0F};
  colors[ImGuiCol_FrameBgHovered] = ImVec4{0.19F, 0.20F, 0.23F, 1.0F};
  colors[ImGuiCol_FrameBgActive] = ImVec4{0.23F, 0.24F, 0.28F, 1.0F};
  colors[ImGuiCol_TitleBg] = ImVec4{0.065F, 0.070F, 0.078F, 1.0F};
  colors[ImGuiCol_TitleBgActive] = ImVec4{0.085F, 0.090F, 0.102F, 1.0F};
  colors[ImGuiCol_MenuBarBg] = ImVec4{0.085F, 0.090F, 0.102F, 1.0F};
  colors[ImGuiCol_ScrollbarBg] = ImVec4{0.065F, 0.070F, 0.078F, 1.0F};
  colors[ImGuiCol_ScrollbarGrab] = ImVec4{0.24F, 0.25F, 0.29F, 1.0F};
  colors[ImGuiCol_CheckMark] = kAccent;
  colors[ImGuiCol_SliderGrab] = kAccent;
  colors[ImGuiCol_SliderGrabActive] = ImVec4{1.0F, 0.56F, 0.20F, 1.0F};
  colors[ImGuiCol_Button] = ImVec4{0.145F, 0.155F, 0.175F, 1.0F};
  colors[ImGuiCol_ButtonHovered] = ImVec4{0.21F, 0.22F, 0.26F, 1.0F};
  colors[ImGuiCol_ButtonActive] = ImVec4{0.27F, 0.28F, 0.32F, 1.0F};
  colors[ImGuiCol_Header] = ImVec4{0.16F, 0.17F, 0.20F, 1.0F};
  colors[ImGuiCol_HeaderHovered] = ImVec4{0.22F, 0.23F, 0.27F, 1.0F};
  colors[ImGuiCol_HeaderActive] = kAccentSoft;
  colors[ImGuiCol_Separator] = ImVec4{0.17F, 0.18F, 0.21F, 1.0F};
  colors[ImGuiCol_ResizeGrip] = ImVec4{0.35F, 0.36F, 0.40F, 0.35F};
  colors[ImGuiCol_ResizeGripHovered] = kAccent;
  colors[ImGuiCol_Tab] = ImVec4{0.12F, 0.13F, 0.15F, 1.0F};
  colors[ImGuiCol_TabHovered] = ImVec4{0.22F, 0.23F, 0.27F, 1.0F};
  colors[ImGuiCol_TabSelected] = ImVec4{0.19F, 0.20F, 0.23F, 1.0F};
  colors[ImGuiCol_PlotLines] = kAccent;
  colors[ImGuiCol_PlotHistogram] = kAccent;
  colors[ImGuiCol_TableBorderStrong] = ImVec4{0.16F, 0.17F, 0.20F, 1.0F};
  colors[ImGuiCol_TableBorderLight] = ImVec4{0.13F, 0.14F, 0.16F, 1.0F};
  colors[ImGuiCol_TableRowBgAlt] = ImVec4{0.11F, 0.115F, 0.13F, 0.55F};
}

void App::initialize_modules() {
  modules_[static_cast<std::size_t>(ModuleSlot::Events)] =
      {"Platform & Events", "SDL window messages and input dispatch", 0.0, 0.0, 1.0};
  modules_[static_cast<std::size_t>(ModuleSlot::Sampling)] =
      {"System Telemetry", "CPU, memory, network and slow inventory collectors", 0.0, 0.0, 8.0};
  modules_[static_cast<std::size_t>(ModuleSlot::UiComposition)] =
      {"UI Composition", "Dear ImGui layout and draw-list generation", 0.0, 0.0, 5.0};
  modules_[static_cast<std::size_t>(ModuleSlot::Rendering)] =
      {"OpenGL Renderer", "GPU submission for the generated interface", 0.0, 0.0, 5.0};
  modules_[static_cast<std::size_t>(ModuleSlot::BufferSwap)] =
      {"Buffer Swap", "VSync wait and front/back buffer presentation", 0.0, 0.0, 18.0};
  modules_[static_cast<std::size_t>(ModuleSlot::FrameThrottle)] =
      {"Frame Throttle", "CPU sleep used to enforce the selected FPS cap", 0.0, 0.0, 34.0};
}

void App::record_module(ModuleSlot slot, double milliseconds) {
  auto& module = modules_[static_cast<std::size_t>(slot)];
  module.last_ms = milliseconds;
  module.average_ms = module.average_ms <= 0.0 ? milliseconds
                                               : module.average_ms * 0.90 + milliseconds * 0.10;
}

void App::update_metrics(bool force) {
  const auto now = Clock::now();
  const bool fast_due = force || last_fast_sample_.time_since_epoch().count() == 0 ||
                        now - last_fast_sample_ >= std::chrono::milliseconds{sampling_interval_ms_};
  if (!fast_due) {
    return;
  }

  const bool slow_due = force || last_slow_sample_.time_since_epoch().count() == 0 ||
                        now - last_slow_sample_ >=
                            std::chrono::milliseconds{slow_refresh_interval_ms_};
  const auto sampling_start = Clock::now();
  dynamic_info_ = monitor_.sample(slow_due);
  record_module(ModuleSlot::Sampling, elapsed_ms(sampling_start, Clock::now()));
  last_fast_sample_ = now;
  if (slow_due) {
    last_slow_sample_ = now;
  }
  push_history_sample();
}

void App::push_history_sample() {
  cpu_history_[history_write_index_] = static_cast<float>(dynamic_info_.cpu_usage_percent);
  memory_history_[history_write_index_] = static_cast<float>(dynamic_info_.memory_usage_percent);
  app_cpu_history_[history_write_index_] = static_cast<float>(dynamic_info_.application_cpu_percent);
  frame_time_history_[history_write_index_] = static_cast<float>(current_frame_ms_);

  history_write_index_ = (history_write_index_ + 1) % static_cast<int>(kHistoryCapacity);
  if (history_count_ < static_cast<int>(kHistoryCapacity)) {
    ++history_count_;
  }
  history_offset_ = history_count_ == static_cast<int>(kHistoryCapacity) ? history_write_index_ : 0;
}

void App::limit_frame_rate(Clock::time_point frame_start) {
  const int effective_fps = adaptive_throttle_ && !window_focused_ ? 10 : std::max(10, target_fps_);
  const double budget_ms = 1000.0 / static_cast<double>(effective_fps);
  const double active_ms = elapsed_ms(frame_start, Clock::now());

  const auto sleep_start = Clock::now();
  if (active_ms < budget_ms) {
    const auto delay = static_cast<Uint32>(std::max(1.0, std::floor(budget_ms - active_ms)));
    SDL_Delay(delay);
  }
  record_module(ModuleSlot::FrameThrottle, elapsed_ms(sleep_start, Clock::now()));

  current_frame_ms_ = elapsed_ms(frame_start, Clock::now());
  current_fps_ = current_frame_ms_ > 0.0 ? 1000.0 / current_frame_ms_ : 0.0;
}

void App::draw_workspace() {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);

  constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoBringToFrontOnFocus |
                                      ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

  ImGui::Begin("BlenderUIDemoWorkspace", nullptr, flags);
  draw_menu_bar();

  const float status_height = 28.0F;
  const ImVec2 workspace_size{0.0F, std::max(100.0F, ImGui::GetContentRegionAvail().y - status_height)};
  constexpr ImGuiTableFlags table_flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable |
                                           ImGuiTableFlags_SizingStretchProp;

  if (ImGui::BeginTable("workspace_layout", 3, table_flags, workspace_size)) {
    ImGui::TableSetupColumn("Navigation", ImGuiTableColumnFlags_WidthFixed, 220.0F);
    ImGui::TableSetupColumn("Editor", ImGuiTableColumnFlags_WidthStretch, 1.0F);
    ImGui::TableSetupColumn("Properties", ImGuiTableColumnFlags_WidthFixed, 315.0F);
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::BeginChild("navigation_panel", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_None);
    draw_navigation();
    ImGui::EndChild();

    ImGui::TableSetColumnIndex(1);
    ImGui::BeginChild("editor_panel", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_None);
    switch (page_) {
      case Page::Overview:
        draw_overview();
        break;
      case Page::Performance:
        draw_performance();
        break;
      case Page::Hardware:
        draw_hardware();
        break;
      case Page::Storage:
        draw_storage();
        break;
      case Page::Network:
        draw_network();
        break;
      case Page::Processes:
        draw_processes();
        break;
      case Page::Diagnostics:
        draw_diagnostics();
        break;
    }
    ImGui::EndChild();

    ImGui::TableSetColumnIndex(2);
    ImGui::BeginChild("properties_panel", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_None);
    draw_inspector();
    ImGui::EndChild();
    ImGui::EndTable();
  }

  draw_status_bar();
  ImGui::End();
}

void App::draw_menu_bar() {
  if (!ImGui::BeginMenuBar()) {
    return;
  }

  ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
  ImGui::TextUnformatted("B");
  ImGui::PopStyleColor();
  ImGui::SameLine(0.0F, 6.0F);
  ImGui::TextUnformatted("Blender UI Demo");
  ImGui::Separator();

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Refresh all", "F5")) {
      update_metrics(true);
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Exit", "Alt+F4")) {
      request_exit_ = true;
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Workspace")) {
    const std::array<std::pair<const char*, Page>, 7> pages{{
        {"Overview", Page::Overview},
        {"Performance", Page::Performance},
        {"Hardware", Page::Hardware},
        {"Storage", Page::Storage},
        {"Network", Page::Network},
        {"Processes", Page::Processes},
        {"Diagnostics", Page::Diagnostics},
    }};
    for (const auto& [label, page] : pages) {
      if (ImGui::MenuItem(label, nullptr, page_ == page)) {
        page_ = page;
      }
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View")) {
    ImGui::MenuItem("Dear ImGui Demo", nullptr, &show_imgui_demo_);
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Help")) {
    if (ImGui::MenuItem("About")) {
      show_about_ = true;
    }
    ImGui::EndMenu();
  }

  const std::string live = "APP " + percent_text(dynamic_info_.application_cpu_percent) +
                           "  |  " + fixed_text(current_fps_, 0) + " FPS";
  const float live_width = ImGui::CalcTextSize(live.c_str()).x;
  ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - live_width - 18.0F));
  ImGui::TextColored(dynamic_info_.application_cpu_percent > 8.0 ? kWarning : kGood,
                     "%s",
                     live.c_str());
  ImGui::EndMenuBar();
}

void App::draw_navigation() {
  ImGui::BeginChild("brand", ImVec2{0.0F, 78.0F}, ImGuiChildFlags_Borders);
  ImGui::TextColored(kAccent, "SYSTEM LAB");
  ImGui::SetWindowFontScale(1.15F);
  ImGui::TextUnformatted(monitor_.static_info().computer_name.c_str());
  ImGui::SetWindowFontScale(1.0F);
  ImGui::TextDisabled("Native telemetry workspace");
  ImGui::EndChild();
  ImGui::Spacing();

  const auto navigation_item = [this](const char* label, Page page) {
    const bool selected = page_ == page;
    if (selected) {
      ImGui::PushStyleColor(ImGuiCol_Header, kAccentSoft);
    }
    const bool clicked = ImGui::Selectable(label, selected, 0, ImVec2{0.0F, 34.0F});
    if (selected) {
      ImGui::PopStyleColor();
    }
    if (clicked) {
      page_ = page;
    }
  };

  ImGui::TextDisabled("MONITOR");
  navigation_item("  Overview", Page::Overview);
  navigation_item("  Performance", Page::Performance);
  ImGui::Spacing();
  ImGui::TextDisabled("INVENTORY");
  navigation_item("  Hardware", Page::Hardware);
  navigation_item("  Storage", Page::Storage);
  navigation_item("  Network", Page::Network);
  navigation_item("  Processes", Page::Processes);
  ImGui::Spacing();
  ImGui::TextDisabled("DEVELOPER");
  navigation_item("  Diagnostics", Page::Diagnostics);

  section_title("QUICK ACTIONS");
  if (ImGui::Button("Refresh all data", ImVec2{-1.0F, 34.0F})) {
    update_metrics(true);
  }
  if (ImGui::Button("Open diagnostics", ImVec2{-1.0F, 34.0F})) {
    page_ = Page::Diagnostics;
  }

  const float footer_height = 70.0F;
  if (ImGui::GetContentRegionAvail().y > footer_height) {
    ImGui::Dummy(ImVec2{0.0F, ImGui::GetContentRegionAvail().y - footer_height});
  }
  ImGui::Separator();
  status_dot(vsync_active_ ? kGood : kWarning, vsync_active_ ? "VSync active" : "Software frame cap");
  ImGui::TextDisabled("MSVC / SDL3 / OpenGL");
}

void App::draw_page_header(const char* eyebrow, const char* title, const char* subtitle) {
  ImGui::BeginChild("page_header", ImVec2{0.0F, 88.0F}, ImGuiChildFlags_Borders);
  const ImVec2 position = ImGui::GetWindowPos();
  ImGui::GetWindowDrawList()->AddRectFilled(position,
                                            ImVec2{position.x + 4.0F,
                                                   position.y + ImGui::GetWindowHeight()},
                                            ImGui::ColorConvertFloat4ToU32(kAccent));
  ImGui::Dummy(ImVec2{5.0F, 0.0F});
  ImGui::SameLine();
  ImGui::BeginGroup();
  ImGui::TextColored(kAccent, "%s", eyebrow);
  ImGui::SetWindowFontScale(1.30F);
  ImGui::TextUnformatted(title);
  ImGui::SetWindowFontScale(1.0F);
  ImGui::TextDisabled("%s", subtitle);
  ImGui::EndGroup();
  ImGui::EndChild();
  ImGui::Spacing();
}

void App::draw_overview() {
  const auto& info = monitor_.static_info();
  draw_page_header("OVERVIEW", "System at a glance", "Live host health, application cost and inventory status");

  const int columns = ImGui::GetContentRegionAvail().x >= 900.0F ? 4 : 2;
  if (ImGui::BeginTable("overview_metrics", columns, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    metric_card("system_cpu",
                "SYSTEM CPU",
                percent_text(dynamic_info_.cpu_usage_percent),
                std::to_string(info.logical_processors) + " logical processors",
                dynamic_info_.cpu_usage_percent,
                kAccent);
    ImGui::TableNextColumn();
    metric_card("memory",
                "MEMORY",
                percent_text(dynamic_info_.memory_usage_percent),
                format_bytes(dynamic_info_.used_memory_bytes) + " of " + format_bytes(info.total_memory_bytes),
                dynamic_info_.memory_usage_percent,
                kInfo);
    ImGui::TableNextColumn();
    metric_card("app_cpu",
                "THIS APPLICATION",
                percent_text(dynamic_info_.application_cpu_percent),
                format_bytes(dynamic_info_.application_working_set_bytes) + " working set",
                dynamic_info_.application_cpu_percent,
                dynamic_info_.application_cpu_percent > 8.0 ? kWarning : kGood);
    ImGui::TableNextColumn();
    metric_card("network",
                "NETWORK NOW",
                format_rate(total_receive_rate()),
                format_rate(total_send_rate()) + " upload",
                -1.0,
                kGood);
    ImGui::EndTable();
  }

  section_title("LIVE HISTORY");
  if (ImGui::BeginTable("overview_charts", 2, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    plot_card("system_chart",
              "System load",
              "CPU percentage across the recent sampling window",
              cpu_history_.data(),
              history_count_,
              history_offset_,
              0.0F,
              100.0F);
    ImGui::TableNextColumn();
    plot_card("app_chart",
              "Application CPU",
              "Normalized across all logical processors",
              app_cpu_history_.data(),
              history_count_,
              history_offset_,
              0.0F,
              std::max(15.0F, *std::max_element(app_cpu_history_.begin(), app_cpu_history_.end()) + 5.0F));
    ImGui::EndTable();
  }

  section_title("HEALTH SUMMARY");
  ImGui::BeginChild("health_summary", ImVec2{0.0F, 116.0F}, ImGuiChildFlags_Borders);
  const auto reasons = diagnose_cpu_usage();
  if (dynamic_info_.application_cpu_percent < 5.0) {
    status_dot(kGood, "Application overhead is currently low");
  } else if (dynamic_info_.application_cpu_percent < 10.0) {
    status_dot(kWarning, "Application overhead is moderate");
  } else {
    status_dot(kDanger, "Application overhead needs attention");
  }
  for (const auto& reason : reasons) {
    ImGui::BulletText("%s", reason.c_str());
  }
  ImGui::EndChild();
}

void App::draw_performance() {
  draw_page_header("PERFORMANCE", "Frame pacing and resource cost", "Tune rendering frequency and inspect application overhead");

  if (ImGui::BeginTable("performance_metrics", 4, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    metric_card("perf_app_cpu",
                "APP CPU",
                percent_text(dynamic_info_.application_cpu_percent),
                "Current process",
                dynamic_info_.application_cpu_percent,
                kAccent);
    ImGui::TableNextColumn();
    metric_card("perf_memory",
                "APP MEMORY",
                format_bytes(dynamic_info_.application_working_set_bytes),
                format_bytes(dynamic_info_.application_private_bytes) + " private",
                -1.0,
                kInfo);
    ImGui::TableNextColumn();
    metric_card("perf_frame",
                "FRAME TIME",
                milliseconds_text(current_frame_ms_),
                fixed_text(current_fps_, 0) + " FPS",
                -1.0,
                kGood);
    ImGui::TableNextColumn();
    metric_card("perf_sample",
                "COLLECTOR",
                milliseconds_text(dynamic_info_.collector_timings.total_ms),
                std::to_string(sampling_interval_ms_) + " ms interval",
                -1.0,
                kWarning);
    ImGui::EndTable();
  }

  section_title("FRAME PACING");
  ImGui::BeginChild("frame_pacing", ImVec2{0.0F, 108.0F}, ImGuiChildFlags_Borders);
  ImGui::TextUnformatted("Foreground frame-rate cap");
  ImGui::SameLine();
  ImGui::RadioButton("15 FPS", &target_fps_, 15);
  ImGui::SameLine();
  ImGui::RadioButton("30 FPS", &target_fps_, 30);
  ImGui::SameLine();
  ImGui::RadioButton("60 FPS", &target_fps_, 60);
  ImGui::Checkbox("Reduce to 10 FPS when the window loses focus", &adaptive_throttle_);
  ImGui::TextDisabled("30 FPS is the default because this dashboard changes slowly and does not need game-rate redraws.");
  ImGui::EndChild();

  section_title("PERFORMANCE HISTORY");
  if (ImGui::BeginTable("performance_charts", 2, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    plot_card("frame_history",
              "Frame time",
              "Lower is better; 33.3 ms corresponds to 30 FPS",
              frame_time_history_.data(),
              history_count_,
              history_offset_,
              0.0F,
              70.0F,
              190.0F);
    ImGui::TableNextColumn();
    plot_card("memory_history",
              "System memory",
              "Physical memory utilization",
              memory_history_.data(),
              history_count_,
              history_offset_,
              0.0F,
              100.0F,
              190.0F);
    ImGui::EndTable();
  }
}

void App::draw_hardware() {
  const auto& info = monitor_.static_info();
  draw_page_header("HARDWARE", "Computer inventory", "Processor, graphics, memory and operating-system identity");

  ImGui::BeginChild("hardware_content", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_Borders);
  section_title("PROCESSOR");
  if (ImGui::BeginTable("processor_properties", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 190.0F);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    property_row("Model", info.cpu_name);
    property_row("Architecture", info.architecture);
    property_row("Logical processors", std::to_string(info.logical_processors));
    property_row("Current system load", percent_text(dynamic_info_.cpu_usage_percent));
    ImGui::EndTable();
  }

  section_title("GRAPHICS");
  if (ImGui::BeginTable("graphics_properties", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 190.0F);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    property_row("Primary adapter", info.gpu_name);
    property_row("Dedicated video memory",
                 info.gpu_dedicated_memory_bytes > 0 ? format_bytes(info.gpu_dedicated_memory_bytes)
                                                     : "Shared or unavailable");
    property_row("UI graphics API", "OpenGL 3.3");
    ImGui::EndTable();
  }

  section_title("MEMORY & OPERATING SYSTEM");
  if (ImGui::BeginTable("system_properties", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 190.0F);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    property_row("Installed memory", format_bytes(info.total_memory_bytes));
    property_row("Operating system", info.operating_system);
    property_row("Computer name", info.computer_name);
    property_row("Signed-in user", info.user_name);
    property_row("Session uptime", format_duration(dynamic_info_.uptime_seconds));
    ImGui::EndTable();
  }
  ImGui::EndChild();
}

void App::draw_storage() {
  draw_page_header("STORAGE", "Local volumes", "Capacity and utilization refreshed by the slow inventory collector");
  if (dynamic_info_.disks.empty()) {
    ImGui::TextDisabled("No readable local volumes were detected.");
    return;
  }

  const int columns = ImGui::GetContentRegionAvail().x >= 760.0F ? 2 : 1;
  if (ImGui::BeginTable("disk_grid", columns, ImGuiTableFlags_SizingStretchSame)) {
    for (std::size_t index = 0; index < dynamic_info_.disks.size(); ++index) {
      const auto& disk = dynamic_info_.disks[index];
      const std::uint64_t used = disk.total_bytes - std::min(disk.free_bytes, disk.total_bytes);
      const double used_percent = disk.total_bytes > 0
                                      ? 100.0 * static_cast<double>(used) /
                                            static_cast<double>(disk.total_bytes)
                                      : 0.0;
      ImGui::TableNextColumn();
      ImGui::PushID(static_cast<int>(index));
      ImGui::BeginChild("disk", ImVec2{0.0F, 146.0F}, ImGuiChildFlags_Borders);
      ImGui::TextColored(kAccent, "%s", disk.name.c_str());
      ImGui::SameLine();
      ImGui::TextDisabled("%s", disk.file_system.c_str());
      ImGui::SetWindowFontScale(1.18F);
      ImGui::Text("%s", format_bytes(used).c_str());
      ImGui::SetWindowFontScale(1.0F);
      ImGui::TextDisabled("used of %s  |  %s available",
                          format_bytes(disk.total_bytes).c_str(),
                          format_bytes(disk.free_bytes).c_str());
      ImGui::Spacing();
      ImGui::PushStyleColor(ImGuiCol_PlotHistogram, usage_color(used_percent));
      const std::string overlay = percent_text(used_percent);
      ImGui::ProgressBar(static_cast<float>(used_percent / 100.0), ImVec2{-1.0F, 18.0F}, overlay.c_str());
      ImGui::PopStyleColor();
      ImGui::EndChild();
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
}

void App::draw_network() {
  draw_page_header("NETWORK", "Adapters and live throughput", "Native IP Helper API counters sampled once per second");

  if (ImGui::BeginTable("network_metrics", 3, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    metric_card("download", "DOWNLOAD", format_rate(total_receive_rate()), "Aggregate receive rate", -1.0, kGood);
    ImGui::TableNextColumn();
    metric_card("upload", "UPLOAD", format_rate(total_send_rate()), "Aggregate send rate", -1.0, kInfo);
    ImGui::TableNextColumn();
    const auto connected = std::count_if(dynamic_info_.network_adapters.begin(),
                                         dynamic_info_.network_adapters.end(),
                                         [](const NetworkAdapterInfo& adapter) { return adapter.connected; });
    metric_card("adapter_count",
                "CONNECTED",
                std::to_string(connected),
                std::to_string(dynamic_info_.network_adapters.size()) + " adapter(s) detected",
                -1.0,
                kAccent);
    ImGui::EndTable();
  }

  section_title("ADAPTERS");
  constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                    ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
  if (ImGui::BeginTable("adapter_table", 6, flags, ImVec2{0.0F, 0.0F})) {
    ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 70.0F);
    ImGui::TableSetupColumn("Adapter", ImGuiTableColumnFlags_WidthStretch, 1.4F);
    ImGui::TableSetupColumn("IPv4", ImGuiTableColumnFlags_WidthFixed, 120.0F);
    ImGui::TableSetupColumn("Down", ImGuiTableColumnFlags_WidthFixed, 95.0F);
    ImGui::TableSetupColumn("Up", ImGuiTableColumnFlags_WidthFixed, 95.0F);
    ImGui::TableSetupColumn("Received", ImGuiTableColumnFlags_WidthFixed, 100.0F);
    ImGui::TableHeadersRow();
    for (const auto& adapter : dynamic_info_.network_adapters) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextColored(adapter.connected ? kGood : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
                         "%s",
                         adapter.connected ? "Online" : "Offline");
      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted(adapter.name.empty() ? adapter.description.c_str() : adapter.name.c_str());
      if (!adapter.description.empty() && adapter.description != adapter.name) {
        ImGui::TextDisabled("%s", adapter.description.c_str());
      }
      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(adapter.ipv4_address.empty() ? "—" : adapter.ipv4_address.c_str());
      ImGui::TableSetColumnIndex(3);
      ImGui::TextUnformatted(format_rate(adapter.receive_bytes_per_second).c_str());
      ImGui::TableSetColumnIndex(4);
      ImGui::TextUnformatted(format_rate(adapter.send_bytes_per_second).c_str());
      ImGui::TableSetColumnIndex(5);
      ImGui::TextUnformatted(format_bytes(adapter.received_bytes).c_str());
    }
    ImGui::EndTable();
  }
}

void App::draw_processes() {
  draw_page_header("PROCESSES", "Top memory consumers", "A lightweight snapshot refreshed every five seconds");

  if (ImGui::BeginTable("process_metrics", 3, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    metric_card("process_count", "PROCESS COUNT", std::to_string(dynamic_info_.process_count), "Current snapshot", -1.0, kAccent);
    ImGui::TableNextColumn();
    metric_card("self_working", "THIS APP", format_bytes(dynamic_info_.application_working_set_bytes), "Working set", -1.0, kInfo);
    ImGui::TableNextColumn();
    metric_card("self_private", "PRIVATE MEMORY", format_bytes(dynamic_info_.application_private_bytes), "Committed by this app", -1.0, kGood);
    ImGui::EndTable();
  }

  section_title("TOP PROCESSES BY WORKING SET");
  constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                    ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
  if (ImGui::BeginTable("process_table", 4, flags, ImVec2{0.0F, 0.0F})) {
    ImGui::TableSetupColumn("Process", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 80.0F);
    ImGui::TableSetupColumn("Threads", ImGuiTableColumnFlags_WidthFixed, 80.0F);
    ImGui::TableSetupColumn("Working set", ImGuiTableColumnFlags_WidthFixed, 120.0F);
    ImGui::TableHeadersRow();
    for (const auto& process : dynamic_info_.top_processes) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(process.name.c_str());
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%u", process.process_id);
      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%u", process.thread_count);
      ImGui::TableSetColumnIndex(3);
      ImGui::TextUnformatted(format_bytes(process.working_set_bytes).c_str());
    }
    ImGui::EndTable();
  }
}

void App::draw_diagnostics() {
  draw_page_header("DIAGNOSTICS", "Why is the application using CPU?", "Per-module timings, active causes and optimizations applied in this build");

  if (ImGui::BeginTable("diagnostic_metrics", 4, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    metric_card("diag_cpu",
                "APP CPU",
                percent_text(dynamic_info_.application_cpu_percent),
                "Normalized process usage",
                dynamic_info_.application_cpu_percent,
                dynamic_info_.application_cpu_percent > 8.0 ? kWarning : kGood);
    ImGui::TableNextColumn();
    metric_card("diag_fps", "FRAME RATE", fixed_text(current_fps_, 0) + " FPS", milliseconds_text(current_frame_ms_), -1.0, kInfo);
    ImGui::TableNextColumn();
    metric_card("diag_ui",
                "UI BUILD",
                milliseconds_text(modules_[static_cast<std::size_t>(ModuleSlot::UiComposition)].average_ms),
                "Average per frame",
                -1.0,
                kAccent);
    ImGui::TableNextColumn();
    metric_card("diag_collector",
                "COLLECTOR",
                milliseconds_text(dynamic_info_.collector_timings.total_ms),
                dynamic_info_.collector_timings.slow_refresh_performed ? "Slow inventory included" : "Fast sample only",
                -1.0,
                kWarning);
    ImGui::EndTable();
  }

  section_title("CURRENT DIAGNOSIS");
  ImGui::BeginChild("diagnosis", ImVec2{0.0F, 138.0F}, ImGuiChildFlags_Borders);
  const auto reasons = diagnose_cpu_usage();
  for (const auto& reason : reasons) {
    ImGui::BulletText("%s", reason.c_str());
  }
  ImGui::EndChild();

  section_title("APPLICATION MODULES");
  constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                    ImGuiTableFlags_Resizable;
  if (ImGui::BeginTable("module_table", 5, flags)) {
    ImGui::TableSetupColumn("Module", ImGuiTableColumnFlags_WidthFixed, 155.0F);
    ImGui::TableSetupColumn("Role", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Last", ImGuiTableColumnFlags_WidthFixed, 80.0F);
    ImGui::TableSetupColumn("Average", ImGuiTableColumnFlags_WidthFixed, 80.0F);
    ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 80.0F);
    ImGui::TableHeadersRow();
    for (const auto& module : modules_) {
      const bool within_budget = module.average_ms <= module.budget_ms;
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(module.name.c_str());
      ImGui::TableSetColumnIndex(1);
      ImGui::TextDisabled("%s", module.description.c_str());
      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(milliseconds_text(module.last_ms).c_str());
      ImGui::TableSetColumnIndex(3);
      ImGui::TextUnformatted(milliseconds_text(module.average_ms).c_str());
      ImGui::TableSetColumnIndex(4);
      ImGui::TextColored(within_budget ? kGood : kWarning, "%s", within_budget ? "Healthy" : "Review");
    }
    ImGui::EndTable();
  }

  section_title("COLLECTOR BREAKDOWN");
  if (ImGui::BeginTable("collector_table", 3, flags)) {
    ImGui::TableSetupColumn("Collector", ImGuiTableColumnFlags_WidthFixed, 180.0F);
    ImGui::TableSetupColumn("Frequency", ImGuiTableColumnFlags_WidthFixed, 150.0F);
    ImGui::TableSetupColumn("Last cost", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();
    const auto collector_row = [](const char* name, const char* frequency, double cost) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(name);
      ImGui::TableSetColumnIndex(1);
      ImGui::TextDisabled("%s", frequency);
      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(milliseconds_text(cost).c_str());
    };
    collector_row("CPU, memory & self process", "Fast sample", dynamic_info_.collector_timings.cpu_memory_ms);
    collector_row("Network adapters", "Fast sample", dynamic_info_.collector_timings.network_ms);
    collector_row("Storage volumes", "Slow sample", dynamic_info_.collector_timings.storage_ms);
    collector_row("Process inventory", "Slow sample", dynamic_info_.collector_timings.processes_ms);
    ImGui::EndTable();
  }

  section_title("OPTIMIZATIONS APPLIED");
  ImGui::BeginChild("resolved", ImVec2{0.0F, 165.0F}, ImGuiChildFlags_Borders);
  ImGui::BulletText("Default rendering reduced from display-rate redraws to a 30 FPS cap.");
  ImGui::BulletText("A software limiter is always available when the graphics driver rejects VSync.");
  ImGui::BulletText("CPU, memory and network are sampled every %d ms; disks and processes every %d ms.",
                    sampling_interval_ms_,
                    slow_refresh_interval_ms_);
  ImGui::BulletText("History charts use fixed circular buffers instead of allocating vectors every frame.");
  ImGui::BulletText("Minimized windows sleep for 120 ms and unfocused windows fall back to 10 FPS.");
  ImGui::EndChild();
}

void App::draw_inspector() {
  const auto& info = monitor_.static_info();
  ImGui::TextColored(kAccent, "PROPERTIES");
  ImGui::SetWindowFontScale(1.12F);
  ImGui::TextUnformatted(current_page_name());
  ImGui::SetWindowFontScale(1.0F);
  ImGui::TextDisabled("Active workspace settings");
  ImGui::Separator();

  section_title("LIVE TELEMETRY");
  if (ImGui::BeginTable("live_inspector", 2, ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 112.0F);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    property_row("System CPU", percent_text(dynamic_info_.cpu_usage_percent));
    property_row("Application", percent_text(dynamic_info_.application_cpu_percent));
    property_row("Memory", percent_text(dynamic_info_.memory_usage_percent));
    property_row("Frame rate", fixed_text(current_fps_, 0) + " FPS");
    property_row("Frame time", milliseconds_text(current_frame_ms_));
    ImGui::EndTable();
  }

  section_title("PERFORMANCE POLICY");
  ImGui::TextDisabled("Foreground frame cap");
  ImGui::RadioButton("15", &target_fps_, 15);
  ImGui::SameLine();
  ImGui::RadioButton("30", &target_fps_, 30);
  ImGui::SameLine();
  ImGui::RadioButton("60", &target_fps_, 60);
  ImGui::Checkbox("Background throttling", &adaptive_throttle_);
  ImGui::SliderInt("Fast sample (ms)", &sampling_interval_ms_, 500, 3000, "%d");
  ImGui::SliderInt("Slow sample (ms)", &slow_refresh_interval_ms_, 3000, 15000, "%d");
  if (ImGui::Button("Apply and refresh", ImVec2{-1.0F, 34.0F})) {
    update_metrics(true);
  }

  section_title("RUNTIME");
  if (ImGui::BeginTable("runtime_inspector", 2, ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 112.0F);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    property_row("VSync", vsync_active_ ? "Active" : "Unavailable; limiter active");
    property_row("Focus policy", window_focused_ ? "Foreground" : "Background 10 FPS");
    property_row("Collector", milliseconds_text(dynamic_info_.collector_timings.total_ms));
    property_row("Host", info.computer_name);
    ImGui::EndTable();
  }

  section_title("TOOLS");
  if (ImGui::Button("Open diagnostics", ImVec2{-1.0F, 32.0F})) {
    page_ = Page::Diagnostics;
  }
  if (ImGui::Button("About this build", ImVec2{-1.0F, 32.0F})) {
    show_about_ = true;
  }
}

void App::draw_status_bar() {
  ImGui::Separator();
  ImGui::TextColored(dynamic_info_.application_cpu_percent > 8.0 ? kWarning : kGood, "●");
  ImGui::SameLine();
  ImGui::TextDisabled("%s  |  App CPU %s  |  %.0f FPS  |  %d ms telemetry  |  %zu process(es)",
                      current_page_name(),
                      percent_text(dynamic_info_.application_cpu_percent).c_str(),
                      current_fps_,
                      sampling_interval_ms_,
                      dynamic_info_.process_count);
}

void App::draw_about_dialog() {
  if (show_about_) {
    ImGui::OpenPopup("About Blender UI Demo");
    show_about_ = false;
  }

  if (ImGui::BeginPopupModal("About Blender UI Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextColored(kAccent, "BLENDER UI DEMO");
    ImGui::TextUnformatted("Native System Information Workspace");
    ImGui::Separator();
    ImGui::TextWrapped("A C++ desktop application that studies Blender's dense editor layout while exposing its own runtime cost and system telemetry.");
    ImGui::Spacing();
    ImGui::BulletText("C++20 and Win32 native collectors");
    ImGui::BulletText("SDL3 window, input and frame pacing");
    ImGui::BulletText("OpenGL 3.3 rendering backend");
    ImGui::BulletText("Dear ImGui immediate-mode interface");
    ImGui::BulletText("GPU, network, process and self-diagnostics modules");
    ImGui::Spacing();
    ImGui::TextDisabled("Independent demonstration project. No Blender source code, logo or bundled assets are included.");
    ImGui::Spacing();
    if (ImGui::Button("Close", ImVec2{120.0F, 0.0F})) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

std::vector<std::string> App::diagnose_cpu_usage() const {
  std::vector<std::string> reasons;
  if (show_imgui_demo_) {
    reasons.emplace_back("Dear ImGui's demonstration window is enabled and adds a large number of widgets every frame.");
  }
  if (!vsync_active_) {
    reasons.emplace_back("The graphics driver did not enable VSync; the built-in software frame cap is preventing an unbounded loop.");
  }
  if (target_fps_ >= 60) {
    reasons.emplace_back("The selected 60 FPS mode doubles UI composition work compared with the recommended 30 FPS mode.");
  }

  const auto& ui = modules_[static_cast<std::size_t>(ModuleSlot::UiComposition)];
  const auto& render = modules_[static_cast<std::size_t>(ModuleSlot::Rendering)];
  if (ui.average_ms > 5.0) {
    reasons.emplace_back("UI composition is above its 5 ms budget; large tables or the demo window may be visible.");
  }
  if (render.average_ms > 5.0) {
    reasons.emplace_back("OpenGL submission is above its 5 ms budget; the GPU driver or high-DPI framebuffer is the likely cost.");
  }
  if (dynamic_info_.collector_timings.total_ms > 12.0) {
    reasons.emplace_back("System collection is temporarily expensive; inspect the storage and process timings below.");
  }
  if (dynamic_info_.collector_timings.processes_ms > 20.0) {
    reasons.emplace_back("The process inventory scan is the dominant slow-sample cost, but it runs only at the slow interval.");
  }
  if (dynamic_info_.application_cpu_percent > 8.0 && reasons.empty()) {
    reasons.emplace_back("CPU usage is elevated without a single dominant module; frame rate and driver scheduling are the likely combined cause.");
  }
  if (reasons.empty()) {
    reasons.emplace_back("No active bottleneck is detected. Current CPU usage is consistent with a throttled native monitoring UI.");
  }
  return reasons;
}

double App::total_receive_rate() const {
  return std::accumulate(dynamic_info_.network_adapters.begin(),
                         dynamic_info_.network_adapters.end(),
                         0.0,
                         [](double total, const NetworkAdapterInfo& adapter) {
                           return total + adapter.receive_bytes_per_second;
                         });
}

double App::total_send_rate() const {
  return std::accumulate(dynamic_info_.network_adapters.begin(),
                         dynamic_info_.network_adapters.end(),
                         0.0,
                         [](double total, const NetworkAdapterInfo& adapter) {
                           return total + adapter.send_bytes_per_second;
                         });
}

const char* App::current_page_name() const {
  switch (page_) {
    case Page::Overview:
      return "Overview";
    case Page::Performance:
      return "Performance";
    case Page::Hardware:
      return "Hardware";
    case Page::Storage:
      return "Storage";
    case Page::Network:
      return "Network";
    case Page::Processes:
      return "Processes";
    case Page::Diagnostics:
      return "Diagnostics";
  }
  return "Overview";
}

}  // namespace blender_ui_demo
