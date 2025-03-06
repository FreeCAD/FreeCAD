#!/bin/bash

# Change to the directory of this script (any subdirectory in the repository should work)
cd "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"

# If the line above does not work then try uncommenting the line below and insert the absolute path to your FreeCAD repository
# cd "absolute/path/to/FreeCAD"

# Activate pixi default environment
eval "$(pixi shell-hook)"
