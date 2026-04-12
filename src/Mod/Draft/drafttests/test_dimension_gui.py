# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Unit tests for the Draft Workbench, GUI dimension tests."""

import os
import tempfile

import Draft
import FreeCAD as App
import FreeCADGui as Gui
from FreeCAD import Vector

from drafttests import test_base


class DraftGuiDimension(test_base.DraftTestCaseDoc):
    """GUI regressions for Draft dimensions."""

    def setUp(self):
        """Set up a temporary Draft document and keep a stable name for teardown."""
        super().setUp()
        self.doc_name = self.doc.Name

    def tearDown(self):
        """Close the temporary document even if save/reopen invalidated the Python wrapper."""
        if self.doc_name in App.listDocuments():
            App.closeDocument(self.doc_name)

    def getAngularOvershootState(self, dimension):
        """Return the angular overshoot scenegraph state used by the regression checks."""
        vobj = dimension.ViewObject
        proxy = vobj.Proxy
        return {
            "dim_children": proxy.marksDimOvershoot.getNumChildren(),
            "ext_children": proxy.marksExtOvershoot.getNumChildren(),
            "dim_scale_start": tuple(proxy.transDimOvershoot1.scaleFactor.getValue()),
            "dim_scale_end": tuple(proxy.transDimOvershoot2.scaleFactor.getValue()),
            "ext_scale_start": tuple(proxy.transExtOvershoot1.scaleFactor.getValue()),
            "ext_scale_end": tuple(proxy.transExtOvershoot2.scaleFactor.getValue()),
            "dim_translation_start": tuple(proxy.transDimOvershoot1.translation.getValue()),
            "dim_translation_end": tuple(proxy.transDimOvershoot2.translation.getValue()),
            "ext_translation_start": tuple(proxy.transExtOvershoot1.translation.getValue()),
            "ext_translation_end": tuple(proxy.transExtOvershoot2.translation.getValue()),
            "coord_start": tuple(proxy.coord1.point.getValues()[0].getValue()),
            "coord_end": tuple(proxy.coord2.point.getValues()[0].getValue()),
            "dim_rotation_start": tuple(proxy.transDimOvershoot1.rotation.getValue().getValue()),
            "dim_rotation_end": tuple(proxy.transDimOvershoot2.rotation.getValue().getValue()),
            "ext_rotation_start": tuple(proxy.transExtOvershoot1.rotation.getValue().getValue()),
            "ext_rotation_end": tuple(proxy.transExtOvershoot2.rotation.getValue().getValue()),
        }

    def assertRotationAlmostEqual(self, actual, expected):
        """Compare Coin quaternion values with a tolerance."""
        for actual_component, expected_component in zip(actual, expected):
            self.assertAlmostEqual(actual_component, expected_component, places=6)

    def assertAngularOvershoots(self, dimension, expected_state=None):
        """Assert that the angular dimension view provider drew both overshoot marks."""
        Gui.updateGui()
        state = self.getAngularOvershootState(dimension)
        self.assertEqual(state["dim_children"], 3)
        self.assertEqual(state["ext_children"], 3)
        self.assertEqual(state["dim_scale_start"], (1.0, 1.0, 1.0))
        self.assertEqual(state["dim_scale_end"], (1.0, 1.0, 1.0))
        self.assertEqual(state["ext_scale_start"], (2.0, 2.0, 2.0))
        self.assertEqual(state["ext_scale_end"], (2.0, 2.0, 2.0))
        self.assertEqual(state["dim_translation_start"], state["coord_start"])
        self.assertEqual(state["dim_translation_end"], state["coord_end"])
        self.assertEqual(state["ext_translation_start"], state["coord_start"])
        self.assertEqual(state["ext_translation_end"], state["coord_end"])

        if expected_state is not None:
            self.assertEqual(state["dim_children"], expected_state["dim_children"])
            self.assertEqual(state["ext_children"], expected_state["ext_children"])
            self.assertEqual(state["dim_scale_start"], expected_state["dim_scale_start"])
            self.assertEqual(state["dim_scale_end"], expected_state["dim_scale_end"])
            self.assertEqual(state["ext_scale_start"], expected_state["ext_scale_start"])
            self.assertEqual(state["ext_scale_end"], expected_state["ext_scale_end"])
            self.assertEqual(
                state["dim_translation_start"], expected_state["dim_translation_start"]
            )
            self.assertEqual(state["dim_translation_end"], expected_state["dim_translation_end"])
            self.assertEqual(
                state["ext_translation_start"], expected_state["ext_translation_start"]
            )
            self.assertEqual(state["ext_translation_end"], expected_state["ext_translation_end"])
            self.assertRotationAlmostEqual(
                state["dim_rotation_start"], expected_state["dim_rotation_start"]
            )
            self.assertRotationAlmostEqual(
                state["dim_rotation_end"], expected_state["dim_rotation_end"]
            )
            self.assertRotationAlmostEqual(
                state["ext_rotation_start"], expected_state["ext_rotation_start"]
            )
            self.assertRotationAlmostEqual(
                state["ext_rotation_end"], expected_state["ext_rotation_end"]
            )

    def test_angular_dimension_draws_overshoots_after_restore(self):
        """Angular dimensions keep both overshoot marks after saving and reopening a document."""
        original_name = self.doc.Name
        dimension = Draft.make_angular_dimension(Vector(0, 0, 0), [20, 70], Vector(3, 1, 0))
        vobj = dimension.ViewObject
        vobj.DisplayMode = "World"
        vobj.DimOvershoot = 1
        vobj.ExtOvershoot = 2

        self.doc.recompute()
        self.assertAngularOvershoots(dimension)
        expected_state = self.getAngularOvershootState(dimension)

        temp_file = tempfile.NamedTemporaryFile(delete=False, suffix=".FCStd")
        temp_file.close()
        reopened = None
        try:
            self.doc.saveAs(temp_file.name)
            self.doc = App.getDocument(original_name)
            reopened = App.openDocument(temp_file.name)
            Gui.updateGui()

            restored = reopened.getObject(dimension.Name)
            self.assertIsNotNone(restored)
            self.assertAngularOvershoots(restored, expected_state)

            reopened.recompute()
            self.assertAngularOvershoots(restored, expected_state)
        finally:
            if reopened is not None:
                App.closeDocument(reopened.Name)
            if original_name in App.listDocuments():
                self.doc = App.getDocument(original_name)
            os.unlink(temp_file.name)
