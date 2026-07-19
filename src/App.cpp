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
#include <cstring>
#include <filesystem>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace blender_ui_demo {
namespace {

using Clock = std::chrono::steady_clock;

float g_ui_scale = 1.0F;

constexpr ImVec4 kAccent{0.96F, 0.43F, 0.10F, 1.0F};
constexpr ImVec4 kAccentSoft{0.34F, 0.17F, 0.08F, 1.0F};
constexpr ImVec4 kGood{0.28F, 0.76F, 0.50F, 1.0F};
constexpr ImVec4 kWarning{0.96F, 0.68F, 0.22F, 1.0F};
constexpr ImVec4 kDanger{0.92F, 0.31F, 0.30F, 1.0F};
constexpr ImVec4 kInfo{0.31F, 0.62F, 0.94F, 1.0F};

[[nodiscard]] float ui(float value) {
  return value * g_ui_scale;
}

[[nodiscard]] ImVec2 ui_size(float x, float y) {
  return ImVec2{ui(x), ui(y)};
}

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
  ImGui::SameLine(0.0F, ui(5.0F));
  ImGui::TextUnformatted(label);
}

void metric_card(const char* id,
                 const char* title,
                 const std::string& value,
                 const std::string& detail,
                 double progress_percent = -1.0,
                 ImVec4 accent = kAccent) {
  ImGui::PushID(id);
  const ImVec2 size{ImGui::GetContentRegionAvail().x, ui(118.0F)};
  ImGui::BeginChild("metric", size, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
  const ImVec2 window_position = ImGui::GetWindowPos();
  const ImVec2 window_size = ImGui::GetWindowSize();
  ImGui::GetWindowDrawList()->AddRectFilled(
      window_position,
      ImVec2{window_position.x + window_size.x, window_position.y + ui(3.0F)},
      ImGui::ColorConvertFloat4ToU32(accent));
  ImGui::Dummy(ui_size(0.0F, 2.0F));
  ImGui::TextDisabled("%s", title);
  ImGui::Spacing();
  ImGui::SetWindowFontScale(1.30F);
  ImGui::TextUnformatted(value.c_str());
  ImGui::SetWindowFontScale(1.0F);
  ImGui::TextDisabled("%s", detail.c_str());
  if (progress_percent >= 0.0) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, usage_color(progress_percent));
    ImGui::ProgressBar(static_cast<float>(progress_percent / 100.0), ImVec2{-1.0F, ui(7.0F)}, "");
    ImGui::PopStyleColor();
  }
  ImGui::EndChild();
  ImGui::PopID();
}

void plot_card(const char* id,
               const char* title,
               const char* subtitle,
               const char* waiting_text,
               const float* values,
               int value_count,
               int offset,
               float minimum,
               float maximum,
               float height = 170.0F) {
  ImGui::PushID(id);
  ImGui::BeginChild("plot_card", ImVec2{0.0F, ui(height)}, ImGuiChildFlags_Borders);
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
                     ImVec2{-1.0F, ui(height - 64.0F)});
  } else {
    ImGui::TextDisabled("%s", waiting_text);
  }
  ImGui::EndChild();
  ImGui::PopID();
}

