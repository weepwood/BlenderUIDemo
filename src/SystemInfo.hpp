#pragma once

#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace blender_ui_demo {

struct DiskInfo {
  std::string name;
  std::string file_system;
  std::uint64_t total_bytes = 0;
  std::uint64_t free_bytes = 0;
};

struct NetworkAdapterInfo {
  std::string name;
  std::string description;
  std::string ipv4_address;
  bool connected = false;
  std::uint64_t received_bytes = 0;
  std::uint64_t sent_bytes = 0;
  double receive_bytes_per_second = 0.0;
  double send_bytes_per_second = 0.0;
};

struct ProcessInfo {
  std::uint32_t process_id = 0;
  std::string name;
  std::uint32_t thread_count = 0;
  std::uint64_t working_set_bytes = 0;
};

struct CollectorTimings {
  double cpu_memory_ms = 0.0;
  double network_ms = 0.0;
  double storage_ms = 0.0;
  double processes_ms = 0.0;
  double total_ms = 0.0;
  bool slow_refresh_performed = false;
};

struct StaticSystemInfo {
  std::string computer_name;
  std::string user_name;
  std::string operating_system;
  std::string architecture;
  std::string cpu_name;
  std::string gpu_name;
  unsigned int logical_processors = 0;
  std::uint64_t total_memory_bytes = 0;
  std::uint64_t gpu_dedicated_memory_bytes = 0;
};

struct DynamicSystemInfo {
  double cpu_usage_percent = 0.0;
  double memory_usage_percent = 0.0;
  std::uint64_t used_memory_bytes = 0;
  std::uint64_t available_memory_bytes = 0;
  std::uint64_t uptime_seconds = 0;

  double application_cpu_percent = 0.0;
  std::uint64_t application_working_set_bytes = 0;
  std::uint64_t application_private_bytes = 0;

  std::size_t process_count = 0;
  std::vector<DiskInfo> disks;
  std::vector<NetworkAdapterInfo> network_adapters;
  std::vector<ProcessInfo> top_processes;
  CollectorTimings collector_timings;
};

class SystemMonitor {
 public:
  SystemMonitor();

  [[nodiscard]] const StaticSystemInfo& static_info() const noexcept;
  [[nodiscard]] DynamicSystemInfo sample(bool refresh_slow_data = false);

 private:
  StaticSystemInfo static_info_;
  std::vector<DiskInfo> cached_disks_;
  std::vector<ProcessInfo> cached_processes_;
  std::size_t cached_process_count_ = 0;

#ifdef _WIN32
  struct NetworkCounter {
    std::uint64_t received_bytes = 0;
    std::uint64_t sent_bytes = 0;
  };

  std::uint64_t previous_idle_ = 0;
  std::uint64_t previous_kernel_ = 0;
  std::uint64_t previous_user_ = 0;
  std::uint64_t previous_process_kernel_ = 0;
  std::uint64_t previous_process_user_ = 0;
  std::chrono::steady_clock::time_point previous_process_sample_{};
  std::chrono::steady_clock::time_point previous_network_sample_{};
  std::unordered_map<std::uint64_t, NetworkCounter> previous_network_counters_;
#else
  std::uint64_t previous_idle_ = 0;
  std::uint64_t previous_total_ = 0;
#endif
};

[[nodiscard]] std::string format_bytes(std::uint64_t bytes);
[[nodiscard]] std::string format_duration(std::uint64_t seconds);
[[nodiscard]] std::string format_rate(double bytes_per_second);

}  // namespace blender_ui_demo
