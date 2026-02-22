# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

import os
from unittest import TestCase


def assertFilePathsEqual(self: TestCase, path1: os.PathLike, path2: os.PathLike):
    self.assertEqual(os.path.realpath(path1), os.path.realpath(path2))
