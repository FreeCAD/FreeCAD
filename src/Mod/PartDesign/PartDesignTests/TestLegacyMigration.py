#***************************************************************************
#*   Copyright (c) 2025 Walter Steffè <walter.steffe@hierarchical-electromagnetics.com>  *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
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


import os, json, tempfile, unittest
import FreeCAD as App
import PartDesign

def _bb_tuple(bb):
    return [bb.XMin, bb.YMin, bb.ZMin, bb.XMax, bb.YMax, bb.ZMax]

def _near(a, b, tol=1e-6):
    return abs(a-b) <= tol

def _bb_equal(bb_now, bb_expected, tol=1e-6):
    now = _bb_tuple(bb_now)
    return all(_near(n, e, tol) for n, e in zip(now, bb_expected))

class TestLegacyMigration(unittest.TestCase):
    def test_body_placement_migration(self):
        tmpdir = tempfile.mkdtemp(prefix="fc_pd_migrate_")
        fcstd = os.path.join(tmpdir, "legacy.FCStd")
        snapf = os.path.join(tmpdir, "before.json")

        # --- create legacy doc (non-identity Body.Placement + inter-body binder)
        doc = App.newDocument("LegacyPDTest")
        part = doc.addObject("App::Part", "MainPart")
        part.Placement = App.Placement(App.Vector(10, 20, 0),
                                       App.Rotation(App.Vector(0,0,1), 12))

        B1 = doc.addObject("PartDesign::Body", "B1"); part.addObject(B1)
        B2 = doc.addObject("PartDesign::Body", "B2"); part.addObject(B2)

        T1 = App.Placement(App.Vector(80, 0, 0), App.Rotation(App.Vector(0,0,1), 45))
        T2 = App.Placement(App.Vector(-40, 25, 0), App.Rotation(App.Vector(0,0,1), -30))
        B1.Placement = T1
        B2.Placement = T2

        box1 = doc.addObject("PartDesign::AdditiveBox", "Box1"); B1.addObject(box1)
        box1.Length = 20; box1.Width = 12; box1.Height = 8

        cyl2 = doc.addObject("PartDesign::AdditiveCylinder", "Cyl2"); B2.addObject(cyl2)
        cyl2.Radius = 6; cyl2.Height = 15
        cyl2.Placement = App.Placement(App.Vector(10,0,0), App.Rotation())

        B1.Tip = box1; B2.Tip = cyl2

        binder = doc.addObject("PartDesign::SubShapeBinder", "B2_ref_in_B1"); B1.addObject(binder)
        binder.Support = [(B2.Tip, [])]
        binder.Relative = True

        doc.recompute()

        snap = {
            "B1": _bb_tuple(B1.Shape.BoundBox),
            "B2": _bb_tuple(B2.Shape.BoundBox),
        }
        with open(snapf, "w") as f: json.dump(snap, f)

        doc.saveAs(fcstd)
        App.closeDocument(doc.Name)

        # --- reopen to trigger migration hook
        doc2 = App.openDocument(fcstd)
        PartDesign.resetBodiesPlacements(doc2)
        doc2.recompute()

        with open(snapf, "r") as f: expect = json.load(f)

        bodies = [o for o in doc2.Objects if getattr(o, "TypeId", "") == "PartDesign::Body"]
        self.assertTrue(bodies, "No PartDesign::Body found after reopen")

        for b in bodies:
            # Body placements must be identity
            self.assertTrue(b.Placement.Base.Length == 0.0 and b.Placement.Rotation.isIdentity(),
                            f"{b.Name} Body.Placement not identity")

            # World bbox preserved
            self.assertIn(b.Name, expect)
            self.assertTrue(_bb_equal(b.Shape.BoundBox, expect[b.Name]),
                            f"{b.Name} bounding box changed by migration")


        App.closeDocument(doc2.Name)

if __name__ == "__main__":
    unittest.main()

