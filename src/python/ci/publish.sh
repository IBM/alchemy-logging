#!/usr/bin/env bash

# Run from the base of the python directory
cd $(dirname ${BASH_SOURCE[0]})/..

# Clear out old publication files in case they're still around
rm -rf build dist alchemy_logging.egg-info/

# Build
python setup.py sdist bdist_wheel

# Publish to PyPi
if [ "${RELEASE_DRY_RUN}" != "true" ]
then
    twine upload \
        --username "__token__" \
        --password "$PYPI_TOKEN" \
        dist/*
else
    echo "Release DRY RUN"
fi

# Clean up
rm -rf build dist alchemy_logging.egg-info/
