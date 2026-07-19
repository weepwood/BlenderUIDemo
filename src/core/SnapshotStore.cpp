#include "core/SnapshotStore.hpp"

#include <utility>

namespace blender_ui_demo {

void SnapshotStore::publish(SystemSnapshot snapshot) {
  auto immutable = std::make_shared<const SystemSnapshot>(std::move(snapshot));
  std::scoped_lock lock{mutex_};
  latest_ = std::move(immutable);
}

std::shared_ptr<const SystemSnapshot> SnapshotStore::latest() const {
  std::scoped_lock lock{mutex_};
  return latest_;
}

}  // namespace blender_ui_demo
