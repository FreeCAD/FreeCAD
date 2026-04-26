# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
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

"""Focused tests for the GUI selection singleton."""

import unittest

import FreeCAD

if FreeCAD.GuiUp:
    import FreeCADGui
else:
    FreeCADGui = None


@unittest.skipUnless(FreeCAD.GuiUp, "Selection tests require the GUI")
class SelectionSingletonTestCase(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestSelection")
        FreeCAD.setActiveDocument(self.doc.Name)
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.clearPreselection()

    def tearDown(self):
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.clearPreselection()
        if FreeCAD.getDocument(self.doc.Name):
            FreeCAD.closeDocument(self.doc.Name)

    def make_box(self, name):
        import Part  # noqa: F401

        box = self.doc.addObject("Part::Box", name)
        self.doc.recompute()
        return box

    def test_document_scoped_selection_can_be_cleared_independently(self):
        other_doc = FreeCAD.newDocument("TestSelectionOther")
        try:
            first = self.doc.addObject("App::FeaturePython", "First")
            second = other_doc.addObject("App::FeaturePython", "Second")

            FreeCADGui.Selection.addSelection(self.doc.Name, first.Name)
            FreeCADGui.Selection.addSelection(other_doc.Name, second.Name)

            self.assertEqual(
                [obj.Name for obj in FreeCADGui.Selection.getSelection(self.doc.Name, 0)],
                [first.Name],
            )
            self.assertEqual(
                [obj.Name for obj in FreeCADGui.Selection.getSelection(other_doc.Name, 0)],
                [second.Name],
            )

            FreeCADGui.Selection.clearSelection(self.doc.Name)

            self.assertFalse(FreeCADGui.Selection.hasSelection(self.doc.Name, 0))
            self.assertTrue(FreeCADGui.Selection.hasSelection(other_doc.Name, 0))
            self.assertEqual(
                [obj.Name for obj in FreeCADGui.Selection.getSelection(other_doc.Name, 0)],
                [second.Name],
            )
        finally:
            FreeCADGui.Selection.clearSelection(other_doc.Name)
            if FreeCAD.getDocument(other_doc.Name):
                FreeCAD.closeDocument(other_doc.Name)

    def test_selection_ex_groups_subelements_and_pick_points(self):
        box = self.make_box("GroupedBox")

        FreeCADGui.Selection.addSelection(box, "Face1", 1.0, 2.0, 3.0)
        FreeCADGui.Selection.addSelection(box, "Face2", 4.0, 5.0, 6.0)

        selection = FreeCADGui.Selection.getSelectionEx(self.doc.Name, 0)

        self.assertEqual(len(selection), 1)
        self.assertEqual(selection[0].ObjectName, box.Name)
        self.assertEqual(len(selection[0].SubElementNames), 2)
        self.assertEqual(len(selection[0].PickedPoints), 2)
        self.assertEqual(
            [(point.x, point.y, point.z) for point in selection[0].PickedPoints],
            [(1.0, 2.0, 3.0), (4.0, 5.0, 6.0)],
        )

    def test_single_selection_flag_rejects_multiple_entries(self):
        first = self.doc.addObject("App::FeaturePython", "First")
        second = self.doc.addObject("App::FeaturePython", "Second")

        FreeCADGui.Selection.addSelection(first)
        self.assertEqual(
            [obj.Name for obj in FreeCADGui.Selection.getSelection(self.doc.Name, 0, True)],
            [first.Name],
        )

        FreeCADGui.Selection.addSelection(second)

        self.assertEqual(FreeCADGui.Selection.getSelection(self.doc.Name, 0, True), [])
        self.assertEqual(FreeCADGui.Selection.getSelectionEx(self.doc.Name, 0, True), [])

    def test_observer_receives_one_add_and_remove_event(self):
        events = []

        class Observer:
            def addSelection(self, document, obj, sub, point):
                events.append(("add", document, obj, sub, point))

            def removeSelection(self, document, obj, sub):
                events.append(("remove", document, obj, sub))

        box = self.make_box("ObservedAddRemoveBox")
        observer = Observer()
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            FreeCADGui.Selection.addSelection(box, "Face1", 1.0, 2.0, 3.0, False)
            FreeCADGui.Selection.removeSelection(box, "Face1")
            FreeCADGui.Selection.removeSelection(box, "Face1")
        finally:
            FreeCADGui.Selection.removeObserver(observer)

        self.assertEqual(
            events,
            [
                ("add", self.doc.Name, box.Name, "Face1", (1.0, 2.0, 3.0)),
                ("remove", self.doc.Name, box.Name, "Face1"),
            ],
        )

    def test_observer_clear_selection_event_is_document_scoped(self):
        events = []

        class Observer:
            def clearSelection(self, document):
                events.append(("clear", document))

        other_doc = FreeCAD.newDocument("TestSelectionObserverOther")
        try:
            first = self.doc.addObject("App::FeaturePython", "First")
            second = other_doc.addObject("App::FeaturePython", "Second")

            FreeCADGui.Selection.addSelection(self.doc.Name, first.Name)
            FreeCADGui.Selection.addSelection(other_doc.Name, second.Name)

            FreeCAD.setActiveDocument(self.doc.Name)

            observer = Observer()
            FreeCADGui.Selection.addObserver(observer, 0)
            try:
                FreeCADGui.Selection.clearSelection(self.doc.Name)
            finally:
                FreeCADGui.Selection.removeObserver(observer)

            self.assertEqual(events, [("clear", self.doc.Name)])
            self.assertFalse(FreeCADGui.Selection.hasSelection(self.doc.Name, 0))
            self.assertTrue(FreeCADGui.Selection.hasSelection(other_doc.Name, 0))
        finally:
            FreeCADGui.Selection.clearSelection(other_doc.Name)
            if FreeCAD.getDocument(other_doc.Name):
                FreeCAD.closeDocument(other_doc.Name)

    def test_preselection_is_cleared_only_when_requested(self):
        box = self.make_box("PreselectionBox")

        FreeCADGui.Selection.setPreselection(box, "Face1", 1.0, 2.0, 3.0)
        self.assertEqual(FreeCADGui.Selection.getPreselection().ObjectName, box.Name)

        FreeCADGui.Selection.addSelection(box, "Face2", 0.0, 0.0, 0.0, False)
        self.assertEqual(FreeCADGui.Selection.getPreselection().ObjectName, box.Name)

        FreeCADGui.Selection.addSelection(box, "Face3")
        self.assertEqual(FreeCADGui.Selection.getPreselection().ObjectName, "")

    def test_clear_selection_respects_clear_preselection_flag(self):
        box = self.make_box("ClearPreselectionBox")

        FreeCADGui.Selection.setPreselection(box, "Face1", 1.0, 2.0, 3.0)
        FreeCADGui.Selection.addSelection(box, "Face2", 0.0, 0.0, 0.0, False)

        FreeCADGui.Selection.clearSelection(self.doc.Name, False)

        self.assertFalse(FreeCADGui.Selection.hasSelection(self.doc.Name, 0))
        self.assertEqual(FreeCADGui.Selection.getPreselection().ObjectName, box.Name)

        FreeCADGui.Selection.clearSelection(self.doc.Name)

        self.assertEqual(FreeCADGui.Selection.getPreselection().ObjectName, "")

    def test_repeated_preselection_does_not_notify_again(self):
        events = []

        class Observer:
            def setPreselection(self, document, obj, sub):
                events.append(("preselect", document, obj, sub))

            def removePreselection(self, document, obj, sub):
                events.append(("remove_preselect", document, obj, sub))

        box = self.make_box("RepeatedPreselectionBox")
        observer = Observer()
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            FreeCADGui.Selection.setPreselection(box, "Face1", 1.0, 2.0, 3.0)
            FreeCADGui.Selection.setPreselection(box, "Face1", 4.0, 5.0, 6.0)
        finally:
            FreeCADGui.Selection.removeObserver(observer)

        preselection = FreeCADGui.Selection.getPreselection()
        self.assertEqual(events, [("preselect", self.doc.Name, box.Name, "Face1")])
        self.assertEqual(preselection.ObjectName, box.Name)
        self.assertEqual(
            [(point.x, point.y, point.z) for point in preselection.PickedPoints],
            [(1.0, 2.0, 3.0)],
        )

    def test_python_observer_receives_selection_changes(self):
        events = []

        class Observer:
            def addSelection(self, document, obj, sub, point):
                events.append(("add", document, obj, sub, point))

            def removeSelection(self, document, obj, sub):
                events.append(("remove", document, obj, sub))

            def clearSelection(self, document):
                events.append(("clear", document))

            def setPreselection(self, document, obj, sub):
                events.append(("preselect", document, obj, sub))

            def removePreselection(self, document, obj, sub):
                events.append(("remove_preselect", document, obj, sub))

        box = self.make_box("ObservedBox")
        observer = Observer()
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            FreeCADGui.Selection.setPreselection(box, "Face1")
            FreeCADGui.Selection.addSelection(box, "Face1", 7.0, 8.0, 9.0, False)
            FreeCADGui.Selection.removeSelection(box, "Face1")
            FreeCADGui.Selection.addSelection(box)
            FreeCADGui.Selection.clearSelection(self.doc.Name)
            FreeCADGui.Selection.clearPreselection()
        finally:
            FreeCADGui.Selection.removeObserver(observer)

        self.assertIn(("preselect", self.doc.Name, box.Name, "Face1"), events)
        self.assertIn(("add", self.doc.Name, box.Name, "Face1", (7.0, 8.0, 9.0)), events)
        self.assertIn(("remove", self.doc.Name, box.Name, "Face1"), events)
        self.assertIn(("add", self.doc.Name, box.Name, "", (0.0, 0.0, 0.0)), events)
        self.assertIn(("clear", self.doc.Name), events)
        self.assertIn(("remove_preselect", self.doc.Name, box.Name, "Face1"), events)
