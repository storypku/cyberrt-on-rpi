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

#include "cyber/scheduler/processor.h"

#include <sched.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <chrono>

#include "cyber/common/global_data.h"
#include "cyber/common/log.h"
#include "cyber/croutine/croutine.h"

namespace apollo {
namespace cyber {
namespace scheduler {

using apollo::cyber::common::GlobalData;

Processor::Processor() { running_.exchange(true); }

Processor::~Processor() { Stop(); }

void Processor::SetSchedAffinity(const std::vector<int> &cpus,
                            const std::string &affinity, int p) {
  if (cpus.empty()) {
    return;
  }

  cpu_set_t set;
  CPU_ZERO(&set);

  if (!affinity.compare("range")) {
    for (const auto cpu : cpus) {
      CPU_SET(cpu, &set);
    }
    pthread_setaffinity_np(thread_.native_handle(), sizeof(set), &set);
  } else if (!affinity.compare("1to1")) {
    CPU_SET(cpus[p], &set);
    pthread_setaffinity_np(thread_.native_handle(), sizeof(set), &set);
  }
}

void Processor::SetSchedPolicy(const std::string& spolicy, int sched_priority) {
  struct sched_param sp;
  memset(reinterpret_cast<void *>(&sp), 0, sizeof(sp));
  sp.sched_priority = sched_priority;

  if (!spolicy.compare("SCHED_FIFO")) {
    int policy = SCHED_FIFO;
    pthread_setschedparam(thread_.native_handle(), policy, &sp);
  } else if (!spolicy.compare("SCHED_RR")) {
    int policy = SCHED_RR;
    pthread_setschedparam(thread_.native_handle(), policy, &sp);
  } else if (!spolicy.compare("SCHED_OTHER")) {
    // Set normal thread nice value.
    while (tid_.load() == -1) {
      cpu_relax();
    }
    setpriority(PRIO_PROCESS, tid_.load(), sched_priority);
  }
}

void Processor::Run() {
  tid_.store(static_cast<int>(syscall(SYS_gettid)));
  AINFO << "processor_tid: " << tid_;

  if (unlikely(context_ == nullptr)) {
    return;
  }

  while (likely(running_.load())) {
    auto croutine = context_->NextRoutine();
    if (croutine) {
      croutine->Resume();
      croutine->Release(); // Acquire() was done in NextRoutine().
    } else {
      context_->Wait();
    }
  }
}

void Processor::Stop() {
  if (!running_.exchange(false)) {
    return;
  }

  if (context_) {
    context_->Shutdown();
  }

  if (thread_.joinable()) {
    thread_.join();
  }
}

void Processor::BindContext(const std::shared_ptr<ProcessorContext> &context) {
  std::call_once(thread_flag_,
                 [this, context]() {
                   this->context_ = context;
                   this->thread_ = std::thread(&Processor::Run, this);
                 });
}

}  // namespace scheduler
}  // namespace cyber
}  // namespace apollo
