# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 David Kaufman
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Tests for slicing operations."""

import pathlib
import unittest

import FreeCAD

FIXTURE_PATH = pathlib.Path(__file__).parent / "Fixtures"


def _get_z_depths(obj, z_max=0):
    """Return sorted list of unique Z values <= z_max from a CAM path object."""
    zs = set()
    for cmd in obj.Path.Commands:
        z = cmd.Parameters.get("Z", z_max + 1)
        if z <= z_max:
            zs.add(z)
    return sorted(zs)


class TestSlicer(unittest.TestCase):
    """Tests for Path.Area slicing through complex geometry."""

    def test_17748_cam_profile(self):
        """
        Tests for regressions on issue #17748. In that issue, the pocket and mill face
        operations do not generate the final/deepest slice. This test asserts that
        each operation generates the expected number of layers.
        """
        doc = FreeCAD.openDocument(str(FIXTURE_PATH / "test_17748_cam_profile.FCStd"))
        try:
            pocket = doc.getObject("Pocket_Shape")
            pocket.recompute()
            self.assertEqual(len(_get_z_depths(pocket)), 4)

            millface = doc.getObject("MillFace")
            millface.recompute()
            self.assertEqual(len(_get_z_depths(millface)), 2)
        finally:
            FreeCAD.closeDocument(doc.Name)

    def test_28534_truncated_pocket(self):
        """
        Tests for regressions on issue #28534. In that issue, the pocket operation does
        not generate the final/deepest slice. This test asserts that exactly 3 layers
        are present in the recomputed toolpath.
        """
        doc = FreeCAD.openDocument(str(FIXTURE_PATH / "test_28534_truncated_pocket.FCStd"))
        try:
            obj = doc.getObject("Pocket_Shape")
            obj.recompute()
            zs = _get_z_depths(obj)
            self.assertEqual(len(zs), 3)
        finally:
            FreeCAD.closeDocument(doc.Name)


if __name__ == "__main__":
    unittest.main()
