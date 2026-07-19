#include "App.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace blender_ui_demo {
namespace {

constexpr ImVec4 kAccent{0.92F, 0.39F, 0.08F, 1.0F};
constexpr ImVec4 kGood{0.28F, 0.72F, 0.48F, 1.0F};
constexpr ImVec4 kWarning{0.95F, 0.67F, 0.20F, 1.0F};
constexpr ImVec4 kDanger{0.90F, 0.30F, 0.28F, 1.0F};

std::string percent_text(double value) {
  std::ostringstream stream;
  stream.setf(std::ios::fixed);
  stream.precision(1);
  stream << value << '%';
  return stream.str();
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

void metric_card(const char* id,
                 const char* title,
                 const std::string& value,
                 const std::string& detail,
                 double progress_percent = -1.0) {
  ImGui::PushID(id);
  const ImVec2 size{ImGui::GetContentRegionAvail().x, 112.0F};
  ImGui::BeginChild("metric", size, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
  ImGui::TextDisabled("%s", title);
  ImGui::Spacing();
  ImGui::SetWindowFontScale(1.28F);
  ImGui::TextUnformatted(value.c_str());
  ImGui::SetWindowFontScale(1.0F);
  ImGui::TextDisabled("%s", detail.c_str());
  if (progress_percent >= 0.0) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, usage_color(progress_percent));
    ImGui::ProgressBar(static_cast<float>(progress_percent / 100.0), ImVec2{-1.0F, 6.0F}, "");
    ImGui::PopStyleColor();
  }
  ImGui::EndChild();
  ImGui::PopID();
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
                                        1440,
                                        900,
                                        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                            SDL_WINDOW_HIGH_PIXEL_DENSITY);
  if (window == nullptr) {
    std::fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  SDL_SetWindowMinimumSize(window, 1024, 680);

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (gl_context == nullptr) {
    std::fprintf(stderr, "OpenGL context creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.IniFilename = "blender_ui_demo.ini";

  apply_blender_theme();
  ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  update_metrics(true);

  while (!request_exit_) {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_QUIT) {
        request_exit_ = true;
      }
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
          event.window.windowID == SDL_GetWindowID(window)) {
        request_exit_ = true;
      }
    }

    update_metrics();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    draw_workspace();
    draw_about_dialog();
    if (show_imgui_demo_) {
      ImGui::ShowDemoWindow(&show_imgui_demo_);
    }

    ImGui::Render();
    int display_width = 0;
    int display_height = 0;
    SDL_GetWindowSizeInPixels(window, &display_width, &display_height);
    glViewport(0, 0, display_width, display_height);
    glClearColor(0.055F, 0.058F, 0.064F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
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
  style.WindowPadding = ImVec2{8.0F, 8.0F};
  style.FramePadding = ImVec2{8.0F, 5.0F};
  style.CellPadding = ImVec2{7.0F, 6.0F};
  style.ItemSpacing = ImVec2{7.0F, 6.0F};
  style.ItemInnerSpacing = ImVec2{6.0F, 5.0F};
  style.ScrollbarSize = 13.0F;
  style.WindowRounding = 0.0F;
  style.ChildRounding = 3.0F;
  style.FrameRounding = 3.0F;
  style.PopupRounding = 4.0F;
  style.GrabRounding = 3.0F;
  style.TabRounding = 3.0F;
  style.WindowBorderSize = 0.0F;
  style.ChildBorderSize = 1.0F;
  style.FrameBorderSize = 0.0F;

  auto& colors = style.Colors;
  colors[ImGuiCol_Text] = ImVec4{0.88F, 0.88F, 0.90F, 1.0F};
  colors[ImGuiCol_TextDisabled] = ImVec4{0.52F, 0.54F, 0.58F, 1.0F};
  colors[ImGuiCol_WindowBg] = ImVec4{0.085F, 0.09F, 0.10F, 1.0F};
  colors[ImGuiCol_ChildBg] = ImVec4{0.105F, 0.11F, 0.12F, 1.0F};
  colors[ImGuiCol_PopupBg] = ImVec4{0.10F, 0.105F, 0.115F, 0.98F};
  colors[ImGuiCol_Border] = ImVec4{0.19F, 0.20F, 0.22F, 1.0F};
  colors[ImGuiCol_FrameBg] = ImVec4{0.15F, 0.16F, 0.18F, 1.0F};
  colors[ImGuiCol_FrameBgHovered] = ImVec4{0.21F, 0.22F, 0.24F, 1.0F};
  colors[ImGuiCol_FrameBgActive] = ImVec4{0.25F, 0.26F, 0.29F, 1.0F};
  colors[ImGuiCol_TitleBg] = ImVec4{0.08F, 0.085F, 0.095F, 1.0F};
  colors[ImGuiCol_TitleBgActive] = ImVec4{0.10F, 0.105F, 0.12F, 1.0F};
  colors[ImGuiCol_MenuBarBg] = ImVec4{0.11F, 0.115F, 0.125F, 1.0F};
  colors[ImGuiCol_ScrollbarBg] = ImVec4{0.08F, 0.085F, 0.095F, 1.0F};
  colors[ImGuiCol_ScrollbarGrab] = ImVec4{0.25F, 0.26F, 0.29F, 1.0F};
  colors[ImGuiCol_CheckMark] = kAccent;
  colors[ImGuiCol_SliderGrab] = kAccent;
  colors[ImGuiCol_SliderGrabActive] = ImVec4{1.0F, 0.52F, 0.15F, 1.0F};
  colors[ImGuiCol_Button] = ImVec4{0.16F, 0.17F, 0.19F, 1.0F};
  colors[ImGuiCol_ButtonHovered] = ImVec4{0.23F, 0.24F, 0.27F, 1.0F};
  colors[ImGuiCol_ButtonActive] = ImVec4{0.29F, 0.30F, 0.33F, 1.0F};
  colors[ImGuiCol_Header] = ImVec4{0.19F, 0.20F, 0.22F, 1.0F};
  colors[ImGuiCol_HeaderHovered] = ImVec4{0.24F, 0.25F, 0.28F, 1.0F};
  colors[ImGuiCol_HeaderActive] = ImVec4{0.29F, 0.30F, 0.33F, 1.0F};
  colors[ImGuiCol_Separator] = ImVec4{0.18F, 0.19F, 0.21F, 1.0F};
  colors[ImGuiCol_ResizeGrip] = ImVec4{0.35F, 0.36F, 0.40F, 0.35F};
  colors[ImGuiCol_ResizeGripHovered] = kAccent;
  colors[ImGuiCol_Tab] = ImVec4{0.13F, 0.14F, 0.16F, 1.0F};
  colors[ImGuiCol_TabHovered] = ImVec4{0.24F, 0.25F, 0.28F, 1.0F};
  colors[ImGuiCol_TabSelected] = ImVec4{0.20F, 0.21F, 0.24F, 1.0F};
  colors[ImGuiCol_PlotLines] = kAccent;
  colors[ImGuiCol_PlotHistogram] = kAccent;
  colors[ImGuiCol_TableBorderStrong] = ImVec4{0.17F, 0.18F, 0.20F, 1.0F};
  colors[ImGuiCol_TableBorderLight] = ImVec4{0.14F, 0.15F, 0.17F, 1.0F};
}

void App::update_metrics(bool force) {
  const auto now = std::chrono::steady_clock::now();
  if (!force && last_sample_.time_since_epoch().count() != 0 &&
      now - last_sample_ < std::chrono::milliseconds{500}) {
    return;
  }

  dynamic_info_ = monitor_.sample();
  last_sample_ = now;

  cpu_history_.push_back(static_cast<float>(dynamic_info_.cpu_usage_percent));
  memory_history_.push_back(static_cast<float>(dynamic_info_.memory_usage_percent));
  constexpr std::size_t maximum_samples = 120;
  while (cpu_history_.size() > maximum_samples) {
    cpu_history_.pop_front();
  }
  while (memory_history_.size() > maximum_samples) {
    memory_history_.pop_front();
  }
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

  const float status_height = 25.0F;
  const ImVec2 workspace_size{0.0F, std::max(100.0F, ImGui::GetContentRegionAvail().y - status_height)};
  constexpr ImGuiTableFlags table_flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable |
                                          ImGuiTableFlags_SizingStretchProp;

  if (ImGui::BeginTable("workspace_layout", 3, table_flags, workspace_size)) {
    ImGui::TableSetupColumn("Navigation", ImGuiTableColumnFlags_WidthFixed, 210.0F);
    ImGui::TableSetupColumn("Editor", ImGuiTableColumnFlags_WidthStretch, 1.0F);
    ImGui::TableSetupColumn("Properties", ImGuiTableColumnFlags_WidthFixed, 300.0F);
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
      case Page::Hardware:
        draw_hardware();
        break;
      case Page::Storage:
        draw_storage();
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

  ImGui::TextColored(kAccent, "B");
  ImGui::SameLine(0.0F, 5.0F);
  ImGui::TextUnformatted("Blender UI Demo");
  ImGui::Separator();

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Refresh", "F5")) {
      update_metrics(true);
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Exit", "Alt+F4")) {
      request_exit_ = true;
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View")) {
    if (ImGui::MenuItem("Overview", nullptr, page_ == Page::Overview)) {
      page_ = Page::Overview;
    }
    if (ImGui::MenuItem("Hardware", nullptr, page_ == Page::Hardware)) {
      page_ = Page::Hardware;
    }
    if (ImGui::MenuItem("Storage", nullptr, page_ == Page::Storage)) {
      page_ = Page::Storage;
    }
    ImGui::Separator();
    ImGui::MenuItem("Dear ImGui Demo", nullptr, &show_imgui_demo_);
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Help")) {
    if (ImGui::MenuItem("About")) {
      show_about_ = true;
    }
    ImGui::EndMenu();
  }

  const float live_width = ImGui::CalcTextSize("LIVE  ").x;
  ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - live_width - 14.0F));
  ImGui::TextColored(kGood, "LIVE");
  ImGui::EndMenuBar();
}

