#***************************************************************************
#*   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
#*                                                                         *
#*   This file is part of FreeCAD.                                         *
#*                                                                         *
#*   FreeCAD is free software: you can redistribute it and/or modify it    *
#*   under the terms of the GNU Lesser General Public License as           *
#*   published by the Free Software Foundation, either version 2.1 of the  *
#*   License, or (at your option) any later version.                       *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful, but        *
#*   WITHOUT ANY WARRANTY; without even the implied warranty of            *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
#*   Lesser General Public License for more details.                       *
#*                                                                         *
#*   You should have received a copy of the GNU Lesser General Public      *
#*   License along with FreeCAD. If not, see                               *
#*   <https://www.gnu.org/licenses/>.                                      *
#*                                                                         *
#***************************************************************************

import unittest

import FreeCAD
import FreeCADGui

""" Test active object list """
class TestActiveObject(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("PartDesignTestSketch")
        self.doc.UndoMode = True

    def testPartBody(self):
        self.doc.openTransaction("Create part")
        part = self.doc.addObject("App::Part", "Part")
        FreeCADGui.activateView('Gui::View3DInventor', True)
        FreeCADGui.activeView().setActiveObject('part', part)
        self.doc.commitTransaction()

        self.doc.openTransaction("Create body")
        body = self.doc.addObject('PartDesign::Body', 'Body')
        part.addObject(body)
        FreeCADGui.activateView('Gui::View3DInventor', True)
        FreeCADGui.activeView().setActiveObject('pdbody', body)
        self.doc.commitTransaction()

        self.doc.undo() # undo body creation
        self.doc.undo() # undo part creation

        FreeCADGui.updateGui()

        self.doc.openTransaction("Create body")
        body = self.doc.addObject('PartDesign::Body', 'Body')
        FreeCADGui.activateView('Gui::View3DInventor', True)
        FreeCADGui.activeView().setActiveObject('pdbody', body)
        self.doc.commitTransaction()

        FreeCADGui.updateGui()

    def tearDown(self):
        FreeCAD.closeDocument("PartDesignTestSketch")
