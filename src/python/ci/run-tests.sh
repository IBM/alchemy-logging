#!/usr/bin/env bash

set -e
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$BASE_DIR"

python3 -m pytest \
    --cov=alog \
    --cov=util \
    --cov-report=term \
    --cov-report=html \
    -Werror "$@"
