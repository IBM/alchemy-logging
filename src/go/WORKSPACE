workspace(name = "com_github_ibm_alog")

## Tools for accessing repositories ############################################

load("@bazel_tools//tools/build_defs/repo:git.bzl",
  "git_repository",
  "new_git_repository",
)

## Tools for building C++ and Golang ###########################################

# Bazel rules for working with Go repositories
# NOTE: A recent change in the naming of buildifier (to buildtools) caused the
# go_repositories routine to break since it was downloading the github archive
# and relying on the name being buildifier. The current hash here fixes that on
# a branch in our local vendor'ed copy of bazelbuild-rules_go.
git_repository(
  name = "io_bazel_rules_go",
  remote = "git@github.ibm.com:chartreuse/bazelbuild-rules_go.git",
  commit = "d110b43ccba10d790784b0ad4c477772ea28976b",
)

load("@io_bazel_rules_go//go:def.bzl",
  "go_repositories",
  "go_repository")
go_repositories(
  go_version = "1.8.2",
)

## Third Party Go ##############################################################

## spew (needed by testify)
go_repository(
  name = "com_github_davecgh_go_spew",
  importpath = "github.com/davecgh/go-spew",
  remote = "https://github.ibm.com/chartreuse/davecgh-go-spew",
  commit = "346938d642f2ec3594ed81d874461961cd0faa76",
  vcs = "git",
)

## go difflib (needed by testify)
go_repository(
  name = "com_github_pmezard_go_difflib",
  importpath = "github.com/pmezard/go-difflib",
  remote = "https://github.ibm.com/chartreuse/pmezard-go-difflib",
  commit = "792786c7400a136282c1664665ae0a8db921c6c2",
  vcs = "git",
)

## testify
go_repository(
  name = "com_github_stretchr_testify",
  importpath = "github.com/stretchr/testify",
  remote = "https://github.ibm.com/chartreuse/stretchr-testify",
  commit = "2402e8e7a02fc811447d11f881aa9746cdc57983",
  vcs = "git",
)
bind(
  name = "go_testify",
  actual = "@com_github_stretchr_testify//:go_default_library",
)
bind(
  name = "go_testify_assert",
  actual = "@com_github_stretchr_testify//assert:go_default_library",
)
bind(
  name = "go_testify_require",
  actual = "@com_github_stretchr_testify//require:go_default_library",
)
