cc_binary(
    name = "hello-sched",
    srcs = ["hello-sched.cc"],
    deps = [
        "//cyber/scheduler",
        "//cyber/task",
        "//cyber/common",
        "//cyber/croutine",
        "//external:gflags",
        "@glog",
    ],
)

cc_test(
    name = "vehicle-param_test",
    srcs = ["vehicle-param_test.cc"],
    deps = [
        "@gtest//:main",
        "@glog",
        ":vehicle_param_cc_proto",
        "//cyber/common",
    ],
)

cc_proto_library(
    name = "vehicle_param_cc_proto",
    deps = [
        ":vehicle_param_proto",
    ],
)

proto_library(
    name = "vehicle_param_proto",
    srcs = [
        "vehicle_param.proto",
    ],
)

cc_binary(
    name = "poller_demo",
    srcs = ["poller_demo.cc"],
    deps = [
        "//cyber/io:poller",
        "@glog",
        "@gtest",
    ],
)
