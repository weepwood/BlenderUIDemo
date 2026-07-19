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
#include <utility>

#ifdef _WIN32
#include <dxgi1_2.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <windows.h>
#include <winternl.h>
#include <ws2tcpip.h>
#else
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif

namespace blender_ui_demo {
namespace {

using Clock = std::chrono::steady_clock;

[[nodiscard]] double elapsed_ms(const Clock::time_point start, const Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

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
  return status == ERROR_SUCCESS ? to_utf8(buffer.data()) : "Unknown processor";
}

std::string query_windows_version() {
  using RtlGetVersionFunction = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
  const HMODULE module = GetModuleHandleW(L"ntdll.dll");
  RtlGetVersionFunction rtl_get_version = nullptr;
  if (module != nullptr) {
    const FARPROC address = GetProcAddress(module, "RtlGetVersion");
    static_assert(sizeof(address) == sizeof(rtl_get_version));
    std::memcpy(&rtl_get_version, &address, sizeof(address));
  }

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

std::pair<std::string, std::uint64_t> query_gpu() {
  IDXGIFactory1* factory = nullptr;
  if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    return {"Unavailable", 0};
  }

  std::string best_name = "Unavailable";
  std::uint64_t best_memory = 0;
  for (UINT index = 0;; ++index) {
    IDXGIAdapter1* adapter = nullptr;
    if (factory->EnumAdapters1(index, &adapter) == DXGI_ERROR_NOT_FOUND) {
      break;
    }
    if (adapter == nullptr) {
      continue;
    }

    DXGI_ADAPTER_DESC1 description{};
    if (SUCCEEDED(adapter->GetDesc1(&description)) &&
        (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
        description.DedicatedVideoMemory >= best_memory) {
      best_name = to_utf8(description.Description);
      best_memory = static_cast<std::uint64_t>(description.DedicatedVideoMemory);
    }
    adapter->Release();
  }
  factory->Release();
  return {best_name, best_memory};
}

std::vector<DiskInfo> query_disks() {
  std::vector<DiskInfo> disks;
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
    disks.push_back(std::move(disk));
  }
  return disks;
}

std::unordered_map<ULONG, std::string> query_ipv4_addresses() {
  std::unordered_map<ULONG, std::string> addresses;
  ULONG buffer_size = 16 * 1024;
  std::vector<unsigned char> buffer(buffer_size);
  auto* adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
  ULONG status = GetAdaptersAddresses(AF_INET,
                                      GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                                          GAA_FLAG_SKIP_DNS_SERVER,
                                      nullptr,
                                      adapters,
                                      &buffer_size);
  if (status == ERROR_BUFFER_OVERFLOW) {
    buffer.resize(buffer_size);
    adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    status = GetAdaptersAddresses(AF_INET,
                                  GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                                      GAA_FLAG_SKIP_DNS_SERVER,
                                  nullptr,
                                  adapters,
                                  &buffer_size);
  }
  if (status != NO_ERROR) {
    return addresses;
  }

  for (auto* adapter = adapters; adapter != nullptr; adapter = adapter->Next) {
    for (auto* address = adapter->FirstUnicastAddress; address != nullptr; address = address->Next) {
      if (address->Address.lpSockaddr == nullptr ||
          address->Address.lpSockaddr->sa_family != AF_INET) {
        continue;
      }
      const auto* ipv4 = reinterpret_cast<const sockaddr_in*>(address->Address.lpSockaddr);
      std::array<char, INET_ADDRSTRLEN> text{};
      if (InetNtopA(AF_INET, &ipv4->sin_addr, text.data(), static_cast<DWORD>(text.size())) != nullptr) {
        addresses.emplace(adapter->IfIndex, text.data());
        break;
      }
    }
  }
  return addresses;
}

std::pair<std::vector<ProcessInfo>, std::size_t> query_processes() {
  std::vector<ProcessInfo> processes;
  std::size_t process_count = 0;
  const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    return {processes, process_count};
  }

  PROCESSENTRY32W entry{};
  entry.dwSize = sizeof(entry);
  if (Process32FirstW(snapshot, &entry)) {
    do {
      ++process_count;
      PROCESS_MEMORY_COUNTERS_EX memory{};
      memory.cb = sizeof(memory);
      const HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                                         FALSE,
                                         entry.th32ProcessID);
      if (process != nullptr) {
        if (GetProcessMemoryInfo(process,
                                 reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),
                                 sizeof(memory))) {
          ProcessInfo info{};
          info.process_id = entry.th32ProcessID;
          info.name = to_utf8(entry.szExeFile);
          info.thread_count = entry.cntThreads;
          info.working_set_bytes = static_cast<std::uint64_t>(memory.WorkingSetSize);
          processes.push_back(std::move(info));
        }
        CloseHandle(process);
      }
    } while (Process32NextW(snapshot, &entry));
  }
  CloseHandle(snapshot);

  std::sort(processes.begin(), processes.end(), [](const ProcessInfo& left, const ProcessInfo& right) {
    return left.working_set_bytes > right.working_set_bytes;
  });
  if (processes.size() > 15) {
    processes.resize(15);
  }
  return {processes, process_count};
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
#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
  const long pages = sysconf(_SC_PHYS_PAGES);
  const long page_size = sysconf(_SC_PAGE_SIZE);
  if (pages > 0 && page_size > 0) {
    return static_cast<std::uint64_t>(pages) * static_cast<std::uint64_t>(page_size);
  }
