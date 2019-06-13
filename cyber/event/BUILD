load("//tools:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "perf_event_cache",
    srcs = [
        "perf_event_cache.cc",
    ],
    hdrs = [
        "perf_event_cache.h",
    ],
    deps = [
        ":perf_event",
        "//cyber:state",
        "//cyber/base:bounded_queue",
        "//cyber/common:global_data",
        "//cyber/common:log",
        "//cyber/common:macros",
        "//cyber/time",
    ],
)

cc_library(
    name = "perf_event",
    hdrs = ["perf_event.h"],
    deps = [
        "//cyber/common:global_data",
    ],
)

cc_test(
    name = "perf_event_test",
    size = "small",
    srcs = [
        "perf_event_test.cc",
    ],
    deps = [
        ":perf_event_cache",
        "//cyber/base:bounded_queue",
        "//cyber/common:file",
        "//cyber/common:global_data",
        "@glog",
        "@gtest//:main",
    ],
)

cpplint()
