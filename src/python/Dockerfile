FROM python:3.6-slim as base

ARG PYPI_TOKEN

# This image is only for building, so we run as root
WORKDIR /src

# Install build, test, andn publish dependencies
COPY requirements_test.txt /src/requirements_test.txt
RUN true && \
    pip install pip --upgrade && \
    pip install twine && \
    pip install -r /src/requirements_test.txt && \
    true

# Copy in everything
COPY . /src

# Run tests
RUN ./ci/run-tests.sh

# Publish
RUN ./ci/publish.sh
