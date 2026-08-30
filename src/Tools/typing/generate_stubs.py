#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

"""CLI entrypoint for the Python binding stub generation pipeline.

Keep this thin so scripts and documentation can point at a stable public
command while the implementation lives under ``stubgen``.
"""

from stubgen.cli import main

if __name__ == "__main__":
    raise SystemExit(main())
