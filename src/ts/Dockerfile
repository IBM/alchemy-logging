## Base #######################################################################
#
# This phase sets up dependenices for the other phases
##
FROM node:14-slim as base

# This image is only for building, so we run as root
WORKDIR /src

# Install build, test, andn publish dependencies
COPY *.json /src/
RUN true && \
    npm install && \
    apt-get update && \
    apt-get install -y jq && \
    apt-get clean autoclean && \
    apt-get autoremove -y && \
    rm -rf /var/lib/{apt,dpkg,cache,log}/ && \
    true

## Test ########################################################################
#
# This phase runs the unit tests for the library
##
FROM base as test
COPY . /src
RUN npm test

## Release #####################################################################
#
# This phase builds the release and publishes it to npm
##
FROM test as release
ARG NPM_TOKEN
ARG TS_RELEASE_VERSION
ARG RELEASE_DRY_RUN
RUN ./ci/publish.sh

## Release Test ################################################################
#
# This phase installs the indicated version from npm and runs the unit tests
# against the installed version
##
FROM base as release_test
ARG TS_RELEASE_VERSION
ARG RELEASE_DRY_RUN
COPY ./test /src/test
RUN true && \
    ([ "$RELEASE_DRY_RUN" != "true" ] && sleep 30 || true) && \
    sed -i'' 's,alchemy-logging,test-alchemy-logging,g' package.json && \
    sed -i'' 's,../src,alchemy-logging,g' test/*.ts && \
    npm install alchemy-logging@${TS_RELEASE_VERSION} && \
    npm test && \
    true