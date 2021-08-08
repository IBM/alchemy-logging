#!/usr/bin/env bash

################################################################################
#
# This is a simple script that takes the example server out for a walk and makes
# a few curl calls against it
################################################################################

# Failed calls fail the script
set -euo pipefail

# Run from the package directory
cd $(dirname ${BASH_SOURCE[0]})

# Run the server in the background
port=55544
./alog_example_server \
    -log.default-level info \
    -log.goroutine-id \
    -log.service-name example-server \
    -port $port &
server_pid=$!
sleep 1

# Make some calls to the server
timeout=2
curl -s "http://localhost:$port/demo"
curl -s "http://localhost:$port/logging?default_level=debug3&timeout=$timeout"
curl -s "http://localhost:$port/demo"

# Wait for the timeout to expire
sleep $timeout

# Kill the server
pkill -15 $server_pid || true
