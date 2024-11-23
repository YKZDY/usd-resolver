#!/usr/bin/env bash

renice -n 19 -p $$ > /dev/null 2>&1
ionice -c  3 -p $$ > /dev/null 2>&1

set -o errexit
SCRIPT_DIR="$(realpath $(dirname "${BASH_SOURCE}"))"
cd "${SCRIPT_DIR}"

: "${OMNI_USD_FLAVOR:=usd}"
: "${OMNI_USD_VER:=24.05}"
: "${OMNI_PYTHON_VER:=3.10}"

export OMNI_USD_FLAVOR OMNI_USD_VER OMNI_PYTHON_VER

# Generate USD Resolver required files
"$SCRIPT_DIR"/tools/generate.sh "$@"

# Build USD Resolver with repo_build
"$SCRIPT_DIR"/repo.sh build "$@"
