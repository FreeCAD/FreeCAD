# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

# Unit test for the Arch module

import os
import unittest

import FreeCAD as App
from FreeCAD import Units

import Arch
import Draft
import Part
import Sketcher
import TechDraw
import WorkingPlane

from draftutils.messages import _msg

if App.GuiUp:
    import FreeCADGui

from bimtests.TestWebGLExportGui import TestWebGLExportGui

class ArchTest(unittest.TestCase):

    def setUp(self):
        # setting a new document to hold the tests
        if App.ActiveDocument:
            if App.ActiveDocument.Name != "ArchTest":
                App.newDocument("ArchTest")
        else:
            App.newDocument("ArchTest")
        App.setActiveDocument("ArchTest")

    def testRebar(self):
        App.Console.PrintLog ('Checking Arch Rebar…\n')
        s = Arch.makeStructure(length=2,width=3,height=5)
        sk = App.ActiveDocument.addObject('Sketcher::SketchObject','Sketch')
        sk.AttachmentSupport = (s,["Face6"])
        sk.addGeometry(Part.LineSegment(App.Vector(-0.85,1.25,0),App.Vector(0.75,1.25,0)))
        sk.addGeometry(Part.LineSegment(App.Vector(0.75,1.25,0),App.Vector(0.75,-1.20,0)))
        sk.addGeometry(Part.LineSegment(App.Vector(0.75,-1.20,0),App.Vector(-0.85,-1.20,0)))
        sk.addGeometry(Part.LineSegment(App.Vector(-0.85,-1.20,0),App.Vector(-0.85,1.25,0)))
        sk.addConstraint(Sketcher.Constraint('Coincident',0,2,1,1))
        sk.addConstraint(Sketcher.Constraint('Coincident',1,2,2,1))
        sk.addConstraint(Sketcher.Constraint('Coincident',2,2,3,1))
        sk.addConstraint(Sketcher.Constraint('Coincident',3,2,0,1))
        r = Arch.makeRebar(s,sk,diameter=.1,amount=2)
        self.assertTrue(r,"Arch Rebar failed")

    def testBuildingPart(self):
        """Create a BuildingPart from a wall with a window and check its shape.
        """
        # Also regression test for:
        # https://github.com/FreeCAD/FreeCAD/issues/6178
        operation = "Arch BuildingPart"
        _msg("  Test '{}'".format(operation))
        # Most of the code below taken from testWindow function.
        line = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(3000, 0, 0))
        wall = Arch.makeWall(line)
        sk = App.ActiveDocument.addObject("Sketcher::SketchObject", "Sketch001")
        sk.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), 90)
        sk.addGeometry(Part.LineSegment(App.Vector( 500,  800, 0), App.Vector(1500,  800, 0)))
        sk.addGeometry(Part.LineSegment(App.Vector(1500,  800, 0), App.Vector(1500, 2000, 0)))
        sk.addGeometry(Part.LineSegment(App.Vector(1500, 2000, 0), App.Vector( 500, 2000, 0)))
        sk.addGeometry(Part.LineSegment(App.Vector( 500, 2000, 0), App.Vector( 500,  800, 0)))
        sk.addConstraint(Sketcher.Constraint('Coincident', 0, 2, 1, 1))
        sk.addConstraint(Sketcher.Constraint('Coincident', 1, 2, 2, 1))
        sk.addConstraint(Sketcher.Constraint('Coincident', 2, 2, 3, 1))
        sk.addConstraint(Sketcher.Constraint('Coincident', 3, 2, 0, 1))
        App.ActiveDocument.recompute()
        win = Arch.makeWindow(sk)
        Arch.removeComponents(win, host=wall)
        App.ActiveDocument.recompute()
        bp = Arch.makeBuildingPart()

        # Wall visibility works when standalone
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection('ArchTest',wall.Name)
        assert wall.Visibility
        FreeCADGui.runCommand('Std_ToggleVisibility',0)
        App.ActiveDocument.recompute()
        assert not wall.Visibility
        FreeCADGui.runCommand('Std_ToggleVisibility',0)
        assert wall.Visibility

        bp.Group = [wall]
        App.ActiveDocument.recompute()
        # Fails with OCC 7.5
        # self.assertTrue(len(bp.Shape.Faces) == 16, "'{}' failed".format(operation))

        # Wall visibility works when inside a BuildingPart
        FreeCADGui.runCommand('Std_ToggleVisibility',0)
        App.ActiveDocument.recompute()
        assert not wall.Visibility
        FreeCADGui.runCommand('Std_ToggleVisibility',0)
        assert wall.Visibility

        # Wall visibility works when BuildingPart Toggled
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection('ArchTest',bp.Name)
        FreeCADGui.runCommand('Std_ToggleVisibility',0)
        assert not wall.Visibility
        FreeCADGui.runCommand('Std_ToggleVisibility',0)
        assert wall.Visibility

        # Wall visibiity works inside group inside BuildingPart Toggled
        grp = App.ActiveDocument.addObject("App::DocumentObjectGroup","Group")
        grp.Label="Group"
        grp.Group = [wall]
        bp.Group = [grp]
        App.ActiveDocument.recompute()
        assert wall.Visibility
        FreeCADGui.runCommand('Std_ToggleVisibility',0)
        App.ActiveDocument.recompute()
        assert not wall.Visibility
        FreeCADGui.runCommand('Std_ToggleVisibility',0)
        App.ActiveDocument.recompute()
        assert wall.Visibility

    def testImportSH3D(self):
        """Import a SweetHome 3D file
        """
        operation = "importers.importSH3D"
        _msg("  Test '{}'".format(operation))
        import BIM.importers.importSH3DHelper
        importer = BIM.importers.importSH3DHelper.SH3DImporter(None)
        importer.import_sh3d_from_string(SH3D_HOME)
        assert App.ActiveDocument.Site
        assert App.ActiveDocument.BuildingPart.Label == "Building"
        assert App.ActiveDocument.BuildingPart001.Label == "Level"
        assert App.ActiveDocument.Wall

    def tearDown(self):
        App.closeDocument("ArchTest")
        pass


