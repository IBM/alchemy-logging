#!/usr/bin/env bash

# Run from the project root
cd $(dirname ${BASH_SOURCE[0]})/..

# Get the tag for this release
tag=$(echo $REF | cut -d'/' -f3-)
echo "The tag is ${tag}!!"

# Parse the tag for prefix and suffix
release_type=$(echo $tag | cut -d'-' -f1)
version=$(echo $tag | cut -d'-' -f2)

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
    echo "NOT IMPLEMENTED YET!"
    exit 1

# Go is special and requires valid semantic versioning for its version tags and
# those tags must be scoped by the subdirectory where the go.mod file lives
elif [[ "$tag" =~ src/go/v[0-9]+\.[0-9]+\.[0-9]+.* ]]
then
    cd src/go
    docker build . \
        --target=release_test \
        --build-arg GO_RELEASE_VERSION=$tag

else
    echo "Unknown release type: [$release_type]"
    exit 1
fi
