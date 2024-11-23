#!/usr/bin/env bash

renice -n 19 -p $$ > /dev/null 2>&1
ionice -c  3 -p $$ > /dev/null 2>&1

set -o errexit
SCRIPT_DIR="$(realpath $(dirname "${BASH_SOURCE}"))"
cd "${SCRIPT_DIR}"

# Run tests using repo_test
exec ./repo.sh test "$@"
