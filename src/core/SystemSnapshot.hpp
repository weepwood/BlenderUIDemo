#pragma once

#include "core/Provider.hpp"

#include <chrono>
#include <cstdint>
#include <vector>

namespace blender_ui_demo {

struct SystemSnapshot {
  std::uint64_t sequence = 0;
  std::chrono::system_clock::time_point captured_at{};
  std::chrono::steady_clock::time_point published_at{};
  DynamicSystemInfo dynamic;
  std::vector<ProviderRuntimeState> providers;
};

}  // namespace blender_ui_demo
