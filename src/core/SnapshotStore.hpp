#pragma once

#include "core/SystemSnapshot.hpp"

#include <memory>
#include <mutex>

namespace blender_ui_demo {

class SnapshotStore {
 public:
  void publish(SystemSnapshot snapshot);
  [[nodiscard]] std::shared_ptr<const SystemSnapshot> latest() const;

 private:
  mutable std::mutex mutex_;
  std::shared_ptr<const SystemSnapshot> latest_;
};

}  // namespace blender_ui_demo
