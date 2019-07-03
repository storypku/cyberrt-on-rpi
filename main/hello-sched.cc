#include "cyber/common/log.h"
#include "cyber/scheduler/scheduler_factory.h"

using GlobalData = apollo::cyber::common::GlobalData;
using SchedulerChoreography = apollo::cyber::scheduler::SchedulerChoreography;

int main(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);
    GlobalData::Instance()->SetProcessGroup("my_sched_choreo");
    auto sched = dynamic_cast<SchedulerChoreography*>(apollo::cyber::scheduler::Instance());
    if (sched == nullptr) {
        LOG(ERROR) << "Failed to create SchedulerChoreography.";
        return -1;
    }
    return 0;
}
