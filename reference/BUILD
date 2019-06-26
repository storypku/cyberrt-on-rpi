cc_library(
    name = "hello-fastrtps",
    srcs = glob(["*.cpp", "*.cxx"]),
    hdrs = glob(["*.h"]),
    deps = [
        "@fastrtps",
    ],
#    linkopts = [
#        "-lglog",
#        "-lgflags",
#    ],
)

cc_binary(
    name = "hello-demo",
    srcs = ["demo.cpp"],
    deps = [
        ":hello-fastrtps",
    ],
#    linkopts = [
#        "-lglog",
#        "-lgflags",
#    ],
)
