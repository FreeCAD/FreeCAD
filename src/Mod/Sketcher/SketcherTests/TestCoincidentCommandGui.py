# SPDX-License-Identifier: LGPL-2.1-or-later

import math

import FreeCAD
import Part
import Sketcher
from PySide import QtCore, QtGui

from SketcherTests.GuiTestCase import FreeCADGui, SketcherGuiTestCase


class TestCoincidentCommandGui(SketcherGuiTestCase):
    def set_boolean_preference_for_test(self, group, name, value):
        preferences = FreeCAD.ParamGet(group)
        contents = preferences.GetContents() or []
        preference_was_set = any(
            kind == "Boolean" and preference_name == name for kind, preference_name, _ in contents
        )
        if preference_was_set:
            previous_value = preferences.GetBool(name)
            self.addCleanup(preferences.SetBool, name, previous_value)
        else:
            self.addCleanup(preferences.RemBool, name)
        preferences.SetBool(name, value)

    def setUp(self):
        super().setUp()

        self.set_boolean_preference_for_test(
            "User parameter:BaseApp/Preferences/NotificationArea",
            "NonIntrusiveNotificationsEnabled",
            True,
        )
        self.set_boolean_preference_for_test(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/General",
            "NotifyConstraintSubstitutions",
            False,
        )

        self.doc = FreeCAD.newDocument("CoincidentCommandGuiTest")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0)),
            True,
        )
        self.sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(20, 0, 0), FreeCAD.Vector(30, 0, 0)),
            True,
        )
        self.sketch.addConstraint(Sketcher.Constraint("DistanceX", 0, 2, 10.0))
        self.sketch.addConstraint(Sketcher.Constraint("DistanceX", 1, 1, 20.0))

    def apply_coincident_command(self, sketch, *sub_element_names):
        FreeCADGui.ActiveDocument.setEdit(sketch.Name)
        self.flush_gui()
        FreeCADGui.Selection.clearSelection()
        for sub_element_name in sub_element_names:
            FreeCADGui.Selection.addSelection(self.doc.Name, sketch.Name, sub_element_name)

        FreeCADGui.runCommand("Sketcher_ConstrainCoincidentUnified", 0)
        self.flush_gui()

    def apply_coincident_command_capturing_warning(self, sketch, *sub_element_names):
        notification_preferences = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/NotificationArea"
        )
        notification_preference = "NonIntrusiveNotificationsEnabled"
        previous_preference = notification_preferences.GetBool(notification_preference, True)
        notification_preferences.SetBool(notification_preference, False)

        warnings = []
        timer = QtCore.QTimer()
        timer.setInterval(10)

        def capture_warning():
            dialog = QtGui.QApplication.activeModalWidget()
            if dialog is None:
                return

            warnings.append((dialog.windowTitle(), dialog.text()))
            timer.stop()
            dialog.accept()

        timer.timeout.connect(capture_warning)
        timer.start()
        try:
            self.apply_coincident_command(sketch, *sub_element_names)
        finally:
            timer.stop()
            notification_preferences.SetBool(notification_preference, previous_preference)

        return warnings

    def selected_sub_elements(self):
        return [
            name
            for selection in FreeCADGui.Selection.getSelectionEx()
            for name in selection.SubElementNames
        ]

    def constraint_signature(self, sketch):
        return [
            (
                constraint.Type,
                constraint.First,
                constraint.FirstPos,
                constraint.Second,
                constraint.SecondPos,
                constraint.Third,
                constraint.ThirdPos,
                constraint.Value,
            )
            for constraint in sketch.Constraints
        ]

    def assert_direct_coincident_conflicts(self):
        constraint_index = self.sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 2, 1, 1))

        self.assertNotEqual(
            self.sketch.solve(),
            0,
            "Test sketch must repro the solver conflict before testing command rollback.",
        )

        self.sketch.delConstraint(constraint_index)
        self.assertEqual(self.sketch.solve(), 0)

    def test_conflicting_coincident_selection_is_rejected(self):
        self.assertEqual(self.sketch.solve(), 0)
        self.assert_direct_coincident_conflicts()

        constraint_count = len(self.sketch.Constraints)

        warnings = self.apply_coincident_command_capturing_warning(
            self.sketch, "Vertex2", "Vertex3"
        )

        self.assertEqual(len(warnings), 1)
        self.assertEqual(warnings[0][0], "Coincident constraint not added")
        warning_text = warnings[0][1].lower()
        self.assertTrue(
            "conflict" in warning_text or "over-constrain" in warning_text,
            warnings[0][1],
        )
        self.assertCountEqual(self.selected_sub_elements(), ["Vertex2", "Vertex3"])
        self.assertEqual(len(self.sketch.Constraints), constraint_count)
        self.assertEqual(self.sketch.solve(), 0)

    def test_already_coincident_selection_shows_warning(self):
        self.sketch.delConstraint(1)
        self.sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        self.assertEqual(self.sketch.solve(), 0)
        constraint_count = len(self.sketch.Constraints)

        warnings = self.apply_coincident_command_capturing_warning(
            self.sketch, "Vertex2", "Vertex3"
        )

        self.assertEqual(
            warnings,
            [
                (
                    "Coincident constraint not added",
                    "The selected points are already coincident.",
                )
            ],
        )
        self.assertCountEqual(self.selected_sub_elements(), ["Vertex2", "Vertex3"])
        self.assertEqual(len(self.sketch.Constraints), constraint_count)
        self.assertEqual(self.sketch.solve(), 0)

    def test_coincident_endpoints_of_same_edge_are_rejected(self):
        """Regression for #20119: the line between two tangent arcs must not collapse."""
        sketch = self.doc.addObject("Sketcher::SketchObject", "Issue20119Sketch")
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0)),
            False,
        )
        sketch.addGeometry(
            Part.ArcOfCircle(
                Part.Circle(FreeCAD.Vector(0, 5, 0), FreeCAD.Vector(0, 0, 1), 5),
                -math.pi / 2,
                0,
            ),
            False,
        )
        sketch.addGeometry(
            Part.ArcOfCircle(
                Part.Circle(FreeCAD.Vector(10, 5, 0), FreeCAD.Vector(0, 0, 1), 5),
                -math.pi / 2,
                0,
            ),
            False,
        )
        sketch.addConstraint(Sketcher.Constraint("Tangent", 0, 1, 1, 1))
        sketch.addConstraint(Sketcher.Constraint("Tangent", 0, 2, 2, 1))
        self.assertEqual(sketch.solve(), 0)

        invalid_constraint = sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 1, 0, 2))
        self.assertNotEqual(
            sketch.solve(),
            0,
            "The fixture must reproduce failure when the connecting line collapses.",
        )
        sketch.delConstraint(invalid_constraint)
        self.assertEqual(sketch.solve(), 0)

        constraint_count = len(sketch.Constraints)

        warnings = self.apply_coincident_command_capturing_warning(sketch, "Vertex1", "Vertex2")

        self.assertEqual(
            warnings,
            [
                (
                    "Coincident constraint not added",
                    "Cannot add a coincident constraint because it would collapse geometry "
                    "to zero length.",
                )
            ],
        )
        self.assertCountEqual(self.selected_sub_elements(), ["Vertex1", "Vertex2"])
        self.assertEqual(len(sketch.Constraints), constraint_count)
        self.assertEqual(sketch.solve(), 0)

    def test_transitive_coincidence_cannot_collapse_intermediate_line(self):
        sketch = self.doc.addObject("Sketcher::SketchObject", "TransitiveSketch")
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0)),
            True,
        )
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(10, 0, 0), FreeCAD.Vector(20, 0, 0)),
            True,
        )
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(20, 0, 0), FreeCAD.Vector(30, 0, 0)),
            True,
        )
        sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        self.assertEqual(sketch.solve(), 0)

        invalid_constraint = sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 2, 2, 1))
        self.assertNotEqual(
            sketch.solve(),
            0,
            "The fixture must reproduce collapse of the intermediate line.",
        )
        sketch.delConstraint(invalid_constraint)
        self.assertEqual(sketch.solve(), 0)

        constraint_count = len(sketch.Constraints)
        self.apply_coincident_command(sketch, "Vertex2", "Vertex5")

        self.assertCountEqual(self.selected_sub_elements(), ["Vertex2", "Vertex5"])
        self.assertEqual(len(sketch.Constraints), constraint_count)
        self.assertEqual(sketch.solve(), 0)

    def test_failed_tangent_substitution_is_rolled_back(self):
        """A failed generic-to-endpoint tangency substitution must roll back."""
        sketch = self.doc.addObject("Sketcher::SketchObject", "TangentSubstitutionSketch")
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(-10, 0, 0), FreeCAD.Vector(10, 0, 0)),
            False,
        )
        sketch.addGeometry(
            Part.ArcOfCircle(
                Part.Circle(FreeCAD.Vector(0, 5, 0), FreeCAD.Vector(0, 0, 1), 5),
                0.5,
                2.5,
            ),
            False,
        )
        # Keep the line and arc endpoints fixed apart while their supporting curves are tangent.
        sketch.addConstraint(Sketcher.Constraint("Horizontal", 0))
        sketch.addConstraint(Sketcher.Constraint("DistanceX", 0, 1, -10.0))
        sketch.addConstraint(Sketcher.Constraint("DistanceY", 0, 1, 0.0))
        sketch.addConstraint(Sketcher.Constraint("Distance", 0, 20.0))
        sketch.addConstraint(Sketcher.Constraint("DistanceX", 1, 3, 0.0))
        sketch.addConstraint(Sketcher.Constraint("Radius", 1, 5.0))
        sketch.addConstraint(Sketcher.Constraint("DistanceX", 1, 1, 5 * math.cos(0.5)))
        sketch.addConstraint(Sketcher.Constraint("DistanceX", 1, 2, 5 * math.cos(2.5)))
        tangent_constraint = sketch.addConstraint(Sketcher.Constraint("Tangent", 0, 1))
        self.assertEqual(sketch.solve(), 0)

        constraints_before = self.constraint_signature(sketch)
        constraint_count = len(sketch.Constraints)

        sketch.delConstraint(tangent_constraint)
        endpoint_tangent = sketch.addConstraint(Sketcher.Constraint("Tangent", 0, 2, 1, 1))
        self.assertNotEqual(
            sketch.solve(),
            0,
            "The endpoint-tangency substitution must fail for this constrained geometry.",
        )
        sketch.delConstraint(endpoint_tangent)
        sketch.addConstraint(Sketcher.Constraint("Tangent", 0, 1))
        self.assertEqual(sketch.solve(), 0)
        self.assertEqual(self.constraint_signature(sketch), constraints_before)

        self.apply_coincident_command(sketch, "Vertex2", "Vertex3")

        self.assertCountEqual(self.selected_sub_elements(), ["Vertex2", "Vertex3"])
        self.assertEqual(len(sketch.Constraints), constraint_count)
        self.assertEqual(self.constraint_signature(sketch), constraints_before)
        self.assertEqual(sketch.solve(), 0)

    def test_tangent_substitution_preserves_preexisting_diagnostics(self):
        """Renumbering existing diagnostics must not reject a valid substitution."""
        sketch = self.doc.addObject("Sketcher::SketchObject", "TangentSubstitutionDiagnosticSketch")
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0)),
            False,
        )
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(20, 0, 0), FreeCAD.Vector(30, 0, 0)),
            False,
        )
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0, 20, 0), FreeCAD.Vector(10, 25, 0)),
            False,
        )
        sketch.addConstraint(Sketcher.Constraint("Tangent", 0, 1))
        sketch.addConstraint(Sketcher.Constraint("Horizontal", 2))
        sketch.addConstraint(Sketcher.Constraint("Horizontal", 2))
        self.assertNotEqual(
            sketch.solve(),
            0,
            "The duplicate horizontal constraints must create a pre-existing diagnostic.",
        )

        constraints_before = self.constraint_signature(sketch)
        constraint_count = len(sketch.Constraints)
        self.assertEqual(constraint_count, 3)
        self.apply_coincident_command(sketch, "Vertex2", "Vertex3")

        self.assertEqual(self.selected_sub_elements(), [])
        self.assertEqual(len(sketch.Constraints), constraint_count)
        self.assertNotEqual(self.constraint_signature(sketch), constraints_before)
        self.assertNotEqual(sketch.solve(), 0, "The original redundancy should remain.")

    def make_point_on_object_sketch(self, targets, initial_coincidences=()):
        """Three independent line starts at the origin, plus three reference edges.

        targets maps point numbers (1..3) to target geometry IDs or axis IDs.
        Keeping all starts at (0, 0) makes the constraints solvable before the command.
        """
        sketch = self.doc.addObject("Sketcher::SketchObject", "PointOnObjectSketch")
        for end in ((4, 7), (8, 5), (6, -7)):
            sketch.addGeometry(
                Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(end[0], end[1], 0)),
                False,
            )
        for start, end in (
            ((-20, 0), (20, 0)),
            ((0, -20), (0, 20)),
            ((-20, -20), (20, 20)),
        ):
            sketch.addGeometry(
                Part.LineSegment(
                    FreeCAD.Vector(start[0], start[1], 0),
                    FreeCAD.Vector(end[0], end[1], 0),
                ),
                True,
            )
        for first, second in initial_coincidences:
            sketch.addConstraint(Sketcher.Constraint("Coincident", first - 1, 1, second - 1, 1))
        for point, edges in targets.items():
            for edge in edges:
                sketch.addConstraint(Sketcher.Constraint("PointOnObject", point - 1, 1, edge))
        self.doc.recompute()
        self.assertEqual(sketch.solve(), 0, "Fixture must solve before coincident command")
        return sketch

    def assert_point_on_object_command(self, sketch, selected, remaining_targets, coincidences):
        """Exercise the GUI command, not just the conflict-detection helper."""
        before = [c for c in sketch.Constraints if c.Type == "Coincident"]
        self.assertEqual(sketch.solve(), 0)
        self.apply_coincident_command(sketch, *selected)
        actual_targets = sorted(
            (c.First + 1, c.Second) for c in sketch.Constraints if c.Type == "PointOnObject"
        )
        self.assertEqual(actual_targets, sorted(remaining_targets))
        actual_coincidences = [c for c in sketch.Constraints if c.Type == "Coincident"]
        self.assertEqual(len(actual_coincidences), len(before) + coincidences)
        self.assertEqual(sketch.solve(), 0)
        FreeCADGui.ActiveDocument.resetEdit()

    def test_point_on_same_edge_through_coincident_point(self):
        # P1 on edge 3; P2 coincident with P1; P3 on edge 3.
        # Both P1/P3 and P2/P3 must remove the redundant PointOnObject.
        for selected in (
            ("Vertex1", "Vertex5"),
            ("Vertex5", "Vertex1"),
            ("Vertex3", "Vertex5"),
            ("Vertex5", "Vertex3"),
        ):
            with self.subTest(selected=selected):
                sketch = self.make_point_on_object_sketch({1: (3,), 3: (3,)}, ((1, 2),))
                self.assert_point_on_object_command(sketch, selected, [(3, 3)], 1)

    def test_three_point_selection_with_multiple_targets(self):
        # Joining P1, P2 and P3 must remove shared targets on either side
        # while preserving constraints onto different edges.
        for selected in (
            ("Vertex1", "Vertex3", "Vertex5"),
            ("Vertex5", "Vertex3", "Vertex1"),
        ):
            with self.subTest(selected=selected):
                sketch = self.make_point_on_object_sketch({1: (3, 4), 2: (3,), 3: (4, 5)})
                self.assertEqual(sketch.solve(), 0)
                self.apply_coincident_command(sketch, *selected)
                self.assertEqual(len([c for c in sketch.Constraints if c.Type == "Coincident"]), 2)
                remaining = [
                    (c.First + 1, c.Second) for c in sketch.Constraints if c.Type == "PointOnObject"
                ]
                self.assertEqual(len(remaining), 3)
                self.assertIn((3, 5), remaining)
                self.assertEqual(sum(edge == 3 for _, edge in remaining), 1)
                self.assertEqual(sum(edge == 4 for _, edge in remaining), 1)
                self.assertEqual(sketch.solve(), 0)
                FreeCADGui.ActiveDocument.resetEdit()

    def test_point_without_target_does_not_remove_unrelated_constraints(self):
        for selected in (("Vertex1", "Vertex3"), ("Vertex3", "Vertex1")):
            with self.subTest(selected=selected):
                sketch = self.make_point_on_object_sketch({1: (3, 4), 3: (5,)})
                self.assert_point_on_object_command(sketch, selected, [(1, 3), (1, 4), (3, 5)], 1)

    def test_axis_targets_and_origin_are_equivalent(self):
        # The origin lies on both built-in axes, which have IDs -1 and -2.
        for target in (-1, -2):
            for selected in (("Vertex1", "RootPoint"), ("RootPoint", "Vertex1")):
                with self.subTest(target=target, selected=selected):
                    sketch = self.make_point_on_object_sketch({1: (target,)})
                    self.assert_point_on_object_command(sketch, selected, [], 1)

    def test_origin_equivalence_through_a_coincident_point(self):
        for target in (-1, -2):
            for selected in (("Vertex3", "RootPoint"), ("RootPoint", "Vertex3")):
                with self.subTest(target=target, selected=selected):
                    sketch = self.make_point_on_object_sketch({1: (target,), 3: (5,)}, ((1, 2),))
                    self.assert_point_on_object_command(sketch, selected, [(3, 5)], 1)
