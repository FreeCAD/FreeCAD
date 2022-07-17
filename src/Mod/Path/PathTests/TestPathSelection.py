# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022                                                    *
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

import FreeCAD
import MeshPart
import Part
import PathScripts.PathSetupSheet as PathSetupSheet
import PathScripts.PathLog as PathLog
import sys

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())

from PathTests.PathTestUtils import PathTestBase
from PathScripts.PathSelection import GateCombinator, FACEGate, MESHGate, SURFACEGate

if FreeCAD.GuiUp:
    import FreeCADGui as Gui


class AbcGate:
    def allow(self, doc, obj, sub):
        return doc == "A" and obj == "B" and sub == "C"


class XyzGate:
    def allow(self, doc, obj, sub):
        return doc == "X" and obj == "Y" and sub == "Z"


class TestGateCombinator(PathTestBase):
    def test_00(self):
        """Cannot select non match"""
        self.assertFalse(GateCombinator(AbcGate(), XyzGate()).allow("L", "M", "N"))

    def test_01(self):
        """Can select if selection is allowed through first gate"""
        self.assertTrue(GateCombinator(AbcGate(), XyzGate()).allow("A", "B", "C"))

    def test_02(self):
        """Can select if selection is allowed through second gate"""
        self.assertTrue(GateCombinator(AbcGate(), XyzGate()).allow("X", "Y", "Z"))


class TestSelectionGates(PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathSetupSheet")
        self.box = self.doc.addObject("Part::Box", "Box")
        self.box.Length = 10
        self.box.Width = 10
        self.box.Height = 10

        wire = Part.show(
            Part.makePolygon(
                [
                    FreeCAD.Vector(0, 0, 0),
                    FreeCAD.Vector(0, 1, 0),
                    FreeCAD.Vector(1, 1, 0),
                    FreeCAD.Vector(1, 0, 0),
                    FreeCAD.Vector(0, 0, 0),
                ]
            )
        )
        self.face = self.doc.addObject("Part::Face", "Face")
        self.face.Sources = wire

        sphere = self.doc.addObject("Part::Sphere", "Sphere")
        cylinder = self.doc.addObject("Part::Cylinder", "Cylinder")
        self.compound = self.doc.addObject("Part::Compound", "Compound")
        self.compound.Links = [sphere, cylinder]

        self.mesh = self.doc.addObject("Mesh::Feature", "Mesh")
        self.mesh.Mesh = MeshPart.meshFromShape(
            Shape=self.box.Shape,
            LinearDeflection=0.1,
            AngularDeflection=0.453786,
            Relative=False,
        )

        self.doc.recompute()

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def assertCanSelectFace(self, gate):
        self.assertTrue(gate.allow(self.doc, self.face, "Face1"))

    def assertCannotSelectFace(self, gate):
        self.assertFalse(gate.allow(self.doc, self.face, "Face1"))

    def assertCanSelectFaceFromSolid(self, gate):
        self.assertTrue(gate.allow(self.doc, self.box, "Face1"))

    def assertCannotSelectFaceFromSolid(self, gate):
        self.assertFalse(gate.allow(self.doc, self.box, "Face1"))

    def assertCanSelectFaceFromCompound(self, gate):
        self.assertTrue(gate.allow(self.doc, self.compound, "Face1"))

    def assertCannotSelectFaceFromCompound(self, gate):
        self.assertFalse(gate.allow(self.doc, self.compound, "Face1"))

    def assertCanSelectMesh(self, gate):
        self.assertTrue(gate.allow(self.doc, self.mesh, ""))

    def assertCannotSelectMesh(self, gate):
        self.assertFalse(gate.allow(self.doc, self.mesh, ""))

    def assertCannotSelectEdge(self, gate):
        self.assertFalse(gate.allow(self.doc, self.box, "Edge1"))

    # FACEGate
    def test00(self):
        """Test FACEGate can select face"""
        self.assertCanSelectFace(FACEGate())

    def test01(self):
        """Test FACEGate can select face from solid"""
        self.assertCanSelectFaceFromSolid(FACEGate())

    def test02(self):
        """Test FACEGate can select face from compound"""
        self.assertCanSelectFaceFromCompound(FACEGate())

    def test03(self):
        """Test FACEGate cannot select mesh"""
        self.assertCannotSelectMesh(FACEGate())

    def test04(self):
        """Test FACEGate cannot select edge"""
        self.assertCannotSelectEdge(FACEGate())

    # MESHGate
    def test10(self):
        """Test MESHGate cannot select face"""
        self.assertCannotSelectFace(MESHGate())

    def test11(self):
        """Test MESHGate cannot select face from solid"""
        self.assertCannotSelectFaceFromSolid(MESHGate())

    def test12(self):
        """Test MESHGate cannot select face from compound"""
        self.assertCannotSelectFaceFromCompound(MESHGate())

    def test13(self):
        """Test MESHGate can select mesh"""
        self.assertCanSelectMesh(MESHGate())

    def test14(self):
        """Test MESHGate cannot select edge"""
        self.assertCannotSelectEdge(MESHGate())

    # SURFACEGate
    def test20(self):
        """Test SURFACEGate can select face"""
        self.assertCanSelectFace(SURFACEGate())

    def test21(self):
        """Test SURFACEGate can select face from solid"""
        self.assertCanSelectFaceFromSolid(SURFACEGate())

    def test22(self):
        """Test SURFACEGate can select face from compound"""
        self.assertCanSelectFaceFromCompound(SURFACEGate())

    def test23(self):
        """Test SURFACEGate can select mesh"""
        self.assertCanSelectMesh(SURFACEGate())

    def test24(self):
        """Test SURFACEGate cannot select edge"""
        self.assertCannotSelectEdge(SURFACEGate())
