# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2026 FreeCAD Developers                                 *
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

import os
import shutil
import tempfile

import Draft
from bimtests import TestArchBase
from importers import exportIFC
from nativeifc import ifc_import


class TestIfcExportImport2D(TestArchBase.TestArchBase):
    """
    Unit test to check the export and import of 2D geometry.
    Currently only straight and circular edges are supported.
    """

    def setUp(self):
        super().setUp()
        self.test_dir = tempfile.mkdtemp()
        self.test_filename = os.path.join(self.test_dir, "TestIFCExportImport2D.ifc")

    def tearDown(self):
        shutil.rmtree(self.test_dir, ignore_errors=True)
        super().tearDown()

    def test_export_import(self):

        # Create 4 Draft objects:
        cir = Draft.make_circle(50)
        arc = Draft.make_circle(60, startangle=0, endangle=90)
        rec_1 = Draft.make_rectangle(60, 40)
        rec_2 = Draft.make_rectangle(80, 60)
        rec_2.FilletRadius = 10
        self.document.recompute()

        # Export:
        exportIFC.export([cir, arc, rec_1, rec_2], self.test_filename)

        # Import:
        ifc_import.insert(
            self.test_filename,
            self.document.Name,
            strategy=2,  # All individual IFC objects.
            shapemode=0,  # Load the shape.
            switchwb=None,  # Document unlocked.
            silent=True,
            singledoc=False,
        )

        # Verify import:
        # We check the number of edges, not their curve.

        # Expected number of edges for the imported 2D objects:
        # 1 circle:              1
        # 1 arc:                 1
        # 1 rectangle:           4
        # 1 filleted rectangle:  8 +
        # --------------------------
        # TOTAL:                14

        # 2D geometry is exported as "IfcAnnotation". We filter for that
        # to exclude old objects and the BIM containers (project and building)
        # created by the exporter:
        objs = Draft.getObjectsOfType(self.document.Objects, "IfcAnnotation")
        total_edges = sum(len(obj.Shape.Edges) for obj in objs)
        self.assertEqual(total_edges, 14)
