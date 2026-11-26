# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
# *   Copyright (c) 2025 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""Unit tests for the Draft Workbench, array tests."""

## @package test_array
# \ingroup drafttests
# \brief Unit tests for the Draft Workbench, array tests.

## \addtogroup drafttests
# @{

from pathlib import Path
from tempfile import TemporaryDirectory

import Draft
import FreeCAD as App
from FreeCAD import Placement, Rotation, Vector
from drafttests import test_base


class DraftArray(test_base.DraftTestCaseDoc):
    """Test Draft array functions."""

    def test_link_array(self):
        """Create a link array."""
        box = self.doc.addObject("Part::Box", "Box")
        box.Label = "Box"
        self.doc.recompute()

        array = Draft.make_ortho_array(
            box,
            v_x=Vector(100.0, 0.0, 0.0),
            v_y=Vector(0.0, 100.0, 0.0),
            v_z=Vector(0.0, 0.0, 100.0),
            n_x=12,
            n_y=1,
            n_z=1,
            use_link=True,
        )

        Draft.autogroup(array)
        array.ExpandArray = True
        array.Fuse = False
        self.doc.recompute(None, True, True)

        array.NumberX = 6
        self.doc.recompute(None, True, True)
        self.assertEqual(array.Count, array.NumberX)

        array.NumberX = 24
        self.doc.recompute(None, True, True)
        self.assertEqual(array.Count, array.NumberX)

        self.doc.recompute(None, True, True)
        self.assertEqual(array.Count, array.NumberX)

    def test_link_array_synced_elements_are_clean_after_recompute(self):
        """Keep expanded link elements synchronized without re-touching them."""
        axis = Vector(0, 0, 1)
        circle = Draft.make_circle(2, placement=Placement(Vector(10, 0, 0), Rotation()))
        self.doc.recompute()

        array = Draft.make_polar_array(
            circle, number=4, angle=360, center=Vector(1, 0, 0), axis=axis, use_link=True
        )
        array.ExpandArray = True
        array.AlwaysSyncPlacement = True
        self.doc.recompute()

        array.Center = Vector(2, 0, 0)
        self.doc.recompute()
        expected = [
            Placement(Vector(10, 0, 0), Rotation(axis, 0)),
            Placement(Vector(2, 8, 0), Rotation(axis, 90)),
            Placement(Vector(-6, 0, 0), Rotation(axis, 180)),
            Placement(Vector(2, -8, 0), Rotation(axis, 270)),
        ]
        elements = array.ElementList
        self.assertEqual(len(elements), len(expected))
        for element, placement in zip(elements, expected):
            self.assertTrue(element.Placement.isSame(placement, 1e-7), element.Name)

        with self.subTest(operation="center change"):
            self.assertEqual(
                [element.Name for element in elements if "Touched" in element.State], []
            )

        array.touch()
        self.doc.recompute()
        with self.subTest(operation="forced repeat"):
            self.assertEqual(
                [element.Name for element in elements if "Touched" in element.State], []
            )

    def test_link_array_synced_elements_are_clean_on_reopen(self):
        """Keep synced elements clean when restoring saved polar placements."""
        circle = Draft.make_circle(2, placement=Placement(Vector(10, 0, 0), Rotation()))
        self.doc.recompute()
        array = Draft.make_polar_array(
            circle, number=5, angle=360, center=Vector(1, 0, 0), use_link=True
        )
        array.ExpandArray = True
        array.AlwaysSyncPlacement = True
        self.doc.recompute()
        array.Center = Vector(2, 0, 0)
        self.doc.recompute()
        expected = [element.Placement for element in array.ElementList]

        with TemporaryDirectory() as directory:
            path = str(Path(directory) / "SyncedArray.FCStd")
            self.doc.saveCopy(path)
            reopened = App.openDocument(path)
            try:
                restored = reopened.getObject(array.Name)
                self.assertTrue(restored.ExpandArray)
                self.assertTrue(restored.AlwaysSyncPlacement)
                elements = restored.ElementList
                self.assertEqual(len(elements), 5)
                for element, placement in zip(elements, expected):
                    self.assertTrue(element.Placement.isSame(placement, 1e-7), element.Name)
                self.assertEqual(
                    [element.Name for element in elements if "Touched" in element.State], []
                )
            finally:
                App.closeDocument(reopened.Name)


## @}
