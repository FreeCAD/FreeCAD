#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
verify-distcc.sh [--host desktop] [--port 3632] [--jobs 12] [--build-jobs 12] [--keep]

Creates a tiny CMake+Ninja C++ build that compiles many translation units and checks
remote distccd logs to confirm compiles were dispatched.

Requires on client:
  - cmake, ninja, clang/clang++, ccache, distcc

Requires on server (host):
  - distccd running (systemd unit), journal readable (or sudo access)

Environment (recommended):
  DISTCC_HOSTS="desktop/12"
  CCACHE_PREFIX=distcc
  CCACHE_CPP2=yes

Example:
  tools/devstack/distcc/verify-distcc.sh --host desktop --jobs 12 --build-jobs 12
USAGE
}

HOST="desktop"
PORT="3632"
JOBS="12"
BUILD_JOBS="12"
KEEP="0"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --host) HOST="${2:-}"; shift 2;;
    --port) PORT="${2:-}"; shift 2;;
    --jobs) JOBS="${2:-}"; shift 2;;
    --build-jobs) BUILD_JOBS="${2:-}"; shift 2;;
    --keep) KEEP="1"; shift;;
    -h|--help) usage; exit 0;;
    *) echo "unknown arg: $1" >&2; usage; exit 2;;
  esac
done

need() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "missing required command: $1" >&2
    exit 1
  fi
}

need cmake
need ninja
need ccache
need distcc
need clang++
need ssh

if ! [[ "${JOBS}" =~ ^[0-9]+$ ]] || [[ "${JOBS}" -lt 1 ]]; then
  echo "--jobs must be a positive integer" >&2
  exit 2
fi
if ! [[ "${BUILD_JOBS}" =~ ^[0-9]+$ ]] || [[ "${BUILD_JOBS}" -lt 1 ]]; then
  echo "--build-jobs must be a positive integer" >&2
  exit 2
fi

export DISTCC_HOSTS="${DISTCC_HOSTS:-${HOST}:${PORT}/${JOBS}}"
export CCACHE_PREFIX="${CCACHE_PREFIX:-distcc}"
export CCACHE_CPP2="${CCACHE_CPP2:-yes}"

echo "DISTCC_HOSTS=${DISTCC_HOSTS}"
echo "CCACHE_PREFIX=${CCACHE_PREFIX}"
echo "CCACHE_CPP2=${CCACHE_CPP2}"

echo
echo "Checking server connectivity..."
ssh -o ConnectTimeout=5 "${HOST}" "true"

echo "Checking distccd log access..."
REMOTE_START="$(ssh "${HOST}" "date --iso-8601=seconds")"
REMOTE_JOURNAL_CMD="journalctl -u distccd --since '${REMOTE_START}' --no-pager"
if ! ssh "${HOST}" "${REMOTE_JOURNAL_CMD} >/dev/null 2>&1"; then
  if ssh "${HOST}" "sudo -n true >/dev/null 2>&1"; then
    REMOTE_JOURNAL_CMD="sudo ${REMOTE_JOURNAL_CMD}"
  else
    echo "warning: cannot read distccd journal (no permission, no passwordless sudo); will still build." >&2
    REMOTE_JOURNAL_CMD=""
  fi
fi

echo
echo "Creating build sandbox..."
TMPDIR="$(mktemp -d -t distcc-smoketest.XXXXXX)"
cleanup() {
  if [[ "${KEEP}" == "1" ]]; then
    echo "kept sandbox: ${TMPDIR}"
    return
  fi
  rm -rf "${TMPDIR}"
}
trap cleanup EXIT

mkdir -p "${TMPDIR}/src"

cat >"${TMPDIR}/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.16)
project(distcc_smoketest LANGUAGES CXX)

add_library(distcc_smoketest STATIC)
target_compile_features(distcc_smoketest PRIVATE cxx_std_17)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
target_sources(distcc_smoketest PRIVATE ${SRC_FILES})

add_executable(distcc_smoketest_main src/main.cpp)
target_link_libraries(distcc_smoketest_main PRIVATE distcc_smoketest)
CMAKE

cat >"${TMPDIR}/src/main.cpp" <<'CPP'
#include <iostream>
int main() {
    std::cout << "distcc-smoketest\n";
    return 0;
}
CPP

for i in $(seq 1 80); do
  cat >"${TMPDIR}/src/tu_${i}.cpp" <<CPP
#include <array>
#include <numeric>
#include <vector>

namespace tu_${i} {
template<int N>
struct Fib { static constexpr int value = Fib<N-1>::value + Fib<N-2>::value; };
template<> struct Fib<1> { static constexpr int value = 1; };
template<> struct Fib<0> { static constexpr int value = 0; };

int work_${i}() {
    std::array<int, 256> a{};
    for (int j = 0; j < static_cast<int>(a.size()); ++j) a[j] = (j + ${i}) % 97;
    const int s = std::accumulate(a.begin(), a.end(), 0);
    return s + Fib<24>::value;
}
} // namespace tu_${i}
CPP
done

echo "Configuring with clang + ccache launcher..."
cmake -S "${TMPDIR}" -B "${TMPDIR}/build" -GNinja \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_BUILD_TYPE=Release >/dev/null

echo
echo "Building (-j${BUILD_JOBS})..."
export DISTCC_VERBOSE="${DISTCC_VERBOSE:-1}"
cmake --build "${TMPDIR}/build" -j "${BUILD_JOBS}"

echo
echo "ccache stats:"
ccache -s | sed -n '1,25p'

if [[ -n "${REMOTE_JOURNAL_CMD}" ]]; then
  echo
  echo "Remote distccd log lines since ${REMOTE_START}:"
  ssh "${HOST}" "${REMOTE_JOURNAL_CMD} | tail -n 30"
  COUNT="$(ssh "${HOST}" "${REMOTE_JOURNAL_CMD} | wc -l | tr -d ' '")"
  echo
  echo "Remote distccd log line count: ${COUNT}"
  if [[ "${COUNT}" -eq 0 ]]; then
    echo "warning: no distccd log lines observed; build may not be dispatching remotely." >&2
  fi
fi

echo
echo "Done."