#endif
  return 0;
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
  const auto [gpu_name, gpu_memory] = query_gpu();
  static_info_.gpu_name = gpu_name;
  static_info_.gpu_dedicated_memory_bytes = gpu_memory;

  FILETIME idle{};
  FILETIME kernel{};
  FILETIME user{};
  if (GetSystemTimes(&idle, &kernel, &user)) {
    previous_idle_ = file_time_to_u64(idle);
    previous_kernel_ = file_time_to_u64(kernel);
    previous_user_ = file_time_to_u64(user);
  }

  FILETIME creation{};
  FILETIME exit{};
  FILETIME process_kernel{};
  FILETIME process_user{};
  if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &process_kernel, &process_user)) {
    previous_process_kernel_ = file_time_to_u64(process_kernel);
    previous_process_user_ = file_time_to_u64(process_user);
  }
  previous_process_sample_ = Clock::now();
  previous_network_sample_ = Clock::now();
#else
  utsname system_name{};
  uname(&system_name);

  static_info_.computer_name = query_host_name();
  static_info_.user_name = query_user_name();
  static_info_.operating_system = std::string(system_name.sysname) + " " + system_name.release;
  static_info_.architecture = system_name.machine;
  static_info_.cpu_name = "System processor";
  static_info_.gpu_name = "Unavailable";
  static_info_.logical_processors = std::max(1u, std::thread::hardware_concurrency());
  static_info_.total_memory_bytes = query_total_memory();
  read_proc_cpu(previous_idle_, previous_total_);
#endif
}

const StaticSystemInfo& SystemMonitor::static_info() const noexcept {
  return static_info_;
}

DynamicSystemInfo SystemMonitor::sample(bool refresh_slow_data) {
  const auto sample_start = Clock::now();
  DynamicSystemInfo result{};

#ifdef _WIN32
  const auto cpu_memory_start = Clock::now();
  FILETIME idle{};
  FILETIME kernel{};
  FILETIME user{};
  if (GetSystemTimes(&idle, &kernel, &user)) {
    const std::uint64_t idle_now = file_time_to_u64(idle);
    const std::uint64_t kernel_now = file_time_to_u64(kernel);
    const std::uint64_t user_now = file_time_to_u64(user);
    const std::uint64_t idle_delta = idle_now - previous_idle_;
    const std::uint64_t total_delta = (kernel_now - previous_kernel_) + (user_now - previous_user_);
    if (total_delta > 0 && total_delta >= idle_delta) {
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

  const auto process_sample_now = Clock::now();
  FILETIME creation{};
  FILETIME exit{};
  FILETIME process_kernel{};
  FILETIME process_user{};
  if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &process_kernel, &process_user)) {
    const std::uint64_t kernel_now = file_time_to_u64(process_kernel);
    const std::uint64_t user_now = file_time_to_u64(process_user);
    const std::uint64_t process_delta = (kernel_now - previous_process_kernel_) +
                                        (user_now - previous_process_user_);
    const double seconds = std::chrono::duration<double>(process_sample_now - previous_process_sample_).count();
    if (seconds > 0.0 && static_info_.logical_processors > 0) {
      result.application_cpu_percent =
          100.0 * static_cast<double>(process_delta) /
          (seconds * 10'000'000.0 * static_cast<double>(static_info_.logical_processors));
    }
    previous_process_kernel_ = kernel_now;
    previous_process_user_ = user_now;
    previous_process_sample_ = process_sample_now;
  }

  PROCESS_MEMORY_COUNTERS_EX process_memory{};
  process_memory.cb = sizeof(process_memory);
  if (GetProcessMemoryInfo(GetCurrentProcess(),
                           reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&process_memory),
                           sizeof(process_memory))) {
    result.application_working_set_bytes = static_cast<std::uint64_t>(process_memory.WorkingSetSize);
    result.application_private_bytes = static_cast<std::uint64_t>(process_memory.PrivateUsage);
  }
  const auto cpu_memory_end = Clock::now();
  result.collector_timings.cpu_memory_ms = elapsed_ms(cpu_memory_start, cpu_memory_end);

  const auto network_start = Clock::now();
  const auto ipv4_addresses = query_ipv4_addresses();
  PMIB_IF_TABLE2 interface_table = nullptr;
  const auto network_now = Clock::now();
  const double network_seconds = std::chrono::duration<double>(network_now - previous_network_sample_).count();
  if (GetIfTable2(&interface_table) == NO_ERROR && interface_table != nullptr) {
    result.network_adapters.reserve(interface_table->NumEntries);
    for (ULONG index = 0; index < interface_table->NumEntries; ++index) {
      const MIB_IF_ROW2& row = interface_table->Table[index];
      if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK) {
        continue;
      }

      NetworkAdapterInfo adapter{};
      adapter.name = to_utf8(row.Alias);
      adapter.description = to_utf8(row.Description);
      adapter.connected = row.OperStatus == IfOperStatusUp;
      adapter.received_bytes = row.InOctets;
      adapter.sent_bytes = row.OutOctets;
      const auto address = ipv4_addresses.find(row.InterfaceIndex);
      if (address != ipv4_addresses.end()) {
        adapter.ipv4_address = address->second;
      }

      const std::uint64_t key = row.InterfaceLuid.Value;
      const auto previous = previous_network_counters_.find(key);
      if (previous != previous_network_counters_.end() && network_seconds > 0.0) {
        if (adapter.received_bytes >= previous->second.received_bytes) {
          adapter.receive_bytes_per_second =
              static_cast<double>(adapter.received_bytes - previous->second.received_bytes) / network_seconds;
        }
        if (adapter.sent_bytes >= previous->second.sent_bytes) {
          adapter.send_bytes_per_second =
              static_cast<double>(adapter.sent_bytes - previous->second.sent_bytes) / network_seconds;
        }
      }
      previous_network_counters_[key] = {adapter.received_bytes, adapter.sent_bytes};
      result.network_adapters.push_back(std::move(adapter));
    }
    FreeMibTable(interface_table);
  }
  previous_network_sample_ = network_now;
  std::sort(result.network_adapters.begin(),
            result.network_adapters.end(),
            [](const NetworkAdapterInfo& left, const NetworkAdapterInfo& right) {
              if (left.connected != right.connected) {
                return left.connected > right.connected;
              }
              return left.receive_bytes_per_second + left.send_bytes_per_second >
                     right.receive_bytes_per_second + right.send_bytes_per_second;
            });
  const auto network_end = Clock::now();
  result.collector_timings.network_ms = elapsed_ms(network_start, network_end);

  if (refresh_slow_data || cached_disks_.empty()) {
    result.collector_timings.slow_refresh_performed = true;
    const auto storage_start = Clock::now();
    cached_disks_ = query_disks();
    const auto storage_end = Clock::now();
    result.collector_timings.storage_ms = elapsed_ms(storage_start, storage_end);

    const auto processes_start = Clock::now();
    auto [processes, process_count] = query_processes();
    cached_processes_ = std::move(processes);
    cached_process_count_ = process_count;
    const auto processes_end = Clock::now();
    result.collector_timings.processes_ms = elapsed_ms(processes_start, processes_end);
  }
