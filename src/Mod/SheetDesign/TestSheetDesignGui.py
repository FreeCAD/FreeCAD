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

"""GUI tests of the SheetDesign module: the resources, the view providers, and the task panels.

Run with the GUI: FreeCAD -t TestSheetDesignGui
"""

import os
import tempfile
import unittest

import FreeCAD
import FreeCADGui
import Part
from PySide import QtCore

import SheetDesign
import SheetDesignGui  # noqa: F401  Loads the resources.
import SheetDesignTaskPanels
import SheetDesignViewProviders

V = FreeCAD.Vector


class TestSheetDesignGui(unittest.TestCase):
    T = 2.0

    def setUp(self):
        self.doc = FreeCAD.newDocument("SheetDesignGuiTest")
        self.doc.UndoMode = 1
        self.rule = self.doc.addObject("App::VarSet", "Rule")
        self.rule.addProperty("App::PropertyLength", "Thickness", "Sheet")
        self.rule.addProperty("App::PropertyLength", "BendRadius", "Sheet")
        self.rule.addProperty("App::PropertyFloat", "KFactor", "Sheet")
        self.rule.Thickness = self.T
        self.rule.BendRadius = 1.0
        self.rule.KFactor = 0.5
        profile = self.doc.addObject("Part::Feature", "Profile")
        profile.Shape = Part.makePolygon([V(0, 0, 0), V(100, 0, 0), V(100, 50, 0), V(0, 50, 0), V(0, 0, 0)])
        self.base = self.add("SheetDesign::BasePanel", "Base")
        self.base.Profile = profile
        self.base.Rule = self.rule
        self.doc.recompute()
        self.flange = self.add("SheetDesign::Flange", "Flange")
        self.flange.Edge = (self.base, [self.edge(V(100, 0, self.T), V(100, 50, self.T))])
        self.doc.recompute()
        self.panel = None

    def tearDown(self):
        if self.panel is not None:
            self.panel.reject()
        FreeCADGui.Control.closeDialog()
        FreeCAD.closeDocument(self.doc.Name)

    def add(self, type_name, name):
        return SheetDesignViewProviders.attach(self.doc.addObject(type_name, name))

    def edge(self, start, end):
        for index, edge in enumerate(self.base.Shape.Edges):
            if len(edge.Vertexes) != 2:
                continue
            p, q = edge.Vertexes[0].Point, edge.Vertexes[1].Point
            if {round(p.distanceToPoint(start), 6), round(q.distanceToPoint(end), 6)} == {0.0} or {
                round(p.distanceToPoint(end), 6),
                round(q.distanceToPoint(start), 6),
            } == {0.0}:
                return f"Edge{index + 1}"
        raise AssertionError("edge not found")

    def open_panel(self, obj):
        self.panel = SheetDesignTaskPanels.panel_for(obj)
        return self.panel

    def close_panel(self, accept):
        panel, self.panel = self.panel, None
        if accept:
            panel.accept()
        else:
            panel.reject()

    # ----------------------------------------------------------------------- resources

    def test_resources_are_in_the_library(self):
        for name in SheetDesignViewProviders.ICONS.values():
            self.assertTrue(QtCore.QFile.exists(f":/icons/{name}.svg"), name)
        for panel_class in SheetDesignTaskPanels.PANELS.values():
            self.assertTrue(QtCore.QFile.exists(":/panels/" + panel_class.ui_file), panel_class.ui_file)
            self.assertTrue(QtCore.QFile.exists(f":/icons/{panel_class.icon}.svg"), panel_class.icon)

    def test_each_command_has_an_icon(self):
        FreeCADGui.activateWorkbench("SheetDesignWorkbench")
        for name in FreeCADGui.listCommands():
            if name.startswith("SheetDesign_"):
                pixmap = FreeCADGui.Command.get(name).getInfo()["pixmap"]
                self.assertTrue(QtCore.QFile.exists(f":/icons/{pixmap}.svg"), name)

    # ------------------------------------------------------------------ view providers

    def test_feature_has_a_python_view_provider(self):
        self.assertEqual(self.flange.ViewObject.TypeId, "PartDesignGui::ViewProviderPython")
        self.assertIsInstance(self.flange.ViewObject.Proxy, SheetDesignViewProviders.ViewProvider)
        self.assertEqual(self.flange.ViewObject.Proxy.getIcon(), ":/icons/SheetDesign_Flange.svg")
        self.assertFalse(self.flange.ViewObject.Icon.isNull())

    def test_edit_opens_the_task_panel(self):
        self.assertFalse(FreeCADGui.Control.activeDialog())
        self.assertTrue(FreeCADGui.ActiveDocument.setEdit(self.flange.Name, 0))
        self.assertTrue(FreeCADGui.Control.activeDialog())
        FreeCADGui.ActiveDocument.resetEdit()
        self.assertFalse(FreeCADGui.Control.activeDialog())
        self.doc.abortTransaction()

    def test_view_provider_stays_after_save_and_open(self):
        path = os.path.join(tempfile.mkdtemp(), "gui.FCStd")
        self.doc.saveAs(path)
        FreeCAD.closeDocument(self.doc.Name)
        self.doc = FreeCAD.openDocument(path)
        flange = self.doc.getObject("Flange")
        self.assertIsInstance(flange.ViewObject.Proxy, SheetDesignViewProviders.ViewProvider)
        self.assertTrue(FreeCADGui.ActiveDocument.setEdit(flange.Name, 0))
        FreeCADGui.ActiveDocument.resetEdit()
        self.doc.abortTransaction()

    # --------------------------------------------------------------------- task panels

    def test_each_feature_type_has_a_panel(self):
        flat = self.add("SheetDesign::FlatPattern", "Flat")
        flat.Source = self.flange
        self.doc.recompute()
        for obj in (self.base, self.flange, flat):
            panel = self.open_panel(obj)
            self.assertIsNotNone(panel, obj.TypeId)
            self.assertEqual(panel.form.statusLabel.text(), "")
            self.close_panel(accept=False)

    def test_panel_shows_the_property_values(self):
        self.flange.Length = 17.0
        self.flange.Position = "Material Inside"
        self.flange.Reversed = False
        self.doc.recompute()
        form = self.open_panel(self.flange).form
        self.assertAlmostEqual(form.Length.property("rawValue"), 17.0)
        self.assertEqual(form.Position.currentText(), "Material inside")
        self.assertFalse(form.Reversed.isChecked())
        self.assertIn("Edge", form.edgeLabel.text())

    def test_panel_changes_the_feature(self):
        form = self.open_panel(self.flange).form
        form.Length.setProperty("rawValue", 25.0)
        self.assertAlmostEqual(self.flange.Length.Value, 25.0)
        self.assertAlmostEqual(self.flange.Shape.BoundBox.ZMax, self.T + 1.0 + 25.0, places=6)
        form.Position.setCurrentIndex(form.Position.findText("Material inside"))
        self.assertEqual(self.flange.Position, "Material Inside")
        self.assertAlmostEqual(self.flange.Shape.BoundBox.XMax, 100.0, places=6)
        form.Angle.setProperty("rawValue", 60.0)
        self.assertAlmostEqual(self.flange.Angle.Value, 60.0)
        self.close_panel(accept=True)
        self.assertAlmostEqual(self.flange.Length.Value, 25.0)

    def test_cancel_puts_the_values_back(self):
        form = self.open_panel(self.flange).form
        form.Length.setProperty("rawValue", 33.0)
        form.Reversed.setChecked(True)
        self.assertTrue(self.flange.Reversed)
        self.close_panel(accept=False)
        self.assertAlmostEqual(self.flange.Length.Value, 10.0)
        self.assertFalse(self.flange.Reversed)
        self.assertTrue(self.flange.isValid())

    def test_panel_enables_the_relief_sizes_for_an_explicit_type(self):
        form = self.open_panel(self.flange).form
        self.assertEqual(self.flange.ReliefType, "Rule")
        self.assertFalse(form.ReliefWidth.isEnabled())
        form.ReliefType.setCurrentIndex(form.ReliefType.findText("Rectangle"))
        self.assertEqual(self.flange.ReliefType, "Rectangle")
        self.assertTrue(form.ReliefWidth.isEnabled())
        self.assertTrue(form.ReliefDepth.isEnabled())

    def test_panel_shows_an_error_of_the_feature(self):
        form = self.open_panel(self.flange).form
        form.Gap1.setProperty("rawValue", 40.0)
        form.Gap2.setProperty("rawValue", 40.0)
        self.assertFalse(self.flange.isValid())
        self.assertIn("gaps", form.statusLabel.text())

    def test_refold_panel_sets_the_angles(self):
        outline = self.doc.addObject("Part::Feature", "Outline")
        outline.Shape = Part.makePolygon([V(0, 0, 0), V(140, 0, 0), V(140, 50, 0), V(0, 50, 0), V(0, 0, 0)])
        lines = self.doc.addObject("Part::Feature", "Lines")
        lines.Shape = Part.makeCompound(
            [Part.makeLine(V(40, 0, 0), V(40, 50, 0)), Part.makeLine(V(100, 0, 0), V(100, 50, 0))]
        )
        refold = self.add("SheetDesign::Refold", "Refold")
        refold.Profile = outline
        refold.BendLines = lines
        refold.Rule = self.rule
        self.doc.recompute()
        panel = self.open_panel(refold)
        self.assertEqual(panel.form.anglesTable.rowCount(), 2)
        panel.spins[1].setValue(-45.0)
        self.assertEqual(list(refold.Angles), [90.0, -45.0])
        self.assertTrue(refold.isValid(), refold.getStatusString())

    # ------------------------------------------------------------------ selection gate

    def test_edge_gate_uses_the_check_of_the_kernel(self):
        gate = SheetDesignTaskPanels.EdgeGate(self.base)
        top = self.edge(V(0, 0, self.T), V(100, 0, self.T))
        upright = self.edge(V(0, 0, 0), V(0, 0, self.T))
        self.assertTrue(gate.allow(self.doc, self.base, top))
        self.assertFalse(gate.allow(self.doc, self.base, upright))
        self.assertFalse(gate.allow(self.doc, self.flange, top))
        info = SheetDesign.edgeInfo(self.base, top)
        self.assertEqual(info["Panel"], "Base")
        self.assertTrue(info["Top"])
        self.assertAlmostEqual(info["Length"], 100.0)
