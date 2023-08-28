#***************************************************************************
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
import math

from os.path import join

__title__ = "ImportCSG OpenSCAD App unit tests"
__author__ = "Chris Hennes"
__url__ = "https://www.freecad.org"


class TestImportCSG(unittest.TestCase):

    MODULE = 'test_importCSG' # file name without extension
    temp_dir = tempfile.TemporaryDirectory()


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

    def utility_create_scad(self, scadCode, name):
        filename = self.temp_dir.name + os.path.sep + name + ".scad"
        print (f"Creating {filename}")
        f = open(filename,"w+")
        f.write(scadCode)
        f.close()
        return importCSG.open(filename)

    def test_import_sphere(self):
        doc = self.utility_create_scad("sphere(10.0);","sphere")
        sphere = doc.getObject("sphere")
        self.assertTrue (sphere is not None)
        self.assertTrue (sphere.Radius == 10.0)
        FreeCAD.closeDocument(doc.Name)

    def test_import_cylinder(self):
        doc = self.utility_create_scad("cylinder(50.0,d=10.0);","cylinder")
        cylinder = doc.getObject("cylinder")
        self.assertTrue (cylinder is not None)
        self.assertTrue (cylinder.Radius == 5.0)
        self.assertTrue (cylinder.Height == 50.0)
        FreeCAD.closeDocument(doc.Name)

    def test_import_cube(self):
        doc = self.utility_create_scad("cube([1.0,2.0,3.0]);","cube")
        cube = doc.getObject("cube")
        self.assertTrue (cube is not None)
        self.assertTrue (cube.Length == 1.0)
        self.assertTrue (cube.Width == 2.0)
        self.assertTrue (cube.Height == 3.0)
        FreeCAD.closeDocument(doc.Name)

    def test_import_circle(self):
        doc = self.utility_create_scad("circle(10.0);","circle")
        circle = doc.getObject("circle")
        self.assertTrue (circle is not None)
        self.assertTrue (circle.Radius == 10.0)
        FreeCAD.closeDocument(doc.Name)

    def test_import_square(self):
        doc = self.utility_create_scad("square([1.0,2.0]);","square")
        square = doc.getObject("square")
        self.assertTrue (square is not None)
        self.assertTrue (square.Length == 1.0)
        self.assertTrue (square.Width == 2.0)
        FreeCAD.closeDocument(doc.Name)

    def test_import_text(self):
        # This uses the DXF importer that may pop-up modal dialogs
        # if not all 3rd party libraries are installed
        if FreeCAD.GuiUp:
            return
        try:
            doc = self.utility_create_scad("text(\"X\");","text") # Keep it short to keep the test fast-ish
            text = doc.getObject("text")
            self.assertTrue (text is not None)
            FreeCAD.closeDocument(doc.Name)
        except Exception:
            return # We may not have the DXF importer available

        # Try a number with a set script:
        doc = self.utility_create_scad("text(\"2\",script=\"latin\");","two_text")
        text = doc.getObject("text")
        self.assertTrue (text is not None)
        FreeCAD.closeDocument(doc.Name)

        # Leave off the script (which is supposed to default to "latin")
        doc = self.utility_create_scad("text(\"1\");","one_text")
        text = doc.getObject("text")
        self.assertTrue (text is not None)
        FreeCAD.closeDocument(doc.Name)

    def test_import_polygon_nopath(self):
        doc = self.utility_create_scad("polygon(points=[[0,0],[100,0],[130,50],[30,50]]);","polygon_nopath")
        polygon = doc.getObject("polygon")
        self.assertTrue (polygon is not None)
        self.assertAlmostEqual (polygon.Shape.Area, 5000.0)
        FreeCAD.closeDocument(doc.Name)

    def test_import_polygon_path(self):
        doc = self.utility_create_scad("polygon([[0,0],[100,0],[130,50],[30,50]], paths=[[0,1,2,3]]);","polygon_path")
        wire = doc.ActiveObject # With paths, the polygon gets created as a wire...
        self.assertTrue (wire is not None)
        self.assertAlmostEqual (wire.Shape.Area, 5000.0)
        FreeCAD.closeDocument(doc.Name)

    def test_import_polyhedron(self):
        doc = self.utility_create_scad(
"""
polyhedron(
  points=[ [10,10,0],[10,-10,0],[-10,-10,0],[-10,10,0], // the four points at base
           [0,0,10]  ],                                 // the apex point
  faces=[ [0,1,4],[1,2,4],[2,3,4],[3,0,4],              // each triangle side
              [1,0,3],[2,1,3] ]                         // two triangles for square base
 );
""","polyhedron"
                )
        polyhedron = doc.ActiveObject # With paths, the polygon gets created as a wire...
        self.assertTrue (polyhedron is not None)
        self.assertAlmostEqual (polyhedron.Shape.Volume, 1333.3333, 4)
        FreeCAD.closeDocument(doc.Name)

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

        doc = self.utility_create_scad("translate([0, 30, 0]) rotate_extrude() polygon( points=[[0,0],[8,4],[4,8],[4,12],[12,16],[0,20]] );", "rotate_extrude_no_hole")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 2412.7431, 3)
        FreeCAD.closeDocument(doc.Name)

        # Bug #4353 - https://tracker.freecad.org/view.php?id=4353
        doc = self.utility_create_scad("rotate_extrude($fn=4, angle=180) polygon([[0,0],[3,3],[0,3]]);", "rotate_extrude_low_fn")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 9.0, 5)
        FreeCAD.closeDocument(doc.Name)

        doc = self.utility_create_scad("rotate_extrude($fn=4, angle=-180) polygon([[0,0],[3,3],[0,3]]);", "rotate_extrude_low_fn_negative_angle")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 9.0, 5)
        FreeCAD.closeDocument(doc.Name)

        doc = self.utility_create_scad("rotate_extrude(angle=180) polygon([[0,0],[3,3],[0,3]]);", "rotate_extrude_angle")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 4.5*math.pi, 5)
        FreeCAD.closeDocument(doc.Name)

        doc = self.utility_create_scad("rotate_extrude(angle=-180) polygon([[0,0],[3,3],[0,3]]);", "rotate_extrude_negative_angle")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 4.5*math.pi, 5)
        FreeCAD.closeDocument(doc.Name)

    def test_import_linear_extrude(self):
        doc = self.utility_create_scad("linear_extrude(height = 20) square([20, 10], center = true);", "linear_extrude_simple")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 4000.000, 3)
        FreeCAD.closeDocument(doc.Name)

        doc = self.utility_create_scad("linear_extrude(height = 20, twist = 90) square([20, 10], center = true);", "linear_extrude_twist")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, 4000.000, 2)
        FreeCAD.closeDocument(doc.Name)

        doc = self.utility_create_scad("linear_extrude(height = 20, scale = 0.2) square([20, 10], center = true);", "linear_extrude_scale")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        h = 20
        a1 = 20*10
        a2 = 20*0.2 * 10*0.2
        expected_volume = (h/3) * (a1+a2+math.sqrt(a1*a2))
        self.assertAlmostEqual (object.Shape.Volume, expected_volume, 3)
        FreeCAD.closeDocument(doc.Name)

        doc = self.utility_create_scad("linear_extrude(height = 20, twist = 180, scale=0.2) square([20, 10], center = true);", "linear_extrude_twist_scale")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Volume, expected_volume, 2)
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
        preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD")
        transfer_mechanism = preferences.GetInt('transfermechanism',0)
        if transfer_mechanism == 2:
            print ("Cannot test STL import, communication with OpenSCAD is via pipes")
            print ("If either OpenSCAD or FreeCAD are installed as sandboxed packages,")
            print ("use of import is not possible.")
            return
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

        # Make sure to test something that isn't just a box (where the bounding box is trivial)
        doc = self.utility_create_scad("""
resize(newsize = [0,0,10], auto = [0,0,0]) {
    sphere($fn = 96, $fa = 12, $fs = 2, r = 8.5);
}""", "resize_non_uniform_sphere")
        object = doc.ActiveObject
        self.assertTrue (object is not None)
        object.Shape.tessellate(0.025) # To ensure the bounding box calculation is correct
        self.assertAlmostEqual (object.Shape.BoundBox.XLength, 2*8.5, 1)
        self.assertAlmostEqual (object.Shape.BoundBox.YLength, 2*8.5, 1)
        self.assertAlmostEqual (object.Shape.BoundBox.ZLength, 10.0, 1)
        FreeCAD.closeDocument(doc.Name)

    def test_import_surface(self):
        # Workaround for absolute vs. relative path issue
        # Inside the OpenSCAD file an absolute path name to Surface.dat is used
        # but by using the OpenSCAD executable to create a CSG file it's converted
        # into a path name relative to the output filename.
        # In order to open the CAG file correctly the cwd must be temporarily changed
        with tempfile.TemporaryDirectory() as temp_dir:
            cwd = os.getcwd()
            os.chdir(temp_dir)

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
            os.chdir(cwd)

    def test_import_projection(self):
        base_shape = "linear_extrude(height=5,center=true,twist=90,scale=0.5){square([1,1],center=true);}"
        hole = "cube([0.25,0.25,6],center=true);"
        cut_shape = f"difference() {{ {base_shape} {hole} }}"

        doc = self.utility_create_scad(f"projection(cut=true) {base_shape}", "projection_slice_square")
        object = doc.getObject("projection_cut")
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Area, 0.75*0.75, 3)
        FreeCAD.closeDocument(doc.Name)

        doc = self.utility_create_scad(f"projection(cut=true) {cut_shape}", "projection_slice_square_with_hole")
        object = doc.getObject("projection_cut")
        self.assertTrue (object is not None)
        self.assertAlmostEqual (object.Shape.Area, 0.75*0.75 - 0.25*0.25, 3)
        FreeCAD.closeDocument(doc.Name)

        # Unimplemented functionality:

        # With cut=false, the twisted unit square projects to a circle of radius sqrt(0.5)
        #doc = self.utility_create_scad(f"projection(cut=false) {base_shape}", "projection_circle")
        #object = doc.getObject("projection")
        #self.assertTrue (object is not None)
        #self.assertAlmostEqual (object.Shape.Area, 2*math.pi*math.sqrt(2), 3)
        #FreeCAD.closeDocument(doc.Name)

        #doc = self.utility_create_scad(f"projection(cut=false) {cut_shape}", "projection_circle_with_hole")
        #object = doc.getObject("projection")
        #self.assertTrue (object is not None)
        #self.assertAlmostEqual (object.Shape.Area, 2*math.pi*math.sqrt(0.5) - 0.125, 3)
        #FreeCAD.closeDocument(doc.Name)

    def test_import_hull(self):
        pass

    def test_import_minkowski(self):
        pass

    def test_import_offset(self):
        pass

    def test_empty_union(self):
        content = """union() {
 color(c = [0.30, 0.50, 0.80, 0.50]) {
  union() {
   union() {
    union() {
     translate(v = [23.0, -9.50, 13.60]) {
      union() {
       difference() {
        difference() {
         difference() {
          difference() {
           difference() {
            difference() {
             difference() {
              union() {
               union() {
                union() {
                 union() {
                  union() {
                   union() {
                    union() {
                     union();
                     translate(v = [2.50, 2.50, 9.50]) {
                      cylinder(h = 19.0, r = 2.50, center = true, $fn = 100);
                     }
                    }
                    translate(v = [11.50, 2.50, 9.50]) {
                     cylinder(h = 19.0, r = 2.50, center = true, $fn = 100);
                    }
                   }
                   translate(v = [11.50, 6.30, 9.50]) {
                    cylinder(h = 19.0, r = 2.50, center = true, $fn = 100);
                   }
                  }
                  translate(v = [2.50, 6.30, 9.50]) {
                   cylinder(h = 19.0, r = 2.50, center = true, $fn = 100);
                  }
                 }
                 translate(v = [2.50, 0.0, 0.0]) {
                  cube(size = [9.0, 8.80, 19.0]);
                 }
                }
                translate(v = [0.0, 2.50, 0.0]) {
                 cube(size = [14.0, 3.80, 19.0]);
                }
               }
              }
              translate(v = [-1.0, 8.40, 3.50]) {
               cube(size = [30.0, 10.0, 12.0]);
              }
             }
             translate(v = [4.0, 4.0, -0.10]) {
              cylinder($fn = 30, h = 20.0, r = 1.750, r1 = 1.80);
             }
            }
            translate(v = [9.0, 4.40, -0.10]) {
             cylinder($fn = 30, h = 20.0, r = 2.150, r1 = 2.20);
            }
           }
           translate(v = [12.30, 2.10, -0.10]) {
            cube(size = [5.0, 2.20, 20.0]);
           }
          }
          translate(v = [1.90, 7.60, -0.10]) {
           cube(size = [2.20, 5.0, 20.0]);
          }
         }
         translate(v = [3.60, 7.20, 1.80]) {
          cube(size = [30.0, 10.0, 2.0]);
         }
        }
        translate(v = [0, 0, 13.40]) {
         translate(v = [3.60, 7.20, 1.80]) {
          cube(size = [30.0, 10.0, 2.0]);
         }
        }
       }
      }
     }
    }
   }
  }
 }
}"""
        doc = self.utility_create_scad(content, "empty_union")
        self.assertEqual (len(doc.RootObjects), 1)
        FreeCAD.closeDocument(doc.Name)

    def test_complex_fuse_no_placement(self):
        # Issue #7878 - https://github.com/FreeCAD/FreeCAD/issues/7878

        csg_data = """
group() {
    multmatrix([[1, 0, 0, 0], [0, 1, 0, -127], [0, 0, 1, -6], [0, 0, 0, 1]]) {
        union() {
            group() {
                difference() {
                    cube(size = [4, 106.538, 12], center = false);
                    group() {
                            polyhedron(points = [[0, 0, 0], [1, 0, 0], [0, 1, 0], [0, 0, 1], [1, 0, 1], [0, 1, 1]], faces = [[0, 1, 2], [5, 4, 3], [3, 1, 0], [1, 3, 4], [0, 2, 3], [5, 3, 2], [4, 2, 1], [2, 4, 5]], convexity = 1);
                    }
                }
            }
            polyhedron(points = [[0, 0, 0], [1, 0, 0], [0, 1, 0], [0, 0, 1], [1, 0, 1], [0, 1, 1]], faces = [[0, 1, 2], [5, 4, 3], [3, 1, 0], [1, 3, 4], [0, 2, 3], [5, 3, 2], [4, 2, 1], [2, 4, 5]], convexity = 1);
            polyhedron(points = [[0, 0, 0], [1, 0, 0], [0, 1, 0], [0, 0, 1], [1, 0, 1], [0, 1, 1]], faces = [[0, 1, 2], [5, 4, 3], [3, 1, 0], [1, 3, 4], [0, 2, 3], [5, 3, 2], [4, 2, 1], [2, 4, 5]], convexity = 1);
        }
    }
    multmatrix([[1, 0, 0, 6.4], [0, 1, 0, -125], [0, 0, 1, -40], [0, 0, 0, 1]]) {
        difference() {
            cylinder($fn = 0, $fa = 12, $fs = 2, h = 80, r1 = 8, r2 = 8, center = false);
            multmatrix([[1, 0, 0, -14.4], [0, 1, 0, -8], [0, 0, 1, -5], [0, 0, 0, 1]]) {
                cube(size = [8, 16, 90], center = false);
            }
        }
    }
}
"""
        doc = self.utility_create_scad(csg_data, "complex-fuse")
        self.assertEqual (doc.RootObjects[0].Placement, FreeCAD.Placement())
        FreeCAD.closeDocument(doc.Name)
