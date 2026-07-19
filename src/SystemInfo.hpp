#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace blender_ui_demo {

struct DiskInfo {
  std::string name;
  std::string file_system;
  std::uint64_t total_bytes = 0;
  std::uint64_t free_bytes = 0;
};

struct StaticSystemInfo {
  std::string computer_name;
  std::string user_name;
  std::string operating_system;
  std::string architecture;
  std::string cpu_name;
  unsigned int logical_processors = 0;
  std::uint64_t total_memory_bytes = 0;
};

struct DynamicSystemInfo {
  double cpu_usage_percent = 0.0;
  double memory_usage_percent = 0.0;
  std::uint64_t used_memory_bytes = 0;
  std::uint64_t available_memory_bytes = 0;
  std::uint64_t uptime_seconds = 0;
  std::vector<DiskInfo> disks;
};

class SystemMonitor {
 public:
  SystemMonitor();

  [[nodiscard]] const StaticSystemInfo& static_info() const noexcept;
  [[nodiscard]] DynamicSystemInfo sample();

 private:
  StaticSystemInfo static_info_;

#ifdef _WIN32
  std::uint64_t previous_idle_ = 0;
  std::uint64_t previous_kernel_ = 0;
  std::uint64_t previous_user_ = 0;
#else
  std::uint64_t previous_idle_ = 0;
  std::uint64_t previous_total_ = 0;
#endif
};

[[nodiscard]] std::string format_bytes(std::uint64_t bytes);
[[nodiscard]] std::string format_duration(std::uint64_t seconds);

}  // namespace blender_ui_demo
