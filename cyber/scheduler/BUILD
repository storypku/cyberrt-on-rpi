load("//tools:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "processor",
    srcs = [
        "processor.cc",
    ],
    hdrs = [
        "processor.h",
    ],
    deps = [
        "//cyber/data",
        ":processor_context",
    ],
)

cc_library(
    name = "processor_context",
    srcs = [
        "processor_context.cc",
    ],
    hdrs = [
        "processor_context.h",
    ],
    deps = [
        "//cyber/croutine",
        "//cyber/data",
    ],
)

cc_library(
    name = "scheduler",
    srcs = [
        "scheduler.cc",
    ],
    hdrs = [
        "scheduler.h",
    ],
    deps = [
        "//cyber/croutine",
        "//cyber/scheduler:processor",
    ],
)

cc_library(
    name = "scheduler_factory",
    srcs = [
        "scheduler_factory.cc",
    ],
    hdrs = [
        "scheduler_factory.h",
    ],
    deps = [
        "//cyber/proto:component_conf_cc_proto",
        ":scheduler_choreography",
        ":scheduler_classic",
    ],
)

cc_library(
    name = "scheduler_choreography",
    srcs = [
        "policy/scheduler_choreography.cc",
    ],
    hdrs = [
        "policy/scheduler_choreography.h",
    ],
    deps = [
        "//cyber/scheduler:scheduler",
        "//cyber/scheduler:choreography_context",
        "//cyber/scheduler:classic_context",
    ],
)

cc_library(
    name = "scheduler_classic",
    srcs = [
        "policy/scheduler_classic.cc",
    ],
    hdrs = [
        "policy/scheduler_classic.h",
    ],
    deps = [
        "//cyber/scheduler",
        "//cyber/scheduler:classic_context",
    ],
)

cc_library(
    name = "choreography_context",
    srcs = [
        "policy/choreography_context.cc",
    ],
    hdrs = [
        "policy/choreography_context.h",
    ],
    deps = [
        "//cyber/croutine",
        "//cyber/proto:choreography_conf_cc_proto",
        "//cyber/scheduler:processor",
    ],
)

cc_library(
    name = "classic_context",
    srcs = [
        "policy/classic_context.cc",
    ],
    hdrs = [
        "policy/classic_context.h",
    ],
    deps = [
        "//cyber/croutine",
        "//cyber/proto:classic_conf_cc_proto",
        ":processor",
    ],
)

cc_test(
    name = "scheduler_test",
    size = "small",
    srcs = [
        "scheduler_test.cc",
    ],
    deps = [
        "//cyber/common",
        "//cyber/croutine",
        "//cyber/data",
        "//cyber/scheduler:scheduler_factory",
        "@glog",
        "@gtest//:main",
    ],
)

cc_test(
    name = "scheduler_classic_test",
    size = "small",
    srcs = [
        "scheduler_classic_test.cc",
    ],
    deps = [
        "//cyber/task",
        "//cyber/common",
        "//cyber/croutine",
        "//cyber/scheduler:scheduler_factory",
        "@glog",
        "@gtest//:main",
    ],
)

cc_test(
    name = "scheduler_choreo_test",
    size = "small",
    srcs = [
        "scheduler_choreo_test.cc",
    ],
    deps = [
        "//cyber/common",
        "//cyber/data",
        "//cyber/croutine",
        "//cyber/scheduler:scheduler_factory",
        "@glog",
        "@gtest//:main",
    ],
)

cpplint()
