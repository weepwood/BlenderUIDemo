#include "App.hpp"

#include <SDL3/SDL_main.h>

int main(int, char**) {
  blender_ui_demo::App app;
  return app.run();
}
