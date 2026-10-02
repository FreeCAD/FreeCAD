# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

"""Test data files for the Import module tests, and a helper to locate them. See README.md."""

import os


def fixture_path(name):
    """Return the full path of the fixture file `name` in this directory."""
    return os.path.join(os.path.dirname(__file__), name)
