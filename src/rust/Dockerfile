## Base #######################################################################
#
# This phase sets up dependencies for the other phases
##
FROM rust:1.97 as base
WORKDIR /src
RUN rustup component add rustfmt clippy

## Test ########################################################################
#
# This phase runs the unit tests, lints, and formatting checks for the library
##
FROM base as test
COPY . /src
# NOTE: deliberately not `--all-features` - the `disable-logging` feature
# strips every logging macro down to a no-op, which would break the
# assertions in most of the functional test suite (by design, not by bug).
# It gets its own clippy/test pass instead; see the crate-level docs on
# `disable-logging` in src/lib.rs for the full story.
RUN true && \
    cargo fmt --check && \
    cargo clippy --all-targets -- -D warnings && \
    cargo test && \
    cargo clippy --all-targets --features disable-logging -- -D warnings && \
    cargo test --features disable-logging && \
    cargo build --release --examples && \
    cargo build --release --examples --features disable-logging && \
    true

## Release #####################################################################
#
# This phase publishes the crate to crates.io
##
FROM base as release
ARG RUST_RELEASE_VERSION
ARG CARGO_REGISTRY_TOKEN
COPY . /src
# Overwrite the sentinel version in Cargo.toml with the real version derived
# from the git tag before publishing.
RUN sed -i.bak "s/^version = \"0\.0\.0\"/version = \"${RUST_RELEASE_VERSION}\"/" Cargo.toml && \
    rm Cargo.toml.bak && \
    cargo publish --token ${CARGO_REGISTRY_TOKEN}

## Release Test ################################################################
#
# This phase builds and runs the fibonacci example against the tagged release
# version pulled from crates.io
##
FROM base as release_test
ARG RUST_RELEASE_VERSION
COPY ./examples/fib.rs /src/examples/fib.rs
COPY ./ci/test_release.sh /src/ci/test_release.sh
RUN ./ci/test_release.sh
