# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.
#
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Furgo                                              *
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
import tempfile
import unittest
import FreeCAD
import Arch
from bimtests import TestArchBaseGui


class TestArchSiteGui(TestArchBaseGui.TestArchBaseGui):

    def test_new_site_creation(self):
        """Test: creating a new Site adds the view properties and sets defaults."""
        site = Arch.makeSite()
        self.assertIsNotNone(site, "makeSite() returned None")
        FreeCAD.ActiveDocument.recompute()
        # Wait briefly so the document loader can attach the ViewObject and let the view provider's
        # queued restore/migration callbacks (setProperties, migration, restoreConstraints) run on
        # the GUI event loop before we inspect properties.
        self.pump_gui_events()

        # ViewObject properties should exist
        vobj = site.ViewObject
        props = vobj.PropertiesList
        expected = [
            "ShowSunPosition",
            "SunDateMonth",
            "SunDateDay",
            "SunTimeHour",
            "SolarDiagramScale",
            "SolarDiagramPosition",
            "ShowHourLabels",
        ]
        for p in expected:
            self.assertIn(p, props, f"Property '{p}' missing from ViewObject")

        # Check defaults where applicable
        if hasattr(vobj, "SunDateMonth"):
            self.assertEqual(vobj.SunDateMonth, 6)
        if hasattr(vobj, "SunDateDay"):
            self.assertEqual(vobj.SunDateDay, 21)
        if hasattr(vobj, "SunTimeHour"):
            self.assertAlmostEqual(vobj.SunTimeHour, 12.0)
        if hasattr(vobj, "SolarDiagramScale"):
            self.assertAlmostEqual(vobj.SolarDiagramScale, 20000.0)

    def test_new_site_save_and_reopen(self):
        """Test: save document and reopen; view properties must be present and constrained."""
        self.printTestMessage("Save and reopen new Site...")
        site = Arch.makeSite()
        FreeCAD.ActiveDocument.recompute()

        # Save to a temporary file
        tf = tempfile.NamedTemporaryFile(delete=False, suffix=".FCStd")
        tf.close()
        path = tf.name
        try:
            FreeCAD.ActiveDocument.saveAs(path)

            # Open the saved document (this returns a new Document instance)
            reopened = FreeCAD.openDocument(path)
            try:
                # Find the Site object in the reopened document by checking proxy type
                found = None
                for o in reopened.Objects:
                    if hasattr(o, "Proxy") and getattr(o.Proxy, "Type", None) == "Site":
                        found = o
                        break
                self.assertIsNotNone(found, "Site object not found after reopen")

                # Ensure async GUI setup completes
                self.pump_gui_events()

                vobj = found.ViewObject

                # 1. Verify that constraints have been reapplied by testing clamping behavior.
                # Setting an out-of-bounds value should not raise an exception but
                # should coerce the value to the nearest limit.
                vobj.SunDateMonth = 13
                self.assertEqual(
                    vobj.SunDateMonth, 12, "Property should be clamped to its max value"
                )

                vobj.SunDateMonth = 0
                self.assertEqual(
                    vobj.SunDateMonth, 1, "Property should be clamped to its min value"
                )

            finally:
                # Close reopened document to keep test isolation
                FreeCAD.closeDocument(reopened.Name)
        finally:
            try:
                os.unlink(path)
            except Exception:
                pass

    def test_legacy_site_migration(self):
        """Legacy migration test (<=1.0.0 -> newer)

        Verifies that loading a legacy (<=1.0.0) FreeCAD file triggers the view provider's restore
        path and performs a conservative migration of legacy values.

        When a Site previously saved with `SolarDiagramScale == 1.0` is loaded, the view provider's
        `loads()` restore path will detect that this is a legacy value and replace it with the
        modern default of 20 m radius (https://github.com/FreeCAD/FreeCAD/pull/22496). The migration
        is conservative: only values less-or-equal to `1.0` (or effectively zero) are considered
        legacy and will be changed.

        The test waits briefly for the loader's deferred binding and normalization callbacks to run
        before asserting the migrated value.
        """
        self.printTestMessage("Save and reopen legacy Site...")
        fixtures_dir = os.path.join(os.path.dirname(__file__), "fixtures")
        fname = os.path.join(fixtures_dir, "FC_site_simple-102.FCStd")
        if not os.path.exists(fname):
            raise unittest.SkipTest("Legacy migration fixture not found; skipping test.")

        # If fixture exists, open and validate migration
        d = FreeCAD.openDocument(fname)
        try:
            # Wait briefly so the document loader can attach the ViewObject and let the view
            # provider's queued restore/migration callbacks (setProperties, migration,
            # restoreConstraints) run on the GUI event loop before we inspect properties.
            self.pump_gui_events()
            site = None
            for o in d.Objects:
                if hasattr(o, "Proxy") and getattr(o.Proxy, "Type", None) == "Site":
                    site = o
                    break
            assert site is not None, "No Site found in fixture document"
            vobj = site.ViewObject
            # Example assertion: SolarDiagramScale should be normalized to 20000
            self.assertAlmostEqual(getattr(vobj, "SolarDiagramScale", 1.0), 20000.0)
        finally:
            FreeCAD.closeDocument(d.Name)
