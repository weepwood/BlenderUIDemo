#include "core/ProviderRegistry.hpp"

#include <chrono>
#include <exception>
#include <stdexcept>
#include <utility>

namespace blender_ui_demo {
namespace {

using Clock = std::chrono::steady_clock;

[[nodiscard]] double elapsed_ms(Clock::time_point start, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

}  // namespace

void ProviderRegistry::register_provider(std::unique_ptr<IDataProvider> provider) {
  if (!provider) {
    throw std::invalid_argument{"provider must not be null"};
  }

  ProviderRuntimeState state;
  state.descriptor = provider->descriptor();
  runtime_states_.push_back(std::move(state));
  providers_.push_back(std::move(provider));
}

std::size_t ProviderRegistry::size() const noexcept {
  return providers_.size();
}

const StaticSystemInfo& ProviderRegistry::static_info() const noexcept {
  return providers_.empty() ? empty_static_info_ : providers_.front()->static_info();
}

ProviderCollectionResult ProviderRegistry::collect(bool refresh_slow_data) {
  ProviderCollectionResult result;

  for (std::size_t index = 0; index < providers_.size(); ++index) {
    auto& provider = providers_[index];
    auto& state = runtime_states_[index];
    state.status = ProviderStatus::Running;
    state.last_error.clear();

    const auto start = Clock::now();
    try {
      DynamicSystemInfo sample = provider->collect(refresh_slow_data);
      state.last_collection_ms = elapsed_ms(start, Clock::now());
      state.status = ProviderStatus::Healthy;
      ++state.completed_samples;

      // The first implementation registers one compatibility provider that owns
      // the complete legacy sample. Domain-specific merge semantics are added as
      // the legacy provider is split into CPU/GPU/disk/network providers.
      if (!result.has_dynamic_data) {
        result.dynamic = std::move(sample);
        result.has_dynamic_data = true;
      }
    } catch (const std::exception& error) {
      state.last_collection_ms = elapsed_ms(start, Clock::now());
      state.status = ProviderStatus::Failed;
      state.last_error = error.what();
    } catch (...) {
      state.last_collection_ms = elapsed_ms(start, Clock::now());
      state.status = ProviderStatus::Failed;
      state.last_error = "unknown provider exception";
    }
  }

  result.providers = runtime_states_;
  return result;
}

}  // namespace blender_ui_demo
