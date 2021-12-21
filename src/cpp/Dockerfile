## Base #######################################################################
#
# This phase sets up dependenices for the other phases
##
FROM gcc:9 as base

# This image is only for building, so we run as root
WORKDIR /src

# Install CMake and dependencies
ARG CMAKE_VERSION=3.21.1
RUN true && \
    wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh && \
    chmod +x cmake-${CMAKE_VERSION}-linux-x86_64.sh && \
    ./cmake-3.21.1-linux-x86_64.sh --skip-license --prefix=/usr/local/ && \
    which cmake && \
    cmake --version && \
    rm cmake-${CMAKE_VERSION}-linux-x86_64.sh && \
    apt-get update -y && \
    apt-get install -y libgtest-dev && \
    apt-get clean autoclean && \
    apt-get autoremove --yes && \
    true

## Test ########################################################################
#
# This phase runs the unit tests for the library
##
FROM base as test
COPY . /src
RUN true && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    ctest && \
    make install && \
    test -e /usr/local/include/alog/logger.hpp && \
    test -e /usr/local/lib/libalog.so && \
    test -e /usr/local/include/nlohmann/json.hpp && \
    true

## Release Test ################################################################
#
# This phase builds the fibonacci example using the tagged release version
##
FROM base as release_test
ARG CPP_RELEASE_VERSION
COPY ./tools/alog_fib_example /src
RUN true && \
    mkdir build && \
    cd build && \
    cmake .. -DALOG_VERSION=${CPP_RELEASE_VERSION} && \
    make && \
    ./alog_fib_example 5 && \
    true
