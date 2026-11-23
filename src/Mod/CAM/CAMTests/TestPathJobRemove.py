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

"""Tests for ObjectJob.removeJob() without the GUI."""

import FreeCAD
import Part

import Path.Dressup.Array as PathDressupArray
import Path.Dressup.Boundary as PathDressupBoundary
import Path.Main.Job as PathJob
import Path.Op.Profile as PathProfile
from CAMTests.PathTestUtils import PathTestBase


class TestPathJobRemove(PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathJobRemove")
        self.model = self.doc.addObject("Part::Feature", "Box")
        self.model.Shape = Part.makeBox(20, 20, 10)
        self.job = PathJob.Create("Job", [self.model])

        # control: an operation with no dressup
        self.plain = PathProfile.Create("ProfilePlain", parentJob=self.job)

        # Array dressup: the base operation leaves Operations.Group
        self.arrayBase = PathProfile.Create("ProfileArrayBase", parentJob=self.job)
        self.array = PathDressupArray.Create(self.arrayBase)

        # Boundary dressup: the base leaves Operations.Group and a stock object is added
        self.boundaryBase = PathProfile.Create("ProfileBoundaryBase", parentJob=self.job)
        self.boundary = PathDressupBoundary.Create(self.boundaryBase)
        self.boundaryStock = self.boundary.Stock

        self.doc.recompute()

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _remove(self):
        """Remove the Job. Return (names before, names after), without the model."""
        before = {o.Name for o in self.doc.Objects} - {self.model.Name}
        self.job.Proxy.removeJob(self.job)
        after = {o.Name for o in self.doc.Objects}
        return before, after

    def test000_setup_is_headless(self):
        """The fixture has the structure that the other tests rely on."""
        self.assertFalse(FreeCAD.GuiUp)
        group = self.job.Operations.Group
        self.assertIn(self.plain, group)
        self.assertIn(self.array, group)
        self.assertIn(self.boundary, group)
        self.assertNotIn(self.arrayBase, group)
        self.assertNotIn(self.boundaryBase, group)
        self.assertIsNotNone(self.boundaryStock)

    def test010_job_and_plain_op_removed(self):
        """removeJob() removes the Job, a plain operation and the dressups."""
        jobName = self.job.Name
        names = (self.plain.Name, self.array.Name, self.boundary.Name)
        _, after = self._remove()
        self.assertNotIn(jobName, after)
        for name in names:
            self.assertNotIn(name, after)

    def test020_model_stays(self):
        """removeJob() does not remove the original model."""
        modelName = self.model.Name
        _, after = self._remove()
        self.assertIn(modelName, after)

    def test030_array_base_removed(self):
        """removeJob() removes the base operation of an Array dressup."""
        name = self.arrayBase.Name
        _, after = self._remove()
        self.assertNotIn(name, after)

    def test040_boundary_base_removed(self):
        """removeJob() removes the base operation of a Boundary dressup."""
        name = self.boundaryBase.Name
        _, after = self._remove()
        self.assertNotIn(name, after)

    def test050_boundary_stock_removed(self):
        """removeJob() removes the stock object of a Boundary dressup."""
        name = self.boundaryStock.Name
        _, after = self._remove()
        self.assertNotIn(name, after)

    def test060_nothing_left(self):
        """removeJob() leaves no object that the Job made."""
        before, after = self._remove()
        self.assertEqual(set(), before & after)
