#include "cyber/event/perf_event_cache.h"
#include "cyber/common/file.h"
#include <cstdlib>
#include <chrono>
#include "gtest/gtest.h"

namespace apollo {
namespace cyber {
namespace event {

TEST(PerfEventTest, SchedEventTest) {
    SchedEvent event;
    std::string s = event.SerializeToString();
    ASSERT_EQ(s, "0\t0\t\t0\t0\t0");
}

TEST(PerfEventTest, TransportEventTest) {
    TransportEvent event;
    std::string s = event.SerializeToString();
    ASSERT_EQ(s, "1\t0\t\t0\t0");
}

TEST(PerfEventTest, PerfEventCacheTest) {
    google::InitGoogleLogging("PerfEventCacheTest");
    setenv("cyber_sched_perf", "1", 1);
    setenv("cyber_trans_perf", "2", 1);
    auto instance = PerfEventCache::Instance();
    for (int i = 1; i <= 10; i++) {
        if (i & 0x01) {
            auto eid = (i % 3)? SchedPerf::NOTIFY_IN : SchedPerf::SWAP_OUT;
            instance->AddSchedEvent(eid, static_cast<uint64_t>(i), 10 + i, 100 + i);
        } else {
            auto eid = (i % 3)? TransPerf::TRANS_FROM : TransPerf::WRITE_NOTIFY; 
            instance->AddTransportEvent(eid, i, 100 + i);
        }
    }
    std::this_thread::sleep_for(std::chrono::microseconds(50));
    PerfEventCache::CleanUp();
    // PerfEventCache::CleanUp(); // try once more
    auto files = apollo::cyber::common::Glob("cyber_perf_*.data");
    EXPECT_NE(0, files.size());
}

} // namespace event
} // namespace cyber
} // namespace apollo
