#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: client-env.sh <host> [jobs] [port]" >&2
  exit 2
fi

HOST="$1"
JOBS="${2:-12}"
PORT="${3:-3632}"

cat <<EOF
export DISTCC_HOSTS="${HOST}:${PORT}/${JOBS}"
export CCACHE_PREFIX=distcc
export CCACHE_CPP2=yes
EOF

