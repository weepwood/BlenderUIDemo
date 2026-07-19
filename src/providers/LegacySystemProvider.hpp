#pragma once

#include "core/Provider.hpp"

namespace blender_ui_demo {

class LegacySystemProvider final : public IDataProvider {
 public:
  LegacySystemProvider();

  [[nodiscard]] const ProviderDescriptor& descriptor() const noexcept override;
  [[nodiscard]] const StaticSystemInfo& static_info() const noexcept override;
  [[nodiscard]] DynamicSystemInfo collect(bool refresh_slow_data) override;

 private:
  ProviderDescriptor descriptor_;
  SystemMonitor monitor_;
};

}  // namespace blender_ui_demo
