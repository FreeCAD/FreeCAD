#!/usr/bin/env bash
#
# Example machine-local devstack build environment for distcc + ccache.
#
# Usage (per-worktree, recommended):
#   cp tools/devstack/examples/env.distcc.sh .devstack/env.sh
#   $EDITOR .devstack/env.sh   # set DISTCC_HOSTS for your network
#
# Then build:
#   ./tools/devstack/devstack.sh build --preset debug --distcc -j12
#
# Notes:
# - `.devstack/env.sh` is gitignored (local-only).
# - `--distcc` implies `CMAKE_*_COMPILER_LAUNCHER=ccache` during configure if not already set.

export DISTCC_HOSTS="distcc-server/12"
export CCACHE_PREFIX=distcc
export CCACHE_CPP2=yes
