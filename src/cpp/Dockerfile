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
    apt-get install -y libboost-locale-dev && \
    apt-get clean autoclean && \
    apt-get autoremove --yes && \
    true