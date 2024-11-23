#!/bin/bash
set -e
SCRIPT_DIR="$(realpath $(dirname "${BASH_SOURCE}"))"

# Run formatting ignoring the last time it was run
source "$SCRIPT_DIR/repo.sh" format --force
