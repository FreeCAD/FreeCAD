# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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

"""Tests of the box style catalog: each style of the Fibre Box Handbook folds to a valid part."""

import unittest

import FreeCAD
import SheetDesign  # noqa: F401  Registers the document object types.
import SheetDesignBoxStyles


class TestBoxStyles(unittest.TestCase):
    """Folds each piece of each box style from its parametric blank."""

    THICKNESS = 4.0
    SIZE = (300.0, 200.0, 150.0)  # Inside length, width, and depth.

    def setUp(self):
        self.doc = FreeCAD.newDocument("BoxStyles")
        self.rule = self.doc.addObject("App::VarSet", "Rule")
        self.rule.addProperty("App::PropertyLength", "Thickness", "Sheet")
        self.rule.addProperty("App::PropertyLength", "BendRadius", "Sheet")
        self.rule.addProperty("App::PropertyFloat", "KFactor", "Sheet")
        self.rule.Thickness = self.THICKNESS
        self.rule.BendRadius = 0.0
        self.rule.KFactor = 0.5

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    #: Style code: the largest permitted overlap in mm^3, as a part of one for the test size.
    #: 0226: the two flap parts of the joint strip are on the corner of the last end panel.
    DOUBLE_BOARD = {"0226": 6000.0}

    def check_style(self, name, size):
        function = SheetDesignBoxStyles.STYLES[name]
        pieces = function(self.THICKNESS, *size)
        self.assertTrue(pieces)
        for index, piece in enumerate(pieces):
            folded = SheetDesignBoxStyles.fold(self.doc, self.rule, piece, f"P{index}")
            flat = self.doc.addObject("SheetDesign::FlatPattern", f"Flat{index}")
            flat.Source = folded
            self.doc.recompute()
            label = f"{name}, {piece.name}"
            self.assertTrue(folded.isValid(), f"{label}: {folded.getStatusString()}")
            self.assertTrue(folded.Shape.isValid(), label)
            self.assertEqual(len(folded.Shape.Solids), 1, label)
            # No two panels have material in the same space, but for a style where the
            # handbook blank has the board double in a zone.
            self.assertLessEqual(folded.Overlap.Value, self.DOUBLE_BOARD.get(name.split()[0], 0.0), label)
            # The flat pattern is the blank. A relief opening at a corner web is very small.
            self.assertTrue(flat.isValid(), f"{label}: {flat.getStatusString()}")
            area = piece.face.Area
            self.assertAlmostEqual(flat.Shape.Faces[0].Area, area, delta=1e-3 * area, msg=label)

    def test_all_styles_with_a_blank(self):
        for name, function in SheetDesignBoxStyles.STYLES.items():
            if function is None:
                continue
            with self.subTest(style=name):
                self.check_style(name, self.SIZE)

    def test_styles_follow_the_box_size(self):
        """The blanks are parametric: the same styles fold for a different box."""
        for name in (
            "0201 Regular Slotted Container",
            "0203 Full Overlap Slotted Container",
            "0215 Snap Bottom Container with Tuck Top",
            "0226 Bellows Style Top and Bottom Container",
            "0301 Full Telescope Design Style Container",
            "0427 Roll End Tray with Locking Cover",
            "0760 Self-Erecting Six Corner Tray",
        ):
            with self.subTest(style=name):
                self.check_style(name, (420.0, 260.0, 180.0))

    def test_catalog_state(self):
        """The count of the styles is a measure of the goal. Change it with the catalog."""
        styles = SheetDesignBoxStyles.STYLES
        missing = sorted(name for name, function in styles.items() if function is None)
        self.assertEqual(len(styles), 56)
        self.assertEqual(missing, [])
