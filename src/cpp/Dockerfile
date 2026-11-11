## Base #######################################################################
#
# This phase sets up dependenices for the other phases
##
FROM gcc:16 AS base

# This image is only for building, so we run as root
WORKDIR /src

# Install CMake and dependencies
ARG CMAKE_VERSION=4.4.5
RUN true && \
    ARCH=$(uname -m) && \
    wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-${ARCH}.sh && \
    chmod +x cmake-${CMAKE_VERSION}-linux-${ARCH}.sh && \
    ./cmake-${CMAKE_VERSION}-linux-${ARCH}.sh --skip-license --prefix=/usr/local/ && \
    which cmake && \
    cmake --version && \
    rm cmake-${CMAKE_VERSION}-linux-${ARCH}.sh && \
    apt-get update -y && \
    apt-get install -y libgtest-dev libgmock-dev && \
    apt-get clean autoclean && \
    apt-get autoremove --yes && \
    true

## Test ########################################################################
#
# This phase runs the unit tests for the library
##
FROM base AS test
COPY . /src
RUN true && \
    mkdir build && \
    cd build && \
    cmake .. -D CMAKE_CXX_FLAGS="-Wall -Werror" -D JSON_Install=ON && \
    make && \
    ctest && \
    make install && \
    true
RUN test -e /usr/local/include/alog/logger.hpp
RUN test -e /usr/local/lib/libalog.so
RUN test -e /usr/local/include/nlohmann/json.hpp

## Release Test ################################################################
#
# This phase builds the fibonacci example using the tagged release version
##
FROM base AS release_test
ARG CPP_RELEASE_VERSION
COPY ./tools/alog_fib_example /src
RUN true && \
    mkdir build && \
    cd build && \
    cmake .. -DALOG_VERSION=${CPP_RELEASE_VERSION} && \
    make && \
    ./alog_fib_example 5 && \
    true
