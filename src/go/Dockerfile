## Base ########################################################################
#
# This phase sets up the working environment for the other phases
##
FROM golang:1.24-alpine as base
WORKDIR /src
RUN apk add gcc musl-dev curl bash

## Test ########################################################################
#
# This phase runs the unit tests for the library
##
FROM base as test
COPY . /src
RUN true && \
    go test -coverprofile coverage.html ./... && \
    cd bin/alog_json_converter && \
    go build && \
    cd ../../example/alog_example_server && \
    go build && \
    true

## Release Test ################################################################
#
# This phase builds the example server with redirects removed, forcing it to use
# the tagged version of alog
##
FROM base as release_test
ARG GO_RELEASE_VERSION
COPY ./example/alog_example_server/ /src/
COPY ./ci/test_release.sh /src/
RUN ./test_release.sh
