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
            f.write("text(\"X\");") # Keep it short to keep the test fast-ish
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

    def utility_create_scad(self, scadCode, name):
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = temp_dir + os.pathsep + name + ".scad"
            f = open(filename,"w+")
            f.write(scadCode)
            f.close()
            return importCSG.open(filename)

    def test_import_difference(self):
        doc = self.utility_create_scad("difference() { cube(15, center=true); sphere(10); }", "difference")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 266.1323, 3)
        FreeCAD.closeDocument(doc.Name)

    def test_import_intersection(self):
        doc = self.utility_create_scad("intersection() { cube(15, center=true); sphere(10); }", "intersection")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 3108.8677, 3)
        FreeCAD.closeDocument(doc.Name)

    def test_import_union(self):
        doc = self.utility_create_scad("union() { cube(15, center=true); sphere(10); }", "union")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 4454.9224, 3)
        FreeCAD.closeDocument(doc.Name)

    def test_import_rotate_extrude(self):
        doc = self.utility_create_scad("rotate_extrude() translate([10, 0]) square(5);", "rotate_extrude_simple")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 1963.4954, 3)
        FreeCAD.closeDocument(doc.Name)

        doc = self.utility_create_scad("translate([0, 30, 0]) rotate_extrude($fn = 80) polygon( points=[[0,0],[8,4],[4,8],[4,12],[12,16],[0,20]] );", "rotate_extrude_no_hole")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 2412.7431, 3)
        FreeCAD.closeDocument(doc.Name)
       
    def test_import_linear_extrude(self):
        doc = self.utility_create_scad("linear_extrude(height = 20) square([20, 10], center = true);", "linear_extrude_simple")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 4000.000, 3)
        FreeCAD.closeDocument(doc.Name)

        doc = self.utility_create_scad("linear_extrude(height = 20, scale = 0.2) square([20, 10], center = true);", "linear_extrude_scale")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 1945.2745, 3)
        FreeCAD.closeDocument(doc.Name)

        doc = self.utility_create_scad("linear_extrude(height = 20, twist = 90) square([20, 10], center = true);", "linear_extrude_twist")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 3999.9961, 3)
        FreeCAD.closeDocument(doc.Name)

        doc = self.utility_create_scad("linear_extrude(height = 40, twist = 180, scale=0.25) square([20, 10], center = true);", "linear_extrude_twist")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 4144.9071, 3)
        FreeCAD.closeDocument(doc.Name)

    def test_import_rotate_extrude_file(self):
        # OpenSCAD doesn't seem to have this feature at this time (March 2021)
        pass

# There is a problem with the DXF code right now, it doesn't like this square.
#    def test_import_import_dxf(self):
#        testfile = join(self.test_dir, "Square.dxf").replace('\\','/')
#        doc = self.utility_create_scad("import(\"{}\");".format(testfile), "import_dxf");
#        object = doc.ActiveObject
#        self.assertTrue (object is not None)
#        FreeCAD.closeDocument(doc.Name)

    def test_import_import_stl(self):
        testfile = join(self.test_dir, "Cube.stl").replace('\\','/')
        doc = self.utility_create_scad("import(\"{}\");".format(testfile), "import_stl");
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        FreeCAD.closeDocument(doc.Name)

    def test_import_resize(self):
        doc = self.utility_create_scad("resize([2,2,2]) cube();", "resize_simple")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 8.000000, 6)
        FreeCAD.closeDocument(doc.Name)
        
        doc = self.utility_create_scad("resize([2,2,0]) cube();", "resize_with_zero")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 4.000000, 6)
        FreeCAD.closeDocument(doc.Name)
        
        doc = self.utility_create_scad("resize([2,0,0], auto=true) cube();", "resize_with_auto")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 8.000000, 6)
        FreeCAD.closeDocument(doc.Name)

        doc = self.utility_create_scad("resize([2,2,2]) cube([2,2,2]);", "resize_no_change")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 8.000000, 6)
        FreeCAD.closeDocument(doc.Name)

        doc = self.utility_create_scad("resize([2,2,2]) cube([4,8,12]);", "resize_non_uniform")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 8.000000, 6)
        FreeCAD.closeDocument(doc.Name)

    def test_import_surface(self):
        testfile = join(self.test_dir, "Surface.dat").replace('\\','/')
        doc = self.utility_create_scad(f"surface(file = \"{testfile}\", center = true, convexity = 5);", "surface_simple_dat")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 275.000000, 6)
        self.assertAlmostEqual (object.Shape.BoundBox.XMin, -4.5, 6)
        self.assertAlmostEqual (object.Shape.BoundBox.XMax, 4.5, 6)
        self.assertAlmostEqual (object.Shape.BoundBox.YMin, -4.5, 6)
        self.assertAlmostEqual (object.Shape.BoundBox.YMax, 4.5, 6)
        FreeCAD.closeDocument(doc.Name)
        
        testfile = join(self.test_dir, "Surface.dat").replace('\\','/')
        doc = self.utility_create_scad(f"surface(file = \"{testfile}\", convexity = 5);", "surface_uncentered_dat")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 275.000000, 6)
        self.assertAlmostEqual (object.Shape.BoundBox.XMin, 0, 6)
        self.assertAlmostEqual (object.Shape.BoundBox.XMax, 9, 6)
        self.assertAlmostEqual (object.Shape.BoundBox.YMin, 0, 6)
        self.assertAlmostEqual (object.Shape.BoundBox.YMax, 9, 6)
        FreeCAD.closeDocument(doc.Name)

        testfile = join(self.test_dir, "Surface2.dat").replace('\\','/')
        doc = self.utility_create_scad(f"surface(file = \"{testfile}\", center = true, convexity = 5);", "surface_rectangular_dat")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 24.5500000, 6)
        self.assertAlmostEqual (object.Shape.BoundBox.XMin, -2, 6)
        self.assertAlmostEqual (object.Shape.BoundBox.XMax, 2, 6)
        self.assertAlmostEqual (object.Shape.BoundBox.YMin, -1.5, 6)
        self.assertAlmostEqual (object.Shape.BoundBox.YMax, 1.5, 6)
        FreeCAD.closeDocument(doc.Name)

    def test_import_projection(self):
        pass

    def test_import_hull(self):
        pass

    def test_import_minkowski(self):
        pass

    def test_import_offset(self):
        pass
