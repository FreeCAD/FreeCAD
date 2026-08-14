# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import Part
import Path.Geom as PathGeom
import Path.Op.Flute as PathFlute

from CAMTests.PathTestUtils import PathTestBase


class TestPathFlute(PathTestBase):
    """Direct tests of Flute's 2D ramp-profile synthesis (_apply_2d_profile).

    No Job/Document is needed for these -- they exercise the pure geometry
    helpers directly, the same way TestPathHelixGenerator tests the helix
    generator without a full operation.
    """

    def test00_ramp_full_linear_single_edge(self):
        """A single straight edge, Ramp Full / Linear, ramps start_z -> floor_z
        end to end and collapses back to just its two endpoints (still a
        straight line in 3D once the Z ramp is applied)."""
        edge = Part.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0))
        tol = PathFlute._make_tolerances(None)

        wire = PathFlute._apply_2d_profile(
            [edge],
            start_z=0.0,
            floor_z=-5.0,
            flute_type="Ramp Full",
            ramp_type="Linear",
            ramp_frac=1.0,
            tol=tol,
        )

        self.assertIsNotNone(wire)
        pts = PathGeom.edgesToPoints(wire.Edges, tol.arc_chord)
        self.assertEqual(len(pts), 2)
        self.assertCoincide(pts[0], FreeCAD.Vector(0, 0, 0))
        self.assertCoincide(pts[-1], FreeCAD.Vector(10, 0, -5))

    def test01_flip_reverses_which_end_is_deep(self):
        """flip=True reverses the wire's travel direction before the ramp is
        applied, so the deep end moves to the opposite physical point."""
        edge = Part.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0))
        tol = PathFlute._make_tolerances(None)

        wire = PathFlute._apply_2d_profile(
            [edge],
            start_z=0.0,
            floor_z=-5.0,
            flute_type="Ramp Full",
            ramp_type="Linear",
            ramp_frac=1.0,
            tol=tol,
            flip=True,
        )

        self.assertIsNotNone(wire)
        pts = PathGeom.edgesToPoints(wire.Edges, tol.arc_chord)
        self.assertCoincide(pts[0], FreeCAD.Vector(10, 0, 0))
        self.assertCoincide(pts[-1], FreeCAD.Vector(0, 0, -5))

    def test02_chains_edges_regardless_of_input_order_and_orientation(self):
        """Two connected edges, given out of order and with mismatched
        orientation, still chain into one continuous ramp -- anchored on the
        first edge's OWN drawn direction (edges[0] is never itself reversed,
        only the edges that follow it are), matching CombineTangentSegments'
        "respect drawn direction" contract."""
        p0 = FreeCAD.Vector(0, 0, 0)
        p1 = FreeCAD.Vector(10, 0, 0)
        p2 = FreeCAD.Vector(10, 10, 0)

        # edge2 is listed FIRST and drawn p2->p1 ("backwards" relative to the
        # p0->p1->p2 path); edge1 is listed second, drawn p0->p1 (its natural
        # direction happens to match the overall chain here).
        edge2 = Part.makeLine(p2, p1)
        edge1 = Part.makeLine(p0, p1)
        tol = PathFlute._make_tolerances(None)

        wire = PathFlute._apply_2d_profile(
            [edge2, edge1],
            start_z=0.0,
            floor_z=-4.0,
            flute_type="Ramp Full",
            ramp_type="Linear",
            ramp_frac=1.0,
            tol=tol,
        )

        self.assertIsNotNone(wire)
        pts = PathGeom.edgesToPoints(wire.Edges, tol.arc_chord)
        # Chain follows edge2's own drawn direction (p2 -> p1), then
        # continues on to p0 via edge1 (reversed to connect).
        self.assertCoincide(pts[0], FreeCAD.Vector(10, 10, 0))
        self.assertCoincide(pts[-1], FreeCAD.Vector(0, 0, -4))
