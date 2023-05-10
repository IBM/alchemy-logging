#!/usr/bin/env bash

# Run from the cpp root
cd $(dirname ${BASH_SOURCE[0]})/..

# Build the base as the develop shell
build_cmd="docker build"
if docker buildx version 2>&1 > /dev/null
then
    build_cmd="docker buildx build --platform=linux/amd64"
fi
$build_cmd . --target=base -t alog-cpp-develop

# Run with source mounted
docker run --rm -it -v $PWD:/src alog-cpp-develop
