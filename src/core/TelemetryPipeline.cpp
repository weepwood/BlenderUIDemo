#include "core/TelemetryPipeline.hpp"

#include "providers/LegacySystemProvider.hpp"

#include <utility>

namespace blender_ui_demo {

TelemetryPipeline::TelemetryPipeline() {
  registry_.register_provider(std::make_unique<LegacySystemProvider>());
  static_info_ = registry_.static_info();
  worker_ = std::thread{&TelemetryPipeline::worker_loop, this};
  request_cv_.notify_one();
}

TelemetryPipeline::~TelemetryPipeline() {
  {
    std::scoped_lock lock{request_mutex_};
    stop_requested_ = true;
  }
  request_cv_.notify_one();
  if (worker_.joinable()) {
    worker_.join();
  }
}

const StaticSystemInfo& TelemetryPipeline::static_info() const noexcept {
  return static_info_;
}

DynamicSystemInfo TelemetryPipeline::sample(bool refresh_slow_data) {
  request_refresh(refresh_slow_data);
  const auto snapshot = snapshots_.latest();
  return snapshot ? snapshot->dynamic : DynamicSystemInfo{};
}

void TelemetryPipeline::request_refresh(bool include_slow_data) {
  {
    std::scoped_lock lock{request_mutex_};
    fast_refresh_pending_ = true;
    slow_refresh_pending_ = slow_refresh_pending_ || include_slow_data;
  }
  request_cv_.notify_one();
}

std::shared_ptr<const SystemSnapshot> TelemetryPipeline::latest_snapshot() const {
  return snapshots_.latest();
}

TelemetryPipelineStatus TelemetryPipeline::status() const {
  TelemetryPipelineStatus result;
  result.worker_running = worker_running_.load(std::memory_order_relaxed);
  result.collection_in_progress = collection_in_progress_.load(std::memory_order_relaxed);
  result.provider_count = registry_.size();

  const auto snapshot = snapshots_.latest();
  if (snapshot) {
    result.latest_sequence = snapshot->sequence;
    result.providers = snapshot->providers;
    const auto now = std::chrono::steady_clock::now();
    if (now >= snapshot->published_at) {
      result.snapshot_age =
          std::chrono::duration_cast<std::chrono::milliseconds>(now - snapshot->published_at);
    }
  }
  return result;
}

void TelemetryPipeline::worker_loop() {
  worker_running_.store(true, std::memory_order_relaxed);

  while (true) {
    bool refresh_slow_data = false;
    {
      std::unique_lock lock{request_mutex_};
      request_cv_.wait(lock, [this] { return stop_requested_ || fast_refresh_pending_; });
      if (stop_requested_) {
        break;
      }

      refresh_slow_data = slow_refresh_pending_;
      fast_refresh_pending_ = false;
      slow_refresh_pending_ = false;
    }

    collection_in_progress_.store(true, std::memory_order_relaxed);
    collect_and_publish(refresh_slow_data);
    collection_in_progress_.store(false, std::memory_order_relaxed);
  }

  collection_in_progress_.store(false, std::memory_order_relaxed);
  worker_running_.store(false, std::memory_order_relaxed);
}

void TelemetryPipeline::collect_and_publish(bool refresh_slow_data) {
  ProviderCollectionResult collection = registry_.collect(refresh_slow_data);
  const auto previous = snapshots_.latest();

  SystemSnapshot snapshot;
  snapshot.sequence = sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
  snapshot.captured_at = std::chrono::system_clock::now();
  snapshot.published_at = std::chrono::steady_clock::now();
  snapshot.providers = std::move(collection.providers);

  if (collection.has_dynamic_data) {
    snapshot.dynamic = std::move(collection.dynamic);
  } else if (previous) {
    snapshot.dynamic = previous->dynamic;
  }

  snapshots_.publish(std::move(snapshot));
}

}  // namespace blender_ui_demo