#ifdef _WIN32
void enable_per_monitor_dpi_awareness() {
  using SetProcessDpiAwarenessContextFunction = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
  const HMODULE user32 = GetModuleHandleW(L"user32.dll");
  if (user32 == nullptr) {
    return;
  }
  const FARPROC address = GetProcAddress(user32, "SetProcessDpiAwarenessContext");
  if (address == nullptr) {
    return;
  }
  SetProcessDpiAwarenessContextFunction function = nullptr;
  static_assert(sizeof(function) == sizeof(address));
  std::memcpy(&function, &address, sizeof(address));
  if (function != nullptr) {
    function(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  }
}
#endif

std::string find_cjk_font() {
  std::vector<std::filesystem::path> candidates;
#ifdef _WIN32
  std::array<wchar_t, MAX_PATH> windows_directory{};
  const UINT length = GetWindowsDirectoryW(windows_directory.data(),
                                           static_cast<UINT>(windows_directory.size()));
  if (length > 0 && length < windows_directory.size()) {
    const std::filesystem::path fonts =
        std::filesystem::path(windows_directory.data()) / L"Fonts";
    candidates.push_back(fonts / L"msyh.ttc");
    candidates.push_back(fonts / L"msyhbd.ttc");
    candidates.push_back(fonts / L"simhei.ttf");
    candidates.push_back(fonts / L"simsun.ttc");
    candidates.push_back(fonts / L"YuGothM.ttc");
    candidates.push_back(fonts / L"meiryo.ttc");
  }
#elif defined(__APPLE__)
  candidates.emplace_back("/System/Library/Fonts/PingFang.ttc");
  candidates.emplace_back("/System/Library/Fonts/STHeiti Light.ttc");
#else
  candidates.emplace_back("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
  candidates.emplace_back("/usr/share/fonts/opentype/noto/NotoSansCJKsc-Regular.otf");
  candidates.emplace_back("/usr/share/fonts/truetype/wqy/wqy-microhei.ttc");
#endif

  std::error_code error;
  for (const auto& candidate : candidates) {
    if (std::filesystem::exists(candidate, error) && !error) {
      return candidate.string();
    }
    error.clear();
  }
  return {};
}

}  // namespace

int App::run() {
#ifdef _WIN32
  enable_per_monitor_dpi_awareness();
#endif

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

  SDL_Window* window = SDL_CreateWindow("Blender UI Demo - 系统工作台",
                                         1500,
                                         940,
                                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                             SDL_WINDOW_HIGH_PIXEL_DENSITY);
  if (window == nullptr) {
    std::fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  SDL_SetWindowMinimumSize(window, 960, 640);

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

  const float initial_scale = SDL_GetWindowDisplayScale(window);
  system_scale_ = initial_scale > 0.0F ? std::clamp(initial_scale, 0.75F, 4.0F) : 1.0F;
  requested_user_scale_ = user_scale_;
  rebuild_ui_scale(false);
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
      } else if (event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED) {
        const float updated_scale = SDL_GetWindowDisplayScale(window);
        if (updated_scale > 0.0F && std::abs(updated_scale - system_scale_) > 0.01F) {
          system_scale_ = std::clamp(updated_scale, 0.75F, 4.0F);
          pending_ui_rebuild_ = true;
        }
      }
    }
    record_module(ModuleSlot::Events, elapsed_ms(events_start, Clock::now()));

    if (request_exit_) {
      break;
    }

    if (pending_ui_rebuild_) {
      rebuild_ui_scale(true);
      pending_ui_rebuild_ = false;
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
  style = ImGuiStyle{};
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
  colors[ImGuiCol_ScrollbarGrab] = ImVec4{0.25F, 0.26F, 0.30F, 1.0F};
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
  style.ScaleAllSizes(effective_scale_);
}

void App::rebuild_ui_scale(bool recreate_font_texture) {
  effective_scale_ = std::clamp(system_scale_ * user_scale_, 0.80F, 3.50F);
  g_ui_scale = effective_scale_;
  apply_blender_theme();

  ImGuiIO& io = ImGui::GetIO();
  if (recreate_font_texture) {
    ImGui_ImplOpenGL3_DestroyFontsTexture();
  }
  io.Fonts->Clear();

  const float font_size = 16.5F * effective_scale_;
  const std::string font_path = find_cjk_font();
  ImFont* font = nullptr;
  if (!font_path.empty()) {
    ImFontConfig config{};
    config.OversampleH = 2;
    config.OversampleV = 2;
    config.PixelSnapH = false;
    font = io.Fonts->AddFontFromFileTTF(font_path.c_str(),
                                        font_size,
                                        &config,
                                        io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
  }

  cjk_font_loaded_ = font != nullptr;
  if (font == nullptr) {
    ImFontConfig fallback{};
    fallback.SizePixels = font_size;
    font = io.Fonts->AddFontDefault(&fallback);
    font_source_ = "Dear ImGui default";
  } else {
    font_source_ = std::filesystem::path(font_path).filename().string();
  }
  io.FontDefault = font;

  if (recreate_font_texture) {
    ImGui_ImplOpenGL3_CreateFontsTexture();
  }
}

void App::initialize_modules() {
  modules_[static_cast<std::size_t>(ModuleSlot::Events)] =
      {"平台与事件", "Platform & Events", "SDL 窗口消息与输入分发", "SDL window messages and input dispatch", 0.0, 0.0, 1.0};
  modules_[static_cast<std::size_t>(ModuleSlot::Sampling)] =
      {"系统遥测", "System Telemetry", "CPU、内存、网络与慢速清单采集", "CPU, memory, network and slow inventory collectors", 0.0, 0.0, 8.0};
  modules_[static_cast<std::size_t>(ModuleSlot::UiComposition)] =
      {"界面构建", "UI Composition", "Dear ImGui 布局与绘制列表生成", "Dear ImGui layout and draw-list generation", 0.0, 0.0, 5.0};
  modules_[static_cast<std::size_t>(ModuleSlot::Rendering)] =
      {"OpenGL 渲染", "OpenGL Renderer", "向 GPU 提交界面绘制命令", "GPU submission for the generated interface", 0.0, 0.0, 5.0};
  modules_[static_cast<std::size_t>(ModuleSlot::BufferSwap)] =
      {"缓冲区交换", "Buffer Swap", "垂直同步等待与前后缓冲区呈现", "VSync wait and front/back buffer presentation", 0.0, 0.0, 18.0};
  modules_[static_cast<std::size_t>(ModuleSlot::FrameThrottle)] =
      {"帧率限制", "Frame Throttle", "为限制帧率而主动执行的 CPU 休眠", "CPU sleep used to enforce the selected FPS cap", 0.0, 0.0, 34.0};
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
                        now - last_slow_sample_ >= std::chrono::milliseconds{slow_refresh_interval_ms_};
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

  const float status_height = ui(30.0F);
  const ImVec2 workspace_size{0.0F, std::max(ui(100.0F), ImGui::GetContentRegionAvail().y - status_height)};
  constexpr ImGuiTableFlags table_flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable |
                                           ImGuiTableFlags_SizingStretchProp;

  if (ImGui::BeginTable("workspace_layout", 3, table_flags, workspace_size)) {
    ImGui::TableSetupColumn("Navigation", ImGuiTableColumnFlags_WidthFixed, ui(220.0F));
    ImGui::TableSetupColumn("Editor", ImGuiTableColumnFlags_WidthStretch, 1.0F);
    ImGui::TableSetupColumn("Properties", ImGuiTableColumnFlags_WidthFixed, ui(325.0F));
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::BeginChild("navigation_panel", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_None);
    draw_navigation();
    ImGui::EndChild();

    ImGui::TableSetColumnIndex(1);
    ImGui::BeginChild("editor_panel", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_None);
    switch (page_) {
      case Page::Overview: draw_overview(); break;
      case Page::Performance: draw_performance(); break;
      case Page::Hardware: draw_hardware(); break;
      case Page::Storage: draw_storage(); break;
      case Page::Network: draw_network(); break;
      case Page::Processes: draw_processes(); break;
      case Page::Diagnostics: draw_diagnostics(); break;
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
  ImGui::SameLine(0.0F, ui(6.0F));
  ImGui::TextUnformatted("Blender UI Demo");
  ImGui::Separator();

  if (ImGui::BeginMenu(text("文件", "File"))) {
    if (ImGui::MenuItem(text("刷新全部数据", "Refresh all"), "F5")) {
      update_metrics(true);
    }
    ImGui::Separator();
    if (ImGui::MenuItem(text("退出", "Exit"), "Alt+F4")) {
      request_exit_ = true;
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu(text("工作区", "Workspace"))) {
    const std::array<Page, 7> pages{Page::Overview, Page::Performance, Page::Hardware, Page::Storage,
                                    Page::Network, Page::Processes, Page::Diagnostics};
    for (const Page page : pages) {
      const Page previous = page_;
      page_ = page;
      const char* label = current_page_name();
      page_ = previous;
      if (ImGui::MenuItem(label, nullptr, page_ == page)) {
        page_ = page;
      }
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu(text("视图", "View"))) {
    ImGui::MenuItem(text("Dear ImGui 演示窗口", "Dear ImGui Demo"), nullptr, &show_imgui_demo_);
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu(text("语言", "Language"))) {
    if (ImGui::MenuItem("简体中文", nullptr, language_ == Language::SimplifiedChinese)) {
      language_ = Language::SimplifiedChinese;
    }
    if (ImGui::MenuItem("English", nullptr, language_ == Language::English)) {
      language_ = Language::English;
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu(text("帮助", "Help"))) {
    if (ImGui::MenuItem(text("关于", "About"))) {
      show_about_ = true;
    }
    ImGui::EndMenu();
  }

  const std::string live = text("应用 ", "APP ") + percent_text(dynamic_info_.application_cpu_percent) +
                           "  |  " + fixed_text(current_fps_, 0) + " FPS";
  const float live_width = ImGui::CalcTextSize(live.c_str()).x;
  ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - live_width - ui(18.0F)));
  ImGui::TextColored(dynamic_info_.application_cpu_percent > 8.0 ? kWarning : kGood,
                     "%s",
                     live.c_str());
  ImGui::EndMenuBar();
}

void App::draw_navigation() {
  ImGui::BeginChild("brand", ImVec2{0.0F, ui(82.0F)}, ImGuiChildFlags_Borders);
  ImGui::TextColored(kAccent, "%s", text("系统实验室", "SYSTEM LAB"));
  ImGui::SetWindowFontScale(1.15F);
  ImGui::TextUnformatted(monitor_.static_info().computer_name.c_str());
  ImGui::SetWindowFontScale(1.0F);
  ImGui::TextDisabled("%s", text("原生系统遥测工作台", "Native telemetry workspace"));
  ImGui::EndChild();
  ImGui::Spacing();

  const auto navigation_item = [this](const char* label, Page page) {
    const bool selected = page_ == page;
    if (selected) {
      ImGui::PushStyleColor(ImGuiCol_Header, kAccentSoft);
    }
    const bool clicked = ImGui::Selectable(label, selected, 0, ImVec2{0.0F, ui(36.0F)});
    if (selected) {
      ImGui::PopStyleColor();
    }
    if (clicked) {
      page_ = page;
    }
  };

  ImGui::TextDisabled("%s", text("监控", "MONITOR"));
  navigation_item(text("  概览", "  Overview"), Page::Overview);
  navigation_item(text("  性能", "  Performance"), Page::Performance);
  ImGui::Spacing();
  ImGui::TextDisabled("%s", text("硬件清单", "INVENTORY"));
  navigation_item(text("  硬件", "  Hardware"), Page::Hardware);
  navigation_item(text("  存储", "  Storage"), Page::Storage);
  navigation_item(text("  网络", "  Network"), Page::Network);
  navigation_item(text("  进程", "  Processes"), Page::Processes);
  ImGui::Spacing();
  ImGui::TextDisabled("%s", text("开发与诊断", "DEVELOPER"));
  navigation_item(text("  诊断", "  Diagnostics"), Page::Diagnostics);

  section_title(text("快捷操作", "QUICK ACTIONS"));
  if (ImGui::Button(text("刷新全部数据##nav_refresh", "Refresh all data##nav_refresh"),
                    ImVec2{-1.0F, ui(36.0F)})) {
    update_metrics(true);
  }
  if (ImGui::Button(text("打开诊断##nav_diag", "Open diagnostics##nav_diag"),
                    ImVec2{-1.0F, ui(36.0F)})) {
    page_ = Page::Diagnostics;
  }

  const float footer_height = ui(76.0F);
  if (ImGui::GetContentRegionAvail().y > footer_height) {
    ImGui::Dummy(ImVec2{0.0F, ImGui::GetContentRegionAvail().y - footer_height});
  }
  ImGui::Separator();
  status_dot(vsync_active_ ? kGood : kWarning,
             vsync_active_ ? text("垂直同步已启用", "VSync active")
                           : text("软件限帧已启用", "Software frame cap"));
  ImGui::TextDisabled("DPI %.0f%%  |  UI %.0f%%", system_scale_ * 100.0F, effective_scale_ * 100.0F);
}

void App::draw_page_header(const char* eyebrow, const char* title, const char* subtitle) {
  ImGui::BeginChild("page_header", ImVec2{0.0F, ui(92.0F)}, ImGuiChildFlags_Borders);
  const ImVec2 position = ImGui::GetWindowPos();
  ImGui::GetWindowDrawList()->AddRectFilled(
      position,
      ImVec2{position.x + ui(4.0F), position.y + ImGui::GetWindowHeight()},
      ImGui::ColorConvertFloat4ToU32(kAccent));
  ImGui::Dummy(ui_size(5.0F, 0.0F));
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
  draw_page_header(text("概览", "OVERVIEW"),
                   text("系统运行概况", "System at a glance"),
                   text("实时查看主机健康、程序开销和硬件清单状态", "Live host health, application cost and inventory status"));

  const int columns = ImGui::GetContentRegionAvail().x >= ui(900.0F) ? 4 : 2;
  if (ImGui::BeginTable("overview_metrics", columns, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    metric_card("system_cpu", text("系统 CPU", "SYSTEM CPU"), percent_text(dynamic_info_.cpu_usage_percent),
                std::to_string(info.logical_processors) + text(" 个逻辑处理器", " logical processors"),
                dynamic_info_.cpu_usage_percent, kAccent);
    ImGui::TableNextColumn();
    metric_card("memory", text("内存", "MEMORY"), percent_text(dynamic_info_.memory_usage_percent),
                format_bytes(dynamic_info_.used_memory_bytes) + text(" / 共 ", " of ") + format_bytes(info.total_memory_bytes),
                dynamic_info_.memory_usage_percent, kInfo);
    ImGui::TableNextColumn();
    metric_card("app_cpu", text("当前程序", "THIS APPLICATION"), percent_text(dynamic_info_.application_cpu_percent),
                format_bytes(dynamic_info_.application_working_set_bytes) + text(" 工作集", " working set"),
                dynamic_info_.application_cpu_percent,
                dynamic_info_.application_cpu_percent > 8.0 ? kWarning : kGood);
    ImGui::TableNextColumn();
    metric_card("network", text("实时网络", "NETWORK NOW"), format_rate(total_receive_rate()),
                format_rate(total_send_rate()) + text(" 上传", " upload"), -1.0, kGood);
    ImGui::EndTable();
  }

  section_title(text("实时趋势", "LIVE HISTORY"));
  if (ImGui::BeginTable("overview_charts", 2, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    plot_card("system_chart", text("系统负载", "System load"),
              text("最近采样窗口中的 CPU 使用率", "CPU percentage across the recent sampling window"),
              text("正在等待采样数据……", "Waiting for samples..."), cpu_history_.data(), history_count_, history_offset_, 0.0F, 100.0F);
    ImGui::TableNextColumn();
    plot_card("app_chart", text("程序 CPU", "Application CPU"),
              text("按全部逻辑处理器归一化", "Normalized across all logical processors"),
              text("正在等待采样数据……", "Waiting for samples..."), app_cpu_history_.data(), history_count_, history_offset_, 0.0F,
              std::max(15.0F, *std::max_element(app_cpu_history_.begin(), app_cpu_history_.end()) + 5.0F));
    ImGui::EndTable();
  }

  section_title(text("健康摘要", "HEALTH SUMMARY"));
  ImGui::BeginChild("health_summary", ImVec2{0.0F, ui(126.0F)}, ImGuiChildFlags_Borders);
  if (dynamic_info_.application_cpu_percent < 5.0) {
    status_dot(kGood, text("当前程序开销较低", "Application overhead is currently low"));
  } else if (dynamic_info_.application_cpu_percent < 10.0) {
    status_dot(kWarning, text("当前程序开销中等", "Application overhead is moderate"));
  } else {
    status_dot(kDanger, text("当前程序开销需要关注", "Application overhead needs attention"));
  }
  for (const auto& reason : diagnose_cpu_usage()) {
    ImGui::BulletText("%s", reason.c_str());
  }
  ImGui::EndChild();
}

void App::draw_performance() {
  draw_page_header(text("性能", "PERFORMANCE"),
                   text("帧率与资源开销", "Frame pacing and resource cost"),
                   text("调节刷新频率并分析程序自身开销", "Tune rendering frequency and inspect application overhead"));

  const int columns = ImGui::GetContentRegionAvail().x >= ui(900.0F) ? 4 : 2;
  if (ImGui::BeginTable("performance_metrics", columns, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    metric_card("perf_app_cpu", text("程序 CPU", "APP CPU"), percent_text(dynamic_info_.application_cpu_percent),
                text("当前进程", "Current process"), dynamic_info_.application_cpu_percent, kAccent);
    ImGui::TableNextColumn();
    metric_card("perf_memory", text("程序内存", "APP MEMORY"), format_bytes(dynamic_info_.application_working_set_bytes),
                format_bytes(dynamic_info_.application_private_bytes) + text(" 私有内存", " private"), -1.0, kInfo);
    ImGui::TableNextColumn();
    metric_card("perf_frame", text("帧时间", "FRAME TIME"), milliseconds_text(current_frame_ms_),
                fixed_text(current_fps_, 0) + " FPS", -1.0, kGood);
    ImGui::TableNextColumn();
    metric_card("perf_sample", text("采集器", "COLLECTOR"), milliseconds_text(dynamic_info_.collector_timings.total_ms),
                std::to_string(sampling_interval_ms_) + text(" ms 间隔", " ms interval"), -1.0, kWarning);
    ImGui::EndTable();
  }

  section_title(text("帧率策略", "FRAME PACING"));
  ImGui::BeginChild("frame_pacing", ImVec2{0.0F, ui(116.0F)}, ImGuiChildFlags_Borders);
  ImGui::TextUnformatted(text("前台帧率上限", "Foreground frame-rate cap"));
  ImGui::SameLine();
  ImGui::RadioButton("15 FPS", &target_fps_, 15);
  ImGui::SameLine();
  ImGui::RadioButton("30 FPS", &target_fps_, 30);
  ImGui::SameLine();
  ImGui::RadioButton("60 FPS", &target_fps_, 60);
  ImGui::Checkbox(text("窗口失去焦点时降至 10 FPS", "Reduce to 10 FPS when the window loses focus"), &adaptive_throttle_);
  ImGui::TextDisabled("%s", text("系统面板数据变化较慢，推荐使用 30 FPS。", "30 FPS is recommended because dashboard data changes slowly."));
  ImGui::EndChild();

  section_title(text("性能趋势", "PERFORMANCE HISTORY"));
  if (ImGui::BeginTable("performance_charts", 2, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    plot_card("frame_history", text("帧时间", "Frame time"),
              text("数值越低越好；30 FPS 对应约 33.3 ms", "Lower is better; 33.3 ms corresponds to 30 FPS"),
              text("正在等待采样数据……", "Waiting for samples..."), frame_time_history_.data(), history_count_, history_offset_, 0.0F, 70.0F, 190.0F);
    ImGui::TableNextColumn();
    plot_card("memory_history", text("系统内存", "System memory"),
              text("物理内存使用率", "Physical memory utilization"),
              text("正在等待采样数据……", "Waiting for samples..."), memory_history_.data(), history_count_, history_offset_, 0.0F, 100.0F, 190.0F);
    ImGui::EndTable();
  }
}

void App::draw_hardware() {
  const auto& info = monitor_.static_info();
  draw_page_header(text("硬件", "HARDWARE"), text("计算机硬件清单", "Computer inventory"),
                   text("处理器、显卡、内存与操作系统信息", "Processor, graphics, memory and operating-system identity"));

  ImGui::BeginChild("hardware_content", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_Borders);
  section_title(text("处理器", "PROCESSOR"));
  if (ImGui::BeginTable("processor_properties", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
    ImGui::TableSetupColumn(text("属性", "Property"), ImGuiTableColumnFlags_WidthFixed, ui(190.0F));
    ImGui::TableSetupColumn(text("值", "Value"), ImGuiTableColumnFlags_WidthStretch);
    property_row(text("型号", "Model"), info.cpu_name);
    property_row(text("架构", "Architecture"), info.architecture);
    property_row(text("逻辑处理器", "Logical processors"), std::to_string(info.logical_processors));
    property_row(text("当前系统负载", "Current system load"), percent_text(dynamic_info_.cpu_usage_percent));
    ImGui::EndTable();
  }

  section_title(text("图形设备", "GRAPHICS"));
  if (ImGui::BeginTable("graphics_properties", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
    ImGui::TableSetupColumn(text("属性", "Property"), ImGuiTableColumnFlags_WidthFixed, ui(190.0F));
    ImGui::TableSetupColumn(text("值", "Value"), ImGuiTableColumnFlags_WidthStretch);
    property_row(text("主要显卡", "Primary adapter"), info.gpu_name);
    property_row(text("专用显存", "Dedicated video memory"),
                 info.gpu_dedicated_memory_bytes > 0 ? format_bytes(info.gpu_dedicated_memory_bytes)
                                                     : text("共享或不可用", "Shared or unavailable"));
    property_row(text("界面图形 API", "UI graphics API"), "OpenGL 3.3");
    ImGui::EndTable();
  }

  section_title(text("内存与操作系统", "MEMORY & OPERATING SYSTEM"));
  if (ImGui::BeginTable("system_properties", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
    ImGui::TableSetupColumn(text("属性", "Property"), ImGuiTableColumnFlags_WidthFixed, ui(190.0F));
    ImGui::TableSetupColumn(text("值", "Value"), ImGuiTableColumnFlags_WidthStretch);
    property_row(text("已安装内存", "Installed memory"), format_bytes(info.total_memory_bytes));
    property_row(text("操作系统", "Operating system"), info.operating_system);
    property_row(text("计算机名称", "Computer name"), info.computer_name);
    property_row(text("当前用户", "Signed-in user"), info.user_name);
    property_row(text("系统运行时间", "Session uptime"), format_duration(dynamic_info_.uptime_seconds));
    ImGui::EndTable();
  }
  ImGui::EndChild();
}

void App::draw_storage() {
  draw_page_header(text("存储", "STORAGE"), text("本地磁盘卷", "Local volumes"),
                   text("容量和使用率由慢速清单采集器定期刷新", "Capacity and utilization refreshed by the slow inventory collector"));
  if (dynamic_info_.disks.empty()) {
    ImGui::TextDisabled("%s", text("未检测到可读取的本地磁盘。", "No readable local volumes were detected."));
    return;
  }

  const int columns = ImGui::GetContentRegionAvail().x >= ui(760.0F) ? 2 : 1;
  if (ImGui::BeginTable("disk_grid", columns, ImGuiTableFlags_SizingStretchSame)) {
    for (std::size_t index = 0; index < dynamic_info_.disks.size(); ++index) {
      const auto& disk = dynamic_info_.disks[index];
      const std::uint64_t used = disk.total_bytes - std::min(disk.free_bytes, disk.total_bytes);
      const double used_percent = disk.total_bytes > 0
                                      ? 100.0 * static_cast<double>(used) / static_cast<double>(disk.total_bytes)
                                      : 0.0;
      ImGui::TableNextColumn();
      ImGui::PushID(static_cast<int>(index));
      ImGui::BeginChild("disk", ImVec2{0.0F, ui(152.0F)}, ImGuiChildFlags_Borders);
      ImGui::TextColored(kAccent, "%s", disk.name.c_str());
      ImGui::SameLine();
      ImGui::TextDisabled("%s", disk.file_system.c_str());
      ImGui::SetWindowFontScale(1.18F);
      ImGui::Text("%s", format_bytes(used).c_str());
      ImGui::SetWindowFontScale(1.0F);
      ImGui::TextDisabled("%s", (format_bytes(used) + text(" 已用 / 共 ", " used of ") + format_bytes(disk.total_bytes) +
                                  "  |  " + format_bytes(disk.free_bytes) + text(" 可用", " available")).c_str());
      ImGui::Spacing();
      ImGui::PushStyleColor(ImGuiCol_PlotHistogram, usage_color(used_percent));
      const std::string overlay = percent_text(used_percent);
      ImGui::ProgressBar(static_cast<float>(used_percent / 100.0), ImVec2{-1.0F, ui(18.0F)}, overlay.c_str());
      ImGui::PopStyleColor();
      ImGui::EndChild();
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
}

void App::draw_network() {
  draw_page_header(text("网络", "NETWORK"), text("网络适配器与实时吞吐量", "Adapters and live throughput"),
                   text("通过 Windows IP Helper API 每秒采集一次", "Native IP Helper API counters sampled once per second"));

  if (ImGui::BeginTable("network_metrics", 3, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    metric_card("download", text("下载", "DOWNLOAD"), format_rate(total_receive_rate()), text("汇总接收速率", "Aggregate receive rate"), -1.0, kGood);
    ImGui::TableNextColumn();
    metric_card("upload", text("上传", "UPLOAD"), format_rate(total_send_rate()), text("汇总发送速率", "Aggregate send rate"), -1.0, kInfo);
    ImGui::TableNextColumn();
    const auto connected = std::count_if(dynamic_info_.network_adapters.begin(), dynamic_info_.network_adapters.end(),
                                         [](const NetworkAdapterInfo& adapter) { return adapter.connected; });
    metric_card("adapter_count", text("已连接", "CONNECTED"), std::to_string(connected),
                std::to_string(dynamic_info_.network_adapters.size()) + text(" 个适配器", " adapter(s) detected"), -1.0, kAccent);
    ImGui::EndTable();
  }

  section_title(text("适配器", "ADAPTERS"));
  constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                    ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
  if (ImGui::BeginTable("adapter_table", 6, flags, ImVec2{0.0F, 0.0F})) {
    ImGui::TableSetupColumn(text("状态", "State"), ImGuiTableColumnFlags_WidthFixed, ui(76.0F));
    ImGui::TableSetupColumn(text("适配器", "Adapter"), ImGuiTableColumnFlags_WidthStretch, 1.4F);
    ImGui::TableSetupColumn("IPv4", ImGuiTableColumnFlags_WidthFixed, ui(126.0F));
    ImGui::TableSetupColumn(text("下载", "Down"), ImGuiTableColumnFlags_WidthFixed, ui(100.0F));
    ImGui::TableSetupColumn(text("上传", "Up"), ImGuiTableColumnFlags_WidthFixed, ui(100.0F));
    ImGui::TableSetupColumn(text("累计接收", "Received"), ImGuiTableColumnFlags_WidthFixed, ui(110.0F));
    ImGui::TableHeadersRow();
    for (const auto& adapter : dynamic_info_.network_adapters) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextColored(adapter.connected ? kGood : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "%s",
                         adapter.connected ? text("在线", "Online") : text("离线", "Offline"));
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
  draw_page_header(text("进程", "PROCESSES"), text("内存占用较高的进程", "Top memory consumers"),
                   text("轻量级进程快照每五秒刷新一次", "A lightweight snapshot refreshed every five seconds"));

  if (ImGui::BeginTable("process_metrics", 3, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    metric_card("process_count", text("进程数量", "PROCESS COUNT"), std::to_string(dynamic_info_.process_count), text("当前快照", "Current snapshot"), -1.0, kAccent);
    ImGui::TableNextColumn();
    metric_card("self_working", text("当前程序", "THIS APP"), format_bytes(dynamic_info_.application_working_set_bytes), text("工作集", "Working set"), -1.0, kInfo);
    ImGui::TableNextColumn();
    metric_card("self_private", text("私有内存", "PRIVATE MEMORY"), format_bytes(dynamic_info_.application_private_bytes), text("由当前程序提交", "Committed by this app"), -1.0, kGood);
    ImGui::EndTable();
  }

  section_title(text("按工作集排序的进程", "TOP PROCESSES BY WORKING SET"));
  constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                    ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
  if (ImGui::BeginTable("process_table", 4, flags, ImVec2{0.0F, 0.0F})) {
    ImGui::TableSetupColumn(text("进程", "Process"), ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, ui(82.0F));
    ImGui::TableSetupColumn(text("线程", "Threads"), ImGuiTableColumnFlags_WidthFixed, ui(82.0F));
    ImGui::TableSetupColumn(text("工作集", "Working set"), ImGuiTableColumnFlags_WidthFixed, ui(126.0F));
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
  draw_page_header(text("诊断", "DIAGNOSTICS"), text("程序为什么会占用 CPU？", "Why is the application using CPU?"),
                   text("查看模块耗时、当前原因和已应用的优化", "Per-module timings, active causes and optimizations applied in this build"));

  const int columns = ImGui::GetContentRegionAvail().x >= ui(900.0F) ? 4 : 2;
  if (ImGui::BeginTable("diagnostic_metrics", columns, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableNextColumn();
    metric_card("diag_cpu", text("程序 CPU", "APP CPU"), percent_text(dynamic_info_.application_cpu_percent),
                text("归一化进程占用", "Normalized process usage"), dynamic_info_.application_cpu_percent,
                dynamic_info_.application_cpu_percent > 8.0 ? kWarning : kGood);
    ImGui::TableNextColumn();
    metric_card("diag_fps", text("帧率", "FRAME RATE"), fixed_text(current_fps_, 0) + " FPS", milliseconds_text(current_frame_ms_), -1.0, kInfo);
    ImGui::TableNextColumn();
    metric_card("diag_ui", text("界面构建", "UI BUILD"), milliseconds_text(modules_[static_cast<std::size_t>(ModuleSlot::UiComposition)].average_ms),
                text("每帧平均", "Average per frame"), -1.0, kAccent);
    ImGui::TableNextColumn();
    metric_card("diag_collector", text("采集器", "COLLECTOR"), milliseconds_text(dynamic_info_.collector_timings.total_ms),
                dynamic_info_.collector_timings.slow_refresh_performed ? text("包含慢速清单", "Slow inventory included")
                                                                      : text("仅快速采样", "Fast sample only"),
                -1.0, kWarning);
    ImGui::EndTable();
  }

  section_title(text("当前诊断", "CURRENT DIAGNOSIS"));
  ImGui::BeginChild("diagnosis", ImVec2{0.0F, ui(150.0F)}, ImGuiChildFlags_Borders);
  for (const auto& reason : diagnose_cpu_usage()) {
    ImGui::BulletText("%s", reason.c_str());
  }
  ImGui::EndChild();

  section_title(text("程序模块", "APPLICATION MODULES"));
  constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;
  if (ImGui::BeginTable("module_table", 5, flags)) {
    ImGui::TableSetupColumn(text("模块", "Module"), ImGuiTableColumnFlags_WidthFixed, ui(165.0F));
    ImGui::TableSetupColumn(text("职责", "Role"), ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn(text("最近", "Last"), ImGuiTableColumnFlags_WidthFixed, ui(86.0F));
    ImGui::TableSetupColumn(text("平均", "Average"), ImGuiTableColumnFlags_WidthFixed, ui(86.0F));
    ImGui::TableSetupColumn(text("状态", "State"), ImGuiTableColumnFlags_WidthFixed, ui(82.0F));
    ImGui::TableHeadersRow();
    for (const auto& module : modules_) {
      const bool within_budget = module.average_ms <= module.budget_ms;
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(text(module.name_zh, module.name_en));
      ImGui::TableSetColumnIndex(1);
      ImGui::TextDisabled("%s", text(module.description_zh, module.description_en));
      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(milliseconds_text(module.last_ms).c_str());
      ImGui::TableSetColumnIndex(3);
      ImGui::TextUnformatted(milliseconds_text(module.average_ms).c_str());
      ImGui::TableSetColumnIndex(4);
      ImGui::TextColored(within_budget ? kGood : kWarning, "%s",
                         within_budget ? text("正常", "Healthy") : text("需检查", "Review"));
    }
    ImGui::EndTable();
  }

  section_title(text("采集器耗时", "COLLECTOR BREAKDOWN"));
  if (ImGui::BeginTable("collector_table", 3, flags)) {
    ImGui::TableSetupColumn(text("采集器", "Collector"), ImGuiTableColumnFlags_WidthFixed, ui(190.0F));
    ImGui::TableSetupColumn(text("频率", "Frequency"), ImGuiTableColumnFlags_WidthFixed, ui(160.0F));
    ImGui::TableSetupColumn(text("最近耗时", "Last cost"), ImGuiTableColumnFlags_WidthStretch);
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
    collector_row(text("CPU、内存与当前进程", "CPU, memory & self process"), text("快速采样", "Fast sample"), dynamic_info_.collector_timings.cpu_memory_ms);
    collector_row(text("网络适配器", "Network adapters"), text("快速采样", "Fast sample"), dynamic_info_.collector_timings.network_ms);
    collector_row(text("存储卷", "Storage volumes"), text("慢速采样", "Slow sample"), dynamic_info_.collector_timings.storage_ms);
    collector_row(text("进程清单", "Process inventory"), text("慢速采样", "Slow sample"), dynamic_info_.collector_timings.processes_ms);
    ImGui::EndTable();
  }

  section_title(text("已应用的优化", "OPTIMIZATIONS APPLIED"));
  ImGui::BeginChild("resolved", ImVec2{0.0F, ui(178.0F)}, ImGuiChildFlags_Borders);
  ImGui::BulletText("%s", text("默认前台帧率限制为 30 FPS。", "Default foreground rendering is capped at 30 FPS."));
  ImGui::BulletText("%s", text("显卡驱动拒绝垂直同步时自动使用软件限帧。", "A software limiter is used when the graphics driver rejects VSync."));
  ImGui::BulletText(text("CPU、内存和网络每 %d ms 采集；磁盘和进程每 %d ms 采集。",
                         "CPU, memory and network are sampled every %d ms; disks and processes every %d ms."),
                    sampling_interval_ms_, slow_refresh_interval_ms_);
  ImGui::BulletText("%s", text("历史图表使用固定环形缓冲区，不再逐帧分配数组。", "History charts use fixed circular buffers instead of per-frame allocations."));
  ImGui::BulletText("%s", text("最小化窗口休眠 120 ms，后台窗口降至 10 FPS。", "Minimized windows sleep for 120 ms and unfocused windows fall back to 10 FPS."));
  ImGui::BulletText("%s", text("界面会根据显示器 DPI 自动重建字体和控件尺寸。", "Fonts and controls are rebuilt automatically when monitor DPI changes."));
  ImGui::EndChild();
}

void App::draw_inspector() {
  const auto& info = monitor_.static_info();
  ImGui::TextColored(kAccent, "%s", text("属性", "PROPERTIES"));
  ImGui::SetWindowFontScale(1.12F);
  ImGui::TextUnformatted(current_page_name());
  ImGui::SetWindowFontScale(1.0F);
  ImGui::TextDisabled("%s", text("当前工作区设置", "Active workspace settings"));
  ImGui::Separator();

  section_title(text("界面与语言", "INTERFACE & LANGUAGE"));
  const char* languages[] = {"简体中文", "English"};
  int selected_language = language_ == Language::SimplifiedChinese ? 0 : 1;
  if (ImGui::Combo(text("界面语言##language", "Language##language"), &selected_language, languages, 2)) {
    language_ = selected_language == 0 ? Language::SimplifiedChinese : Language::English;
  }
  ImGui::TextDisabled(text("显示器缩放：%.0f%%", "Display scale: %.0f%%"), system_scale_ * 100.0F);
  ImGui::SliderFloat(text("额外界面缩放##ui_scale", "Additional UI scale##ui_scale"),
                     &requested_user_scale_, 0.80F, 1.80F, "x%.2f", ImGuiSliderFlags_None);
  ImGui::TextDisabled(text("最终缩放：%.0f%%", "Effective scale: %.0f%%"),
                      system_scale_ * requested_user_scale_ * 100.0F);
  if (ImGui::Button(text("应用界面缩放##apply_scale", "Apply UI scale##apply_scale"), ImVec2{-1.0F, ui(36.0F)})) {
    user_scale_ = requested_user_scale_;
    pending_ui_rebuild_ = true;
  }
  ImGui::SameLine();
  if (ImGui::Button(text("重置##reset_scale", "Reset##reset_scale"))) {
    requested_user_scale_ = 1.0F;
    user_scale_ = 1.0F;
    pending_ui_rebuild_ = true;
  }
  const std::string font_status = cjk_font_loaded_
                                      ? std::string(text("中文字体：", "CJK font: ")) + font_source_
                                      : text("未找到系统中文字体，部分中文可能显示为方框。", "No system CJK font was found; Chinese text may be missing.");
  ImGui::TextWrapped("%s", font_status.c_str());

  section_title(text("实时遥测", "LIVE TELEMETRY"));
  if (ImGui::BeginTable("live_inspector", 2, ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn(text("名称", "Name"), ImGuiTableColumnFlags_WidthFixed, ui(118.0F));
    ImGui::TableSetupColumn(text("数值", "Value"), ImGuiTableColumnFlags_WidthStretch);
    property_row(text("系统 CPU", "System CPU"), percent_text(dynamic_info_.cpu_usage_percent));
    property_row(text("当前程序", "Application"), percent_text(dynamic_info_.application_cpu_percent));
    property_row(text("内存", "Memory"), percent_text(dynamic_info_.memory_usage_percent));
    property_row(text("帧率", "Frame rate"), fixed_text(current_fps_, 0) + " FPS");
    property_row(text("帧时间", "Frame time"), milliseconds_text(current_frame_ms_));
    ImGui::EndTable();
  }

  section_title(text("性能策略", "PERFORMANCE POLICY"));
  ImGui::TextDisabled("%s", text("前台帧率上限", "Foreground frame cap"));
  ImGui::RadioButton("15", &target_fps_, 15);
  ImGui::SameLine();
  ImGui::RadioButton("30", &target_fps_, 30);
  ImGui::SameLine();
  ImGui::RadioButton("60", &target_fps_, 60);
  ImGui::Checkbox(text("后台自动降频", "Background throttling"), &adaptive_throttle_);
  ImGui::SliderInt(text("快速采样（ms）", "Fast sample (ms)"), &sampling_interval_ms_, 500, 3000, "%d");
  ImGui::SliderInt(text("慢速采样（ms）", "Slow sample (ms)"), &slow_refresh_interval_ms_, 3000, 15000, "%d");
  if (ImGui::Button(text("应用并刷新##apply_perf", "Apply and refresh##apply_perf"), ImVec2{-1.0F, ui(36.0F)})) {
    update_metrics(true);
  }

  section_title(text("运行状态", "RUNTIME"));
  if (ImGui::BeginTable("runtime_inspector", 2, ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn(text("名称", "Name"), ImGuiTableColumnFlags_WidthFixed, ui(118.0F));
    ImGui::TableSetupColumn(text("数值", "Value"), ImGuiTableColumnFlags_WidthStretch);
    property_row("VSync", vsync_active_ ? text("已启用", "Active") : text("不可用；软件限帧已启用", "Unavailable; limiter active"));
    property_row(text("焦点策略", "Focus policy"), window_focused_ ? text("前台", "Foreground") : text("后台 10 FPS", "Background 10 FPS"));
    property_row(text("采集器耗时", "Collector"), milliseconds_text(dynamic_info_.collector_timings.total_ms));
    property_row(text("主机", "Host"), info.computer_name);
    ImGui::EndTable();
  }

  section_title(text("工具", "TOOLS"));
  if (ImGui::Button(text("打开诊断##tool_diag", "Open diagnostics##tool_diag"), ImVec2{-1.0F, ui(34.0F)})) {
    page_ = Page::Diagnostics;
  }
  if (ImGui::Button(text("关于当前版本##tool_about", "About this build##tool_about"), ImVec2{-1.0F, ui(34.0F)})) {
    show_about_ = true;
  }
}

void App::draw_status_bar() {
  ImGui::Separator();
  ImGui::TextColored(dynamic_info_.application_cpu_percent > 8.0 ? kWarning : kGood, "●");
  ImGui::SameLine();
  ImGui::TextDisabled(text("%s  |  程序 CPU %s  |  %.0f FPS  |  DPI %.0f%%  |  UI %.0f%%  |  %zu 个进程",
                           "%s  |  App CPU %s  |  %.0f FPS  |  DPI %.0f%%  |  UI %.0f%%  |  %zu process(es)"),
                      current_page_name(), percent_text(dynamic_info_.application_cpu_percent).c_str(), current_fps_,
                      system_scale_ * 100.0F, effective_scale_ * 100.0F, dynamic_info_.process_count);
}

void App::draw_about_dialog() {
  const char* popup_title = text("关于 Blender UI Demo##about_dialog", "About Blender UI Demo##about_dialog");
  if (show_about_) {
    ImGui::OpenPopup(popup_title);
    show_about_ = false;
  }

  if (ImGui::BeginPopupModal(popup_title, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextColored(kAccent, "BLENDER UI DEMO");
    ImGui::TextUnformatted(text("原生系统信息工作台", "Native System Information Workspace"));
    ImGui::Separator();
    ImGui::TextWrapped("%s", text("这是一款使用 C++ 构建的原生桌面应用，采用类似 Blender 的高密度编辑器布局，并展示系统遥测与程序自身运行开销。",
                                  "A native C++ desktop application with a Blender-inspired editor layout, system telemetry and self-diagnostics."));
    ImGui::Spacing();
    ImGui::BulletText("%s", text("简体中文与英文界面", "Simplified Chinese and English interface"));
    ImGui::BulletText("%s", text("显示器 DPI 自动缩放与手动比例调节", "Automatic monitor-DPI scaling and manual scale control"));
    ImGui::BulletText("%s", text("Windows 系统中文字体自动发现", "Automatic Windows CJK system-font discovery"));
    ImGui::BulletText("%s", text("SDL3、OpenGL 3.3 与 Dear ImGui", "SDL3, OpenGL 3.3 and Dear ImGui"));
    ImGui::Spacing();
    ImGui::TextDisabled("%s", text("独立演示项目，不包含 Blender 源码、Logo 或官方资源。",
                                   "Independent demonstration project. No Blender source code, logo or bundled assets are included."));
    ImGui::Spacing();
    if (ImGui::Button(text("关闭##about_close", "Close##about_close"), ImVec2{ui(120.0F), 0.0F})) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

const char* App::text(const char* chinese, const char* english) const noexcept {
  return is_chinese() ? chinese : english;
}

bool App::is_chinese() const noexcept {
  return language_ == Language::SimplifiedChinese;
}

std::vector<std::string> App::diagnose_cpu_usage() const {
  std::vector<std::string> reasons;
  if (show_imgui_demo_) {
    reasons.emplace_back(text("Dear ImGui 演示窗口包含大量控件，会增加每帧界面构建开销。",
                              "Dear ImGui's demonstration window adds a large number of widgets every frame."));
  }
  if (!vsync_active_) {
    reasons.emplace_back(text("显卡驱动未启用垂直同步，内置软件限帧正在防止主循环无限运行。",
                              "The graphics driver did not enable VSync; the software frame cap prevents an unbounded loop."));
  }
  if (target_fps_ >= 60) {
    reasons.emplace_back(text("当前选择 60 FPS，界面构建次数约为推荐 30 FPS 模式的两倍。",
                              "The selected 60 FPS mode doubles UI composition work compared with 30 FPS."));
  }

  const auto& ui_module = modules_[static_cast<std::size_t>(ModuleSlot::UiComposition)];
  const auto& render = modules_[static_cast<std::size_t>(ModuleSlot::Rendering)];
  if (ui_module.average_ms > 5.0) {
    reasons.emplace_back(text("界面构建超过 5 ms 预算，可能正在显示大型表格或演示窗口。",
                              "UI composition is above its 5 ms budget; a large table or demo window may be visible."));
  }
  if (render.average_ms > 5.0) {
    reasons.emplace_back(text("OpenGL 提交超过 5 ms，显卡驱动或高 DPI 帧缓冲区可能是主要开销。",
                              "OpenGL submission is above 5 ms; the GPU driver or high-DPI framebuffer is the likely cost."));
  }
  if (effective_scale_ >= 2.0F && render.average_ms > 2.0) {
    reasons.emplace_back(text("当前处于高 DPI 缩放模式，像素数量增加会提高 GPU 绘制与缓冲区交换成本。",
                              "High-DPI mode increases pixel count and can raise GPU rendering and buffer-swap cost."));
  }
  if (dynamic_info_.collector_timings.total_ms > 12.0) {
    reasons.emplace_back(text("系统采集本次耗时较高，可在下方查看磁盘和进程采集明细。",
                              "System collection is temporarily expensive; inspect storage and process timings below."));
  }
  if (dynamic_info_.collector_timings.processes_ms > 20.0) {
    reasons.emplace_back(text("进程清单扫描是本次慢速采样的主要开销，但它只按慢速周期运行。",
                              "The process inventory scan is the dominant slow-sample cost, but runs only at the slow interval."));
  }
  if (dynamic_info_.application_cpu_percent > 8.0 && reasons.empty()) {
    reasons.emplace_back(text("未发现单一异常模块，较高占用可能由帧率、驱动调度和高 DPI 渲染共同造成。",
                              "No single dominant module was found; frame rate, driver scheduling and high-DPI rendering are likely combined causes."));
  }
  if (reasons.empty()) {
    reasons.emplace_back(text("未检测到活动瓶颈，当前 CPU 占用符合已限帧的原生监控界面预期。",
                              "No active bottleneck is detected. CPU usage is consistent with a throttled native monitoring UI."));
  }
  return reasons;
}

double App::total_receive_rate() const {
  return std::accumulate(dynamic_info_.network_adapters.begin(), dynamic_info_.network_adapters.end(), 0.0,
                         [](double total, const NetworkAdapterInfo& adapter) {
                           return total + adapter.receive_bytes_per_second;
                         });
}

double App::total_send_rate() const {
  return std::accumulate(dynamic_info_.network_adapters.begin(), dynamic_info_.network_adapters.end(), 0.0,
                         [](double total, const NetworkAdapterInfo& adapter) {
                           return total + adapter.send_bytes_per_second;
                         });
}

const char* App::current_page_name() const {
  switch (page_) {
    case Page::Overview: return text("概览", "Overview");
    case Page::Performance: return text("性能", "Performance");
    case Page::Hardware: return text("硬件", "Hardware");
    case Page::Storage: return text("存储", "Storage");
    case Page::Network: return text("网络", "Network");
    case Page::Processes: return text("进程", "Processes");
    case Page::Diagnostics: return text("诊断", "Diagnostics");
  }
  return text("概览", "Overview");
}

}  // namespace blender_ui_demo
