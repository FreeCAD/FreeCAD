#!/usr/bin/env bash
# Run FreeCAD with Nix environment's Python packages

SITE_PACKAGES=$(python3 -c "import site; print(site.getsitepackages()[0])")
exec ./build/bin/FreeCAD --python-path "$SITE_PACKAGES" "$@"