#!/usr/bin/env bash

################################################################################
# This script tests a given release to make sure it has propagated through
# crates.io and can successfully build and run the fibonacci example. It is
# designed to be run inside the docker build only!
################################################################################

set -e
cd $(dirname ${BASH_SOURCE[0]})/..

# Set up a throwaway crate that depends on the tagged release and builds the
# fibonacci example against it
cat > Cargo.toml <<EOF
[package]
name = "alog-release-test"
version = "0.0.0"
edition = "2021"
publish = false

[dependencies]
alchemy-logging = "${RUST_RELEASE_VERSION}"
serde_json = "1"

[[bin]]
name = "fib"
path = "examples/fib.rs"
EOF
rm -f Cargo.lock

# Try to fetch the release until it either times out or we succeed
retry_sleep=10
total_time=0
timeout=600
until [ $total_time -eq $timeout ] || cargo build --release --bin fib
do
    echo "Waiting for release ${RUST_RELEASE_VERSION} to propagate to crates.io..."
    sleep $retry_sleep
    total_time=$(expr $total_time + $retry_sleep)
done

if [ ! -f target/release/fib ]
then
    echo "Failed to build against release ${RUST_RELEASE_VERSION}"
    exit 1
fi

./target/release/fib 5