SH3D_HOME = """<?xml version='1.0'?>
<home version='7200' name='0-JoinWall.sh3d' camera='topCamera' wallHeight='250.0'>
  <property name='com.eteks.sweethome3d.SweetHome3D.CatalogPaneDividerLocation' value='327'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.ColumnWidths' value='100,84,82,85,84'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.FrameHeight' value='576'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.FrameMaximized' value='true'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.FrameWidth' value='1092'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.FrameX' value='50'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.FrameY' value='87'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.MainPaneDividerLocation' value='441'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.PlanPaneDividerLocation' value='263'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.PlanScale' value='0.21343713'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.PlanViewportX' value='0'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.PlanViewportY' value='0'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.ScreenHeight' value='720'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.ScreenWidth' value='1366'/>
  <furnitureVisibleProperty name='NAME'/>
  <furnitureVisibleProperty name='WIDTH'/>
  <furnitureVisibleProperty name='DEPTH'/>
  <furnitureVisibleProperty name='HEIGHT'/>
  <furnitureVisibleProperty name='VISIBLE'/>
  <environment groundColor='FFB78744' skyColor='00CCE4FC' lightColor='00D0D0D0' ceillingLightColor='00D0D0D0' photoWidth='400' photoHeight='300' photoAspectRatio='VIEW_3D_RATIO' photoQuality='0' videoWidth='320' videoAspectRatio='RATIO_4_3' videoQuality='0' videoFrameRate='25'>
    <texture attribute='skyTexture' name='Cloudy' creator='eTeks' catalogId='eTeks#cloudy' width='100.0' height='27.6' image='0'/>
  </environment>
  <compass x='-100.0' y='50.0' diameter='100.0' northDirection='0.0' longitude='-0.06428629' latitude='0.70511305' timeZone='Europe/Madrid'/>
  <observerCamera attribute='observerCamera' lens='PINHOLE' x='50.0' y='50.0' z='170.0' yaw='5.4977875' pitch='0.19634955' fieldOfView='1.0995575' time='1729080000000'/>
  <camera attribute='topCamera' lens='PINHOLE' x='1304.082' y='1936.5889' z='1526.6199' yaw='8.98363' pitch='0.7049999' fieldOfView='1.0995575' time='1729080000000'/>
  <wall id='wall0' wallAtEnd='wall1' xStart='0.0' yStart='0.0' xEnd='100.0' yEnd='0.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall1' wallAtStart='wall0' wallAtEnd='wall2' xStart='100.0' yStart='0.0' xEnd='200.0' yEnd='0.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall2' wallAtStart='wall1' xStart='200.0' yStart='0.0' xEnd='300.0' yEnd='0.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall3' wallAtEnd='wall4' xStart='0.0' yStart='50.0' xEnd='100.0' yEnd='100.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall4' wallAtStart='wall3' wallAtEnd='wall5' xStart='100.0' yStart='100.0' xEnd='200.0' yEnd='100.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall5' wallAtStart='wall4' xStart='200.0' yStart='100.0' xEnd='300.0' yEnd='50.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall6' wallAtEnd='wall7' xStart='0.0' yStart='200.0' xEnd='100.0' yEnd='300.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall7' wallAtStart='wall6' wallAtEnd='wall8' xStart='100.0' yStart='300.0' xEnd='200.0' yEnd='300.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall8' wallAtStart='wall7' xStart='200.0' yStart='300.0' xEnd='300.0' yEnd='200.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall9' wallAtEnd='wall10' xStart='100.0' yStart='400.0' xEnd='100.0' yEnd='500.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall10' wallAtStart='wall9' wallAtEnd='wall11' xStart='100.0' yStart='500.0' xEnd='200.0' yEnd='500.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall11' wallAtStart='wall10' xStart='200.0' yStart='500.0' xEnd='200.0' yEnd='400.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall12' wallAtStart='wall14' wallAtEnd='wall13' xStart='150.0' yStart='600.0' xEnd='100.0' yEnd='700.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall13' wallAtStart='wall12' wallAtEnd='wall14' xStart='100.0' yStart='700.0' xEnd='200.0' yEnd='700.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall14' wallAtStart='wall13' wallAtEnd='wall12' xStart='200.0' yStart='700.0' xEnd='150.0' yEnd='600.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall15' wallAtEnd='wall16' xStart='400.0' yStart='150.0' xEnd='500.0' yEnd='100.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall16' wallAtStart='wall15' wallAtEnd='wall17' xStart='500.0' yStart='100.0' xEnd='600.0' yEnd='100.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall17' wallAtStart='wall16' xStart='600.0' yStart='100.0' xEnd='700.0' yEnd='50.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall18' wallAtEnd='wall19' xStart='400.0' yStart='400.0' xEnd='500.0' yEnd='300.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall19' wallAtStart='wall18' wallAtEnd='wall20' xStart='500.0' yStart='300.0' xEnd='600.0' yEnd='300.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall20' wallAtStart='wall19' xStart='600.0' yStart='300.0' xEnd='700.0' yEnd='200.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall21' wallAtEnd='wall22' xStart='400.0' yStart='600.0' xEnd='400.0' yEnd='500.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall22' wallAtStart='wall21' wallAtEnd='wall23' xStart='400.0' yStart='500.0' xEnd='600.0' yEnd='500.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall23' wallAtStart='wall22' xStart='600.0' yStart='500.0' xEnd='600.0' yEnd='400.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall24' wallAtEnd='wall25' xStart='600.0' yStart='800.0' xEnd='500.0' yEnd='700.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall25' wallAtStart='wall24' wallAtEnd='wall26' xStart='500.0' yStart='700.0' xEnd='600.0' yEnd='700.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall26' wallAtStart='wall25' xStart='600.0' yStart='700.0' xEnd='500.0' yEnd='600.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall27' wallAtStart='wall30' wallAtEnd='wall28' xStart='800.0' yStart='0.0' xEnd='1000.0' yEnd='0.0' height='250.0' thickness='10.0' arcExtent='1.0471976' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall28' wallAtStart='wall27' wallAtEnd='wall29' xStart='1000.0' yStart='0.0' xEnd='1000.0' yEnd='100.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall29' wallAtStart='wall28' wallAtEnd='wall30' xStart='1000.0' yStart='100.0' xEnd='800.0' yEnd='100.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall30' wallAtStart='wall29' wallAtEnd='wall27' xStart='800.0' yStart='100.0' xEnd='800.0' yEnd='0.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall31' wallAtEnd='wall32' xStart='800.0' yStart='400.0' xEnd='1000.0' yEnd='200.0' height='250.0' thickness='10.0' arcExtent='1.5707964' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall32' wallAtStart='wall31' wallAtEnd='wall33' xStart='1000.0' yStart='200.0' xEnd='1200.0' yEnd='400.0' height='250.0' thickness='10.0' arcExtent='1.5707964' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall33' wallAtStart='wall32' wallAtEnd='wall34' xStart='1200.0' yStart='400.0' xEnd='1000.0' yEnd='600.0' height='250.0' thickness='10.0' arcExtent='1.5707964' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall34' wallAtStart='wall33' xStart='1000.0' yStart='600.0' xEnd='800.0' yEnd='400.0' height='250.0' thickness='10.0' arcExtent='1.5707964' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall35' wallAtEnd='wall36' xStart='800.0' yStart='800.0' xEnd='900.0' yEnd='900.0' height='250.0' thickness='10.0' arcExtent='-3.1415927' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall36' wallAtStart='wall35' wallAtEnd='wall37' xStart='900.0' yStart='900.0' xEnd='1000.0' yEnd='800.0' height='250.0' thickness='10.0' arcExtent='-3.1415927' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall37' wallAtStart='wall36' wallAtEnd='wall38' xStart='1000.0' yStart='800.0' xEnd='900.0' yEnd='700.0' height='250.0' thickness='10.0' arcExtent='-3.1415927' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall38' wallAtStart='wall37' xStart='900.0' yStart='700.0' xEnd='800.0' yEnd='800.0' height='250.0' thickness='10.0' arcExtent='-3.1415927' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
</home>
"""
