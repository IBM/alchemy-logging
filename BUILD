load(
  "@io_bazel_rules_go//go:def.bzl",
  "go_prefix",
  "go_library",
)
go_prefix("github.ibm.com/watson-discovery/alog")
go_library(
  name = "go_default_library",
  srcs = glob(["lib/*.go"]),
  visibility = ["//visibility:public"]
)
