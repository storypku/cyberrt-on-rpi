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

#ifndef CYBER_BASE_CONCURRENT_HASH_MAP_H_
#define CYBER_BASE_CONCURRENT_HASH_MAP_H_

#include "cyber/base/atomic_rw_lock.h"
#include "cyber/base/rw_lock_guard.h"

#include <cstdint>
#include <list>
#include <type_traits>
#include <algorithm>

namespace apollo {
namespace cyber {
namespace base {
/**
 * @brief A implementation of lock-free fixed size hash map
 *
 * @tparam K Type of key, must be integral
 * @tparam V Type of value
 * @tparam 128 Size of hash table
 * @tparam 0 Type traits, use for checking types of key & value
 */
template <typename K, typename V, std::size_t TableSize = 128,
          typename std::enable_if<std::is_integral<K>::value &&
                                      (TableSize & (TableSize - 1)) == 0,
                                  int>::type = 0>
class ConcurrentHashMap {
 public:
  ConcurrentHashMap() : capacity_(TableSize), mode_num_(capacity_ - 1) {}
  ConcurrentHashMap(const ConcurrentHashMap &other) = delete;
  ConcurrentHashMap &operator=(const ConcurrentHashMap &other) = delete;

  bool Has(K const& key) {
    uint64_t index = key & mode_num_;
    return table_[index].Has(key);
  }

  bool Get(K const& key, V *value) {
    uint64_t index = key & mode_num_;
    return table_[index].Get(key, value);
  }

  void Set(K const& key) {
    uint64_t index = key & mode_num_;
    table_[index].Insert(key, V());
  }

  void Set(K const& key, const V &value) {
    uint64_t index = key & mode_num_;
    table_[index].Insert(key, value);
  }

  void Set(K const& key, V &&value) {
    uint64_t index = key & mode_num_;
    table_[index].Insert(key, std::forward<V>(value));
  }

 private:
  class Bucket {
   public:
    typedef std::pair<K, V> BucketValue;
    typedef std::list<BucketValue> BucketData;
    typedef typename BucketData::iterator BucketIterator;
   private:
    BucketData data_;
    mutable AtomicRWLock mutex;
    BucketIterator find_entry_for(K const& key) {
      return std::find_if(data_.begin(), data_.end(),
            [&](BucketValue const & item) {
              return item.first == key;
            });
    }
   public:
    bool Has(K const& key) {
      ReadLockGuard<AtomicRWLock> lg(mutex);
      BucketIterator const found_entry = find_entry_for(key);
      return found_entry != data_.end();
    }

    bool Get(K const& key,  V* value) {
      ReadLockGuard<AtomicRWLock> lg(mutex);
      BucketIterator const found_entry = find_entry_for(key);
      bool found = (found_entry != data_.end());
      if (found) {
        *value = found_entry->second;
      }
      return found;
    }

    void Insert(K const& key, V const& value) {
      WriteLockGuard<AtomicRWLock> lg(mutex);
      BucketIterator const found_entry = find_entry_for(key);
      if (found_entry == data_.end()) {
        data_.emplace_back(key, value);
      } else {
        found_entry->second = value;
      }
    }

    void Insert(K const& key, V && value) {
      WriteLockGuard<AtomicRWLock> lg(mutex);
      BucketIterator const found_entry = find_entry_for(key);
      if (found_entry == data_.end()) {
        data_.emplace_back(key, std::forward<V>(value));
      } else {
        found_entry->second = std::forward<V>(value);
      }
    }

    void Erase(K const& key) {
      WriteLockGuard<AtomicRWLock> lg(mutex);
      BucketIterator const found_entry = find_entry_for(key);

      if (found_entry != data_.end()) {
        data_.erase(found_entry);
      }
    }

  }; 

 private:
  Bucket table_[TableSize];
  uint64_t capacity_;
  uint64_t mode_num_;
};

}  // namespace base
}  // namespace cyber
}  // namespace apollo

#endif  // CYBER_BASE_CONCURRENT_HASH_MAP_H_
