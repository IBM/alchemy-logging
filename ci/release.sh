#!/usr/bin/env bash

# Run from the project root
cd $(dirname ${BASH_SOURCE[0]})/..

# Get the tag for this release
tag=$(echo $REF | cut -d'/' -f3-)
echo "The tag is ${tag}!!"

# Parse the tag for prefix and suffix
release_type=$(echo $tag | cut -d'-' -f1)
version=$(echo $tag | cut -d'-' -f2)

# We explicitly don't want to run with buildkit so that the docker builds happen
# in a linear fashion since our `release_test` stages intentionally don't
# inherit from the stages where the publication happens.
export DOCKER_BUILDKIT=0

# Dispatch to the various types of releases
if [ "$release_type" == "py" ]
then
    cd src/python
    docker build . \
        --target=release_test \
        --build-arg PYTHON_RELEASE_VERSION=$version \
        --build-arg PYPI_TOKEN=$PYPI_TOKEN
elif [ "$release_type" == "ts" ]
then
    cd src/ts
    docker build . \
        --target=release_test \
        --build-arg TS_RELEASE_VERSION=$version \
        --build-arg NPM_TOKEN=$NPM_TOKEN

elif [ "$release_type" == "cpp" ]
then
    cd src/cpp
    docker build . \
        --target=release_test \
        --build-arg CPP_RELEASE_VERSION=$tag

# Go is special and requires valid semantic versioning for its version tags and
# those tags must be scoped by the subdirectory where the go.mod file lives
elif [[ "$tag" =~ src/go/v[0-9]+\.[0-9]+\.[0-9]+.* ]]
then
    cd src/go
    version=$(echo $tag | rev | cut -d'/' -f 1 | rev)
    docker build . \
        --target=release_test \
        --build-arg GO_RELEASE_VERSION=$version

else
    echo "Unknown release type: [$release_type]"
    exit 1
fi
