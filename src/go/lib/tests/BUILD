load(
  "@io_bazel_rules_go//go:def.bzl",
  "go_library",
  "go_test",
)

## Test Lib ##

go_library(
  name = "alogtest",
  srcs = glob(
    include = ["*.go"],
    exclude = ["*_test.go"],
  ),
  deps = [
    "//:go_default_library",
    "//external:go_testify_assert",
  ],
  visibility = ["//visibility:public"]
)

## Tests ##

[go_test(
  name = testfile.rsplit('.', 1)[0],
  srcs = [testfile],
  library = ":alogtest",
  deps = [
    "//external:go_testify_assert",
    ":alogtest",
  ],
  size = "small"
) for testfile in glob(["*_test.go"])]
