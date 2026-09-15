#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/../../.." && pwd)"

cd "${repo_root}"

python3 src/Tools/typing/generate_stubs.py check \
    --root . \
    --out-dir src/Tools/typing/generated
