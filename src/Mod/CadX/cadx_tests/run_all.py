# SPDX-License-Identifier: LGPL-2.1-or-later

"""Run every cad-x unit test with any Python >= 3.10 (FreeCAD not required).

Usage::

    python3 run_all.py            # from anywhere
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path


if __name__ == "__main__":
    here = Path(__file__).resolve().parent
    sys.path.insert(0, str(here.parent))
    loader = unittest.defaultTestLoader
    suite = loader.discover(start_dir=str(here), top_level_dir=str(here))
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    sys.exit(0 if result.wasSuccessful() else 1)
