/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#ifndef CYBER_DATA_DATA_NOTIFIER_H_
#define CYBER_DATA_DATA_NOTIFIER_H_

#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "cyber/common/log.h"
#include "cyber/common/macros.h"
#include "cyber/data/cache_buffer.h"
#include "cyber/base/atomic_rw_lock.h"
// #include "cyber/event/perf_event_cache.h"
// #include "cyber/time/time.h"

namespace apollo {
namespace cyber {
namespace data {

// using apollo::cyber::Time;
using apollo::cyber::base::AtomicRWLock;
using apollo::cyber::base::ReadLockGuard;
using apollo::cyber::base::WriteLockGuard;

struct Notifier {
  std::function<void()> callback;
};

class DataNotifier {
 public:
  using NotifyVector = std::vector<std::shared_ptr<Notifier>>;
  ~DataNotifier() {}

  void AddNotifier(uint64_t channel_id,
                   const std::shared_ptr<Notifier>& notifier);

  bool Notify(const uint64_t channel_id);

 private:
  AtomicRWLock notifies_map_mutex_;
  std::unordered_map<uint64_t, NotifyVector> notifies_map_;

  DECLARE_SINGLETON(DataNotifier)
};

inline DataNotifier::DataNotifier() {}

inline void DataNotifier::AddNotifier(
    uint64_t channel_id, const std::shared_ptr<Notifier>& notifier) {
  WriteLockGuard<AtomicRWLock> lock(notifies_map_mutex_);
  auto iter = notifies_map_.find(channel_id);
  if (iter == notifies_map_.end()) {
    notifies_map_.emplace(channel_id, std::vector<std::shared_ptr<Notifier>>{notifier});
  } else {
    iter->second.emplace_back(notifier);
  }
}

inline bool DataNotifier::Notify(const uint64_t channel_id) {
  NotifyVector notifiers;
  {
    ReadLockGuard<AtomicRWLock> lock(notifies_map_mutex_);
    auto iter = notifies_map_.find(channel_id);
    if (iter == notifies_map_.cend()) {
      return false;
    }
    notifiers = iter->second;
  }
  for (auto& notifier : notifiers) {
    if (notifier && notifier->callback) {
      notifier->callback();
    }
  }
  return true;
}

}  // namespace data
}  // namespace cyber
}  // namespace apollo

#endif  // CYBER_DATA_DATA_NOTIFIER_H_
