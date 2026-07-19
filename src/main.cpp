#include "App.hpp"

#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

int SDLCALL run_application(int, char**) {
  blender_ui_demo::App app;
  return app.run();
}

}  // namespace

#ifdef _WIN32

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
  return SDL_RunApp(0, nullptr, run_application, nullptr);
}

#else

int main(int argc, char** argv) {
  return SDL_RunApp(argc, argv, run_application, nullptr);
}

#endif
