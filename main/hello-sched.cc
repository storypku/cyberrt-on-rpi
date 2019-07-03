#include "cyber/common/log.h"
#include "cyber/scheduler/scheduler_factory.h"
#include "cyber/croutine/croutine.h"
#include <cassert>
#include <cstdlib>
#include <iostream>

using GlobalData = apollo::cyber::common::GlobalData;
using CRoutine = apollo::cyber::croutine::CRoutine;
using RoutineState = apollo::cyber::croutine::RoutineState;
using SchedulerChoreography = apollo::cyber::scheduler::SchedulerChoreography;

#define EXPECT_EQ(a, b, msg)    \
if ((a) != (b)) {               \
    LOG(ERROR) << "Left: " << (a) << ", Right: " << (b) << ": " << msg; \
                exit(1); }

void monitor_func() {
    LOG(INFO) << "MonitorFunc started";
    CRoutine::GetCurrentRoutine()->HangUp();
    // CRoutine::GetCurrentRoutine()->Sleep(std::chrono::microseconds(50));
    LOG(INFO) << "MonitorFunc finished";
}

int main(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);
    GlobalData::Instance()->SetProcessGroup("my_sched_choreo");
    auto sched = dynamic_cast<SchedulerChoreography*>(apollo::cyber::scheduler::Instance());
    if (sched == nullptr) {
        LOG(ERROR) << "Failed to create SchedulerChoreography.";
        return -1;
    }

    auto cr1 = std::make_shared<CRoutine>(monitor_func);
    auto cr1_id = GlobalData::RegisterTaskName("sched_monitor");
    cr1->set_id(cr1_id);
    cr1->set_name("sched_monitor");
    bool success = sched->DispatchTask(cr1);
    EXPECT_EQ(success, true, "DispatchTask failed");
    std::this_thread::sleep_for(std::chrono::microseconds(50));
    success = sched->NotifyTask(cr1_id);
    EXPECT_EQ(success, true, "Notifyfailed");
    std::cin.ignore();
    return 0;
}
