# ***************************************************************************
# *   Copyright (c) 2019 Markus Hovorka <m.hovorka@live.de>                 *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

"""
ATM these test classes are not activated in TestFemGui
But they run on femtest if not explicit skipped
they can be run with the appropriate commands (see test_commands file)
TODO get the tests activated
methods which test if a gmsh binary is available, only than the test can be run
"""

# from PySide import QtCore
from PySide import QtGui
from PySide.QtTest import QTest

# import FreeCAD
# import FreeCADGui

import GuiTestSupport
import ObjectsFem


class TestGmshTaskPanel(GuiTestSupport.TaskPanelTest):

    def setUp(self):
        self.skipTest("See comment in test_problems module")

    def openTaskPanel(self, mesh):
        vDoc = mesh.ViewObject.Document
        vDoc.setEdit(mesh.Name)

    def closeTaskPanel(self, mesh):
        vDoc = mesh.ViewObject.Document
        vDoc.resetEdit()

    def testCubeDefaultMesh(self):
        cube = self.doc.addObject("Part::Box")
        self.doc.recompute()
        mesh = ObjectsFem.makeMeshGmsh(self.doc, "FEMMeshGmsh")
        mesh.Part = cube
        self.openTaskPanel(mesh)
        self.assertEqual(mesh.FemMesh.NodeCount, 0)
        self.clickButton(QtGui.QDialogButtonBox.Apply)
        self.clickButton(QtGui.QDialogButtonBox.Ok)
        self.assertNotEqual(mesh.FemMesh.NodeCount, 0)

    def testCubeNoMesh(self):
        cube = self.doc.addObject("Part::Box")
        self.doc.recompute()
        mesh = ObjectsFem.makeMeshGmsh(self.doc, "FEMMeshGmsh")
        mesh.Part = cube
        self.openTaskPanel(mesh)
        self.assertEqual(mesh.FemMesh.NodeCount, 0)
        self.clickButton(QtGui.QDialogButtonBox.Ok)
        self.assertEqual(mesh.FemMesh.NodeCount, 0)

    def testCubeMeshResolution(self):
        cube = self.doc.addObject("Part::Box")
        self.doc.recompute()

        meshMany = ObjectsFem.makeMeshGmsh(self.doc, "FEMMeshGmsh")
        meshMany.Part = cube
        self.openTaskPanel(meshMany)
        self.loadTaskPanel("GmshMesh")
        maxIf = self.getChild("if_max")
        minIf = self.getChild("if_min")
        maxIf.setText("")
        minIf.setText("")
        QTest.keyClicks(maxIf, "1.5")
        QTest.keyClicks(minIf, "0.5")
        self.clickButton(QtGui.QDialogButtonBox.Apply)
        self.clickButton(QtGui.QDialogButtonBox.Ok)

        meshLess = ObjectsFem.makeMeshGmsh(self.doc, "FEMMeshGmsh")
        meshLess.Part = cube
        self.openTaskPanel(meshLess)
        self.loadTaskPanel("GmshMesh")
        maxIf = self.getChild("if_max")
        minIf = self.getChild("if_min")
        maxIf.setText("")
        minIf.setText("")
        QTest.keyClicks(maxIf, "2")
        QTest.keyClicks(minIf, "1.5")
        self.clickButton(QtGui.QDialogButtonBox.Apply)
        self.clickButton(QtGui.QDialogButtonBox.Ok)

        self.assertGreater(
            meshMany.FemMesh.NodeCount,
            meshLess.FemMesh.NodeCount)
