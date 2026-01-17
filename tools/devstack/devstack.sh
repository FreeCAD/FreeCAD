#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
REPO_DIR="$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel)"

cd "${REPO_DIR}"
exec python3 "${REPO_DIR}/tools/devstack/devstack.py" "$@"
