cmake_minimum_required(VERSION 3.24)

project(BlenderUIDemo VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.4.8
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.8
  GIT_SHALLOW TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif()

find_package(OpenGL REQUIRED)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC SDL3::SDL3 OpenGL::GL)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

add_executable(BlenderUIDemo
  src/main.cpp
  src/App.cpp
  src/SystemInfo.cpp
)

if(WIN32)
  set_target_properties(BlenderUIDemo PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_include_directories(BlenderUIDemo PRIVATE src)
target_link_libraries(BlenderUIDemo PRIVATE imgui SDL3::SDL3 OpenGL::GL)

target_compile_definitions(BlenderUIDemo PRIVATE
  $<$<PLATFORM_ID:Windows>:NOMINMAX;WIN32_LEAN_AND_MEAN;UNICODE;_UNICODE>
)

if(MSVC)
  target_compile_options(BlenderUIDemo PRIVATE /W4 /permissive- /utf-8)
else()
  target_compile_options(BlenderUIDemo PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
  target_link_libraries(BlenderUIDemo PRIVATE advapi32 iphlpapi ws2_32)
endif()

if(TARGET SDL3::SDL3-shared)
  add_custom_command(TARGET BlenderUIDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:SDL3::SDL3-shared>
      $<TARGET_FILE_DIR:BlenderUIDemo>
  )
endif()

include(GNUInstallDirs)
install(TARGETS BlenderUIDemo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(TARGET SDL3::SDL3-shared)
  install(FILES $<TARGET_FILE:SDL3::SDL3-shared> DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
