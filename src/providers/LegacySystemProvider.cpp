#include "providers/LegacySystemProvider.hpp"

namespace blender_ui_demo {

LegacySystemProvider::LegacySystemProvider()
    : descriptor_{ProviderId::LegacySystem,
                  "Legacy System Monitor",
                  "Compatibility provider for the existing Win32 collectors",
                  ProviderCadence::FastAndSlow,
                  false} {}

const ProviderDescriptor& LegacySystemProvider::descriptor() const noexcept {
  return descriptor_;
}

const StaticSystemInfo& LegacySystemProvider::static_info() const noexcept {
  return monitor_.static_info();
}

DynamicSystemInfo LegacySystemProvider::collect(bool refresh_slow_data) {
  return monitor_.sample(refresh_slow_data);
}

}  // namespace blender_ui_demo
