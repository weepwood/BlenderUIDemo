#pragma once

#include "core/ProviderRegistry.hpp"
#include "core/SnapshotStore.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace blender_ui_demo {

struct TelemetryPipelineStatus {
  bool worker_running = false;
  bool collection_in_progress = false;
  std::uint64_t latest_sequence = 0;
  std::size_t provider_count = 0;
  std::chrono::milliseconds snapshot_age{0};
  std::vector<ProviderRuntimeState> providers;
};

class TelemetryPipeline {
 public:
  TelemetryPipeline();
  ~TelemetryPipeline();

  TelemetryPipeline(const TelemetryPipeline&) = delete;
  TelemetryPipeline& operator=(const TelemetryPipeline&) = delete;

  [[nodiscard]] const StaticSystemInfo& static_info() const noexcept;

  // Compatibility facade for the existing UI. This method never executes the
  // Win32 collectors on the calling thread: it queues a refresh and returns a
  // value copied from the latest immutable snapshot.
  [[nodiscard]] DynamicSystemInfo sample(bool refresh_slow_data = false);

  void request_refresh(bool include_slow_data = false);
  [[nodiscard]] std::shared_ptr<const SystemSnapshot> latest_snapshot() const;
  [[nodiscard]] TelemetryPipelineStatus status() const;

 private:
  void worker_loop();
  void collect_and_publish(bool refresh_slow_data);

  ProviderRegistry registry_;
  SnapshotStore snapshots_;
  StaticSystemInfo static_info_;

  std::thread worker_;
  mutable std::mutex request_mutex_;
  std::condition_variable request_cv_;
  bool stop_requested_ = false;
  bool fast_refresh_pending_ = true;
  bool slow_refresh_pending_ = true;

  std::atomic<bool> worker_running_{false};
  std::atomic<bool> collection_in_progress_{false};
  std::atomic<std::uint64_t> sequence_{0};
};

}  // namespace blender_ui_demo
