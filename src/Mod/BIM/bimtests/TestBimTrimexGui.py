# SPDX-License-Identifier: LGPL-2.1-or-later

"""GUI tests for BIM Trimex."""

from types import SimpleNamespace
from unittest.mock import patch

import Draft
import FreeCAD as App
import FreeCADGui as Gui
import Part
from bimcommands.BimTrimex import BimTrimex
from bimtests import TestArchBaseGui
from draftguitools.gui_trimex import ExtrudeFace


class TestBimTrimexGui(TestArchBaseGui.TestArchBaseGui):

    def test_command_is_registered_separately(self):
        self.assertIn("BIM_Trimex", Gui.listCommands())

    def _edit_axis(self, command, setup, end, distance):
        def edit(face, points, axes, setter):
            command.lockedActivePoint = end
            new_points = list(points)
            new_points[end] = App.Vector(points[end]) + axes[end] * distance
            setter(new_points)
            return True

        with patch.object(command, "_setupAxisTrimex", side_effect=edit):
            self.assertTrue(setup(None))

    def test_based_object_uses_host_and_base_placements(self):
        base = Draft.make_wire([App.Vector(), App.Vector(1000, 0, 0)])
        base.Placement = App.Placement(
            App.Vector(10, 20, 30), App.Rotation(App.Vector(0, 0, 1), 20)
        )
        self.document.recompute()

        host = SimpleNamespace(
            Base=base,
            Placement=App.Placement(
                App.Vector(100, 200, 300), App.Rotation(App.Vector(0, 0, 1), 35)
            ),
        )
        command = BimTrimex()
        command.obj = host

        self._edit_axis(command, command._setupBaseTrimex, 1, 200)

        self.assertTrue(base.Points[-1].isEqual(App.Vector(1200, 0, 0), 1e-7))
        self.assertIs(command.obj, host)

    def test_baseless_wall_preserves_rotation(self):
        rotation = App.Rotation(App.Vector(1, 0, 0), 30)
        wall = SimpleNamespace(
            Length=SimpleNamespace(Value=1000),
            Placement=App.Placement(App.Vector(100, 200, 300), rotation),
        )
        command = BimTrimex()
        command.obj = wall

        self._edit_axis(command, command._setupWallTrimex, 1, -200)

        self.assertEqual(wall.Length, 800)
        self.assertTrue(wall.Placement.Rotation.isSame(rotation, 1e-7))
        self.assertTrue(wall.Placement.Base.isEqual(App.Vector(0, 200, 300), 1e-7))

    def test_offset_pipe_edits_along_terminal_segment(self):
        base = Draft.make_wire([App.Vector(), App.Vector(1000, 0, 0), App.Vector(1000, 1000, 0)])
        self.document.recompute()
        pipe = SimpleNamespace(
            Base=base,
            OffsetStart=SimpleNamespace(Value=0),
            OffsetEnd=SimpleNamespace(Value=100),
            Placement=App.Placement(),
        )
        command = BimTrimex()
        command.obj = pipe

        def edit(face, points, axes, setter):
            self.assertTrue(axes[1].isEqual(App.Vector(0, 1, 0), 1e-7))
            self.assertTrue(points[1].isEqual(App.Vector(1000, 900, 0), 1e-7))
            command.lockedActivePoint = 1
            new_points = list(points)
            new_points[1] = App.Vector(points[1]) + axes[1] * 200
            setter(new_points)
            return True

        with patch.object(command, "_setupAxisTrimex", side_effect=edit):
            self.assertTrue(command._setupPipeTrimex(None))

        self.assertTrue(base.Points[-1].isEqual(App.Vector(1000, 1200, 0), 1e-7))

    def test_structure_end_edits_update_height_and_placement(self):
        for end, distance, height, base in (
            (0, -100, 900, App.Vector(0, 0, 100)),
            (1, 100, 1100, App.Vector()),
        ):
            with self.subTest(end=end):
                proxy = SimpleNamespace(
                    getExtrusionData=lambda obj: (
                        None,
                        App.Vector(0, 0, 1000),
                        App.Placement(),
                    )
                )
                structure = SimpleNamespace(
                    Tool=None,
                    Proxy=proxy,
                    Placement=App.Placement(),
                    IfcType="Slab",
                    Length=SimpleNamespace(Value=100),
                    Height=SimpleNamespace(Value=1000),
                )
                command = BimTrimex()
                command.obj = structure

                self._edit_axis(command, command._setupStructureTrimex, end, distance)

                self.assertEqual(structure.Height, height)
                self.assertTrue(structure.Placement.Base.isEqual(base, 1e-7))

    def test_structure_rejects_multiple_base_placements(self):
        structure = SimpleNamespace(
            Tool=None,
            Proxy=SimpleNamespace(
                getExtrusionData=lambda obj: (
                    None,
                    App.Vector(0, 0, 1000),
                    [App.Placement(), App.Placement()],
                )
            ),
        )
        command = BimTrimex()
        command.obj = structure

        self.assertFalse(command._setupStructureTrimex(None))

    def test_face_extrude_setup_does_not_create_an_object(self):
        source = self.document.addObject("Part::Feature", "Box")
        source.Shape = Part.makeBox(10, 10, 10)
        selection = SimpleNamespace(Object=source, SubObjects=[source.Shape.Faces[0]])
        command = ExtrudeFace()
        command.ui = SimpleNamespace(trimUi=lambda **kwargs: None)
        command.featureName = "Extrude face"
        command.extrudeBase = None
        count = len(self.document.Objects)

        with (
            patch("draftguitools.gui_trimex.trackers.ghostTracker", return_value=object()),
            patch("draftguitools.gui_trimex.trackers.lineTracker", return_value=object()),
            patch.object(command, "_startInteraction"),
        ):
            command._startFaceExtrude(selection)

        self.assertEqual(len(self.document.Objects), count)
        self.assertIsNone(command.extrudeBase)
