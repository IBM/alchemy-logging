FROM python:3.7-slim as base

#-- Wheel ----------------------------------------------------------------------
FROM base as wheel

COPY alog /src/alog
COPY setup.py /src/
COPY README.md /src/
WORKDIR /src

RUN python3 setup.py bdist_wheel

#-- Test -----------------------------------------------------------------------
FROM base as test

COPY --from=wheel /src/dist/alog-*-py3-none-any.whl /src/dist/
RUN pip3 install /src/dist/alog-*-py3-none-any.whl

COPY tests /src/tests

RUN python3 /src/tests/test_alog.py

#-- Publish --------------------------------------------------------------------
FROM wheel as publish

# Set up the env to publish to the local artifactory repo
ARG ARTIFACTORY_REPOSITORY=https://na.artifactory.swg-devops.com/artifactory/api/pypi/wcp-nlp-pypi-virtual
ARG ARTIFACTORY_USERNAME
ARG ARTIFACTORY_API_KEY

# Create the .pypirc file to allow remote pushes
#
# NOTE: This build target should NEVER be pushed to a registry since it contains
#   personal credentials
RUN echo "[distutils]\n\
index-servers = local\n\
[local]\n\
repository: ${ARTIFACTORY_REPOSITORY}\n\
username: ${ARTIFACTORY_USERNAME}\n\
password: ${ARTIFACTORY_API_KEY}\n\
" > $HOME/.pypirc
RUN python3 setup.py sdist bdist_wheel upload -r local
