#!/usr/bin/env bash

# Run from the cpp root
cd $(dirname ${BASH_SOURCE[0]})/..

# Build the base as the develop shell
docker build . --target=base -t alog-go-develop

# Run with source mounted
docker run --rm -it -v $PWD:/src alog-go-develop
