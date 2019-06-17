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

#ifndef CYBER_DATA_DATA_DISPATCHER_H_
#define CYBER_DATA_DATA_DISPATCHER_H_

#include <memory>
#include <mutex>
#include <vector>

#include "cyber/common/log.h"
#include "cyber/common/macros.h"
#include "cyber/base/atomic_rw_lock.h"
#include "cyber/data/channel_buffer.h"
#include "cyber/state.h"
// #include "cyber/time/time.h"

namespace apollo {
namespace cyber {
namespace data {

// using apollo::cyber::Time;
using apollo::cyber::base::AtomicRWLock;
using apollo::cyber::base::ReadLockGuard;
using apollo::cyber::base::WriteLockGuard;

template <typename T>
class DataDispatcher {
 public:
  using BufferVector =
      std::vector<std::weak_ptr<CacheBuffer<std::shared_ptr<T>>>>;
  ~DataDispatcher() {}

  void AddBuffer(const ChannelBuffer<T>& channel_buffer);

  bool Dispatch(const uint64_t channel_id, const std::shared_ptr<T>& msg);

 private:
  DataNotifier* notifier_ = DataNotifier::Instance();
  AtomicRWLock buffers_map_mutex_;
  std::unordered_map<uint64_t, BufferVector> buffers_map_;

  DECLARE_SINGLETON(DataDispatcher)
};

template <typename T>
inline DataDispatcher<T>::DataDispatcher() {}

template <typename T>
void DataDispatcher<T>::AddBuffer(const ChannelBuffer<T>& channel_buffer) {
  auto buffer = channel_buffer.Buffer();
  const uint64_t channel_id = channel_buffer.channel_id();

  WriteLockGuard<AtomicRWLock> lock(buffers_map_mutex_);
  auto iter = buffers_map_.find(channel_id);
  if (iter == buffers_map_.end()) {
    buffers_map_.emplace(channel_id, BufferVector{ buffer });
  } else {
    iter->second.emplace_back(buffer);
  }
}

template <typename T>
bool DataDispatcher<T>::Dispatch(const uint64_t channel_id,
                                 const std::shared_ptr<T>& msg) {
  if (apollo::cyber::IsShutdown()) {
    return false;
  }
  BufferVector buffers;
  {
    ReadLockGuard<AtomicRWLock> lock(buffers_map_mutex_);
    auto iter = buffers_map_.find(channel_id);
    if (iter == buffers_map_.end()) {
      return false;
    }
    buffers = iter->second;
  }

  for (auto& buffer_wptr : buffers) {
    if (auto buffer = buffer_wptr.lock()) {
      std::lock_guard<std::mutex> lock(buffer->Mutex());
      buffer->Fill(msg);
    }
  }
  return notifier_->Notify(channel_id);
}

}  // namespace data
}  // namespace cyber
}  // namespace apollo

#endif  // CYBER_DATA_DATA_DISPATCHER_H_
