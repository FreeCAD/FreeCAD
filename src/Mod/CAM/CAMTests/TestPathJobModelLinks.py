# SPDX-License-Identifier: LGPL-2.1-or-later

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

"""What is made from a Job's model when the model is moved, or taken out and added back: the
operations' base geometry and the work planes keep to the part."""

import FreeCAD

import Path.Main.Job as PathJob
import Path.Main.Workplane as PathWorkplane
import Path.Op.Custom as PathCustom
import CAMTests.PathTestUtils as PathTestUtils

from FreeCAD import Vector


class TestPathJobModelLinks(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathJobModelLinks")
        self.box = self.doc.addObject("Part::Box", "Box")
        self.box.Length, self.box.Width, self.box.Height = 100, 60, 40
        self.doc.recompute()
        self.job = PathJob.Create("Job", [self.box], None)
        self.doc.recompute()

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _model(self):
        return self.job.Model.Group[0]

    def _faceNamed(self, obj, normal):
        for i, face in enumerate(obj.Shape.Faces):
            if (face.normalAt(0, 0) - normal).Length < 1e-6:
                return "Face%d" % (i + 1)
        return None

    def _op(self, name="Op"):
        op = PathCustom.Create(name, parentJob=self.job)
        op.ToolController.Tool.Diameter = 5.0
        self.doc.recompute()
        return op

    def test00_model_added_back_is_linked_again(self):
        """Taking the model out unlinks what is made from it; adding it back links it again."""
        model = self._model()
        top = self._faceNamed(model, Vector(0, 0, 1))
        op = self._op()
        op.addProperty("App::PropertyLinkSubListGlobal", "Faces", "Test", "")
        op.Faces = [(model, [top])]
        plane = PathWorkplane.createWorkplane(self.job, model, top)
        self.doc.recompute()

        self.job.Proxy.removeBase(self.job, model, True)
        self.doc.recompute()
        self.assertEqual(op.Faces, [])
        self.assertFalse(plane.AttachmentSupport)

        clone = self.job.Proxy.addModel(self.job, self.box)
        self.doc.recompute()
        self.assertEqual(len(op.Faces), 1)
        self.assertIs(op.Faces[0][0], clone)
        self.assertEqual(list(op.Faces[0][1]), [top])
        self.assertIs(plane.AttachmentSupport[0][0], clone)
        self.assertEqual(dict(self.job.DetachedModelLinks), {})

    def test01_model_added_fresh_links_nothing(self):
        """A model never taken out is added with nothing linked to it."""
        other = self.doc.addObject("Part::Box", "Other")
        self.doc.recompute()
        clone = self.job.Proxy.addModel(self.job, other)
        self.assertIn(clone, self.job.Model.Group)
        self.assertEqual(PathJob.linksTo(clone, (self.job, self.job.Model)), [])

    def test02_placed_workplane_follows_the_model(self):
        """A work plane made at a placement stays where it is on the part when the model moves."""
        at = FreeCAD.Placement(Vector(10, 20, 30), FreeCAD.Rotation(Vector(0, 1, 0), -20))
        plane = PathWorkplane.createWorkplane(self.job, placement=at)
        self.assertEqual(plane.MapMode, "ObjectXY")
        self.assertTrue(plane.Placement.isSame(at, 1e-9))

        model = self._model()
        model.Placement = FreeCAD.Placement(Vector(0, 0, 7.5), FreeCAD.Rotation())
        self.doc.recompute()
        self.assertRoughly(plane.Placement.Base.z, 37.5)
        self.assertRoughly(plane.Placement.Base.x, 10)

    def test03_unattached_things_are_carried(self):
        """What is placed on the part without being attached to it is what a move carries."""
        loose = PathWorkplane.createWorkplane(self.job)
        loose.AttachmentSupport = []
        loose.MapMode = "Deactivated"
        attached = PathWorkplane.createWorkplane(self.job, placement=FreeCAD.Placement())
        text = self.doc.addObject("Part::Feature", "Text")
        op = self._op()
        op.addProperty("App::PropertyLinkList", "BaseShapes", "Test", "")
        op.BaseShapes = [text]
        carried = PathJob.objectsInModelFrame(self.job)
        self.assertIn(loose, carried)
        self.assertIn(text, carried)
        self.assertNotIn(attached, carried)
        self.assertNotIn(self._model(), carried)

    def test05_scaled_model_keeps_what_is_made_from_it(self):
        """The model scaled: what is attached to it, by its own frame or a face, and what is set
        on it unattached stay where they were on the part, each its own size."""
        model = self._model()
        top = self._faceNamed(model, Vector(0, 0, 1))
        at = FreeCAD.Placement(Vector(10, 20, 30), FreeCAD.Rotation(Vector(0, 1, 0), -20))
        placed = PathWorkplane.createWorkplane(self.job, placement=at)
        onFace = PathWorkplane.createWorkplane(self.job, model, top)
        onFace.AttachmentOffset = FreeCAD.Placement(Vector(40, 30, 0), FreeCAD.Rotation())
        text = self.doc.addObject("Part::Feature", "Text")
        text.Placement = FreeCAD.Placement(
            Vector(50, 40, 40), FreeCAD.Rotation(Vector(0, 0, 1), 30)
        )
        op = self._op()
        op.addProperty("App::PropertyLinkList", "BaseShapes", "Test", "")
        op.BaseShapes = [text]
        self.doc.recompute()
        before = {o.Name: FreeCAD.Placement(o.Placement) for o in (placed, onFace, text)}

        model.Scale = Vector(0.5, 0.5, 0.5)
        self.doc.recompute()
        for obj in (placed, onFace, text):
            was = before[obj.Name]
            self.assertTrue(obj.Placement.Base.isEqual(was.Base * 0.5, 1e-6), obj.Label)
            self.assertTrue(obj.Placement.Rotation.isSame(was.Rotation, 1e-9), obj.Label)
        # on the top face, now half as high
        self.assertRoughly(onFace.Placement.Base.z, 20)
        # scaled back, back where they were
        self.doc.openTransaction("Scale")
        model.Scale = Vector(1, 1, 1)
        self.doc.commitTransaction()
        self.doc.recompute()
        for obj in (placed, onFace, text):
            self.assertTrue(obj.Placement.Base.isEqual(before[obj.Name].Base, 1e-6), obj.Label)
