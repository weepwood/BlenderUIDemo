#pragma once

#include "SystemInfo.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace blender_ui_demo {

enum class ProviderId {
  LegacySystem,
};

enum class ProviderCadence {
  Fast,
  FastAndSlow,
};

enum class ProviderStatus {
  Idle,
  Running,
  Healthy,
  Degraded,
  Failed,
  Stopped,
};

struct ProviderDescriptor {
  ProviderId id = ProviderId::LegacySystem;
  std::string name;
  std::string description;
  ProviderCadence cadence = ProviderCadence::Fast;
  bool requires_elevation = false;
};

struct ProviderRuntimeState {
  ProviderDescriptor descriptor;
  ProviderStatus status = ProviderStatus::Idle;
  double last_collection_ms = 0.0;
  std::uint64_t completed_samples = 0;
  std::string last_error;
};

class IDataProvider {
 public:
  virtual ~IDataProvider() = default;

  [[nodiscard]] virtual const ProviderDescriptor& descriptor() const noexcept = 0;
  [[nodiscard]] virtual const StaticSystemInfo& static_info() const noexcept = 0;
  [[nodiscard]] virtual DynamicSystemInfo collect(bool refresh_slow_data) = 0;
};

struct ProviderCollectionResult {
  DynamicSystemInfo dynamic;
  std::vector<ProviderRuntimeState> providers;
  bool has_dynamic_data = false;
};

[[nodiscard]] const char* provider_status_name(ProviderStatus status) noexcept;

}  // namespace blender_ui_demo
