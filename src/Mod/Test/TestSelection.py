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

    def test_is_selected_matches_document_and_subelement(self):
        other_doc = FreeCAD.newDocument("TestSelectionIsSelectedOther")
        try:
            first = self.make_box("SharedName")
            second = other_doc.addObject("Part::Box", "SharedName")
            other_doc.recompute()

            FreeCADGui.Selection.addSelection(second, "Face1")

            self.assertFalse(FreeCADGui.Selection.isSelected(first, "Face1", 0))
            self.assertFalse(FreeCADGui.Selection.isSelected(second, "Face2", 0))
            self.assertTrue(FreeCADGui.Selection.isSelected(second, "Face1", 0))
        finally:
            FreeCADGui.Selection.clearSelection(other_doc.Name)
            if FreeCAD.getDocument(other_doc.Name):
                FreeCAD.closeDocument(other_doc.Name)

    def test_duplicate_subelement_selection_is_ignored(self):
        events = []

        class Observer:
            def addSelection(self, document, obj, sub, point):
                events.append(("add", document, obj, sub, point))

        box = self.make_box("DuplicateSelectionBox")
        observer = Observer()
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            FreeCADGui.Selection.addSelection(box, "Face1", 1.0, 2.0, 3.0)
            FreeCADGui.Selection.addSelection(box, "Face1", 4.0, 5.0, 6.0)
        finally:
            FreeCADGui.Selection.removeObserver(observer)

        selection = FreeCADGui.Selection.getSelectionEx(self.doc.Name, 0)

        self.assertTrue(FreeCADGui.Selection.isSelected(box, "Face1", 0))
        self.assertEqual(len(selection), 1)
        self.assertEqual(list(selection[0].SubElementNames), ["Face1"])
        self.assertEqual(len(selection[0].PickedPoints), 1)
        self.assertEqual(
            [(point.x, point.y, point.z) for point in selection[0].PickedPoints],
            [(1.0, 2.0, 3.0)],
        )
        self.assertEqual(
            events,
            [("add", self.doc.Name, box.Name, "Face1", (1.0, 2.0, 3.0))],
        )

    def test_invalid_object_selection_is_ignored_without_notification(self):
        events = []

        class Observer:
            def addSelection(self, document, obj, sub, point):
                events.append(("add", document, obj, sub, point))

        observer = Observer()
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            FreeCADGui.Selection.addSelection(self.doc.Name, "MissingObject", "Face1")
        finally:
            FreeCADGui.Selection.removeObserver(observer)

        self.assertEqual(events, [])
        self.assertFalse(FreeCADGui.Selection.hasSelection(self.doc.Name, 0))

    def test_selection_gate_rejects_without_selection_change_or_notification(self):
        gate_calls = []
        events = []

        class Gate:
            def allow(self, document, obj, sub):
                gate_calls.append((document.Name, obj.Name, sub))
                return False

        class Observer:
            def addSelection(self, document, obj, sub, point):
                events.append(("add", document, obj, sub, point))

        box = self.make_box("RejectedByGateBox")
        gate = Gate()
        observer = Observer()
        FreeCADGui.Selection.addSelectionGate(gate, 0)
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            FreeCADGui.Selection.addSelection(box, "Face1", 1.0, 2.0, 3.0)
        finally:
            FreeCADGui.Selection.removeObserver(observer)
            FreeCADGui.Selection.removeSelectionGate()

        self.assertEqual(gate_calls, [(self.doc.Name, box.Name, "Face1")])
        self.assertEqual(events, [])
        self.assertFalse(FreeCADGui.Selection.hasSelection(self.doc.Name, 0))

    def test_selection_gate_allows_single_selection_notification(self):
        gate_calls = []
        events = []

        class Gate:
            def allow(self, document, obj, sub):
                gate_calls.append((document.Name, obj.Name, sub))
                return True

        class Observer:
            def addSelection(self, document, obj, sub, point):
                events.append(("add", document, obj, sub, point))

        box = self.make_box("AcceptedByGateBox")
        gate = Gate()
        observer = Observer()
        FreeCADGui.Selection.addSelectionGate(gate, 0)
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            FreeCADGui.Selection.addSelection(box, "Face1", 1.0, 2.0, 3.0)
        finally:
            FreeCADGui.Selection.removeObserver(observer)
            FreeCADGui.Selection.removeSelectionGate()

        self.assertEqual(gate_calls, [(self.doc.Name, box.Name, "Face1")])
        self.assertEqual(
            events,
            [("add", self.doc.Name, box.Name, "Face1", (1.0, 2.0, 3.0))],
        )
        self.assertTrue(FreeCADGui.Selection.isSelected(box, "Face1", 0))

    def test_preselection_gate_rejects_without_preselection_or_notification(self):
        gate_calls = []
        events = []

        class Gate:
            def allow(self, document, obj, sub):
                gate_calls.append((document.Name, obj.Name, sub))
                return False

        class Observer:
            def setPreselection(self, document, obj, sub):
                events.append(("preselect", document, obj, sub))

        box = self.make_box("RejectedPreselectionGateBox")
        gate = Gate()
        observer = Observer()
        FreeCADGui.Selection.addSelectionGate(gate, 0)
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            FreeCADGui.Selection.setPreselection(box, "Face1", 1.0, 2.0, 3.0, 0)
        finally:
            FreeCADGui.Selection.removeObserver(observer)
            FreeCADGui.Selection.removeSelectionGate()

        self.assertEqual(gate_calls, [(self.doc.Name, box.Name, "Face1")])
        self.assertEqual(events, [])
        self.assertEqual(FreeCADGui.Selection.getPreselection().ObjectName, "")

    def test_preselection_gate_allows_single_preselection_notification(self):
        gate_calls = []
        events = []

        class Gate:
            def allow(self, document, obj, sub):
                gate_calls.append((document.Name, obj.Name, sub))
                return True

        class Observer:
            def setPreselection(self, document, obj, sub):
                events.append(("preselect", document, obj, sub))

        box = self.make_box("AcceptedPreselectionGateBox")
        gate = Gate()
        observer = Observer()
        FreeCADGui.Selection.addSelectionGate(gate, 0)
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            FreeCADGui.Selection.setPreselection(box, "Face1", 1.0, 2.0, 3.0, 0)
        finally:
            FreeCADGui.Selection.removeObserver(observer)
            FreeCADGui.Selection.removeSelectionGate()

        preselection = FreeCADGui.Selection.getPreselection()
        self.assertEqual(gate_calls, [(self.doc.Name, box.Name, "Face1")])
        self.assertEqual(events, [("preselect", self.doc.Name, box.Name, "Face1")])
        self.assertEqual(preselection.ObjectName, box.Name)
        self.assertEqual(
            [(point.x, point.y, point.z) for point in preselection.PickedPoints],
            [(1.0, 2.0, 3.0)],
        )

    def test_selection_stack_returns_last_pushed_selection(self):
        box = self.make_box("StackLastPushedBox")

        FreeCADGui.Selection.addSelection(box, "Face1")
        FreeCADGui.Selection.pushSelStack()

        stack_selection = FreeCADGui.Selection.getSelectionFromStack(self.doc.Name, 0, 0)

        self.assertEqual(len(stack_selection), 1)
        self.assertEqual(stack_selection[0].ObjectName, box.Name)
        self.assertEqual(list(stack_selection[0].SubElementNames), ["Face1"])

    def test_selection_stack_ignores_repeated_push_of_same_selection(self):
        box = self.make_box("StackRepeatedPushBox")

        FreeCADGui.Selection.addSelection(box, "Face1")
        FreeCADGui.Selection.pushSelStack()
        FreeCADGui.Selection.pushSelStack()

        self.assertEqual(
            FreeCADGui.Selection.getSelectionFromStack(self.doc.Name, 0, 1),
            [],
        )

    def test_selection_stack_is_document_scoped(self):
        other_doc = FreeCAD.newDocument("TestSelectionStackOther")
        try:
            first = self.make_box("FirstStackBox")
            second = other_doc.addObject("Part::Box", "SecondStackBox")
            other_doc.recompute()

            FreeCAD.setActiveDocument(self.doc.Name)
            FreeCADGui.Selection.addSelection(first, "Face1")
            FreeCADGui.Selection.pushSelStack()

            FreeCAD.setActiveDocument(other_doc.Name)
            FreeCADGui.Selection.addSelection(second, "Face2")
            FreeCADGui.Selection.pushSelStack()

            first_stack = FreeCADGui.Selection.getSelectionFromStack(self.doc.Name, 0, 0)
            second_stack = FreeCADGui.Selection.getSelectionFromStack(other_doc.Name, 0, 0)

            self.assertEqual(len(first_stack), 1)
            self.assertEqual(first_stack[0].ObjectName, first.Name)
            self.assertEqual(list(first_stack[0].SubElementNames), ["Face1"])
            self.assertEqual(len(second_stack), 1)
            self.assertEqual(second_stack[0].ObjectName, second.Name)
            self.assertEqual(list(second_stack[0].SubElementNames), ["Face2"])
        finally:
            FreeCADGui.Selection.clearSelection(other_doc.Name)
            if FreeCAD.getDocument(other_doc.Name):
                FreeCAD.closeDocument(other_doc.Name)
            FreeCAD.setActiveDocument(self.doc.Name)

    def test_selection_stack_back_and_forward_restore_selection(self):
        box = self.make_box("StackBackForwardBox")

        FreeCADGui.Selection.addSelection(box, "Face1")
        FreeCADGui.Selection.pushSelStack()
        FreeCADGui.Selection.clearSelection(self.doc.Name)
        FreeCADGui.Selection.addSelection(box, "Face2")
        FreeCADGui.Selection.pushSelStack()

        FreeCADGui.runCommand("Std_SelBack")

        self.assertTrue(FreeCADGui.Selection.isSelected(box, "Face1", 0))
        self.assertFalse(FreeCADGui.Selection.isSelected(box, "Face2", 0))

        FreeCADGui.runCommand("Std_SelForward")

        self.assertFalse(FreeCADGui.Selection.isSelected(box, "Face1", 0))
        self.assertTrue(FreeCADGui.Selection.isSelected(box, "Face2", 0))

    def test_set_visible_hides_shows_and_toggles_selected_object(self):
        box = self.make_box("SetVisibleBox")

        FreeCADGui.Selection.addSelection(box)

        FreeCADGui.Selection.setVisible(False)
        self.assertFalse(box.ViewObject.Visibility)
        self.assertTrue(FreeCADGui.Selection.isSelected(box, "", 0))

        FreeCADGui.Selection.setVisible(True)
        self.assertTrue(box.ViewObject.Visibility)
        self.assertTrue(FreeCADGui.Selection.isSelected(box, "", 0))

        FreeCADGui.Selection.setVisible()
        self.assertFalse(box.ViewObject.Visibility)
        self.assertTrue(FreeCADGui.Selection.isSelected(box, "", 0))

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

    def test_remove_selection_subelement_leaves_other_subelements(self):
        events = []

        class Observer:
            def removeSelection(self, document, obj, sub):
                events.append(("remove", document, obj, sub))

        box = self.make_box("RemoveOneSubelementBox")
        FreeCADGui.Selection.addSelection(box, "Face1", 1.0, 2.0, 3.0)
        FreeCADGui.Selection.addSelection(box, "Face2", 4.0, 5.0, 6.0)

        observer = Observer()
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            FreeCADGui.Selection.removeSelection(box, "Face1")
        finally:
            FreeCADGui.Selection.removeObserver(observer)

        selection = FreeCADGui.Selection.getSelectionEx(self.doc.Name, 0)

        self.assertEqual(events, [("remove", self.doc.Name, box.Name, "Face1")])
        self.assertFalse(FreeCADGui.Selection.isSelected(box, "Face1", 0))
        self.assertTrue(FreeCADGui.Selection.isSelected(box, "Face2", 0))
        self.assertEqual(len(selection), 1)
        self.assertEqual(list(selection[0].SubElementNames), ["Face2"])
        self.assertEqual(
            [(point.x, point.y, point.z) for point in selection[0].PickedPoints],
            [(4.0, 5.0, 6.0)],
        )

    def test_remove_selection_subelement_requires_whole_prefix_match(self):
        events = []

        class Observer:
            def removeSelection(self, document, obj, sub):
                events.append(("remove", document, obj, sub))

        box = self.make_box("RemovePartialPrefixBox")
        FreeCADGui.Selection.addSelection(box, "Edge1", 1.0, 2.0, 3.0)
        FreeCADGui.Selection.addSelection(box, "Edge10", 4.0, 5.0, 6.0)

        observer = Observer()
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            FreeCADGui.Selection.removeSelection(box, "Edge1")
        finally:
            FreeCADGui.Selection.removeObserver(observer)

        selection = FreeCADGui.Selection.getSelectionEx(self.doc.Name, 0)

        self.assertEqual(events, [("remove", self.doc.Name, box.Name, "Edge1")])
        self.assertFalse(FreeCADGui.Selection.isSelected(box, "Edge1", 0))
        self.assertTrue(FreeCADGui.Selection.isSelected(box, "Edge10", 0))
        self.assertEqual(len(selection), 1)
        self.assertEqual(list(selection[0].SubElementNames), ["Edge10"])
        self.assertEqual(
            [(point.x, point.y, point.z) for point in selection[0].PickedPoints],
            [(4.0, 5.0, 6.0)],
        )

    def test_remove_missing_subelement_does_not_notify(self):
        events = []

        class Observer:
            def removeSelection(self, document, obj, sub):
                events.append(("remove", document, obj, sub))

        box = self.make_box("RemoveMissingSubelementBox")
        FreeCADGui.Selection.addSelection(box, "Face1")

        observer = Observer()
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            FreeCADGui.Selection.removeSelection(box, "Face2")
        finally:
            FreeCADGui.Selection.removeObserver(observer)

        self.assertEqual(events, [])
        self.assertTrue(FreeCADGui.Selection.isSelected(box, "Face1", 0))

    def test_remove_object_without_subelement_removes_all_subelements(self):
        events = []

        class Observer:
            def removeSelection(self, document, obj, sub):
                events.append(("remove", document, obj, sub))

        box = self.make_box("RemoveWholeObjectBox")
        FreeCADGui.Selection.addSelection(box, "Face1")
        FreeCADGui.Selection.addSelection(box, "Face2")

        observer = Observer()
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            FreeCADGui.Selection.removeSelection(box)
        finally:
            FreeCADGui.Selection.removeObserver(observer)

        self.assertEqual(
            events,
            [
                ("remove", self.doc.Name, box.Name, "Face1"),
                ("remove", self.doc.Name, box.Name, "Face2"),
            ],
        )
        self.assertFalse(FreeCADGui.Selection.hasSelection(self.doc.Name, 0))

    def test_deleted_selected_object_is_removed_from_selection(self):
        events = []

        class Observer:
            def removeSelection(self, document, obj, sub):
                events.append(("remove", document, obj, sub))

        target = self.make_box("DeletedSelectedBox")
        survivor = self.make_box("SurvivingSelectedBox")
        target_name = target.Name

        FreeCADGui.Selection.addSelection(target, "Face1")
        FreeCADGui.Selection.addSelection(survivor, "Face2")

        observer = Observer()
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            self.doc.removeObject(target_name)
            self.doc.recompute()
        finally:
            FreeCADGui.Selection.removeObserver(observer)

        self.assertEqual(events, [("remove", self.doc.Name, target_name, "Face1")])
        self.assertTrue(FreeCADGui.Selection.isSelected(survivor, "Face2", 0))
        self.assertEqual(
            [obj.Name for obj in FreeCADGui.Selection.getSelection(self.doc.Name, 0)],
            [survivor.Name],
        )

    def test_deleted_preselected_object_clears_preselection(self):
        events = []

        class Observer:
            def removePreselection(self, document, obj, sub):
                events.append(("remove_preselect", document, obj, sub))

        box = self.make_box("DeletedPreselectedBox")
        box_name = box.Name

        FreeCADGui.Selection.setPreselection(box, "Face1")

        observer = Observer()
        FreeCADGui.Selection.addObserver(observer, 0)
        try:
            self.doc.removeObject(box_name)
            self.doc.recompute()
        finally:
            FreeCADGui.Selection.removeObserver(observer)

        self.assertEqual(events, [("remove_preselect", self.doc.Name, box_name, "Face1")])
        self.assertEqual(FreeCADGui.Selection.getPreselection().ObjectName, "")

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

    def test_clear_selection_without_document_uses_active_document(self):
        events = []

        class Observer:
            def clearSelection(self, document):
                events.append(("clear", document))

        other_doc = FreeCAD.newDocument("TestSelectionActiveClearOther")
        try:
            first = self.doc.addObject("App::FeaturePython", "First")
            second = other_doc.addObject("App::FeaturePython", "Second")

            FreeCADGui.Selection.addSelection(self.doc.Name, first.Name)
            FreeCADGui.Selection.addSelection(other_doc.Name, second.Name)
            FreeCAD.setActiveDocument(self.doc.Name)

            observer = Observer()
            FreeCADGui.Selection.addObserver(observer, 0)
            try:
                FreeCADGui.Selection.clearSelection()
            finally:
                FreeCADGui.Selection.removeObserver(observer)

            self.assertEqual(events, [("clear", self.doc.Name)])
            self.assertFalse(FreeCADGui.Selection.hasSelection(self.doc.Name, 0))
            self.assertTrue(FreeCADGui.Selection.hasSelection(other_doc.Name, 0))
        finally:
            FreeCADGui.Selection.clearSelection(other_doc.Name)
            if FreeCAD.getDocument(other_doc.Name):
                FreeCAD.closeDocument(other_doc.Name)

    def test_clear_selection_without_document_respects_clear_preselection_flag(self):
        box = self.make_box("ClearActivePreselectionBox")

        FreeCADGui.Selection.setPreselection(box, "Face1", 1.0, 2.0, 3.0)
        FreeCADGui.Selection.addSelection(box, "Face2", 0.0, 0.0, 0.0, False)

        FreeCADGui.Selection.clearSelection(False)

        self.assertFalse(FreeCADGui.Selection.hasSelection(self.doc.Name, 0))
        self.assertEqual(FreeCADGui.Selection.getPreselection().ObjectName, box.Name)

        FreeCADGui.Selection.clearSelection()

        self.assertEqual(FreeCADGui.Selection.getPreselection().ObjectName, "")

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
