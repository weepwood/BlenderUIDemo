#include "core/Provider.hpp"

namespace blender_ui_demo {

const char* provider_status_name(ProviderStatus status) noexcept {
  switch (status) {
    case ProviderStatus::Idle:
      return "Idle";
    case ProviderStatus::Running:
      return "Running";
    case ProviderStatus::Healthy:
      return "Healthy";
    case ProviderStatus::Degraded:
      return "Degraded";
    case ProviderStatus::Failed:
      return "Failed";
    case ProviderStatus::Stopped:
      return "Stopped";
  }
  return "Unknown";
}

}  // namespace blender_ui_demo
