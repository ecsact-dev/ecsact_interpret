copts = select({
    "@bazel_tools//tools/cpp:msvc": ["/std:c++latest"],
    "//conditions:default": ["-std=c++20"],
})
