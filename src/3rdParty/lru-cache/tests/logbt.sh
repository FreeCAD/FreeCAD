#!/bin/bash

# Taken from:
# https://github.com/mapbox/logbt

set -eu
set -o pipefail
shopt -s nullglob

export CORE_DIRECTORY=/tmp/logbt-coredumps

function error() {
  >&2 echo "$@"
  exit 1
}

if [[ $(uname -s) == 'Linux' ]]; then
  if ! which gdb > /dev/null; then
    error "Could not find required command 'gdb'"
  fi

  # if we have sudo then set core pattern
  if [[ $(id -u) == 0 ]]; then
    echo "Setting $(cat /proc/sys/kernel/core_pattern) -> ${CORE_DIRECTORY}/core.%p.%E"
    echo "${CORE_DIRECTORY}/core.%p.%E" > /proc/sys/kernel/core_pattern
  else
    # if we cannot modify the pattern we assert it has
    # already been set as we expect and need
    if [[ $(cat /proc/sys/kernel/core_pattern) != '/tmp/logbt-coredumps/core.%p.%E' ]]; then
      error "unexpected core_pattern: $(cat /proc/sys/kernel/core_pattern)"
      exit 1
    fi
    echo "Using existing corefile location: $(cat /proc/sys/kernel/core_pattern)"
  fi
else
  if ! which lldb > /dev/null; then
    error "Could not find required command 'lldb'"
    exit 1
  fi

  # if we have sudo then set core pattern
  if [[ $(id -u) == 0 ]]; then
    sudo sysctl kern.corefile=${CORE_DIRECTORY}/core.%P
  else
    if [[ $(sysctl -n kern.corefile) == '/cores/core.%P' ]]; then
      # OS X default is /cores/core.%P which works for logbt out of the box
      export CORE_DIRECTORY=/cores
    elif [[ $(sysctl -n kern.corefile) == '${CORE_DIRECTORY}/core.%P' ]]; then
      # all good, previously set
      :
    else
      # restore default with:
      # sudo sysctl kern.corefile=/cores/core.%P
      error "unexpected core_pattern: $(sysctl -n kern.corefile)"
      exit 1
    fi
    echo "Using existing corefile location: $(sysctl -n kern.corefile)"
  fi

  # Recommend running with the following setting to only show crashes
  # in the notification center
  # defaults write com.apple.CrashReporter UseUNC 1
fi

if [[ ! -d ${CORE_DIRECTORY} ]]; then
  # TODO: enable this once tests are adapted to extra stdout
  # echo "Creating directory for core files at '${CORE_DIRECTORY}'"
  mkdir -p ${CORE_DIRECTORY}
fi

# ensure we can write to the directory, otherwise
# core files might not be able to be written
WRITE_RETURN=0
touch ${CORE_DIRECTORY}/test.txt || WRITE_RETURN=$?
if [[ ${WRITE_RETURN} != 0 ]]; then
  error "Permissions problem: unable to write to ${CORE_DIRECTORY} (exited with ${WRITE_RETURN})"
  exit 1
else
  # cleanup from test
  rm ${CORE_DIRECTORY}/test.txt
fi

function process_core() {
  if [[ $(uname -s) == 'Darwin' ]]; then
    lldb --core ${2} --batch -o 'thread backtrace all' -o 'quit'
  else
    gdb ${1} --core ${2} -ex "set pagination 0" -ex "thread apply all bt" --batch
  fi
  # note: on OS X the -f avoids a hang on prompt 'remove write-protected regular file?'
  rm -f ${2}
}

function backtrace {
  local code=$?
  echo "$1 exited with code:${code}"
  if [[ $(uname -s) == 'Darwin' ]]; then
    local COREFILE="${CORE_DIRECTORY}/core.${CHILD_PID}"
    if [ -e ${COREFILE} ]; then
      echo "Found core at ${COREFILE}"
      process_core $1 ${COREFILE}
    else
      if [[ ${code} != 0 ]]; then
          echo "No core found at ${COREFILE}"
      fi
    fi
  else
    local SEARCH_PATTERN_BY_PID="core.${CHILD_PID}.*"
    local hit=false
    for corefile in ${CORE_DIRECTORY}/${SEARCH_PATTERN_BY_PID}; do
      echo "Found core at ${corefile}"
      # extract program name from corefile
      filename=$(basename "${corefile}")
      binary_program=/$(echo ${filename##*.\!} | tr '!' '/')
      process_core ${binary_program} ${corefile}
      hit=true
    done
    if [[ ${hit} == false ]] && [[ ${code} != 0 ]]; then
        echo "No core found at ${CORE_DIRECTORY}/${SEARCH_PATTERN_BY_PID}"
    fi
  fi
  local SEARCH_PATTERN_NON_TRACKED="core.*"
  local hit=false
  for corefile in ${CORE_DIRECTORY}/${SEARCH_PATTERN_NON_TRACKED}; do
    echo "Found non-tracked core at ${corefile}"
    hit=true
  done
  if [[ ${code} != 0 ]]; then
    if [[ ${hit} == true ]]; then
      echo "Processing cores..."
    fi
    for corefile in ${CORE_DIRECTORY}/${SEARCH_PATTERN_NON_TRACKED}; do
      filename=$(basename "${corefile}")
      binary_program=/$(echo ${filename##*.\!} | tr '!' '/')
      process_core ${binary_program} ${corefile}
    done
  else
    if [[ ${hit} == true ]]; then
      echo "Skipping processing cores..."
    fi
  fi
  exit $code
}

function warn_on_existing_cores() {
  local SEARCH_PATTERN_NON_TRACKED="core.*"
  # at startup warn about existing corefiles, since these are unexpected
  for corefile in ${CORE_DIRECTORY}/${SEARCH_PATTERN_NON_TRACKED}; do
    echo "WARNING: Found existing corefile at ${corefile}"
  done
}

warn_on_existing_cores

# Hook up function to run when logbt exits
trap "backtrace $1" EXIT

# Enable corefile generation
ulimit -c unlimited

# Run the child process in a background process
# in order to get the PID
$* & export CHILD_PID=$!

# Keep logbt running as long as the child is running
# to be able to hook into a potential crash
wait ${CHILD_PID}
