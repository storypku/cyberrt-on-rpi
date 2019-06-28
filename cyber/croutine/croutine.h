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

#ifndef CYBER_CROUTINE_CROUTINE_H_
#define CYBER_CROUTINE_CROUTINE_H_

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include "cyber/common/log.h"
#include "cyber/croutine/detail/routine_context.h"

namespace apollo {
namespace cyber {
namespace croutine {

using RoutineFunc = std::function<void()>;
using Duration = std::chrono::microseconds;

enum class RoutineState { READY, FINISHED, SLEEP, IO_WAIT, DATA_WAIT };

class CRoutine {
 public:
  explicit CRoutine(const RoutineFunc &func);
  virtual ~CRoutine();

  // static interfaces
  static void Yield();
  static void Yield(const RoutineState &state);
  static CRoutine *GetCurrentRoutine() { return current_routine_; }
  static char **GetMainStack() { return &main_stack_; }

  // public interfaces
  bool Acquire() {
    return !lock_.test_and_set(std::memory_order_acquire);
  }

  void Release() {
    return lock_.clear(std::memory_order_release);
  }

  // It is caller's responsibility to check if state_ is valid before calling
  // SetUpdateFlag().
  void SetUpdateFlag() {
    updated_.clear(std::memory_order_release);
  }

  // acquire && release should be called before Resume
  // when work-steal like mechanism used
  RoutineState Resume();
  RoutineState UpdateState();

  RoutineContext *GetContext() { return context_.get(); }
  char **GetStack() { return &(context_->sp); }

  void Run() { func_(); }
  void Stop() { force_stop_ = true; }
  void Wake() { state_ = RoutineState::READY; }
  void HangUp() { CRoutine::Yield(RoutineState::DATA_WAIT); }
  void Sleep(const Duration &sleep_duration) {
    wake_time_ = std::chrono::steady_clock::now() + sleep_duration;
    CRoutine::Yield(RoutineState::SLEEP);
  }
  // getter and setter
  RoutineState state() const { return state_; }
  void set_state(const RoutineState &state) { state_ = state; }

  uint64_t id() const { return id_; }
  void set_id(uint64_t id) { id_ = id; }

  const std::string &name() const { return name_; }
  void set_name(const std::string &name) { name_ = name; }

  int processor_id() const { return processor_id_; }
  void set_processor_id(int processor_id) { processor_id_ = processor_id; }

  uint32_t priority() const { return priority_; }
  void set_priority(uint32_t priority) { priority_ = priority; }

  std::chrono::steady_clock::time_point wake_time() const { return wake_time_; }

  void set_group_name(const std::string &group_name) { group_name_ = group_name; }
  const std::string &group_name() { return group_name_; }

 private:
  CRoutine(CRoutine &) = delete;
  CRoutine &operator=(CRoutine &) = delete;

  std::chrono::steady_clock::time_point wake_time_ =
      std::chrono::steady_clock::now();

  RoutineFunc func_;
  RoutineState state_;

  std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
  std::atomic_flag updated_ = ATOMIC_FLAG_INIT;

  bool force_stop_ = false;

  int processor_id_ = -1;
  uint32_t priority_ = 0;
  uint64_t id_ = 0;

  std::string name_;
  std::string group_name_;

  std::shared_ptr<RoutineContext> context_;
  static thread_local CRoutine *current_routine_;
  static thread_local char *main_stack_;
};

inline void CRoutine::Yield(const RoutineState &state) {
  auto routine = GetCurrentRoutine();
  routine->set_state(state);
  SwapContext(routine->GetStack(), GetMainStack());
}

inline void CRoutine::Yield() {
  SwapContext(GetCurrentRoutine()->GetStack(), GetMainStack());
}

inline RoutineState CRoutine::UpdateState() {
  // Synchronous Event Mechanism
  if (state_ == RoutineState::SLEEP &&
      std::chrono::steady_clock::now() > wake_time_) {
    state_ = RoutineState::READY;
    return state_;
  }

  // Asynchronous Event Mechanism
  // if (!updated_.test_and_set(std::memory_order_release)) { LIUJIAMING
  if (!updated_.test_and_set(std::memory_order_acquire)) {
    if (state_ == RoutineState::DATA_WAIT || state_ == RoutineState::IO_WAIT) {
      state_ = RoutineState::READY;
    }
  }
  return state_;
}

}  // namespace croutine
}  // namespace cyber
}  // namespace apollo

#endif  // CYBER_CROUTINE_CROUTINE_H_