void App::draw_navigation() {
  ImGui::TextDisabled("SYSTEM WORKSPACE");
  ImGui::Text("%s", monitor_.static_info().computer_name.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  if (ImGui::Selectable("  Overview", page_ == Page::Overview, 0, ImVec2{0.0F, 34.0F})) {
    page_ = Page::Overview;
  }
  if (ImGui::Selectable("  Hardware", page_ == Page::Hardware, 0, ImVec2{0.0F, 34.0F})) {
    page_ = Page::Hardware;
  }
  if (ImGui::Selectable("  Storage", page_ == Page::Storage, 0, ImVec2{0.0F, 34.0F})) {
    page_ = Page::Storage;
  }

  section_title("QUICK ACTIONS");
  if (ImGui::Button("Refresh Metrics", ImVec2{-1.0F, 32.0F})) {
    update_metrics(true);
  }
  if (ImGui::Button("Open About", ImVec2{-1.0F, 32.0F})) {
    show_about_ = true;
  }

  const float footer_height = 60.0F;
  if (ImGui::GetContentRegionAvail().y > footer_height) {
    ImGui::Dummy(ImVec2{0.0F, ImGui::GetContentRegionAvail().y - footer_height});
  }
  ImGui::Separator();
  ImGui::TextDisabled("Native C++ Workspace");
  ImGui::TextDisabled("SDL3 / OpenGL");
}

void App::draw_overview() {
  const auto& info = monitor_.static_info();
  ImGui::TextDisabled("OVERVIEW");
  ImGui::SetWindowFontScale(1.22F);
  ImGui::Text("System Dashboard");
  ImGui::SetWindowFontScale(1.0F);
  ImGui::TextDisabled("%s  |  %s", info.computer_name.c_str(), info.operating_system.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  std::uint64_t storage_total = 0;
  std::uint64_t storage_free = 0;
  for (const auto& disk : dynamic_info_.disks) {
    storage_total += disk.total_bytes;
    storage_free += disk.free_bytes;
  }
  const double storage_used_percent = storage_total > 0
                                          ? 100.0 * static_cast<double>(storage_total - storage_free) /
                                                static_cast<double>(storage_total)
                                          : 0.0;

  if (ImGui::BeginTable("overview_metrics", 2, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    metric_card("cpu",
                "CPU LOAD",
                percent_text(dynamic_info_.cpu_usage_percent),
                std::to_string(info.logical_processors) + " logical processors",
                dynamic_info_.cpu_usage_percent);
    ImGui::TableNextColumn();
    metric_card("memory",
                "MEMORY",
                percent_text(dynamic_info_.memory_usage_percent),
                format_bytes(dynamic_info_.used_memory_bytes) + " used of " +
                    format_bytes(info.total_memory_bytes),
                dynamic_info_.memory_usage_percent);
    ImGui::TableNextColumn();
    metric_card("uptime",
                "UPTIME",
                format_duration(dynamic_info_.uptime_seconds),
                "Current Windows session");
    ImGui::TableNextColumn();
    metric_card("storage",
                "LOCAL STORAGE",
                format_bytes(storage_total),
                format_bytes(storage_free) + " available",
                storage_used_percent);
    ImGui::EndTable();
  }

  section_title("PERFORMANCE HISTORY");
  const std::vector<float> cpu(cpu_history_.begin(), cpu_history_.end());
  const std::vector<float> memory(memory_history_.begin(), memory_history_.end());
  ImGui::BeginChild("history_panel", ImVec2{0.0F, 245.0F}, ImGuiChildFlags_Borders);
  ImGui::TextDisabled("CPU - last 60 seconds");
  if (!cpu.empty()) {
    ImGui::PlotLines("##cpu_history", cpu.data(), static_cast<int>(cpu.size()), 0, nullptr, 0.0F, 100.0F,
                     ImVec2{-1.0F, 82.0F});
  }
  ImGui::TextDisabled("Memory - last 60 seconds");
  if (!memory.empty()) {
    ImGui::PlotLines("##memory_history",
                     memory.data(),
                     static_cast<int>(memory.size()),
                     0,
                     nullptr,
                     0.0F,
                     100.0F,
                     ImVec2{-1.0F, 82.0F});
  }
  ImGui::EndChild();
}

void App::draw_hardware() {
  const auto& info = monitor_.static_info();
  ImGui::TextDisabled("HARDWARE");
  ImGui::SetWindowFontScale(1.22F);
  ImGui::TextUnformatted("Computer Details");
  ImGui::SetWindowFontScale(1.0F);
  ImGui::TextDisabled("Native information collected from the operating system");
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::BeginChild("hardware_card", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_Borders);
  section_title("PROCESSOR");
  if (ImGui::BeginTable("processor_properties", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 180.0F);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    property_row("Model", info.cpu_name);
    property_row("Architecture", info.architecture);
    property_row("Logical processors", std::to_string(info.logical_processors));
    property_row("Current load", percent_text(dynamic_info_.cpu_usage_percent));
    ImGui::EndTable();
  }

  section_title("MEMORY");
  if (ImGui::BeginTable("memory_properties", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 180.0F);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    property_row("Installed", format_bytes(info.total_memory_bytes));
    property_row("Used", format_bytes(dynamic_info_.used_memory_bytes));
    property_row("Available", format_bytes(dynamic_info_.available_memory_bytes));
    property_row("Utilization", percent_text(dynamic_info_.memory_usage_percent));
    ImGui::EndTable();
  }

  section_title("OPERATING SYSTEM");
  if (ImGui::BeginTable("os_properties", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 180.0F);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    property_row("Operating system", info.operating_system);
    property_row("Computer name", info.computer_name);
    property_row("Signed-in user", info.user_name);
    property_row("Session uptime", format_duration(dynamic_info_.uptime_seconds));
    ImGui::EndTable();
  }
  ImGui::EndChild();
}

void App::draw_storage() {
  ImGui::TextDisabled("STORAGE");
  ImGui::SetWindowFontScale(1.22F);
  ImGui::TextUnformatted("Local Volumes");
  ImGui::SetWindowFontScale(1.0F);
  ImGui::TextDisabled("Fixed and removable local drives");
  ImGui::Separator();
  ImGui::Spacing();

  if (dynamic_info_.disks.empty()) {
    ImGui::TextDisabled("No local volumes were detected.");
    return;
  }

  for (std::size_t index = 0; index < dynamic_info_.disks.size(); ++index) {
    const auto& disk = dynamic_info_.disks[index];
    const std::uint64_t used = disk.total_bytes - std::min(disk.free_bytes, disk.total_bytes);
    const double used_percent = disk.total_bytes > 0
                                    ? 100.0 * static_cast<double>(used) /
                                          static_cast<double>(disk.total_bytes)
                                    : 0.0;

    ImGui::PushID(static_cast<int>(index));
    ImGui::BeginChild("disk", ImVec2{0.0F, 126.0F}, ImGuiChildFlags_Borders);
    ImGui::SetWindowFontScale(1.15F);
    ImGui::Text("%s", disk.name.c_str());
    ImGui::SetWindowFontScale(1.0F);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", disk.file_system.c_str());
    ImGui::Text("%s used of %s", format_bytes(used).c_str(), format_bytes(disk.total_bytes).c_str());
    ImGui::TextDisabled("%s available", format_bytes(disk.free_bytes).c_str());
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, usage_color(used_percent));
    const std::string overlay = percent_text(used_percent);
    ImGui::ProgressBar(static_cast<float>(used_percent / 100.0), ImVec2{-1.0F, 16.0F}, overlay.c_str());
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopID();
    ImGui::Spacing();
  }
}

void App::draw_inspector() {
  const auto& info = monitor_.static_info();
  ImGui::TextDisabled("PROPERTIES");
  ImGui::TextUnformatted("Active System");
  ImGui::Separator();

  section_title("LIVE TELEMETRY");
  if (ImGui::BeginTable("live_inspector", 2, ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 105.0F);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    property_row("CPU", percent_text(dynamic_info_.cpu_usage_percent));
    property_row("Memory", percent_text(dynamic_info_.memory_usage_percent));
    property_row("Uptime", format_duration(dynamic_info_.uptime_seconds));
    property_row("Volumes", std::to_string(dynamic_info_.disks.size()));
    ImGui::EndTable();
  }

  section_title("IDENTITY");
  if (ImGui::BeginTable("identity_inspector", 2, ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 105.0F);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    property_row("Host", info.computer_name);
    property_row("User", info.user_name);
    property_row("Platform", info.architecture);
    ImGui::EndTable();
  }

  section_title("DISPLAY");
  ImGui::TextWrapped("The application uses a Blender-inspired editor layout with native SDL windows and GPU-rendered controls.");
  ImGui::Spacing();
  ImGui::TextDisabled("Theme accent");
  ImGui::SameLine();
  ImGui::ColorButton("accent", kAccent, ImGuiColorEditFlags_NoTooltip, ImVec2{28.0F, 14.0F});
}

void App::draw_status_bar() {
  ImGui::Separator();
  ImGui::TextColored(kGood, "●");
  ImGui::SameLine();
  ImGui::TextDisabled("Live telemetry");
  ImGui::SameLine();
  ImGui::TextDisabled(" |  500 ms refresh  |  %zu volume(s)  |  SDL3 + OpenGL + Dear ImGui",
                      dynamic_info_.disks.size());
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
    ImGui::TextWrapped("A small C++ desktop application that studies Blender's dense editor layout, dark neutral palette and properties-oriented interaction model.");
    ImGui::Spacing();
    ImGui::BulletText("C++20 application core");
    ImGui::BulletText("SDL3 native window and input layer");
    ImGui::BulletText("OpenGL rendering backend");
    ImGui::BulletText("Dear ImGui immediate-mode UI");
    ImGui::BulletText("Win32 system information collector");
    ImGui::Spacing();
    ImGui::TextDisabled("Independent demonstration project. No Blender source code, logo or bundled assets are included.");
    ImGui::Spacing();
    if (ImGui::Button("Close", ImVec2{120.0F, 0.0F})) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

}  // namespace blender_ui_demo
