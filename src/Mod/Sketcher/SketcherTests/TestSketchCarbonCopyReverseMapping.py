# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

import FreeCAD
import math, os, unittest


class TestSketchCarbonCopyReverseMapping(unittest.TestCase):
    def setUp(self):
        location = os.path.dirname(os.path.realpath(__file__))
        self.Doc = FreeCAD.openDocument(
            os.path.join(location, "TestSketchCarbonCopyReverseMapping.FCStd"), True
        )

    def test_CarbonCopyReverseMapping(self):
        r = 10.0
        rad30 = math.radians(30)
        cos30 = math.cos(rad30)
        sin30 = math.sin(rad30)

        expected_geometries_pattern_001 = {
            "Sketch001": {
                0: {"start": FreeCAD.Vector(10, 10, 0), "end": FreeCAD.Vector(20, 10, 0)},
                1: {
                    "start": FreeCAD.Vector(10, 10, 0),
                    "end": FreeCAD.Vector(10 + cos30 * r, 10 + sin30 * r, 0),
                },
                2: {
                    "start": FreeCAD.Vector(10, 10, 0),
                    "end": FreeCAD.Vector(10 + sin30 * r, 10 + cos30 * r, 0),
                },
                3: {
                    "start": FreeCAD.Vector(10, 10, 0),
                    "end": FreeCAD.Vector(10 - sin30 * r, 10 + cos30 * r, 0),
                },
                4: {"location": FreeCAD.Vector(-10, 10, 0)},
                5: {"location": FreeCAD.Vector(10, -10, 0)},
                6: {
                    "start": FreeCAD.Vector(5, 15, 0),
                    "end": FreeCAD.Vector(5 + sin30 * r, 15 + cos30 * r, 0),
                },
                7: {
                    "start": FreeCAD.Vector(15, 15, 0),
                    "end": FreeCAD.Vector(15 - sin30 * r, 15 + cos30 * r, 0),
                },
            },
            "Sketch002": {
                0: {"start": FreeCAD.Vector(-10, 10, 0), "end": FreeCAD.Vector(-20, 10, 0)},
                1: {
                    "start": FreeCAD.Vector(-10, 10, 0),
                    "end": FreeCAD.Vector(-10 - cos30 * r, 10 + sin30 * r, 0),
                },
                2: {
                    "start": FreeCAD.Vector(-10, 10, 0),
                    "end": FreeCAD.Vector(-10 - sin30 * r, 10 + cos30 * r, 0),
                },
                3: {
                    "start": FreeCAD.Vector(-10, 10, 0),
                    "end": FreeCAD.Vector(-10 + sin30 * r, 10 + cos30 * r, 0),
                },
                4: {"location": FreeCAD.Vector(10, 10, 0)},
                5: {"location": FreeCAD.Vector(-10, -10, 0)},
                6: {
                    "start": FreeCAD.Vector(-5, 15, 0),
                    "end": FreeCAD.Vector(-5 - sin30 * r, 15 + cos30 * r, 0),
                },
                7: {
                    "start": FreeCAD.Vector(-15, 15, 0),
                    "end": FreeCAD.Vector(-15 + sin30 * r, 15 + cos30 * r, 0),
                },
            },
            "Sketch003": {
                0: {"start": FreeCAD.Vector(10, -10, 0), "end": FreeCAD.Vector(20, -10, 0)},
                1: {
                    "start": FreeCAD.Vector(10, -10, 0),
                    "end": FreeCAD.Vector(10 + cos30 * r, -10 - sin30 * r, 0),
                },
                2: {
                    "start": FreeCAD.Vector(10, -10, 0),
                    "end": FreeCAD.Vector(10 + sin30 * r, -10 - cos30 * r, 0),
                },
                3: {
                    "start": FreeCAD.Vector(10, -10, 0),
                    "end": FreeCAD.Vector(10 - sin30 * r, -10 - cos30 * r, 0),
                },
                4: {"location": FreeCAD.Vector(-10, -10, 0)},
                5: {"location": FreeCAD.Vector(10, 10, 0)},
                6: {
                    "start": FreeCAD.Vector(5, -15, 0),
                    "end": FreeCAD.Vector(5 + sin30 * r, -15 - cos30 * r, 0),
                },
                7: {
                    "start": FreeCAD.Vector(15, -15, 0),
                    "end": FreeCAD.Vector(15 - sin30 * r, -15 - cos30 * r, 0),
                },
            },
            "Sketch004": {
                0: {"start": FreeCAD.Vector(-10, -10, 0), "end": FreeCAD.Vector(-20, -10, 0)},
                1: {
                    "start": FreeCAD.Vector(-10, -10, 0),
                    "end": FreeCAD.Vector(-10 - cos30 * r, -10 - sin30 * r, 0),
                },
                2: {
                    "start": FreeCAD.Vector(-10, -10, 0),
                    "end": FreeCAD.Vector(-10 - sin30 * r, -10 - cos30 * r, 0),
                },
                3: {
                    "start": FreeCAD.Vector(-10, -10, 0),
                    "end": FreeCAD.Vector(-10 + sin30 * r, -10 - cos30 * r, 0),
                },
                4: {"location": FreeCAD.Vector(10, -10, 0)},
                5: {"location": FreeCAD.Vector(-10, 10, 0)},
                6: {
                    "start": FreeCAD.Vector(-5, -15, 0),
                    "end": FreeCAD.Vector(-5 - sin30 * r, -15 - cos30 * r, 0),
                },
                7: {
                    "start": FreeCAD.Vector(-15, -15, 0),
                    "end": FreeCAD.Vector(-15 + sin30 * r, -15 - cos30 * r, 0),
                },
            },
            "Sketch005": {
                0: {"start": FreeCAD.Vector(10, 10, 0), "end": FreeCAD.Vector(20, 10, 0)},
                1: {
                    "start": FreeCAD.Vector(10, 10, 0),
                    "end": FreeCAD.Vector(10 + cos30 * r, 10 + sin30 * r, 0),
                },
                2: {
                    "start": FreeCAD.Vector(10, 10, 0),
                    "end": FreeCAD.Vector(10 + sin30 * r, 10 + cos30 * r, 0),
                },
                3: {
                    "start": FreeCAD.Vector(10, 10, 0),
                    "end": FreeCAD.Vector(10 - sin30 * r, 10 + cos30 * r, 0),
                },
                4: {"location": FreeCAD.Vector(-10, 10, 0)},
                5: {"location": FreeCAD.Vector(10, -10, 0)},
                6: {
                    "start": FreeCAD.Vector(5, 15, 0),
                    "end": FreeCAD.Vector(5 + sin30 * r, 15 + cos30 * r, 0),
                },
                7: {
                    "start": FreeCAD.Vector(15, 15, 0),
                    "end": FreeCAD.Vector(15 - sin30 * r, 15 + cos30 * r, 0),
                },
            },
        }

        expected_geometries_pattern_006 = {
            "Sketch007": {
                0: {"start": FreeCAD.Vector(-35, -15, 0), "end": FreeCAD.Vector(-35, -5, 0)},
                1: {"start": FreeCAD.Vector(-35, -5, 0), "end": FreeCAD.Vector(-20, -5, 0)},
                2: {"start": FreeCAD.Vector(-20, -5, 0), "end": FreeCAD.Vector(-20, -15, 0)},
                3: {"start": FreeCAD.Vector(-20, -15, 0), "end": FreeCAD.Vector(-35, -15, 0)},
            }
        }

        def execute_carbon_copy(src, dist):
            obj = FreeCAD.ActiveDocument.getObject(dist)
            obj.setAllowUnaligned(False)
            obj.carbonCopy(src, False)
            FreeCAD.ActiveDocument.recompute()

        def check_placement(expected_dict):
            for sketch, correct_points in expected_dict.items():
                obj = FreeCAD.ActiveDocument.getObject(sketch)
                for i, pts in correct_points.items():
                    geom = obj.Geometry[i]
                    if "start" in pts:
                        start = geom.StartPoint
                        end = geom.EndPoint
                        self.assertAlmostEqual((start - pts["start"]).Length, 0)
                        self.assertAlmostEqual((end - pts["end"]).Length, 0)
                    elif "location" in pts:
                        loc = geom.Location
                        self.assertAlmostEqual((loc - pts["location"]).Length, 0)

        execute_carbon_copy("Sketch001", "Sketch002")
        execute_carbon_copy("Sketch001", "Sketch003")
        execute_carbon_copy("Sketch001", "Sketch004")
        execute_carbon_copy("Sketch001", "Sketch005")
        execute_carbon_copy("Sketch006", "Sketch007")

        check_placement(expected_geometries_pattern_001)
        check_placement(expected_geometries_pattern_006)

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)
