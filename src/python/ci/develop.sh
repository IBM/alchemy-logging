#!/usr/bin/env bash

# Run from the cpp root
cd $(dirname ${BASH_SOURCE[0]})/..

# Build the base as the develop shell
docker build . --target=base -t alog-py-develop $@

# Run with source mounted
docker run --rm -it -v $PWD:/src --entrypoint /bin/bash -w /src alog-py-develop
