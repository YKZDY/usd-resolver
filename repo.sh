#!/bin/bash

set -e

SCRIPT_DIR=$(dirname ${BASH_SOURCE})
cd "$SCRIPT_DIR"

# Use "exec" to ensure that envrionment variables don't accidentally affect other processes.
OMNI_REPO_ROOT="$SCRIPT_DIR" exec "tools/packman/python.sh" tools/repoman/repoman.py "$@"
