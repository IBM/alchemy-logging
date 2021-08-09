#!/usr/bin/env bash

################################################################################
# This script tests a given release to make sure it has propagated through the
# necessary proxies and can successfully build and run the test server. It is
# designed to be run inside the docker build only!
################################################################################

# Set up a basic go.mod file to build the server
echo "module alog_example_server" > go.mod

# Try to fetch the release until it either times out or we succeed
retry_sleep=10
total_time=0
timeout=600
until [ $total_time -eq $timeout ] || \
    go get github.com/IBM/alchemy-logging/src/go/alog@${GO_RELEASE_VERSION}
do
    sleep $retry_sleep
    total_time=$(expr $total_time + $retry_sleep)
done

# If we did not successfully find the release, don't let it find any-old one
if ! grep ${GO_RELEASE_VERSION} go.mod
then
    echo "Failed to find release ${GO_RELEASE_VERSION}"
    exit 1
fi

# Tidy up and build then test the server
go mod tidy
go build
./test_example_server.sh