#else
  const auto cpu_memory_start = Clock::now();
  std::uint64_t idle_now = 0;
  std::uint64_t total_now = 0;
  if (read_proc_cpu(idle_now, total_now)) {
    const std::uint64_t idle_delta = idle_now - previous_idle_;
    const std::uint64_t total_delta = total_now - previous_total_;
    if (total_delta > 0 && total_delta >= idle_delta) {
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
  const auto cpu_memory_end = Clock::now();
  result.collector_timings.cpu_memory_ms = elapsed_ms(cpu_memory_start, cpu_memory_end);

  if (refresh_slow_data || cached_disks_.empty()) {
    result.collector_timings.slow_refresh_performed = true;
    const auto storage_start = Clock::now();
    struct statvfs disk_status {};
    if (statvfs("/", &disk_status) == 0) {
      DiskInfo disk{};
      disk.name = "/";
      disk.file_system = "System";
      disk.total_bytes = static_cast<std::uint64_t>(disk_status.f_blocks) * disk_status.f_frsize;
      disk.free_bytes = static_cast<std::uint64_t>(disk_status.f_bavail) * disk_status.f_frsize;
      cached_disks_ = {disk};
    }
    result.collector_timings.storage_ms = elapsed_ms(storage_start, Clock::now());
  }
#endif

  result.disks = cached_disks_;
  result.top_processes = cached_processes_;
  result.process_count = cached_process_count_;
  result.cpu_usage_percent = std::clamp(result.cpu_usage_percent, 0.0, 100.0);
  result.memory_usage_percent = std::clamp(result.memory_usage_percent, 0.0, 100.0);
  result.application_cpu_percent = std::clamp(result.application_cpu_percent, 0.0, 100.0);
  result.collector_timings.total_ms = elapsed_ms(sample_start, Clock::now());
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
  stream << std::setfill('0') << std::setw(2) << hours << ':' << std::setw(2) << minutes;
  return stream.str();
}

std::string format_rate(double bytes_per_second) {
  if (bytes_per_second < 0.0) {
    bytes_per_second = 0.0;
  }
  return format_bytes(static_cast<std::uint64_t>(bytes_per_second)) + "/s";
}

}  // namespace blender_ui_demo
