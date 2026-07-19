#include "SystemInfo.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <winternl.h>
#else
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif

namespace blender_ui_demo {
namespace {

#ifdef _WIN32

std::uint64_t file_time_to_u64(const FILETIME& value) {
  ULARGE_INTEGER integer{};
  integer.LowPart = value.dwLowDateTime;
  integer.HighPart = value.dwHighDateTime;
  return integer.QuadPart;
}

std::string to_utf8(std::wstring_view value) {
  if (value.empty()) {
    return {};
  }

  const int required = WideCharToMultiByte(
      CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
  if (required <= 0) {
    return {};
  }

  std::string result(static_cast<std::size_t>(required), '\0');
  WideCharToMultiByte(CP_UTF8,
                      0,
                      value.data(),
                      static_cast<int>(value.size()),
                      result.data(),
                      required,
                      nullptr,
                      nullptr);
  return result;
}

std::string query_computer_name() {
  std::array<wchar_t, 256> buffer{};
  DWORD size = static_cast<DWORD>(buffer.size());
  return GetComputerNameW(buffer.data(), &size) ? to_utf8({buffer.data(), size}) : "Unknown PC";
}

std::string query_user_name() {
  std::array<wchar_t, 256> buffer{};
  DWORD size = static_cast<DWORD>(buffer.size());
  if (!GetUserNameW(buffer.data(), &size) || size == 0) {
    return "Unknown User";
  }
  return to_utf8({buffer.data(), size - 1});
}

std::string query_cpu_name() {
  std::array<wchar_t, 512> buffer{};
  DWORD size = static_cast<DWORD>(buffer.size() * sizeof(wchar_t));
  const LONG status = RegGetValueW(HKEY_LOCAL_MACHINE,
                                   L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                                   L"ProcessorNameString",
                                   RRF_RT_REG_SZ,
                                   nullptr,
                                   buffer.data(),
                                   &size);
  if (status != ERROR_SUCCESS) {
    return "Unknown processor";
  }
  return to_utf8(buffer.data());
}

std::string query_windows_version() {
  using RtlGetVersionFunction = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
  const auto module = GetModuleHandleW(L"ntdll.dll");
  const auto rtl_get_version = reinterpret_cast<RtlGetVersionFunction>(
      module != nullptr ? GetProcAddress(module, "RtlGetVersion") : nullptr);

  RTL_OSVERSIONINFOW info{};
  info.dwOSVersionInfoSize = sizeof(info);
  if (rtl_get_version == nullptr || rtl_get_version(&info) != 0) {
    return "Microsoft Windows";
  }

  std::ostringstream stream;
  stream << "Windows " << info.dwMajorVersion << '.' << info.dwMinorVersion
         << " (Build " << info.dwBuildNumber << ')';
  return stream.str();
}

std::string query_architecture(WORD architecture) {
  switch (architecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
      return "x86-64";
    case PROCESSOR_ARCHITECTURE_ARM64:
      return "ARM64";
    case PROCESSOR_ARCHITECTURE_INTEL:
      return "x86";
    default:
      return "Unknown";
  }
}

#else

std::string query_host_name() {
  std::array<char, 256> buffer{};
  return gethostname(buffer.data(), buffer.size()) == 0 ? buffer.data() : "Unknown PC";
}

std::string query_user_name() {
  const char* user = std::getenv("USER");
  return user != nullptr ? user : "Unknown User";
}

std::uint64_t query_total_memory() {
  const long pages = sysconf(_SC_PHYS_PAGES);
  const long page_size = sysconf(_SC_PAGE_SIZE);
  if (pages <= 0 || page_size <= 0) {
    return 0;
  }
  return static_cast<std::uint64_t>(pages) * static_cast<std::uint64_t>(page_size);
}

bool read_proc_cpu(std::uint64_t& idle, std::uint64_t& total) {
  std::ifstream input("/proc/stat");
  std::string label;
  std::uint64_t user = 0;
  std::uint64_t nice = 0;
  std::uint64_t system = 0;
  std::uint64_t idle_value = 0;
  std::uint64_t io_wait = 0;
  std::uint64_t irq = 0;
  std::uint64_t soft_irq = 0;
  std::uint64_t steal = 0;
  if (!(input >> label >> user >> nice >> system >> idle_value >> io_wait >> irq >> soft_irq >> steal)) {
    return false;
  }
  idle = idle_value + io_wait;
  total = user + nice + system + idle_value + io_wait + irq + soft_irq + steal;
  return true;
}

#endif

}  // namespace

SystemMonitor::SystemMonitor() {
#ifdef _WIN32
  SYSTEM_INFO system_info{};
  GetNativeSystemInfo(&system_info);

  MEMORYSTATUSEX memory{};
  memory.dwLength = sizeof(memory);
  GlobalMemoryStatusEx(&memory);

  static_info_.computer_name = query_computer_name();
  static_info_.user_name = query_user_name();
  static_info_.operating_system = query_windows_version();
  static_info_.architecture = query_architecture(system_info.wProcessorArchitecture);
  static_info_.cpu_name = query_cpu_name();
  static_info_.logical_processors = system_info.dwNumberOfProcessors;
  static_info_.total_memory_bytes = memory.ullTotalPhys;

  FILETIME idle{};
  FILETIME kernel{};
  FILETIME user{};
  if (GetSystemTimes(&idle, &kernel, &user)) {
    previous_idle_ = file_time_to_u64(idle);
    previous_kernel_ = file_time_to_u64(kernel);
    previous_user_ = file_time_to_u64(user);
  }
#else
  utsname system_name{};
  uname(&system_name);

  static_info_.computer_name = query_host_name();
  static_info_.user_name = query_user_name();
  static_info_.operating_system = std::string(system_name.sysname) + " " + system_name.release;
  static_info_.architecture = system_name.machine;
  static_info_.cpu_name = "System processor";
  static_info_.logical_processors = std::max(1u, std::thread::hardware_concurrency());
  static_info_.total_memory_bytes = query_total_memory();
  read_proc_cpu(previous_idle_, previous_total_);
#endif
}

const StaticSystemInfo& SystemMonitor::static_info() const noexcept {
  return static_info_;
}

DynamicSystemInfo SystemMonitor::sample() {
  DynamicSystemInfo result{};

#ifdef _WIN32
  FILETIME idle{};
  FILETIME kernel{};
  FILETIME user{};
  if (GetSystemTimes(&idle, &kernel, &user)) {
    const std::uint64_t idle_now = file_time_to_u64(idle);
    const std::uint64_t kernel_now = file_time_to_u64(kernel);
    const std::uint64_t user_now = file_time_to_u64(user);
    const std::uint64_t idle_delta = idle_now - previous_idle_;
    const std::uint64_t total_delta = (kernel_now - previous_kernel_) + (user_now - previous_user_);
    if (total_delta > 0) {
      result.cpu_usage_percent = 100.0 * static_cast<double>(total_delta - idle_delta) /
                                 static_cast<double>(total_delta);
    }
    previous_idle_ = idle_now;
    previous_kernel_ = kernel_now;
    previous_user_ = user_now;
  }

  MEMORYSTATUSEX memory{};
  memory.dwLength = sizeof(memory);
  if (GlobalMemoryStatusEx(&memory)) {
    result.available_memory_bytes = memory.ullAvailPhys;
    result.used_memory_bytes = memory.ullTotalPhys - memory.ullAvailPhys;
    result.memory_usage_percent = static_cast<double>(memory.dwMemoryLoad);
  }

  result.uptime_seconds = GetTickCount64() / 1000ULL;

  const DWORD drive_mask = GetLogicalDrives();
  for (int index = 0; index < 26; ++index) {
    if ((drive_mask & (1UL << index)) == 0) {
      continue;
    }

    wchar_t root[] = L"A:\\";
    root[0] = static_cast<wchar_t>(L'A' + index);
    const UINT type = GetDriveTypeW(root);
    if (type != DRIVE_FIXED && type != DRIVE_REMOVABLE) {
      continue;
    }

    ULARGE_INTEGER available{};
    ULARGE_INTEGER total{};
    ULARGE_INTEGER free{};
    if (!GetDiskFreeSpaceExW(root, &available, &total, &free)) {
      continue;
    }

    std::array<wchar_t, 64> file_system{};
    GetVolumeInformationW(root,
                          nullptr,
                          0,
                          nullptr,
                          nullptr,
                          nullptr,
                          file_system.data(),
                          static_cast<DWORD>(file_system.size()));

    DiskInfo disk{};
    disk.name = to_utf8(root);
    disk.file_system = file_system[0] != L'\0' ? to_utf8(file_system.data()) : "Unknown";
    disk.total_bytes = total.QuadPart;
    disk.free_bytes = free.QuadPart;
    result.disks.push_back(std::move(disk));
  }
#else
  std::uint64_t idle_now = 0;
  std::uint64_t total_now = 0;
  if (read_proc_cpu(idle_now, total_now)) {
    const std::uint64_t idle_delta = idle_now - previous_idle_;
    const std::uint64_t total_delta = total_now - previous_total_;
    if (total_delta > 0) {
      result.cpu_usage_percent = 100.0 * static_cast<double>(total_delta - idle_delta) /
                                 static_cast<double>(total_delta);
    }
    previous_idle_ = idle_now;
    previous_total_ = total_now;
  }

  std::ifstream memory_file("/proc/meminfo");
  std::string key;
  std::uint64_t value = 0;
  std::string unit;
  std::uint64_t total_kb = 0;
  std::uint64_t available_kb = 0;
  while (memory_file >> key >> value >> unit) {
    if (key == "MemTotal:") {
      total_kb = value;
    } else if (key == "MemAvailable:") {
      available_kb = value;
    }
  }
  if (total_kb > 0) {
    result.available_memory_bytes = available_kb * 1024ULL;
    result.used_memory_bytes = (total_kb - available_kb) * 1024ULL;
    result.memory_usage_percent = 100.0 * static_cast<double>(total_kb - available_kb) /
                                  static_cast<double>(total_kb);
  }

  std::ifstream uptime_file("/proc/uptime");
  double uptime = 0.0;
  if (uptime_file >> uptime) {
    result.uptime_seconds = static_cast<std::uint64_t>(uptime);
  }

  statvfs disk_status{};
  if (statvfs("/", &disk_status) == 0) {
    DiskInfo disk{};
    disk.name = "/";
    disk.file_system = "System";
    disk.total_bytes = static_cast<std::uint64_t>(disk_status.f_blocks) * disk_status.f_frsize;
    disk.free_bytes = static_cast<std::uint64_t>(disk_status.f_bavail) * disk_status.f_frsize;
    result.disks.push_back(std::move(disk));
  }
#endif

  result.cpu_usage_percent = std::clamp(result.cpu_usage_percent, 0.0, 100.0);
  result.memory_usage_percent = std::clamp(result.memory_usage_percent, 0.0, 100.0);
  return result;
}

std::string format_bytes(std::uint64_t bytes) {
  constexpr std::array<const char*, 5> units{"B", "KB", "MB", "GB", "TB"};
  double value = static_cast<double>(bytes);
  std::size_t unit = 0;
  while (value >= 1024.0 && unit + 1 < units.size()) {
    value /= 1024.0;
    ++unit;
  }

  std::ostringstream stream;
  stream << std::fixed << std::setprecision(unit == 0 ? 0 : 1) << value << ' ' << units[unit];
  return stream.str();
}

std::string format_duration(std::uint64_t seconds) {
  const std::uint64_t days = seconds / 86400ULL;
  seconds %= 86400ULL;
  const std::uint64_t hours = seconds / 3600ULL;
  seconds %= 3600ULL;
  const std::uint64_t minutes = seconds / 60ULL;

  std::ostringstream stream;
  if (days > 0) {
    stream << days << "d ";
  }
  stream << hours << "h " << minutes << "m";
  return stream.str();
}

}  // namespace blender_ui_demo
