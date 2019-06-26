cc_library(
    name = "hello-greet",
    srcs = ["hello-greet.cc"],
    hdrs = ["hello-greet.h"],
    deps = [
        "@glog",
        "//external:gflags",
        "@udev",
    ],
#    linkopts = [
#        "-lglog",
#        "-lgflags",
#    ],
)

cc_test(
    name = "hello-greet_test",
    srcs = ["hello-greet_test.cc"],
    deps = [
        "@gtest//:main",
        ":hello-greet",
        "//cyber/common:log",
    ],
)

cc_binary(
    name = "hello-world",
    srcs = ["hello-world.cc"],
    deps = [
        ":hello-greet",
    ],
#    linkopts = [
#        "-lglog",
#        "-lgflags",
#    ],
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

