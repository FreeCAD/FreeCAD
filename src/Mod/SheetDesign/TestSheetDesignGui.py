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

    def edge(self, start, end, shape=None):
        for index, edge in enumerate((shape or self.base.Shape).Edges):
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

    def test_flat_pattern_panel_lists_the_bends(self):
        down = self.add("SheetDesign::Flange", "Down")
        down.Edge = (self.flange, [self.edge(V(0, 0, 0), V(0, 50, 0), self.flange.Shape)])
        down.Angle = 45.0
        flat = self.add("SheetDesign::FlatPattern", "Flat")
        flat.Source = down
        self.doc.recompute()
        table = self.open_panel(flat).form.bendsTable
        self.assertEqual(table.rowCount(), 2)
        self.assertEqual([table.item(0, c).text() for c in range(4)], ["1", "Up", "90 °", "1 mm"])
        self.assertEqual([table.item(1, c).text() for c in range(4)], ["2", "Down", "45 °", "1 mm"])

    # ----------------------------------------------------------------- Stage 4 features

    def test_stage_4_features_have_panels(self):
        line = self.doc.addObject("Part::Feature", "Line")
        line.Shape = Part.makeLine(V(60, 0, self.T), V(60, 50, self.T))
        top = [self.edge(V(0, 0, self.T), V(0, 50, self.T))]
        extend = self.add("SheetDesign::Extend", "Extend")
        extend.Edge = (self.base, top)
        fold = self.add("SheetDesign::Fold", "Fold")
        fold.Source = self.base
        fold.BendLines = line
        hem = self.add("SheetDesign::Hem", "Hem")
        hem.Edge = (self.base, top)
        shape = self.add("SheetDesign::BaseShape", "Shape")
        shape.Rule = self.rule
        shape.Type = "Tub"
        self.doc.recompute()
        relief = self.add("SheetDesign::CornerRelief", "Relief")
        relief.Source = shape
        self.doc.recompute()
        for obj in (extend, fold, hem, shape, relief):
            self.assertTrue(obj.isValid(), obj.Name + ": " + obj.getStatusString())
            panel = self.open_panel(obj)
            self.assertIsNotNone(panel, obj.TypeId)
            self.assertEqual(panel.form.statusLabel.text(), "", obj.TypeId)
            self.assertTrue(obj.ViewObject.Icon.isNull() is False, obj.TypeId)
            self.close_panel(accept=False)

    def test_hem_panel_enables_the_values_of_the_type(self):
        hem = self.add("SheetDesign::Hem", "Hem")
        hem.Edge = (self.base, [self.edge(V(0, 0, self.T), V(0, 50, self.T))])
        self.doc.recompute()
        form = self.open_panel(hem).form
        self.assertFalse(form.Gap.isEnabled())
        form.Type.setCurrentIndex(form.Type.findText("Rolled"))
        self.assertTrue(form.Gap.isEnabled())
        self.assertFalse(form.Length.isEnabled())
        self.assertEqual(hem.Type, "Rolled")
        self.assertTrue(hem.isValid(), hem.getStatusString())

    def test_base_shape_panel_changes_the_type(self):
        shape = self.add("SheetDesign::BaseShape", "Shape")
        shape.Rule = self.rule
        self.doc.recompute()
        form = self.open_panel(shape).form
        self.assertFalse(form.FlangeWidth.isEnabled())
        form.Type.setCurrentIndex(form.Type.findText("Hat"))
        self.assertTrue(form.FlangeWidth.isEnabled())
        self.assertEqual(shape.Type, "Hat")
        self.assertEqual(len(shape.Shape.Solids), 1)

    # -------------------------------------------------------- cancel of a new feature

    def test_cancel_of_a_new_feature_removes_it(self):
        """The command leaves its transaction open, and the panel takes it."""
        flat = self.add("SheetDesign::FlatPattern", "Flat")
        flat.Source = self.flange
        self.doc.recompute()
        FreeCADGui.activateWorkbench("SheetDesignWorkbench")
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(self.flange)
        # The part has one bend, thus the corner relief has an error.
        FreeCADGui.runCommand("SheetDesign_CornerRelief")
        relief = self.doc.getObject("CornerRelief")
        self.assertIsNotNone(relief)
        self.assertFalse(relief.isValid())
        self.assertEqual(flat.Source, relief)
        self.assertTrue(FreeCADGui.Control.activeDialog())
        SheetDesignViewProviders.ACTIVE_PANEL.reject()  # The Cancel button.
        FreeCADGui.Control.closeDialog()
        self.assertIsNone(self.doc.getObject("CornerRelief"))
        self.assertEqual(flat.Source, self.flange)
        self.assertTrue(flat.isValid(), flat.getStatusString())
        self.assertTrue(self.flange.ViewObject.Visibility)
        self.assertEqual([o.Name for o in self.doc.Objects if not o.isValid()], [])

    def test_ok_of_a_new_feature_keeps_it_in_one_undo_step(self):
        FreeCADGui.activateWorkbench("SheetDesignWorkbench")
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(self.base, self.edge(V(0, 0, self.T), V(0, 50, self.T)))
        FreeCADGui.runCommand("SheetDesign_Hem")
        hem = self.doc.getObject("Hem")
        self.assertIsNotNone(hem)
        dialog_panel = SheetDesignViewProviders.ACTIVE_PANEL
        self.assertIsNotNone(dialog_panel)
        dialog_panel.accept()
        FreeCADGui.Control.closeDialog()
        self.assertIsNotNone(self.doc.getObject("Hem"))
        self.assertTrue(hem.isValid(), hem.getStatusString())
        self.doc.undo()
        self.assertIsNone(self.doc.getObject("Hem"))

    def test_delete_puts_the_flat_pattern_back(self):
        flat = self.add("SheetDesign::FlatPattern", "Flat")
        flat.Source = self.flange
        self.doc.recompute()
        self.flange.ViewObject.Proxy.onDelete(self.flange.ViewObject, [])
        self.doc.removeObject(self.flange.Name)
        self.doc.recompute()
        self.assertEqual(flat.Source, self.base)
        self.assertTrue(flat.isValid(), flat.getStatusString())
        self.assertTrue(self.base.ViewObject.Visibility)

    # ----------------------------------------------------------------- Stage 5 features

    def test_stage_5_features_have_panels(self):
        block = self.doc.addObject("Part::Feature", "Block")
        block.Shape = Part.makeBox(80, 50, 30, V(0, 100, 0))
        faces = [f"Face{i + 1}" for i, f in enumerate(block.Shape.Faces) if f.normalAt(0, 0).z < 0.5]
        faces.sort(key=lambda n: block.Shape.getElement(n).normalAt(0, 0).z)
        convert = self.add("SheetDesign::Convert", "Convert")
        convert.Faces = (block, faces)
        convert.Rule = self.rule
        tool = self.doc.addObject("Part::Feature", "Tool")
        tool.Shape = Part.makeBox(30, 20, 5, V(35, 15, self.T))
        base_face = [f"Face{i + 1}" for i, f in enumerate(tool.Shape.Faces) if f.normalAt(0, 0).z < -0.5]
        form = self.add("SheetDesign::Form", "Form")
        form.Source = self.base
        form.Tool = (tool, base_face)
        perforation = self.add("SheetDesign::Perforation", "Perforation")
        perforation.Edges = (self.flange, [])
        self.doc.recompute()
        for obj in (convert, form, perforation):
            self.assertTrue(obj.isValid(), obj.Name + ": " + obj.getStatusString())
            panel = self.open_panel(obj)
            self.assertIsNotNone(panel, obj.TypeId)
            self.assertEqual(panel.form.statusLabel.text(), "", obj.TypeId)
            self.close_panel(accept=False)
        self.assertEqual(self.open_panel(perforation).form.edgesLabel.text(), "All bends")
        self.close_panel(accept=False)

    def test_convert_command(self):
        block = self.doc.addObject("Part::Feature", "Block")
        block.Shape = Part.makeBox(80, 50, 30, V(0, 100, 0))
        self.doc.recompute()
        FreeCADGui.activateWorkbench("SheetDesignWorkbench")
        FreeCADGui.Selection.clearSelection()
        names = [f"Face{i + 1}" for i, f in enumerate(block.Shape.Faces) if f.normalAt(0, 0).z < 0.5]
        names.sort(key=lambda n: block.Shape.getElement(n).normalAt(0, 0).z)
        for name in names:
            FreeCADGui.Selection.addSelection(block, name)
        FreeCADGui.runCommand("SheetDesign_Convert")
        convert = self.doc.getObject("Convert")
        self.assertIsNotNone(convert)
        self.assertTrue(convert.isValid(), convert.getStatusString())
        self.assertEqual(len(convert.Faces[1]), 5)
        SheetDesignViewProviders.ACTIVE_PANEL.accept()
        FreeCADGui.Control.closeDialog()
        self.assertEqual(len(convert.Shape.Solids), 1)

    # ------------------------------------------------------ flange on several edges

    def test_flange_command_makes_one_feature_for_all_edges(self):
        FreeCADGui.activateWorkbench("SheetDesignWorkbench")
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(self.base, self.edge(V(0, 0, self.T), V(0, 50, self.T)))
        FreeCADGui.Selection.addSelection(self.base, self.edge(V(0, 0, self.T), V(100, 0, self.T)))
        count = len(self.doc.Objects)
        FreeCADGui.runCommand("SheetDesign_Flange")
        self.assertEqual(len(self.doc.Objects), count + 1)
        flange = self.doc.ActiveObject
        self.assertEqual(len(flange.Edge[1]), 2)
        self.assertTrue(flange.isValid(), flange.getStatusString())
        form = SheetDesignViewProviders.ACTIVE_PANEL.form
        self.assertTrue(form.CornerType.isEnabled())
        self.assertFalse(form.CornerGap.isEnabled())
        self.assertFalse(form.Radius.isEnabled())
        form.CornerType.setCurrentIndex(form.CornerType.findText("Overlap"))
        self.assertEqual(flange.CornerType, "Overlap")
        self.assertTrue(form.CornerGap.isEnabled())
        self.assertEqual(form.statusLabel.text(), "")
        SheetDesignViewProviders.ACTIVE_PANEL.accept()
        FreeCADGui.Control.closeDialog()
        self.assertEqual(flange.Overlap.Value, 0.0)

    # ------------------------------------------------------------------- drawing

    def flat_with_two_bends(self):
        down = self.add("SheetDesign::Flange", "Down")
        down.Edge = (self.flange, [self.edge(V(0, 0, 0), V(0, 50, 0), self.flange.Shape)])
        flat = self.add("SheetDesign::FlatPattern", "Flat")
        flat.Source = down
        self.doc.recompute()
        return flat

    def test_panel_sets_the_bend_sequence(self):
        flat = self.flat_with_two_bends()
        table = self.open_panel(flat).form.bendsTable
        self.assertEqual([table.item(r, 0).text() for r in range(2)], ["1", "2"])
        table.item(0, 0).setText("2")
        self.assertEqual(list(flat.BendOrder), [2, 1])
        self.assertEqual([table.item(r, 0).text() for r in range(2)], ["2", "1"])
        # A number that is not in the sequence is not accepted.
        table.item(0, 0).setText("7")
        self.assertEqual(list(flat.BendOrder), [2, 1])
        self.assertEqual(table.item(0, 0).text(), "2")

    def test_drawing_command_makes_a_page(self):
        flat = self.flat_with_two_bends()
        FreeCADGui.activateWorkbench("SheetDesignWorkbench")
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(flat)
        FreeCADGui.runCommand("SheetDesign_Drawing")
        pages = [o for o in self.doc.Objects if o.isDerivedFrom("TechDraw::DrawPage")]
        self.assertEqual(len(pages), 1)
        self.assertEqual(len(pages[0].Views), 3)
        notes = self.doc.getObject("BendNotes")
        self.assertEqual(notes.Symbol.count("<circle"), 2)
        self.assertEqual([o.Name for o in self.doc.Objects if not o.isValid()], [])

    def test_proxy_module_loads_the_resource_library(self):
        """A document can open when a different workbench is active."""
        import subprocess
        import sys

        code = (
            "import FreeCAD, FreeCADGui, sys\n"
            "import SheetDesignViewProviders, SheetDesignTaskPanels\n"
            "from PySide import QtCore\n"
            "ok = QtCore.QFile.exists(':/panels/TaskFlange.ui') and QtCore.QFile.exists(':/icons/SheetDesign_Flange.svg')\n"
            "open(sys.argv[-1], 'w').write('yes' if ok else 'no')\n"
            "import os; os._exit(0)\n"
        )
        folder = tempfile.mkdtemp()
        script = os.path.join(folder, "load_check.py")
        result = os.path.join(folder, "result.txt")
        with open(script, "w", encoding="utf-8") as handle:
            handle.write(code.replace("sys.argv[-1]", repr(result)))
        binary = os.path.join(FreeCAD.getHomePath(), "bin", "FreeCAD")
        subprocess.run([binary, script], timeout=120, check=False)
        with open(result, encoding="utf-8") as handle:
            self.assertEqual(handle.read(), "yes")

    # ------------------------------------------------------------------- fold state

    def test_fold_state_leaves_no_object_in_the_document(self):
        flat = self.flat_with_two_bends()
        source = flat.Source
        count = len(self.doc.Objects)
        panel = self.open_panel(flat)
        form = panel.form
        self.assertTrue(form.foldSequence.isChecked())
        form.foldSlider.setValue(50)
        self.assertTrue(panel.preview.is_shown())
        self.assertFalse(source.ViewObject.Visibility)
        self.assertEqual(form.foldLabel.text(), "Fold state: 50 %")
        panel.fold_measured()
        self.assertIn("Overlap: 0.0", form.foldLabel.text())
        # The preview is one temporary object. The source is not changed.
        self.assertEqual(len(self.doc.Objects), count + 1)
        self.assertGreater(self.doc.getObject("FoldPreview").Shape.Volume, 0.0)
        form.foldSlider.setValue(100)
        self.assertEqual(len(self.doc.Objects), count)
        self.assertFalse(panel.preview.is_shown())
        self.assertTrue(source.ViewObject.Visibility)
        form.foldSlider.setValue(20)
        self.close_panel(accept=False)
        self.assertFalse(panel.preview.is_shown())
        self.assertTrue(source.ViewObject.Visibility)
        self.assertEqual(len(self.doc.Objects), count)

    def test_fold_check_and_frames(self):
        flat = self.flat_with_two_bends()
        panel = self.open_panel(flat)
        panel.fold_check()
        self.assertIn("No overlap", panel.form.foldLabel.text())
        folder = tempfile.mkdtemp()
        self.assertEqual(panel.fold_frames(folder, steps=4, size=(320, 180)), 5)
        self.assertEqual(sorted(os.listdir(folder)), ["fold_%04d.png" % i for i in range(5)])
        self.assertEqual(panel.form.foldSlider.value(), 100)

    def test_fold_slider_on_a_flat_pattern_with_no_bend_data(self):
        """The state of a flat pattern from an older file."""
        flat = self.add("SheetDesign::FlatPattern", "OldFlat")
        flat.Source = self.flange  # No recompute.
        panel = self.open_panel(flat)
        self.assertEqual(len(flat.BendAngles), 1)
        self.assertEqual(panel.form.bendsTable.rowCount(), 1)
        panel.form.foldSequence.setChecked(False)
        panel.form.foldSlider.setValue(0)
        preview = self.doc.getObject("FoldPreview")
        self.assertLess(preview.Shape.BoundBox.ZLength, self.T + 0.2)

    def test_panel_table_has_a_row_for_each_score(self):
        import SheetDesignBoxStyles

        self.rule.Thickness = 4.0
        self.rule.BendRadius = 0.0
        piece = SheetDesignBoxStyles.STYLES["0226 Bellows Style Top and Bottom Container"](4.0, 300.0, 200.0, 150.0)[0]
        box = SheetDesignViewProviders.attach(SheetDesignBoxStyles.fold(self.doc, self.rule, piece, "Bellows"))
        flat = self.add("SheetDesign::FlatPattern", "BoxFlat")
        flat.Source = box
        self.doc.recompute()
        table = self.open_panel(flat).form.bendsTable
        self.assertEqual(table.rowCount(), 16)
        self.assertEqual(table.item(0, 2).text(), "90 °/180 °")
        table.item(0, 0).setText("3")
        order = list(flat.BendOrder)
        self.assertEqual(len(order), 32)
        self.assertEqual(sorted(set(order)), list(range(1, 17)))
        self.assertEqual(table.item(0, 0).text(), "3")
        self.assertEqual(table.item(2, 0).text(), "1")

    def test_fold_slider_uses_the_solver_for_coupled_folds(self):
        import SheetDesignBoxStyles
        import SheetDesignFold

        self.rule.Thickness = 4.0
        self.rule.BendRadius = 0.0
        piece = SheetDesignBoxStyles.STYLES["0226 Bellows Style Top and Bottom Container"](4.0, 300.0, 200.0, 150.0)[0]
        box = SheetDesignViewProviders.attach(SheetDesignBoxStyles.fold(self.doc, self.rule, piece, "Bellows"))
        flat = self.add("SheetDesign::FlatPattern", "BoxFlat")
        flat.Source = box
        self.doc.recompute()
        panel = self.open_panel(flat)
        panel.form.foldSlider.setValue(60)
        self.assertIn("Coupled folds", panel.form.foldLabel.text())
        preview = self.doc.getObject("FoldPreview")
        self.assertTrue(preview.Shape.isValid())
        self.assertGreater(preview.Shape.Volume, 0.9 * box.Shape.Volume)
        self.assertTrue(SheetDesignFold.motion(flat).complete)
