# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Furgo
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

"""DXF import tests."""

import FreeCAD
import Import
from importtests.TestImportBase import TestImportBase
from importtests.fixtures import fixture_path

OPTIONS_PARENT = "User parameter:BaseApp/Preferences/Mod/Import"
OPTIONS_GROUP = "TestImportDXF"

# Value of ImportMode::IndividualShapes in App/dxf/ImpExpDxf.h: one shape per DXF entity.
INDIVIDUAL_SHAPES = 2


class TestImportDXF(TestImportBase):
    """Tests for the DXF importer."""

    def setUp(self):
        """Set the DXF import options, independent of the user's preferences."""
        super().setUp()
        # Reading the group creates it, so it is removed after each test.
        parent = FreeCAD.ParamGet(OPTIONS_PARENT)
        self.addCleanup(parent.RemGroup, OPTIONS_GROUP)
        options = parent.GetGroup(OPTIONS_GROUP)
        options.SetInt("DxfImportMode", INDIVIDUAL_SHAPES)
        # Layers are imported as Draft objects; disabled to keep these tests independent of Draft.
        options.SetBool("dxfUseDraftVisGroups", False)
        self.option_source = f"{OPTIONS_PARENT}/{OPTIONS_GROUP}"

    def test_read_lines(self):
        """Import four DXF lines forming a 10 x 5 rectangle as four edge shapes."""
        stats = Import.readDXF(
            fixture_path("rectangle_lines.dxf"), self.document.Name, True, self.option_source
        )

        self.assertEqual(stats["entityCounts"], {"LINE": 4})
        self.assertEqual(stats["totalEntitiesCreated"], 4)
        # The file declares millimeters, the unit of FreeCAD documents, so no scaling is applied.
        self.assertEqual(stats["finalScalingFactor"], 1.0)

        objects = self.document.Objects
        self.assertEqual(len(objects), 4)
        self.assertTrue(all(obj.isDerivedFrom("Part::Feature") for obj in objects))

        lengths = sorted(round(obj.Shape.Length, 6) for obj in objects)
        self.assertEqual(lengths, [5.0, 5.0, 10.0, 10.0])

        bound_box = objects[0].Shape.BoundBox
        for obj in objects[1:]:
            bound_box.add(obj.Shape.BoundBox)
        self.assertAlmostEqual(bound_box.XMin, 0.0)
        self.assertAlmostEqual(bound_box.YMin, 0.0)
        self.assertAlmostEqual(bound_box.XMax, 10.0)
        self.assertAlmostEqual(bound_box.YMax, 5.0)
        self.assertAlmostEqual(bound_box.ZLength, 0.0)
