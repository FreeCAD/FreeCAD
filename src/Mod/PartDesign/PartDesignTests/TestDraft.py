# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import math
import unittest

import FreeCAD
import Part

App = FreeCAD


class TestDraft(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestDraft")

    def testSimpleDraft(self):
        # fix: create datum plane on YZ. create datum line on Z + 10i
        # find top face by first making list comprehension of Z-normal faces
        # and then find which has the higher center of mass Z-value
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.Box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        self.Body.addObject(self.Box)
        self.Box.Length = 10.00
        self.Box.Width = 10.00
        self.Box.Height = 10.00
        self.Doc.recompute()
        self.DatumPlane = self.Doc.addObject("PartDesign::Plane", "DatumPlane")
        self.DatumPlane.AttachmentSupport = [(self.Doc.YZ_Plane, "")]
        self.DatumPlane.MapMode = "FlatFace"
        self.Body.addObject(self.DatumPlane)
        self.Doc.recompute()
        self.DatumLine = self.Doc.addObject("PartDesign::Line", "DatumLine")
        self.DatumLine.AttachmentSupport = [(self.Doc.X_Axis, "")]
        self.DatumLine.MapMode = "TwoPointLine"
        self.Body.addObject(self.DatumLine)
        self.Doc.recompute()
        self.Draft = self.Doc.addObject("PartDesign::Draft", "Draft")
        # Draft.Base needs to be top face
        self.Faces = self.Box.Shape.Faces
        # Grab the two faces with Z-normals and find the higher one
        self.ZFaceIndexes = [
            i for i in range(len(self.Faces)) if self.Faces[i].Surface.Axis == App.Vector(0, 0, 1)
        ]
        if (
            self.Faces[self.ZFaceIndexes[0]].CenterOfMass.z
            > self.Faces[self.ZFaceIndexes[1]].CenterOfMass.z
        ):
            self.TopFaceIndex = self.ZFaceIndexes[0]
        else:
            self.TopFaceIndex = self.ZFaceIndexes[1]
        self.Draft.Base = (self.Box, ["Face" + str(self.TopFaceIndex + 1)])
        self.Draft.NeutralPlane = (self.DatumPlane, [""])
        self.Draft.PullDirection = (self.DatumLine, [""])
        self.Draft.Angle = 45.0
        self.Draft.Reversed = 1
        self.Body.addObject(self.Draft)
        self.Doc.recompute()
        if "Invalid" in self.Draft.State:
            self.Draft.Reversed = 0
            self.Doc.recompute()
        self.assertAlmostEqual(self.Draft.Shape.Volume, 1500)

    def testDraftDoesNotKeepInvalidShape(self):
        # Some OCCT versions build an invalid solid for this model. The feature
        # must then report an error instead of keeping that shape.
        halfSize = 25.0
        radius = 4.0
        inner = halfSize - radius
        normal = App.Vector(0, 0, 1)

        def arc(x, y, start):
            circle = Part.Circle(App.Vector(x, y, 0), normal, radius)
            return Part.ArcOfCircle(circle, start, start + math.pi / 2)

        body = self.Doc.addObject("PartDesign::Body", "Body")
        self.Doc.recompute()
        planes = {
            feature.Role: feature
            for feature in body.Origin.OriginFeatures
            if feature.Role.endswith("_Plane")
        }
        sketch = body.newObject("Sketcher::SketchObject", "Sketch")
        sketch.AttachmentSupport = [(planes["XZ_Plane"], "")]
        sketch.MapMode = "FlatFace"
        sketch.addGeometry(
            [
                Part.LineSegment(App.Vector(-inner, -halfSize, 0), App.Vector(inner, -halfSize, 0)),
                arc(inner, -inner, -math.pi / 2),
                Part.LineSegment(App.Vector(halfSize, -inner, 0), App.Vector(halfSize, inner, 0)),
                arc(inner, inner, 0),
                Part.LineSegment(App.Vector(inner, halfSize, 0), App.Vector(-inner, halfSize, 0)),
                arc(-inner, inner, math.pi / 2),
                Part.LineSegment(App.Vector(-halfSize, inner, 0), App.Vector(-halfSize, -inner, 0)),
                arc(-inner, -inner, math.pi),
            ],
            False,
        )
        pad = body.newObject("PartDesign::Pad", "Pad")
        pad.Profile = sketch
        pad.Length = 250.0
        self.Doc.recompute()

        caps = [
            "Face" + str(index + 1)
            for index, face in enumerate(pad.Shape.Faces)
            if face.Surface.TypeId == "Part::GeomPlane" and abs(face.normalAt(0, 0).y) > 0.99
        ]
        self.assertEqual(len(caps), 2)

        draft = body.newObject("PartDesign::Draft", "Draft")
        draft.Base = (pad, caps)
        draft.Angle = 45.0
        draft.NeutralPlane = (planes["XZ_Plane"], [""])
        self.Doc.recompute()

        if "Invalid" not in draft.State:
            self.assertTrue(draft.Shape.isValid())

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument("PartDesignTestDraft")
        # print ("omit closing document for debugging")
