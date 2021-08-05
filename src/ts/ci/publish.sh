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
if [ "$TS_RELEASE_VERSION" == "" ]
then
    echo "Must specify TS_RELEASE_VERSION"
    exit 1
fi
version_placeholder="0.0.0-REPLACEME"
if [[ "$OSTYPE" == "darwin"* ]]
then
    sed_cmd="sed -i .bak"
else
    sed_cmd="sed -i.bak"
fi
$sed_cmd "s,$version_placeholder,$TS_RELEASE_VERSION,g" package.json
rm package.json.bak

# If this is a dry run, add the flag
dry_run_flag=""
if [ "$RELEASE_DRY_RUN" == "true" ]
then
    dry_run_flag="--dry-run"
fi

# Run the build
npm run build

# Run publish
npm whoami
npm publish $dry_run_flag

# Replace the placeholder
$sed_cmd "s,$TS_RELEASE_VERSION,$version_placeholder,g" package.json
rm package.json.bak
