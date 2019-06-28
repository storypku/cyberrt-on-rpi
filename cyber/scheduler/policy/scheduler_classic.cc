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

#include "cyber/scheduler/policy/scheduler_classic.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "cyber/common/environment.h"
#include "cyber/common/file.h"
#include "cyber/event/perf_event_cache.h"
#include "cyber/scheduler/policy/classic_context.h"
#include "cyber/scheduler/processor.h"

namespace apollo {
namespace cyber {
namespace scheduler {

using apollo::cyber::base::ReadLockGuard;
using apollo::cyber::base::WriteLockGuard;
using apollo::cyber::common::GetAbsolutePath;
using apollo::cyber::common::GetProtoFromFile;
using apollo::cyber::common::GlobalData;
using apollo::cyber::common::PathExists;
using apollo::cyber::common::WorkRoot;
using apollo::cyber::croutine::RoutineState;
using apollo::cyber::event::PerfEventCache;
using apollo::cyber::event::SchedPerf;

SchedulerClassic::SchedulerClassic() {
  std::string conf("conf/");
  conf.append(GlobalData::Instance()->ProcessGroup()).append(".conf");
  auto cfg_file = GetAbsolutePath(WorkRoot(), conf);

  apollo::cyber::proto::CyberConfig cfg;
  if (PathExists(cfg_file) && GetProtoFromFile(cfg_file, &cfg)) {
    for (auto& thr : cfg.scheduler_conf().threads()) {
      inner_thr_confs_[thr.name()] = thr;
    }

    if (cfg.scheduler_conf().has_process_level_cpuset()) {
      process_level_cpuset_ = cfg.scheduler_conf().process_level_cpuset();
      ProcessLevelResourceControl();
    }

    classic_conf_ = cfg.scheduler_conf().classic_conf();
    for (auto& group : classic_conf_.groups()) {
      auto& group_name = group.name();
      for (auto task : group.tasks()) {
        task.set_group_name(group_name);
        cr_confs_[task.name()] = task;
      }
    }
  } else {
    // if do not set default_proc_num in scheduler conf
    // give a default value
    uint32_t proc_num = 2;
    auto& global_conf = GlobalData::Instance()->Config();
    if (global_conf.has_scheduler_conf() &&
        global_conf.scheduler_conf().has_default_proc_num()) {
      proc_num = global_conf.scheduler_conf().default_proc_num();
    }

    auto sched_group = classic_conf_.add_groups();
    sched_group->set_name(DEFAULT_GROUP_NAME);
    sched_group->set_processor_num(proc_num);
  }

  CreateProcessor();
}

void SchedulerClassic::CreateProcessor() {
  for (auto& group : classic_conf_.groups()) {
    auto& group_name = group.name();
    auto proc_num = group.processor_num();

    auto& affinity = group.affinity();
    auto& processor_policy = group.processor_policy();
    auto processor_prio = group.processor_prio();
    std::vector<int> cpuset;
    ParseCpuset(group.cpuset(), &cpuset);

    for (uint32_t i = 0; i < proc_num; i++) {
      auto ctx = std::make_shared<ClassicContext>(group_name);
      pctxs_.emplace_back(ctx);

      auto proc = std::make_shared<Processor>();
      proc->BindContext(ctx);
      proc->SetSchedAffinity(cpuset, affinity, i);
      proc->SetSchedPolicy(processor_policy, processor_prio);
      processors_.emplace_back(proc);
    }
  }
}

bool SchedulerClassic::DispatchTask(const std::shared_ptr<CRoutine>& cr) {
  // we use multi-key mutex to prevent race condition
  // when del && add cr with same crid
  auto crid = cr->id();
  MutexWrapper wrapper;
  {
    std::lock_guard<std::mutex> wl_lg(cr_wl_mtx_);
    auto iter = id_map_mutex_.find(crid);
    if (iter != id_map_mutex_.end()) {
        wrapper = iter->second;
    } else {
        wrapper = std::make_shared<std::mutex>();
        id_map_mutex_.insert(std::make_pair(crid, wrapper));
    }
  }

  std::lock_guard<std::mutex> lg(*wrapper);
  {
    {
      WriteLockGuard<AtomicRWLock> lk(id_cr_lock_);
      if (id_cr_.find(cr->id()) != id_cr_.end()) {
        return false;
      }
      id_cr_.emplace(crid, cr);
    }
  
    const auto& cr_name = cr->name();
    {
      auto iter = cr_confs_.find(cr_name);
      if (iter != cr_confs_.end()) {
        const ClassicTask& task = iter->second;
        cr->set_priority(task.prio());
        cr->set_group_name(task.group_name());
      } else {
        // croutine that not exist in conf
        cr->set_group_name(classic_conf_.groups(0).name());
      }
    }
  
    if (cr->priority() >= MAX_PRIO) {
      AWARN << cr_name << " prio is greater than MAX_PRIO[ << " << MAX_PRIO << "].";
      cr->set_priority(MAX_PRIO - 1);
    }
  
    const auto& group_name = cr->group_name();
    auto cr_prio = cr->priority();
    // Enqueue task.
    {
      WriteLockGuard<AtomicRWLock> lk(
          ClassicContext::rq_locks_[group_name][cr_prio]);
      ClassicContext::cr_group_[group_name][cr_prio]
          .emplace_back(cr);
    }
  
    PerfEventCache::Instance()->AddSchedEvent(SchedPerf::RT_CREATE, crid,
                                              cr->processor_id());
  }
  ClassicContext::Notify(cr->group_name()); // Notify for?
  return true;
}

bool SchedulerClassic::NotifyProcessor(uint64_t crid) {
  if (unlikely(stop_.load())) {
    return true;
  }

  {
    ReadLockGuard<AtomicRWLock> lk(id_cr_lock_);
    auto iter = id_cr_.find(crid);
    if (iter != id_cr_.end()) {
      auto cr = iter->second;
      if (cr->state() == RoutineState::DATA_WAIT) { // ...???
        cr->SetUpdateFlag();
      }

      ClassicContext::Notify(cr->group_name());
      return true;
    }
  }
  return false;
}

bool SchedulerClassic::RemoveTask(const std::string& name) {
  if (unlikely(stop_.load())) {
    return true;
  }

  auto crid = GlobalData::GenerateHashId(name);
  return RemoveCRoutine(crid);
}

bool SchedulerClassic::RemoveCRoutine(uint64_t crid) {
  // we use multi-key mutex to prevent race condition
  // when del && add cr with same crid
  MutexWrapper wrapper;
  {
    std::lock_guard<std::mutex> wl_lg(cr_wl_mtx_);
    auto iter = id_map_mutex_.find(crid);
    if (iter != id_map_mutex_.end()) {
      wrapper = iter->second;
    } else {
      wrapper = std::make_shared<std::mutex>();
      id_map_mutex_.emplace(crid, wrapper);
    }
  }
    
  std::lock_guard<std::mutex> lg(*wrapper);

  std::shared_ptr<CRoutine> cr = nullptr;
  int prio;
  std::string group_name;
  {
    WriteLockGuard<AtomicRWLock> lk(id_cr_lock_);
    auto iter = id_cr_.find(crid);
    if (iter == id_cr_.end()) {
      return false;
    } else {
      cr = iter->second;
      prio = cr->priority();
      group_name = cr->group_name();
      cr->Stop();
      id_cr_.erase(crid);
    }
  }

  WriteLockGuard<AtomicRWLock> lk(
      ClassicContext::rq_locks_[group_name][prio]);
  auto& cr_queue = ClassicContext::cr_group_[group_name][prio];
  for (auto it = cr_queue.begin(); it != cr_queue.end(); ++it) {
    if ((*it)->id() == crid) {
      cr = *it;

      (*it)->Stop();
      it = cr_queue.erase(it);
      cr->Release();
      return true;
    }
  }

  return false;
}

}  // namespace scheduler
}  // namespace cyber
}  // namespace apollo
