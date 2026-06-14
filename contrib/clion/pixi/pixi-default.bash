#!/bin/bash

# Change to the directory of this script (any subdirectory in the repository should work)
cd "$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" &> /dev/null && pwd -P)"

# Activate pixi default environment
eval "$(pixi shell-hook)"
