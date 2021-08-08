## Base ########################################################################
#
# This phase sets up the working environment for the other phases
##
FROM golang:1.16-alpine as base
WORKDIR /src
RUN apk add gcc musl-dev

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

## Release #####################################################################
#
# This phase builds
##
