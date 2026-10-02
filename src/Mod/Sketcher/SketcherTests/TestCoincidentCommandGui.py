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
        self.apply_coincident_command(sketch, "Vertex2", "Vertex3")

        self.assertEqual(self.selected_sub_elements(), [])
        self.assertEqual(len(sketch.Constraints), constraint_count)
        self.assertNotEqual(self.constraint_signature(sketch), constraints_before)
        self.assertNotEqual(sketch.solve(), 0, "The original redundancy should remain.")
