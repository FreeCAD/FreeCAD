#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENSE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import unittest
import FreeCAD
import OpenSCAD
import importCSG
import tempfile
import os

from os.path import join

__title__ = "ImportCSG OpenSCAD App unit tests"
__author__ = "Chris Hennes"
__url__ = "https://www.freecadweb.org"


class TestImportCSG(unittest.TestCase):

    MODULE = 'test_importCSG' # file name without extension


    def setUp(self):
        self.test_dir = join(FreeCAD.getHomePath(), "Mod", "OpenSCAD", "OpenSCADTest", "data")
        pass

    def test_open_scad(self):
        testfile = join(self.test_dir, "CSG.scad")
        doc = importCSG.open(testfile)

        # Doc should now contain three solids: a union, an intersection, and a difference
        union = doc.getObject("union")
        intersection = doc.getObject("intersection")
        difference = doc.getObject("difference")

        self.assertTrue (union is not None)
        self.assertTrue (intersection is not None)
        self.assertTrue (difference is not None)

        FreeCAD.closeDocument("CSG")

    def test_open_csg(self):
        testfile = join(self.test_dir, "CSG.csg")
        doc = importCSG.open(testfile)

        # Doc should now contain three solids: a union, an intersection, and a difference
        union = doc.getObject("union")
        intersection = doc.getObject("intersection")
        difference = doc.getObject("difference")

        self.assertTrue (union is not None)
        self.assertTrue (intersection is not None)
        self.assertTrue (difference is not None)

        FreeCAD.closeDocument("CSG")

    def test_import_sphere(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = temp_dir + os.pathsep + "sphere.scad"
            f = open(filename,"w+")
            f.write("sphere(10.0);")
            f.close()
            doc = importCSG.open(filename)
            sphere = doc.getObject("sphere")
            self.assertTrue (sphere is not None)
            self.assertTrue (sphere.Radius == 10.0)
            FreeCAD.closeDocument(doc.Name)

    def test_import_cylinder(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = temp_dir + os.pathsep + "cylinder.scad"
            f = open(filename,"w+")
            f.write("cylinder(50.0,d=10.0);")
            f.close()
            doc = importCSG.open(filename)
            cylinder = doc.getObject("cylinder")
            self.assertTrue (cylinder is not None)
            self.assertTrue (cylinder.Radius == 5.0)
            self.assertTrue (cylinder.Height == 50.0)
            FreeCAD.closeDocument(doc.Name)

    def test_import_cube(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = temp_dir + os.pathsep + "cube.scad"
            f = open(filename,"w+")
            f.write("cube([1.0,2.0,3.0]);")
            f.close()
            doc = importCSG.open(filename)
            cube = doc.getObject("cube")
            self.assertTrue (cube is not None)
            self.assertTrue (cube.Length == 1.0)
            self.assertTrue (cube.Width == 2.0)
            self.assertTrue (cube.Height == 3.0)
            FreeCAD.closeDocument(doc.Name)

    def test_import_circle(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = temp_dir + os.pathsep + "circle.scad"
            f = open(filename,"w+")
            f.write("circle(10.0);")
            f.close()
            doc = importCSG.open(filename)
            circle = doc.getObject("circle")
            self.assertTrue (circle is not None)
            self.assertTrue (circle.Radius == 10.0)
            FreeCAD.closeDocument(doc.Name)

    def test_import_square(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = temp_dir + os.pathsep + "square.scad"
            f = open(filename,"w+")
            f.write("square([1.0,2.0]);")
            f.close()
            doc = importCSG.open(filename)
            square = doc.getObject("square")
            self.assertTrue (square is not None)
            self.assertTrue (square.Length == 1.0)
            self.assertTrue (square.Width == 2.0)
            FreeCAD.closeDocument(doc.Name)

    def test_import_text(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = temp_dir + os.pathsep + "text.scad"
            f = open(filename,"w+")
            f.write("text(\"FreeCAD\");")
            f.close()
            try:
                doc = importCSG.open(filename)
                text = doc.getObject("text")
                self.assertTrue (text is not None)
                FreeCAD.closeDocument(doc.Name)
            except Exception:
                pass # We may not have the DXF importer available

    def test_import_polygon_nopath(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = temp_dir + os.pathsep + "polygon_nopath.scad"
            f = open(filename,"w+")
            f.write("polygon(points=[[0,0],[100,0],[130,50],[30,50]]);")
            f.close()
            doc = importCSG.open(filename)
            polygon = doc.getObject("polygon")
            self.assertTrue (polygon is not None)
            self.assertAlmostEqual (polygon.Shape.Area, 5000.0)
            FreeCAD.closeDocument(doc.Name)

    def test_import_polygon_path(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = temp_dir + os.pathsep + "polygon_path.scad"
            f = open(filename,"w+")
            f.write("polygon([[0,0],[100,0],[130,50],[30,50]], paths=[[0,1,2,3]]);")
            f.close()
            doc = importCSG.open(filename)
            wire = doc.ActiveObject # With paths, the polygon gets created as a wire...
            self.assertTrue (wire is not None)
            self.assertAlmostEqual (wire.Shape.Area, 5000.0)
            FreeCAD.closeDocument(doc.Name)
    
    def test_import_polyhedron(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = temp_dir + os.pathsep + "polyhedron.scad"
            f = open(filename,"w+")
            f.write(
"""
polyhedron(
  points=[ [10,10,0],[10,-10,0],[-10,-10,0],[-10,10,0], // the four points at base
           [0,0,10]  ],                                 // the apex point 
  faces=[ [0,1,4],[1,2,4],[2,3,4],[3,0,4],              // each triangle side
              [1,0,3],[2,1,3] ]                         // two triangles for square base
 );
"""
                )
            f.close()
            doc = importCSG.open(filename)
            polyhedron = doc.ActiveObject # With paths, the polygon gets created as a wire...
            self.assertTrue (polyhedron is not None)
            self.assertAlmostEqual (polyhedron.Shape.Volume, 1333.3333, 4)
            FreeCAD.closeDocument(doc.Name)


"""
Actions to test:
-----------------
difference_action
intersection_action
union_action
rotate_extrude_action
linear_extrude_with_twist
rotate_extrude_file
import_file1
resize_action
surface_action
projection_action
hull_action
minkowski_action
offset_action
"""