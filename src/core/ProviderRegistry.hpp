#pragma once

#include "core/Provider.hpp"

#include <memory>
#include <vector>

namespace blender_ui_demo {

class ProviderRegistry {
 public:
  void register_provider(std::unique_ptr<IDataProvider> provider);

  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] const StaticSystemInfo& static_info() const noexcept;
  [[nodiscard]] ProviderCollectionResult collect(bool refresh_slow_data);

 private:
  std::vector<std::unique_ptr<IDataProvider>> providers_;
  std::vector<ProviderRuntimeState> runtime_states_;
  StaticSystemInfo empty_static_info_;
};

}  // namespace blender_ui_demo
