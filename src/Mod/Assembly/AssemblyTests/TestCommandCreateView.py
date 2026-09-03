# SPDX-License-Identifier: LGPL-2.1-or-later

import math
import unittest
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

import FreeCAD as App

if App.GuiUp:
    import FreeCADGui as Gui
    import CommandCreateView
    from PySide import QtCore, QtWidgets


class _Move:
    def __init__(self, move_type, placement):
        self.MoveType = move_type
        self.MovementTransform = placement


@unittest.skipIf(not App.GuiUp, "GUI tests require FreeCAD GUI mode")
class TestCommandCreateView(unittest.TestCase):
    def test_move_list_row_edits_movement_distance(self):
        move = _Move("Normal", App.Placement(App.Vector(3, 4, 0), App.Rotation()))
        move.Name = "Move"

        task = CommandCreateView.TaskAssemblyCreateView.__new__(
            CommandCreateView.TaskAssemblyCreateView
        )
        QtCore.QObject.__init__(task)
        task.form = SimpleNamespace(stepList=QtWidgets.QListWidget())
        task.initialPlcs = {}
        task.assembly = SimpleNamespace(
            ViewObject=SimpleNamespace(DraggerPlacement=App.Placement()),
        )
        task.selectedObjs = []
        task.selectedObjsInitPlc = []
        task.com = App.Vector()
        task.size = 100
        task.moveSpinboxes = {}
        task.moveDirections = {}
        task.viewObj = SimpleNamespace(
            Group=[move],
            Proxy=SimpleNamespace(applyMoves=MagicMock()),
        )

        with patch("CommandCreateView.UtilsAssembly.restoreAssemblyPartsPlacements"):
            task.onMovesChanged()
            spinbox = task.moveSpinboxes[move.Name]
            spinbox.setProperty("rawValue", 10)
            Gui.updateGui()

        self.assertEqual(task.form.stepList.count(), 1)
        self.assertEqual(task.form.stepList.item(0).text(), "")
        self.assertEqual(
            task.form.stepList.item(0).data(QtCore.Qt.UserRole),
            move.Name,
        )
        self.assertAlmostEqual(move.MovementTransform.Base.Length, 10)

    def test_set_movement_distance_preserves_direction_and_rotation(self):
        rotation = App.Rotation(App.Vector(0, 0, 1), 30)
        move = _Move("Normal", App.Placement(App.Vector(3, 4, 0), rotation))

        direction = CommandCreateView.setMovementDistance(move, 10)

        self.assertAlmostEqual(move.MovementTransform.Base.Length, 10)
        self.assertAlmostEqual(move.MovementTransform.Base.x, 6)
        self.assertAlmostEqual(move.MovementTransform.Base.y, 8)
        self.assertEqual(move.MovementTransform.Rotation, rotation)
        self.assertAlmostEqual(direction.Length, 1)

    def test_set_movement_distance_reuses_direction_after_zero(self):
        move = _Move("Normal", App.Placement(App.Vector(0, 5, 0), App.Rotation()))
        direction = CommandCreateView.setMovementDistance(move, 0)

        CommandCreateView.setMovementDistance(move, 12, direction)

        self.assertEqual(move.MovementTransform.Base, App.Vector(0, 12, 0))

    def test_set_radial_distance_from_zero(self):
        move = _Move("Radial", App.Placement())

        CommandCreateView.setMovementDistance(move, 7)

        self.assertAlmostEqual(move.MovementTransform.Base.Length, 7)

    def test_mixed_movement_uses_distance_editor(self):
        move = _Move(
            "Normal",
            App.Placement(
                App.Vector(0, 0, 10),
                App.Rotation(App.Vector(0, 0, 1), 45),
            ),
        )

        self.assertEqual(CommandCreateView.movementEditMode(move), "Distance")

    def test_rotation_with_axis_translation_uses_distance_editor(self):
        center = App.Vector(10, 20, 30)
        axis = App.Vector(0, 0, 1)
        pivot = App.Placement(center, App.Rotation())
        move = _Move(
            "Normal",
            App.Placement(center, App.Rotation(axis, 45)) * pivot.inverse(),
        )
        move.MovementTransform.Base += axis * 5

        self.assertEqual(CommandCreateView.movementEditMode(move), "Distance")

    def test_dragger_translation_tolerance_accounts_for_float_noise(self):
        initial = App.Placement(App.Vector(100, 50, 25), App.Rotation())
        noisy = App.Placement(App.Vector(100.000002, 50, 25), App.Rotation())

        tolerance = CommandCreateView.draggerTranslationTolerance(initial, noisy)

        self.assertGreater(tolerance, (noisy.Base - initial.Base).Length)
        self.assertGreaterEqual(tolerance, CommandCreateView.Precision.confusion())

    def test_rotation_switches_to_angle_editor_during_drag(self):
        initial = App.Placement(App.Vector(100, 50, 25), App.Rotation())
        current = App.Placement(
            App.Vector(100.000002, 50, 25),
            App.Rotation(App.Vector(0, 0, 1), 30),
        )
        move = _Move("Normal", App.Placement())
        move.Proxy = SimpleNamespace(applyStep=MagicMock())

        task = CommandCreateView.TaskAssemblyCreateView.__new__(
            CommandCreateView.TaskAssemblyCreateView
        )
        QtCore.QObject.__init__(task)
        task.blockDraggerMove = False
        task.currentStep = move
        task.selectedObjs = []
        task.selectedObjsInitPlc = []
        task.initialDraggerPlc = initial
        task.assembly = SimpleNamespace(
            ViewObject=SimpleNamespace(DraggerPlacement=current),
        )
        task.com = App.Vector()
        task.size = 100
        task.rebuildStepList = MagicMock()
        task.updateMoveSpinbox = MagicMock()

        task.draggerMoved(None)

        self.assertEqual(CommandCreateView.movementEditMode(move), "Angle")
        self.assertAlmostEqual(
            (move.MovementTransform.multVec(initial.Base) - initial.Base).Length,
            0,
        )
        task.rebuildStepList.assert_called_once_with()
        task.updateMoveSpinbox.assert_called_once_with(move)

    def test_drag_rebuilds_row_when_distance_move_becomes_angle_move(self):
        move = _Move(
            "Normal",
            App.Placement(App.Vector(10, 0, 0), App.Rotation()),
        )
        move.Proxy = SimpleNamespace(applyStep=MagicMock())

        task = CommandCreateView.TaskAssemblyCreateView.__new__(
            CommandCreateView.TaskAssemblyCreateView
        )
        QtCore.QObject.__init__(task)
        task.blockDraggerMove = False
        task.currentStep = move
        task.selectedObjs = []
        task.selectedObjsInitPlc = []
        task.initialDraggerPlc = App.Placement()
        task.assembly = SimpleNamespace(
            ViewObject=SimpleNamespace(
                DraggerPlacement=App.Placement(
                    App.Vector(),
                    App.Rotation(App.Vector(0, 0, 1), 30),
                ),
            ),
        )
        task.com = App.Vector()
        task.size = 100
        task.rebuildStepList = MagicMock()
        task.updateMoveSpinbox = MagicMock()

        task.draggerMoved(None)

        task.rebuildStepList.assert_called_once_with()

    def test_drag_translation_is_stored_as_distance_even_with_rotation_like_transform(self):
        move = _Move("Normal", App.Placement())
        move.Proxy = SimpleNamespace(applyStep=MagicMock())

        task = CommandCreateView.TaskAssemblyCreateView.__new__(
            CommandCreateView.TaskAssemblyCreateView
        )
        QtCore.QObject.__init__(task)
        task.blockDraggerMove = False
        task.currentStep = move
        task.selectedObjs = []
        task.selectedObjsInitPlc = []
        task.initialDraggerPlc = App.Placement()
        task.assembly = SimpleNamespace(
            ViewObject=SimpleNamespace(
                DraggerPlacement=App.Placement(
                    App.Vector(10, 0, 0),
                    App.Rotation(App.Vector(0, 0, 1), 30),
                ),
            ),
        )
        task.com = App.Vector()
        task.size = 100
        task.rebuildStepList = MagicMock()
        task.updateMoveSpinbox = MagicMock()

        task.draggerMoved(None)

        self.assertEqual(CommandCreateView.movementEditMode(move), "Distance")
        self.assertAlmostEqual(move.MovementTransform.Base.Length, 10)
        self.assertLessEqual(
            move.MovementTransform.Rotation.Angle,
            CommandCreateView.Precision.angular(),
        )

    def test_spinbox_change_aligns_dragger_to_moved_objects(self):
        move = _Move("Normal", App.Placement())
        move.Name = "Move"
        obj = SimpleNamespace(Name="Box", Placement=App.Placement())
        ref = [SimpleNamespace(Name="Assembly"), ["Box"]]
        center = App.Vector(15, 20, 30)
        placement = App.Placement(App.Vector(10, 20, 30), App.Rotation())

        task = CommandCreateView.TaskAssemblyCreateView.__new__(
            CommandCreateView.TaskAssemblyCreateView
        )
        QtCore.QObject.__init__(task)
        task.blockDraggerMove = False
        task.assembly = SimpleNamespace(
            ViewObject=SimpleNamespace(DraggerPlacement=App.Placement()),
        )

        with (
            patch("CommandCreateView.moveObjectsAndReferences", return_value=([obj], [ref])),
            patch(
                "CommandCreateView.UtilsAssembly.getGlobalPlacement",
                return_value=placement,
            ),
            patch(
                "CommandCreateView.UtilsAssembly.getCenterOfBoundingBox",
                return_value=center,
            ),
        ):
            task.updateDraggerFromMoveObjects(move)

        expected = App.Placement(placement)
        expected.Base = center
        self.assertEqual(task.assembly.ViewObject.DraggerPlacement, expected)
        self.assertFalse(task.blockDraggerMove)

    def test_sync_dragger_baseline_without_selection(self):
        draggerPlacement = App.Placement(App.Vector(1, 2, 3), App.Rotation())
        task = CommandCreateView.TaskAssemblyCreateView.__new__(
            CommandCreateView.TaskAssemblyCreateView
        )
        QtCore.QObject.__init__(task)
        task.assembly = SimpleNamespace(
            ViewObject=SimpleNamespace(DraggerPlacement=draggerPlacement),
        )

        task.syncDraggerBaseline()

        self.assertEqual(task.initialDraggerPlc, draggerPlacement)

    def test_set_movement_angle_preserves_rotation_center(self):
        center = App.Vector(10, 20, 30)
        axis = App.Vector(0, 0, 1)
        pivot = App.Placement(center, App.Rotation())
        move = _Move(
            "Normal",
            App.Placement(center, App.Rotation(axis, 90)) * pivot.inverse(),
        )

        CommandCreateView.setMovementAngle(move, 45)

        transformedCenter = move.MovementTransform.multVec(center)
        self.assertAlmostEqual((transformedCenter - center).Length, 0)
        self.assertAlmostEqual(math.degrees(move.MovementTransform.Rotation.Angle), 45)

    def test_move_list_row_edits_rotation_angle(self):
        center = App.Vector(10, 20, 30)
        axis = App.Vector(0, 0, 1)
        pivot = App.Placement(center, App.Rotation())
        move = _Move(
            "Normal",
            App.Placement(center, App.Rotation(axis, 90)) * pivot.inverse(),
        )
        move.Name = "Move"

        task = CommandCreateView.TaskAssemblyCreateView.__new__(
            CommandCreateView.TaskAssemblyCreateView
        )
        QtCore.QObject.__init__(task)
        task.form = SimpleNamespace(stepList=QtWidgets.QListWidget())
        task.initialPlcs = {}
        task.assembly = SimpleNamespace(
            ViewObject=SimpleNamespace(DraggerPlacement=App.Placement()),
        )
        task.selectedObjs = []
        task.selectedObjsInitPlc = []
        task.com = App.Vector()
        task.size = 100
        task.moveSpinboxes = {}
        task.moveDirections = {}
        task.viewObj = SimpleNamespace(
            Group=[move],
            Proxy=SimpleNamespace(applyMoves=MagicMock()),
        )

        with patch("CommandCreateView.UtilsAssembly.restoreAssemblyPartsPlacements"):
            task.onMovesChanged()
            spinbox = task.moveSpinboxes[move.Name]
            spinbox.setProperty("rawValue", 45)
            Gui.updateGui()

        self.assertEqual(spinbox.property("unit"), "°")
        self.assertAlmostEqual(math.degrees(move.MovementTransform.Rotation.Angle), 45)
        self.assertAlmostEqual((move.MovementTransform.multVec(center) - center).Length, 0)

    def test_editing_previous_move_updates_later_rotation_center(self):
        oldCenter = App.Vector(0, 100, 0)
        newCenter = App.Vector(0, 50, 0)
        axis = App.Vector(1, 0, 0)
        pivot = App.Placement(oldCenter, App.Rotation())
        move1 = _Move(
            "Normal",
            App.Placement(newCenter, App.Rotation()),
        )
        move2 = _Move(
            "Normal",
            App.Placement(oldCenter, App.Rotation(axis, 45)) * pivot.inverse(),
        )
        obj = SimpleNamespace(Name="Box")
        ref = [SimpleNamespace(Name="Assembly"), ["Box"]]

        task = CommandCreateView.TaskAssemblyCreateView.__new__(
            CommandCreateView.TaskAssemblyCreateView
        )
        QtCore.QObject.__init__(task)
        task.initialPlcs = {"Box": App.Placement()}
        task.viewObj = SimpleNamespace(Group=[move1, move2])

        with patch(
            "CommandCreateView.moveObjectsAndReferences",
            return_value=([obj], [ref]),
        ):
            task.adjustDependentMoves(
                move1,
                App.Placement(oldCenter, App.Rotation()),
            )

        self.assertAlmostEqual((move2.MovementTransform.multVec(newCenter) - newCenter).Length, 0)
        self.assertAlmostEqual(math.degrees(move2.MovementTransform.Rotation.Angle), 45)

    def test_editing_previous_rotation_updates_later_translation_direction(self):
        center = App.Vector(0, 100, 0)
        axis = App.Vector(1, 0, 0)
        oldRotation = (
            App.Placement(center, App.Rotation(axis, 45))
            * App.Placement(
                center,
                App.Rotation(),
            ).inverse()
        )
        newRotation = (
            App.Placement(center, App.Rotation(axis, 30))
            * App.Placement(
                center,
                App.Rotation(),
            ).inverse()
        )
        move1 = _Move("Normal", App.Placement(App.Vector(0, 100, 0), App.Rotation()))
        move2 = _Move("Normal", newRotation)
        oldDirection = App.Rotation(axis, 45).multVec(App.Vector(0, 70, 0))
        move3 = _Move("Normal", App.Placement(oldDirection, App.Rotation()))
        obj = SimpleNamespace(Name="Box")
        ref = [SimpleNamespace(Name="Assembly"), ["Box"]]

        task = CommandCreateView.TaskAssemblyCreateView.__new__(
            CommandCreateView.TaskAssemblyCreateView
        )
        QtCore.QObject.__init__(task)
        task.initialPlcs = {"Box": App.Placement()}
        task.viewObj = SimpleNamespace(Group=[move1, move2, move3])

        with patch(
            "CommandCreateView.moveObjectsAndReferences",
            return_value=([obj], [ref]),
        ):
            task.adjustDependentMoves(move2, oldRotation)

        expectedDirection = App.Rotation(axis, 30).multVec(App.Vector(0, 70, 0))
        self.assertAlmostEqual((move3.MovementTransform.Base - expectedDirection).Length, 0)

    def test_rotation_center_is_recovered_from_transform(self):
        center = App.Vector(10, 20, 0)
        pivot = App.Placement(center, App.Rotation())
        transform = (
            App.Placement(
                center,
                App.Rotation(App.Vector(0, 0, 1), 60),
            )
            * pivot.inverse()
        )

        recovered = CommandCreateView.rotationCenterFromTransform(transform)

        self.assertAlmostEqual((recovered - center).Length, 0)

    def test_rotation_center_is_recovered_from_transform_with_arbitrary_axis(self):
        center = App.Vector(10, 20, 30)
        axis = App.Vector(1, 2, 3)
        axis.normalize()
        pivot = App.Placement(center, App.Rotation())
        transform = (
            App.Placement(
                center,
                App.Rotation(axis, 60),
            )
            * pivot.inverse()
        )

        recovered = CommandCreateView.rotationCenterFromTransform(transform)

        self.assertAlmostEqual((transform.multVec(recovered) - recovered).Length, 0)
        self.assertAlmostEqual(((recovered - center).cross(axis)).Length, 0)

    def test_move_context_menu_opens_placement_editor(self):
        move = _Move("Normal", App.Placement())
        move.Name = "Move"
        item = QtWidgets.QListWidgetItem()
        item.setData(QtCore.Qt.UserRole, move.Name)

        task = CommandCreateView.TaskAssemblyCreateView.__new__(
            CommandCreateView.TaskAssemblyCreateView
        )
        QtCore.QObject.__init__(task)
        task.form = SimpleNamespace(stepList=QtWidgets.QListWidget())
        task.form.stepList.addItem(item)
        task.viewObj = SimpleNamespace(
            Document=SimpleNamespace(getObject=lambda _name: move),
        )
        task.onMovesChanged = MagicMock()

        with (
            patch.object(
                QtWidgets.QMenu,
                "exec_",
                lambda menu, _position: menu.actions()[0],
            ),
            patch(
                "CommandCreateView.UtilsAssembly.openEditingPlacementDialog"
            ) as openPlacementEditor,
        ):
            task.showStepContextMenu(item, QtCore.QPoint())

        openPlacementEditor.assert_called_once()
        args = openPlacementEditor.call_args.args
        self.assertEqual(args[:2], (move, "MovementTransform"))
        self.assertTrue(callable(args[2]))
