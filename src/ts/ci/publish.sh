#!/usr/bin/env bash

# Run from the base of the ts directory
cd $(dirname ${BASH_SOURCE[0]})/..

# Make sure NPM_TOKEN is set
if [ "$NPM_TOKEN" == "" ]
then
    echo "Must set NPM_TOKEN"
    exit 1
fi

# Make sure the provided version matches the version in package.json
package_version=$(cat package.json | jq -r '.version')
if [ "$TS_RELEASE_VERSION" != $package_version ]
then
    echo "Version mismatch. Attempting to publish [$TS_RELEASE_VERSION] but package at [$package_version]"
    exit 1
fi

# If this is a dry run, add the flag
dry_run_flag=""
if [ "$RELEASE_DRY_RUN" == "true" ]
then
    dry_run_flag="--dry-run"
fi

# Run publish
npm publish $dry_run_flag
