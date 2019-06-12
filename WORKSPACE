load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

http_archive(
    name = "gtest",
    build_file = "gtest.BUILD",
    url = "https://github.com/google/googletest/archive/release-1.8.0.tar.gz",
    sha256 = "58a6f4277ca2bc8565222b3bbd58a177609e9c488e8a72649359ba51450db7d8",
    strip_prefix = "googletest-release-1.8.0",
)

http_archive(
    name = "com_google_protobuf",
    strip_prefix = "protobuf-3.8.0",
    url = "https://github.com/protocolbuffers/protobuf/archive/v3.8.0.tar.gz",
    sha256 = "03d2e5ef101aee4c2f6ddcf145d2a04926b9c19e7086944df3842b1b8502b783",
)

# bazel-skylib 0.8.0 released 2019.03.20 (https://github.com/bazelbuild/bazel-skylib/releases/tag/0.8.0)
#skylib_version = "0.8.0"
#http_archive(
#    name = "bazel_skylib",
#    type = "tar.gz",
#    url = "https://github.com/bazelbuild/bazel-skylib/releases/download/{}/bazel-skylib.{}.tar.gz".format (skylib_version, skylib_version),
#    sha256 = "2ef429f5d7ce7111263289644d233707dba35e39696377ebab8b0bc701f7818e",
#)

new_local_repository(
    name = "zlib",
    build_file = "external/zlib.BUILD",
    # pkg-config --variable=libdir zlib
    #path = "/usr/lib/x86_64-linux-gnu",
    path = "/usr/lib/arm-linux-gnueabihf",
)
