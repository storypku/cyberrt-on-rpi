#include "gtest/gtest.h"
#include "cyber/common/file.h"
#include "main/vehicle_param.pb.h"

TEST(VehicleParamTest, ascii_txt_2_msg) {
    const std::string filename = "/home/nvidia/liujiaming02/cyberrt-on-rpi.git/main/vehicle_param.prototxt";
    using VehicleParam = vp::common::config::VehicleParam;
    VehicleParam veh;
    bool success = apollo::cyber::common::GetProtoFromASCIIFile(filename, &veh);
    EXPECT_EQ(success, true);
    EXPECT_EQ(veh.brand(), VehicleParam::LINCOLN_MKZ);
    EXPECT_FLOAT_EQ(veh.length(), 4.933f);
}
