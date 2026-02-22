#!/usr/bin/env bash
# SPDX-FileNotice: Part of the FreeCAD project.

if [[ "$OSTYPE" =~ (msys*|cygwin*|mingw*) ]]; then
    echo windows
elif [[ "$OSTYPE" == darwin* ]]; then
    echo osx
elif [[ "$OSTYPE" == linux* ]]; then
    echo linux
fi
