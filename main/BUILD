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
