# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2026 FreeCAD Project Association                        *
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

import unittest

import FreeCAD as App
from draftutils import params


class DraftGridSettings(unittest.TestCase):
    """Tests for document-backed Draft grid settings."""

    def setUp(self):
        self.doc = App.newDocument(self.id().split(".")[-1])
        self._param_group = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        self._saved_string_params = {
            "gridSpacing": (
                "gridSpacing" in self._param_group.GetStrings(),
                self._param_group.GetString("gridSpacing", ""),
            ),
        }
        self._saved_int_params = {
            entry: (
                entry in self._param_group.GetInts(),
                self._param_group.GetInt(entry, 0),
            )
            for entry in ("gridEvery", "gridSize")
        }
        self._param_group.SetString("gridSpacing", "2 mm")
        self._param_group.SetInt("gridEvery", 8)
        self._param_group.SetInt("gridSize", 20)

    def tearDown(self):
        try:
            self._restore_preferences()
        finally:
            App.closeDocument(self.doc.Name)

    def _restore_preferences(self):
        for entry, (exists, value) in self._saved_string_params.items():
            if exists:
                self._param_group.SetString(entry, value)
            else:
                self._param_group.RemString(entry)
        for entry, (exists, value) in self._saved_int_params.items():
            if exists:
                self._param_group.SetInt(entry, value)
            else:
                self._param_group.RemInt(entry)

    def assertQuantityEqual(self, actual, expected):
        self.assertAlmostEqual(
            App.Units.Quantity(actual).Value,
            App.Units.Quantity(expected).Value,
        )

    def test_document_grid_settings_override_preferences(self):
        self.doc.Meta = {
            "Draft.GridSpacing": "7 mm",
            "Draft.GridMainlines": "12",
            "Draft.GridSize": "24",
        }

        self.assertQuantityEqual(params.get_grid_param("gridSpacing", self.doc), "7 mm")
        self.assertEqual(params.get_grid_param("gridEvery", self.doc), 12)
        self.assertEqual(params.get_grid_param("gridSize", self.doc), 24)

    def test_invalid_document_grid_settings_fall_back_to_preferences(self):
        self.doc.Meta = {
            "Draft.GridSpacing": "0 mm",
            "Draft.GridMainlines": "1",
            "Draft.GridSize": "0",
        }

        self.assertQuantityEqual(params.get_grid_param("gridSpacing", self.doc), "2 mm")
        self.assertEqual(params.get_grid_param("gridEvery", self.doc), 8)
        self.assertEqual(params.get_grid_param("gridSize", self.doc), 20)

        self.doc.Meta = {"Draft.GridSpacing": "invalid"}
        self.assertQuantityEqual(params.get_grid_param("gridSpacing", self.doc), "2 mm")

    def test_set_grid_param_preserves_unrelated_metadata_and_preferences(self):
        self.doc.Meta = {
            "BIM.GridSpacing": "1 m",
            "Draft.Other": "keep",
        }

        self.assertTrue(params.set_grid_param("gridSpacing", "5 mm", self.doc))
        self.assertTrue(params.set_grid_param("gridEvery", 14, self.doc))
        self.assertTrue(params.set_grid_param("gridSize", 30, self.doc))

        self.assertQuantityEqual(self.doc.Meta["Draft.GridSpacing"], "5 mm")
        self.assertEqual(self.doc.Meta["Draft.GridMainlines"], "14")
        self.assertEqual(self.doc.Meta["Draft.GridSize"], "30")
        self.assertEqual(self.doc.Meta["BIM.GridSpacing"], "1 m")
        self.assertEqual(self.doc.Meta["Draft.Other"], "keep")
        self.assertQuantityEqual(params.get_param("gridSpacing"), "2 mm")
        self.assertEqual(params.get_param("gridEvery"), 8)
        self.assertEqual(params.get_param("gridSize"), 20)

    def test_set_grid_param_rejects_invalid_document_values(self):
        self.assertFalse(params.set_grid_param("gridSpacing", "0 mm", self.doc))
        self.assertFalse(params.set_grid_param("gridSpacing", "-5 mm", self.doc))
        self.assertFalse(params.set_grid_param("gridEvery", 1, self.doc))
        self.assertEqual(self.doc.Meta, {})

    def test_explicit_missing_document_does_not_write_preferences(self):
        self.assertFalse(params.set_grid_param("gridSize", 40, "MissingDocument"))

        self.assertEqual(params.get_param("gridSize"), 20)
