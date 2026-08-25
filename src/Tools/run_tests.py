#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

"""Run the Python tests owned by ``src/Tools`` from one canonical command."""

from __future__ import annotations

from pathlib import Path
import sys
import unittest

TOOLS_DIR = Path(__file__).resolve().parent
TYPING_DIR = TOOLS_DIR / "typing"
ROOT_DIR = TOOLS_DIR.parents[1]

# The tests intentionally use the same top-level imports as the executable
# tools. Keep path setup here rather than repeating it in individual tests.
sys.path[0:0] = [str(ROOT_DIR), str(TOOLS_DIR), str(TYPING_DIR)]


def load_suite() -> unittest.TestSuite:
    """Discover model, StubGen, and legacy Tools tests in one suite."""

    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    suite.addTests(
        loader.discover(
            str(TOOLS_DIR / "python_api_model"),
            pattern="test_*.py",
            top_level_dir=str(TOOLS_DIR),
        )
    )
    suite.addTests(
        loader.discover(
            str(TYPING_DIR / "stubgen"),
            pattern="test_*.py",
            top_level_dir=str(TYPING_DIR),
        )
    )
    # Use a fresh loader because ``discover`` retains the previous loader's
    # top-level directory when no explicit top-level directory is supplied.
    suite.addTests(unittest.TestLoader().discover(str(TOOLS_DIR / "tests"), pattern="test_*.py"))
    return suite


if __name__ == "__main__":
    result = unittest.TextTestRunner(verbosity=2).run(load_suite())
    raise SystemExit(not result.wasSuccessful())
