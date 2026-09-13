# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import tempfile

import Arch
import FreeCAD as App
from bimtests.TestArchBaseGui import TestArchBaseGui


class TestArchSectionPlaneGui(TestArchBaseGui):

    def test_new_section_has_no_cut_distance(self):
        section = Arch.makeSectionPlane()
        self.assertNotIn("CutDistance", section.ViewObject.PropertiesList)

    def test_restore_removes_cut_distance(self):
        for locked in (True, False):
            with self.subTest(locked=locked), tempfile.TemporaryDirectory() as directory:
                section = Arch.makeSectionPlane()
                name = section.Name
                view = section.ViewObject
                if "CutDistance" not in view.PropertiesList:
                    view.addProperty("App::PropertyLength", "CutDistance", "SectionPlane")
                view.setPropertyStatus("CutDistance", "LockDynamic" if locked else "-LockDynamic")
                view.CutDistance = 25
                view.CutMargin = 7
                self.document.recompute()

                for filename in ("legacy.FCStd", "cleaned.FCStd"):
                    path = os.path.join(directory, filename)
                    self.document.saveAs(path)
                    self.pump_gui_events()
                    App.closeDocument(self.document.Name)
                    self.document = None
                    self.document = App.openDocument(path)
                    self.pump_gui_events()
                    view = self.document.getObject(name).ViewObject
                    self.assertNotIn("CutDistance", view.PropertiesList)
                    self.assertEqual(view.CutMargin.Value, 7)
