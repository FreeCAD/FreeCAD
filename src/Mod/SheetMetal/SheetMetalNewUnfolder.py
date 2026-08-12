########################################################################
#
#  SheetMetalNewUnfolder.py
#
#  Copyright 2025 Alex Neufeld <alex.d.neufeld@gmail.com>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
########################################################################

from enum import Enum, auto
from functools import reduce
from itertools import combinations
from math import degrees, log10, pi, radians, sin, tan
from operator import mul as multiply_operator
from statistics import StatisticsError, mode

import FreeCAD
import Part
from FreeCAD import Matrix, Placement, Rotation, Vector
from TechDraw import projectEx as project_shape_to_plane

import SheetMetalTools
import SheetMetalText

try:
    import networkx as nx
except ImportError:
    FreeCAD.Console.PrintUserError(
        "The NetworkX Python package could not be imported. "
        "Consider checking that it is installed, "
        "or reinstalling the SheetMetal workbench using the addon manager\n"
    )
try:
    test_graph = nx.Graph
except AttributeError:
    FreeCAD.Console.PrintUserError(
        "The NetworkX Python package is version "
        + str(nx.__version__)
        + "\n"
        + "Consider checking that it is at least version 3.4.2\n "
    )

# We need to VERY CAREFULLY choose multiple different 'epsilon' values
# for different types of numerical comparisons.
#
# Default eps value to use for most numerical approximations.
# This is intentionally larger than OCC's tolerance requirements,
# so that out of tolerance geometry can still be processed
# and then fixed later with cleanup passes
eps = FreeCAD.Base.Precision.approximation()
# This is used instead of 'eps' when comparing angles.
eps_angular = FreeCAD.Base.Precision.angular()
# When running cleanup passes, it will be assumed that points that are
# closer together than this value should be altered to be exactly
# coincident.
fuzz = 1e-3  # <-- 1 / 1000 * 1mm  = one micrometer
# This is OCC's upper bound for tolerance errors when building geometry.
# Make sure to use it as an acceptance criterion before passing data
# to OCC!
tol = FreeCAD.Base.Precision.confusion()
# When converting B-Splines to Arcs, use a much larger tolerance value,
# so that we don't end up with too many small segments.
spline2arc_tol = 0.1  # One tenth of one millimeter.
# Used when splitting an edge into a set number of small segments.
discretization_quantity = 10


class EstimateThickness:
    """This class provides helper functions to determine the sheet
    thickness of a solid-modelled sheet metal part.
    """

    @staticmethod
    def from_normal_edges(shp: Part.Shape, selected_face: int) -> float:
        """Get the modal length of all straight edges that share
        a vertex with the selected root face, and are oriented in line
        with the root faces normal direction. Edges that meet these
        criteria usually correspond to the sheet thickness.
        """
        num_places = abs(int(log10(eps)))
        root_face = shp.Faces[selected_face]
        normal = root_face.Surface.Axis
        # Checking membership of an edge in a shape directly won't work.
        # We must compare via hashCodes instead.
        root_face_edge_hashes = [e.hashCode() for e in root_face.Edges]
        length_values = []
        for v in root_face.Vertexes:
            for e in shp.ancestorsOfType(v, Part.Edge):
                if (
                    e.hashCode() not in root_face_edge_hashes
                    and e.Curve.TypeId == "Part::GeomLine"
                    and SheetMetalTools.smIsParallel(e.Curve.Direction, normal)
                ):
                    length_values.append(round(e.Length, num_places))
        try:
            thickness_value = mode(length_values)
            return thickness_value
        except StatisticsError:
            return 0.0

    @staticmethod
    def from_cylinders(shp: Part.Shape) -> float:
        """In a typical sheet metal part, the solid model has lots of bends, each
        bend having 2 concentric cylindrical faces. If we take the modal
        difference between all possible combinations of radii present in the
        subset of shape faces which are cylindrical, we will usually get the
        exact thickness of the sheet metal part.
        """
        num_places = abs(int(log10(eps)))
        curv_map = {}
        for face in shp.Faces:
            if face.Surface.TypeId == "Part::GeomCylinder":
                # Normalize the axis and center-point.
                normalized_axis = face.Surface.Axis.normalize()
                if normalized_axis.dot(Vector(0, 0, -1)) < 0:
                    normalized_axis = normalized_axis.negative()
                cleaned_axis = Vector(*[round(d, num_places) for d in normalized_axis])
                adjusted_center = face.Surface.Center.projectToPlane(
                    Vector(), normalized_axis
                )
                cleaned_center = Vector(
                    *[round(d, num_places) for d in adjusted_center]
                )
                key = (*cleaned_axis, *cleaned_center)
                if key in curv_map:
                    curv_map[key].append(face.Surface.Radius)
                else:
                    curv_map[key] = [
                        face.Surface.Radius,
                    ]
        combined_list_of_thicknesses = [
            val
            for radset in curv_map.values()
            if len(radset) > 1
            for r1, r2 in combinations(radset, 2)
            if (val := abs(r1 - r2)) > eps
        ]
        try:
            thickness_value = mode(combined_list_of_thicknesses)
            return thickness_value
        except StatisticsError:
            return 0.0

    @staticmethod
    def from_face(shape: Part.Shape, selected_face: int) -> float:
        ref_face = shape.Faces[selected_face]
        # Find all planar faces that are parallel to the chosen face.
        candidates = [
            f
            for f in shape.Faces
            if f.hashCode() != ref_face.hashCode()
            and f.Surface.TypeId == "Part::GeomPlane"
            and SheetMetalTools.smIsParallel(ref_face.Surface.Axis, f.Surface.Axis)
        ]
        if not candidates:
            return 0.0
        opposite_face = sorted(candidates, key=lambda x: abs(x.Area - ref_face.Area))[0]
        return abs(
            opposite_face.valueAt(0, 0).distanceToPlane(
                ref_face.Surface.Position, ref_face.Surface.Axis
            )
        )

    @staticmethod
    def using_best_method(shape: Part.Shape, selected_face: int) -> float:
        thickness = EstimateThickness.from_normal_edges(shape, selected_face)
        if not thickness:
            thickness = EstimateThickness.from_face(shape, selected_face)
        if not thickness:
            thickness = EstimateThickness.from_cylinders(shape)
        if not thickness:
            errmsg = "Couldn't estimate thickness for shape!"
            raise RuntimeError(errmsg)
        return thickness


class TangentFaces:
    """This class provides functions to check if brep faces are tangent
    to each other. each compare_x_x function accepts two surfaces of a
    particular type, and returns a boolean value indicating tangency.
    The compare function accepts two faces and selects the correct
    compare_x_x function automatically.
    """

    @staticmethod
    def compare_plane_plane(p1: Part.Plane, p2: Part.Plane) -> bool:
        # Returns True if the two planes have similar normals and
        # the base point of the first plane is (nearly) coincident with
        # the second plane.
        return (
            SheetMetalTools.smIsParallel(p1.Axis, p2.Axis)
            and p1.Position.distanceToPlane(p2.Position, p2.Axis) < eps
        )

    @staticmethod
    def compare_plane_cylinder(p: Part.Plane, c: Part.Cylinder) -> bool:
        # Returns True if the cylinder is tangent to the plane
        # (there is 'line contact' between the surfaces).
        return (
            SheetMetalTools.smIsNormal(p.Axis, c.Axis)
            and abs(abs(c.Center.distanceToPlane(p.Position, p.Axis)) - c.Radius) < eps
        )

    @staticmethod
    def compare_cylinder_cylinder(c1: Part.Cylinder, c2: Part.Cylinder) -> bool:
        # Returns True if the two cylinders have parallel axis' and
        # those axis' are separated by a distance of
        # approximately r1 + r2.
        return (
            SheetMetalTools.smIsParallel(c1.Axis, c2.Axis)
            and (
                abs(
                    c1.Center.distanceToLine(c2.Center, c2.Axis)
                    - (c1.Radius + c2.Radius)
                )
                < eps
            )
            # alternative condition: 2 coaxial cylinders with the same radius
            or (
                abs(c1.Center.distanceToLine(c2.Center, c2.Axis) < eps)
                and SheetMetalTools.smIsParallel(c1.Axis, c2.Axis)
                and (abs(c1.Radius - c2.Radius) < eps)
            )
        )

    @staticmethod
    def compare_plane_torus(p: Part.Plane, t: Part.Toroid) -> bool:
        # Imagine a donut sitting flat on a table.
        # That's our tangency condition for a plane and a toroid.
        return (
            SheetMetalTools.smIsParallel(p.Axis, t.Axis)
            and abs(abs(t.Center.distanceToPlane(p.Position, p.Axis)) - t.MinorRadius)
            < eps
        )

    @staticmethod
    def compare_cylinder_torus(c: Part.Cylinder, t: Part.Toroid) -> bool:
        # If the surfaces are tangent, either we have:
        # - a donut inside a circular container, with no gap at the
        #     container perimeter;
        # - a donut shoved onto a shaft with no wiggle room;
        # - a cylinder with an axis tangent to the central circle of
        #     the donut.
        return (
            SheetMetalTools.smIsParallel(c.Axis, t.Axis)
            and c.Center.distanceToLine(t.Center, t.Axis) < eps
            and (
                abs(c.Radius - abs(t.MajorRadius - t.MinorRadius)) < eps
                or abs(c.Radius - abs(t.MajorRadius + t.MinorRadius)) < eps
            )
        ) or (
            SheetMetalTools.smIsNormal(c.Axis, t.Axis)
            and abs(abs(t.Center.distanceToLine(c.Center, c.Axis)) - t.MajorRadius)
            < eps
            and abs(c.Radius - t.MinorRadius) < eps
        )

    @staticmethod
    def compare_sphere_sphere(s1: Part.Sphere, s2: Part.Sphere) -> bool:
        # Only segments of identical spheres are tangent to each other.
        return (
            s1.Center.distanceToPoint(s2.Center) < eps
            and abs(s1.Radius - s2.Radius) < eps
        )

    @staticmethod
    def compare_plane_sphere(p: Part.Plane, s: Part.Sphere) -> bool:
        # This function will probably never actually return True,
        # because a plane and a sphere only ever share a vertex if
        # they are tangent to each other.
        return abs(abs(s.Center.distanceToPlane(p.Position, p.Axis)) - s.Radius) < eps

    @staticmethod
    def compare_torus_sphere(t: Part.Toroid, s: Part.Sphere) -> bool:
        return (
            s.Center.distanceToPoint(t.Center) < eps
            and (
                abs(t.MajorRadius - t.MinorRadius - s.Radius) < eps
                or abs(t.MajorRadius + t.MinorRadius - s.Radius) < eps
            )
        ) or (
            abs(s.Radius - t.MinorRadius) < eps
            and SheetMetalTools.smIsNormal(t.Axis, s.Center - t.Center)
            and abs(t.Center.distanceToPoint(s.Center) - t.MajorRadius) < eps
        )

    @staticmethod
    def compare_torus_torus(t1: Part.Toroid, t2: Part.Toroid) -> bool:
        return (
            t1.Center.distanceToLine(t2.Center, t2.Axis) < eps
            and SheetMetalTools.smIsParallel(t1.Axis, t2.Axis)
            and abs(
                t1.Center.distanceToPoint(t2.Center) ** 2
                + (t1.MajorRadius - t2.MajorRadius) ** 2
                - (t1.MinorRadius + t2.MinorRadius) ** 2
            )
            < eps
        )

    @staticmethod
    def compare_cylinder_sphere(c: Part.Cylinder, s: Part.Sphere) -> bool:
        # The sphere must be sized/positioned like a ball sliding down
        # a tube with no wiggle room.
        return (
            (
                s.Center.distanceToLine(c.Center, c.Axis) < eps
                and abs(s.Radius - c.Radius) < eps
            )
            # Point contact case.
            or (
                abs(s.Center.distanceToLine(c.Center, c.Axis) - s.Radius - c.Radius)
                < eps
            )
        )

    @staticmethod
    def compare_plane_cone(p: Part.Plane, cn: Part.Cone) -> bool:
        return abs(cn.Apex.distanceToPlane(p.Position, p.Axis)) < eps and (
            abs(cn.Axis.getAngle(p.Axis) - abs(cn.SemiAngle) - pi / 2) < eps_angular
            or abs(cn.Axis.getAngle(p.Axis) + abs(cn.SemiAngle) - pi / 2) < eps_angular
        )

    @staticmethod
    def compare_cone_cone(cn1: Part.Cone, cn2: Part.Cone) -> bool:
        return (
            cn1.Apex.distanceToPoint(cn2.Apex) < eps
            and abs(cn1.Axis.getAngle(cn2.Axis) - cn1.SemiAngle - cn2.SemiAngle)
            < eps_angular
        )

    @staticmethod
    def compare_sphere_cone(s: Part.Sphere, cn: Part.Cone) -> bool:
        return (
            s.Center.distanceToLine(cn.Apex, cn.Axis) < eps
            and (cn.Apex.distanceToPoint(s.Center) * sin(cn.SemiAngle) - s.Radius) < eps
        )

    @staticmethod
    def compare_cylinder_cone(c: Part.Cylinder, cn: Part.Cone) -> bool:
        return abs(cn.Apex.distanceToLine(c.Center, c.Axis) - c.Radius) < eps and (
            abs(c.Axis.getAngle(cn.Axis) - cn.SemiAngle) < eps_angular
            or abs(pi - c.Axis.getAngle(cn.Axis) - abs(cn.SemiAngle)) < eps_angular
        )

    @staticmethod
    def compare_torus_cone(t: Part.Toroid, cn: Part.Cone) -> bool:
        return (
            SheetMetalTools.smIsParallel(t.Axis, cn.Axis)
            and cn.Apex.distanceToLine(t.Center, t.Axis) < eps
            and (
                abs(
                    t.MajorRadius / tan(cn.SemiAngle)
                    - t.MinorRadius / sin(cn.SemiAngle)
                    - cn.Apex.distanceToPoint(t.Center)
                )
                < eps
                or abs(
                    t.MajorRadius / tan(cn.SemiAngle)
                    + t.MinorRadius / sin(cn.SemiAngle)
                    - cn.Apex.distanceToPoint(t.Center)
                )
                < eps
            )
        )

    @staticmethod
    def compare_surfaces_via_sample(s1, s2, shared_edge: Part.Edge) -> bool:
        # compare surface tangency by sampling at the centerpoint of a linear edge
        if shared_edge.Curve.TypeId != "Part::GeomLine":
            return False
        test_point = shared_edge.CenterOfMass
        value_1 = s1.projectPoint(test_point, "LowerDistanceParameters")
        value_2 = s2.projectPoint(test_point, "LowerDistanceParameters")
        normal_1 = s1.normal(*value_1)
        normal_2 = s2.normal(*value_2)
        return SheetMetalTools.smIsParallel(normal_1, normal_2)

    @staticmethod
    def compare_plane_extrusion(
        p: Part.Plane, ex: Part.SurfaceOfExtrusion, shared_edge: Part.Edge
    ) -> bool:
        return TangentFaces.compare_surfaces_via_sample(p, ex, shared_edge)

    @staticmethod
    def compare_cylinder_extrusion(
        c: Part.Cylinder, ex: Part.SurfaceOfExtrusion, shared_edge: Part.Edge
    ) -> bool:
        return TangentFaces.compare_surfaces_via_sample(c, ex, shared_edge)

    @staticmethod
    def compare_torus_extrusion(t: Part.Toroid, ex: Part.SurfaceOfExtrusion) -> bool:
        # a toroid and surface of revolution are only ever tangent at individial points, not across a line.
        # Threefore, just return false
        return False

    @staticmethod
    def compare_sphere_extrusion(s: Part.Sphere, ex: Part.SurfaceOfExtrusion) -> bool:
        # these 2 surface types are never tangent across a line
        return False

    @staticmethod
    def compare_extrusion_extrusion(
        ex1: Part.SurfaceOfExtrusion,
        ex2: Part.SurfaceOfExtrusion,
        shared_edge: Part.Edge,
    ) -> bool:
        return TangentFaces.compare_surfaces_via_sample(ex1, ex2, shared_edge)

    @staticmethod
    def compare_extrusion_cone(
        ex: Part.SurfaceOfExtrusion, cn: Part.Cone, shared_edge: Part.Edge
    ) -> bool:
        return TangentFaces.compare_surfaces_via_sample(ex, cn, shared_edge)

    @staticmethod
    def compare(
        face1: Part.Face, face2: Part.Face, shared_edge: Part.Edge
    ) -> tuple[bool, bool]:
        # determine tangency of two faces, checking as many surface geometry
        # combinations as possible
        s1 = face1.Surface
        s2 = face2.Surface
        type1 = s1.TypeId
        type2 = s2.TypeId
        order = [
            "Part::GeomPlane",
            "Part::GeomCylinder",
            "Part::GeomToroid",
            "Part::GeomSphere",
            "Part::GeomSurfaceOfExtrusion",
            "Part::GeomCone",
        ]
        needs_swap = (
            type1 in order
            and type2 in order
            and order.index(type1) > order.index(type2)
        )
        if needs_swap:
            s2, s1 = s1, s2
        cls = TangentFaces
        res = False
        if s1.TypeId == "Part::GeomPlane":
            # Plane.
            if s2.TypeId == "Part::GeomPlane":
                res = cls.compare_plane_plane(s1, s2)
            elif s2.TypeId == "Part::GeomCylinder":
                res = cls.compare_plane_cylinder(s1, s2)
            elif s2.TypeId == "Part::GeomToroid":
                res = cls.compare_plane_torus(s1, s2)
            elif s2.TypeId == "Part::GeomSphere":
                res = cls.compare_plane_sphere(s1, s2)
            elif s2.TypeId == "Part::GeomSurfaceOfExtrusion":
                res = cls.compare_plane_extrusion(s1, s2, shared_edge)
            elif s2.TypeId == "Part::GeomCone":
                res = cls.compare_plane_cone(s1, s2)
        elif s1.TypeId == "Part::GeomCylinder":
            # Cylinder.
            if s2.TypeId == "Part::GeomCylinder":
                res = cls.compare_cylinder_cylinder(s1, s2)
            elif s2.TypeId == "Part::GeomToroid":
                res = cls.compare_cylinder_torus(s1, s2)
            elif s2.TypeId == "Part::GeomSphere":
                res = cls.compare_cylinder_sphere(s1, s2)
            elif s2.TypeId == "Part::GeomSurfaceOfExtrusion":
                res = cls.compare_cylinder_extrusion(s1, s2, shared_edge)
            elif s2.TypeId == "Part::GeomCone":
                res = cls.compare_cylinder_cone(s1, s2)
        elif s1.TypeId == "Part::GeomToroid":
            # Torus.
            if s2.TypeId == "Part::GeomToroid":
                res = cls.compare_torus_torus(s1, s2)
            elif s2.TypeId == "Part::GeomSphere":
                res = cls.compare_torus_sphere(s1, s2)
            elif s2.TypeId == "Part::GeomSurfaceOfExtrusion":
                res = cls.compare_torus_extrusion(s1, s2)
            elif s2.TypeId == "Part::GeomCone":
                res = cls.compare_torus_cone(s1, s2)
        elif s1.TypeId == "Part::GeomSphere":
            # Sphere.
            if s2.TypeId == "Part::GeomSphere":
                res = cls.compare_sphere_sphere(s1, s2)
            elif s2.TypeId == "Part::GeomSurfaceOfExtrusion":
                res = cls.compare_sphere_extrusion(s1, s2)
            elif s2.TypeId == "Part::GeomCone":
                res = cls.compare_sphere_cone(s1, s2)
        elif s1.TypeId == "Part::GeomSurfaceOfExtrusion":
            # Extrusion.
            if s2.TypeId == "Part::GeomSurfaceOfExtrusion":
                res = cls.compare_extrusion_extrusion(s1, s2, shared_edge)
            elif s2.TypeId == "Part::GeomCone":
                res = cls.compare_extrusion_cone(s1, s2, shared_edge)
        elif s1.TypeId == "Part::GeomCone":
            # Cone.
            if s2.TypeId == "Part::GeomCone":
                res = cls.compare_cone_cone(s1, s2)
        # emit a warning if there are bends across unsupported geometry types
        well_supported_types = {"Part::GeomPlane", "Part::GeomCylinder"}
        warn = res and not set([type1, type2]) <= well_supported_types
        return res, warn


class UVRef(Enum):
    """Describes reference corner for a rectangular-ish surface patch."""

    BOTTOM_LEFT = auto()
    BOTTOM_RIGHT = auto()
    TOP_LEFT = auto()
    TOP_RIGHT = auto()


class BendDirection(Enum):
    """Up is like a tray with a raised lip, down is like the rolled
    over edges of a table.
    """

    UP = auto()
    DOWN = auto()

    @staticmethod
    def from_face(bent_face: Part.Face):
        """Cylindrical faces may be convex or concave, and the boundary
        representation can be forward or reversed. The bend direction
        may be determined according to these values.
        """
        curv_a, curv_b = bent_face.curvatureAt(0, 0)
        if curv_a < 0 and abs(curv_b) < eps:
            if bent_face.Orientation == "Forward":
                return BendDirection.DOWN
            else:
                return BendDirection.UP
        elif curv_b > 0 and abs(curv_a) < eps:
            if bent_face.Orientation == "Forward":
                return BendDirection.UP
            else:
                return BendDirection.DOWN
        else:
            errmsg = "Unable to determine bend direction from cylindrical face"
            raise RuntimeError(errmsg)


class SketchExtraction:
    """Helper functions to produce clean 2D geometry from unfolded shapes."""

    @staticmethod
    def edges_to_sketch_object(
        edges: list[Part.Edge],
        object_name: str,
        existing_sketches: list[str] = None,
        color: str = "#00FF00",
    ) -> FreeCAD.DocumentObject:
        """Converts a list of edges to an un-constrained sketch object.
        This allows the user to more easily make small changes to the
        sheet metal cutting pattern when prepping it for fabrication.
        """
        cleaned_up_edges = edges  # Edge2DCleanup.cleanup_sketch(edges, spline2arc_tol)
        # See if there is an existing sketch with the same name,
        # use it instead of creating a new one.
        if existing_sketches is None:
            existing_sketch_name = ""
        else:
            existing_sketch_name = next(
                (item for item in existing_sketches if item.startswith(object_name)), ""
            )
        sketch = FreeCAD.ActiveDocument.getObject(existing_sketch_name)
        if sketch is not None:
            sketch.deleteAllGeometry()
        else:
            # If there is not already an existing sketch, create one.
            sketch = FreeCAD.ActiveDocument.addObject(
                "Sketcher::SketchObject", object_name
            )
            sketch.Placement = Placement()

        for edge in cleaned_up_edges:
            startpoint = edge.firstVertex().Point
            endpoint = edge.lastVertex().Point
            curvetype = edge.Curve.TypeId
            if curvetype == "Part::GeomLine":
                if startpoint.distanceToPoint(endpoint) > eps:
                    sketch.addGeometry(Part.LineSegment(startpoint, endpoint))
            elif curvetype == "Part::GeomCircle":
                if startpoint.distanceToPoint(endpoint) < eps:
                    # Full circle.
                    sketch.addGeometry(
                        Part.Circle(
                            edge.Curve.Center, Vector(0, 0, 1), edge.Curve.Radius
                        )
                    )
                else:
                    # Arc.
                    pmin, pmax = edge.ParameterRange
                    midpoint = edge.valueAt(pmin + 0.5 * (pmax - pmin))
                    sketch.addGeometry(Part.Arc(startpoint, midpoint, endpoint))
            else:
                errmsg = (
                    "Unusable curve type found during sketch creation: " + curvetype
                )
                raise RuntimeError(errmsg)
        sketch.Label = object_name
        sketch.recompute()
        # If the gui is running, change the color of the sketch lines
        # and vertices.
        if FreeCAD.GuiUp:
            rgb_color = tuple(int(color[i : i + 2], 16) for i in (1, 3, 5))
            v = FreeCAD.Version()
            if v[0] == "0" and int(v[1]) < 21:
                rgb_color = tuple(i / 255 for i in rgb_color)
            sketch.ViewObject.LineColor = rgb_color
            sketch.ViewObject.PointColor = rgb_color
            if hasattr(sketch.ViewObject, "AutoColor"):
                sketch.ViewObject.AutoColor = False
        return sketch

    @staticmethod
    def wire_is_a_hole(w: Part.Wire) -> bool:
        return (
            len(w.Edges) == 1
            and w.Edges[0].Curve.TypeId == "Part::GeomCircle"
            and abs(w.Edges[0].Length - 2 * pi * w.Edges[0].Curve.Radius) < eps
        )

    @staticmethod
    def extract_manually(
        unfolded_shape: Part.Shape, normal: Vector
    ) -> tuple[Part.Shape]:
        """Extract sketch lines from the topmost flattened face."""
        # Another approach would be to slice the flattened solid with
        # a plane to get a cross-section of the middle of the unfolded
        # shape. This would probably be slower, but might be more robust
        # in cases where the outerwire is not cleanly defined.
        top_face = [
            f
            for f in unfolded_shape.Faces
            if f.normalAt(0, 0).getAngle(normal) < eps_angular
        ][0]
        sketch_profile = top_face.OuterWire
        inner_wires = []
        hole_wires = []
        for w in top_face.Wires:
            if w.hashCode() != sketch_profile.hashCode():
                if SketchExtraction.wire_is_a_hole(w):
                    hole_wires.append(w)
                else:
                    inner_wires.append(w)
        return sketch_profile, inner_wires, hole_wires

    @staticmethod
    def extract_with_techdraw(solid: Part.Shape, direction: Vector) -> Part.Shape:
        """Uses functionality from the TechDraw API to project
        a 3D shape onto a particular 2D plane.
        """
        # this is a slow but robust method of sketch profile extraction
        # ref:
        # https://github.com/FreeCAD/FreeCAD/blob/main/src/Mod/Draft/draftobjects/shape2dview.py
        raw_output = project_shape_to_plane(solid, direction)
        edges = [group for group in raw_output[:5] if not group.isNull()]
        compound = Part.makeCompound(edges)
        return compound

    @staticmethod
    def move_to_origin(sketch: Part.Compound, root_face: Part.Face) -> Matrix:
        """Given a 2d shape and a reference face, compute a transformation matrix
        that aligns the shape's bounding box to the origin of the XY-plane, with
        the reference face oriented Z-up and rotated square to the global
        coordinate system.
        """
        # Find the orientation of the root face that aligns
        # the U-direction with the x-axis.
        origin = root_face.valueAt(0, 0)
        x_axis = root_face.valueAt(1, 0) - origin
        z_axis = root_face.normalAt(0, 0)
        rotation = Rotation(x_axis, Vector(), z_axis, "ZXY")
        alignment_transform = Placement(origin, rotation).toMatrix().inverse()
        sketch_aligned_to_xy_plane = sketch.transformed(alignment_transform)
        # Move in x and y so that the bounding box is entirely in
        # the +x, +y quadrant.
        mov_x = -1 * sketch_aligned_to_xy_plane.BoundBox.XMin
        mov_y = -1 * sketch_aligned_to_xy_plane.BoundBox.YMin
        mov_z = -1 * sketch_aligned_to_xy_plane.BoundBox.ZMin
        shift_transform = Placement(Vector(mov_x, mov_y, mov_z), Rotation()).toMatrix()
        overall_transform = Matrix()
        overall_transform.transform(Vector(), alignment_transform)
        overall_transform.transform(Vector(), shift_transform)
        return overall_transform


class BendAllowanceCalculator:
    def __init__(self) -> None:
        self.k_factor_standard = None
        self.radius_thickness_values = None
        self.k_factor_values = None

    @classmethod
    def from_single_value(cls, k_factor: float, kfactor_standard: str):
        """One k-factor for all radius:thickness ratios."""
        instance = cls()
        instance.k_factor_standard = (
            cls.KFactorStandard.ANSI
            if kfactor_standard == "ansi"
            else cls.KFactorStandard.DIN
        )
        instance.radius_thickness_values = [
            1.0,
        ]
        instance.k_factor_values = [
            k_factor,
        ]
        return instance

    def get_k_factor(self, radius: float, thickness: float) -> float:
        # If we are below the lowest tabulated value for the radius over
        # thickness relation, return the smallest noted k-factor.
        r_over_t = radius / thickness
        if r_over_t <= self.radius_thickness_values[0]:
            kf_val = self.k_factor_values[0]
        # Apply similar logic to radius:thickness values greater than
        # the largest available.
        elif r_over_t >= self.radius_thickness_values[-1]:
            kf_val = self.k_factor_values[-1]
        # If we are within the range of specified radius: thickness
        # values, perform piecewise linear interpolation.
        else:
            i = 0
            while r_over_t <= self.radius_thickness_values[i]:
                i += 1
            kf1 = self.k_factor_values[i]
            kf2 = self.k_factor_values[i + 1]
            rt1 = self.radius_thickness_values[i]
            rt2 = self.radius_thickness_values[i + 1]
            kf_val = kf1 + (kf2 - kf1) * ((r_over_t - rt1) / (rt2 - rt1))
        # We use the ansi definition of the k-factor everywhere
        # internally.
        return self._convert_to_ansi_kfactor(kf_val)

    def get_bend_allowance(
        self,
        bend_direction: BendDirection,
        radius: float,
        thickness: float,
        bend_angle: float,
    ) -> float:
        factor = self.get_k_factor(radius, thickness)
        if bend_direction == BendDirection.DOWN:
            factor -= 1
        bend_allowance = (radius + factor * thickness) * bend_angle
        return bend_allowance

    class KFactorStandard(Enum):
        ANSI = auto()
        DIN = auto()

    @classmethod
    def from_spreadsheet(cls, sheet: FreeCAD.DocumentObject):
        instance = cls()
        r_t_header = sheet.getContents("A1")
        r_t_header = "".join(c for c in r_t_header if c not in "' ").lower()
        if r_t_header != "radius/thickness":
            errmsg = (
                "Cell A1 of material definition sheet must "
                'be exactly "Radius/Thickness"'
            )
            raise ValueError(errmsg)
        kf_header = sheet.getContents("B1")
        kf_header = "".join(c for c in kf_header if c not in "' -()").lower()
        if kf_header == "kfactoransi":
            instance.k_factor_standard = cls.KFactorStandard.ANSI
        elif kf_header == "kfactordin":
            instance.k_factor_standard = cls.KFactorStandard.DIN
        else:
            errmsg = (
                "Cell B1 of material definition sheet must be "
                'one of "K-factor (ANSI)" or "K-factor (DIN)"'
            )
            raise ValueError(errmsg)
        # Read cells from the A column until we get to an empty cell.
        number_of_columns = 0
        radius_thickness_list = []
        k_factor_list = []
        while sheet.getContents("A" + str(number_of_columns + 2)):
            radius_thickness_list.append(sheet.get("A" + str(number_of_columns + 2)))
            number_of_columns += 1
        # Read corresponding k-factor values from the B column
        # and throw an error if we find an empty cell too early.
        for i in range(number_of_columns):
            if not sheet.getContents("B" + str(i + 2)):
                errmsg = (
                    "material definition sheet has an empty "
                    f"cell in the K-factors column (cell B{i + 2})"
                )
                raise ValueError(errmsg)
            k_factor_list.append(sheet.get("B" + str(i + 2)))
        instance.radius_thickness_values = radius_thickness_list
        instance.k_factor_values = k_factor_list
        return instance

    def _convert_to_ansi_kfactor(self, k_factor: float) -> float:
        if self.k_factor_standard == self.KFactorStandard.DIN:
            return k_factor / 2.0
        else:
            return k_factor


class Edge2DCleanup:
    """Many sheet metal fabrication suppliers, as well as CAM systems
    and laser cutting software, don't have good support for geometric
    primitives other than lines and arcs. This class features tools to
    replace bezier curves and other geometry types with lines and arcs.
    """

    @staticmethod
    def bspline_to_line(curve: Part.Edge) -> tuple[Part.Edge, float]:
        p1 = curve.firstVertex().Point
        p2 = curve.lastVertex().Point
        if p1.distanceToPoint(p2) < eps:
            return Part.Edge(), float("inf")
        line = Part.makeLine(p1, p2)
        max_err = Edge2DCleanup.check_err(curve, line)
        return line, max_err

    @staticmethod
    def check_err(curve1: Part.Edge, curve2: Part.Edge) -> float:
        max_err = 0.00
        # Use the discretize function to make sure that we will compare
        # equidistants points. Some BSplineCurve that are in fact straight lines
        # have not always their parameters equidistants.
        pts1 = curve1.discretize(discretization_quantity)
        pts2 = curve2.discretize(discretization_quantity)
        for i in range(discretization_quantity):
            err = pts1[i].distanceToPoint(pts2[i])
            if err > max_err:
                max_err = err
        return max_err

    @staticmethod
    def bspline_to_arc(curve: Part.Edge) -> tuple[Part.Edge, float]:
        point1 = curve.firstVertex().Point
        point2 = curve.valueAt(
            curve.FirstParameter + 0.5 * (curve.LastParameter - curve.FirstParameter)
        )
        point3 = curve.lastVertex().Point
        if point1.distanceToPoint(point3) < eps:
            # Full circle.
            point4 = curve.valueAt(
                curve.FirstParameter
                + 0.25 * (curve.LastParameter - curve.FirstParameter)
            )
            radius = point1.distanceToPoint(point2) / 2
            center = point1 + 0.5 * (point2 - point1)
            axis = (point1 - center).cross(point4 - center)
            arc = Part.makeCircle(radius, center, axis)
        else:
            # Partial circle.
            arc = Part.Arc(point1, point2, point3).toShape().Edges[0]
        max_err = Edge2DCleanup.check_err(curve, arc)
        return arc, max_err

    @staticmethod
    def curve_to_bisected_arcs(edge: Part.Edge, tolerance: float) -> list[Part.Edge]:
        """For a curved edge that isn't a straight line or circular arc,
        choose the best available method to convert it to a series of
        connected arcs.
        """
        if edge.Curve.TypeId == "Part::GeomBSplineCurve":
            c = edge.Curve
        elif edge.Curve.TypeId == "Part::GeomBezierCurve":
            c = edge.Curve.toBSpline()
        elif edge.Curve.TypeId in (
            "Part::GeomParabola",
            "Part::GeomEllipse",
            "Part::GeomHyperbola",
        ):
            c = edge.toNurbs().Edges[0].Curve
        else:
            errmsg = (
                f"Unhandled curve type found during edge cleanup: {edge.Curve.TypeId}"
            )
            raise RuntimeError(errmsg)
        arcs = c.toBiArcs(tolerance)
        return [a.toShape().Edges[0] for a in arcs]

    @staticmethod
    def eliminate_bsplines(
        sketch: list[Part.Edge], tolerance: float
    ) -> list[Part.Edge]:
        """Convert all geometry in the sketch to only straight lines
        and arcs.
        """
        new_edge_list = []
        for edge in sketch:
            if edge.Curve.TypeId in ["Part::GeomLine", "Part::GeomCircle"]:
                new_edge_list.append(edge)
            else:
                new_edge, max_err = Edge2DCleanup.bspline_to_line(edge)
                if max_err < tolerance:
                    new_edge_list.append(new_edge)
                    continue
                new_edge, max_err = Edge2DCleanup.bspline_to_arc(edge)
                if max_err < tolerance:
                    new_edge_list.append(new_edge)
                    continue
                new_edge_list.extend(
                    Edge2DCleanup.curve_to_bisected_arcs(edge, tolerance)
                )
        return new_edge_list

    @staticmethod
    def line_xy(p1: Vector, p2: Vector) -> Part.Edge:
        """Flatten a straight line to the XY-plane."""
        return Part.makeLine(Vector(p1.x, p1.y, 0.0), Vector(p2.x, p2.y, 0.0))

    @staticmethod
    def arc_xy(start: Vector, middle: Vector, end: Vector) -> Part.Edge:
        """Flatten a circular arc to the XY-plane."""
        return (
            Part.Arc(
                Vector(start.x, start.y, 0.0),
                Vector(middle.x, middle.y, 0.0),
                Vector(end.x, end.y, 0.0),
            )
            .toShape()
            .Edges[0]
        )

    @staticmethod
    def circle_xy(center: Vector, radius: Vector) -> Part.Edge:
        """Flatten a circle to the XY-plane."""
        return (
            Part.Circle(
                Vector(center.x, center.y, 0.0),
                Vector(0.0, 0.0, 1.0),
                radius,
            )
            .toShape()
            .Edges[0]
        )

    @staticmethod
    def fix_coincidence(edgelist: list[Part.Edge], fuzzvalue: float) -> list[Part.Wire]:
        """Given a list of edges, finds pairs of edges with endpoints
        that are nearly (but not exactly) coincident.

        Returns:
            A list of wires with improved coincidence between edges.

        """
        try:
            list_of_lists_of_edges = Part.sortEdges(edgelist, fuzzvalue)
        except Part.OCCError:
            # The optional fuzz-value argument is not available
            # in FreeCAD version <= 0.21. Users should not expect good
            # results with out-of tolerance shapes if the fuzz argument
            # wasn't used.
            list_of_lists_of_edges = Part.sortEdges(edgelist)
        wires = []
        for list_of_edges in list_of_lists_of_edges:
            # Skip tiny edge segments.
            useable_edges = [e for e in list_of_edges if e.Length > fuzzvalue]
            if not useable_edges:
                # Skip this edge list entirely if it was made up of
                # only tiny segments.
                continue
            edgeloop_length = len(useable_edges)
            if edgeloop_length > 1:
                new_edges = []
                for i in range(edgeloop_length):
                    e1 = useable_edges[i % edgeloop_length]
                    e2 = useable_edges[(i + 1) % edgeloop_length]
                    e1_start = e1.firstVertex().Point
                    e1_end = e1.lastVertex().Point
                    e2_start = e2.firstVertex().Point
                    e2_end = e2.lastVertex().Point
                    # This should be the correct error.
                    err1 = e1_end.distanceToPoint(e2_start)
                    # But one of these other ones might be the case we
                    # need to use if Part.sortEdges has failed to do its
                    # job properly.
                    err2 = e1_end.distanceToPoint(e2_end)
                    err3 = e1_start.distanceToPoint(e2_start)
                    err4 = e1_start.distanceToPoint(e2_end)
                    if err1 < err2 and err1 < err3 and err1 < err4:
                        # orientation is End ->*-> Start
                        startpoint = e1.firstVertex().Point
                        endpoint = e2.firstVertex().Point
                    elif err2 < err1 and err2 < err3 and err2 < err4:
                        # "orientation is End ->*-> End"
                        startpoint = e1.firstVertex().Point
                        endpoint = e2.lastVertex().Point
                    elif err3 < err1 and err3 < err2 and err3 < err4:
                        # orientation is Start ->*-> Start
                        startpoint = e1.lastVertex().Point
                        endpoint = e2.firstVertex().Point
                    elif err4 < err2 and err4 < err3 and err4 < err1:
                        # orientation is Start ->*-> End
                        startpoint = e1.lastVertex().Point
                        endpoint = e2.lastVertex().Point
                    else:
                        # Orientation is ambiguous - the best we can do
                        # is assume that the edges were sorted
                        # correctly.
                        startpoint = e1.firstVertex().Point
                        endpoint = e2.firstVertex().Point
                    if e1.Curve.TypeId == "Part::GeomLine":
                        new_edges.append(Edge2DCleanup.line_xy(startpoint, endpoint))
                    elif e1.Curve.TypeId == "Part::GeomCircle":
                        pmin, pmax = e1.ParameterRange
                        midpoint = e1.valueAt((pmax + pmin) / 2)
                        new_edges.append(
                            Edge2DCleanup.arc_xy(startpoint, midpoint, endpoint)
                        )
                    else:
                        errmsg = (
                            f"Can't process edge with curve type = {e1.Curve.TypeId}"
                        )
                        raise RuntimeError(errmsg)
                w = Part.Wire(new_edges)
                wires.append(w)
            else:
                # Single edge loops.
                edge = useable_edges[0]
                if edge.Curve.TypeId != "Part::GeomCircle":
                    errmsg = "Can't process non-circular single-edge loop"
                    raise RuntimeError(errmsg)
                w = Part.Wire(
                    [Edge2DCleanup.circle_xy(edge.Curve.Center, edge.Curve.Radius)]
                )
                wires.append(w)
        return wires

    @staticmethod
    def merge_segmented_circles(wirelist: list[Part.Wire]) -> list[Part.Wire]:
        """Combine circles that are split into multiple edges so that they
        can be recognized as holes properly.
        """
        fixed_wire_list = []
        for w in wirelist:
            if (
                len(w.Edges) > 1
                and all([e.Curve.TypeId == "Part::GeomCircle" for e in w.Edges])
                and all(
                    [
                        e.Curve.Center.distanceToPoint(w.Edges[0].Curve.Center) < eps
                        for e in w.Edges[1:]
                    ]
                )
                and all(
                    [
                        abs(e.Curve.Radius - w.Edges[0].Curve.Radius) < eps
                        for e in w.Edges[1:]
                    ]
                )
            ):
                new_wire = Part.Wire(
                    [
                        Edge2DCleanup.circle_xy(
                            w.Edges[0].Curve.Center, w.Edges[0].Curve.Radius
                        )
                    ]
                )
                fixed_wire_list.append(new_wire)
            else:
                fixed_wire_list.append(w)
        return fixed_wire_list

    @staticmethod
    def clean_and_structure_geometry(edges: list[Part.Edge]) -> list[Part.Wire]:
        """Run all available clean up passes."""
        intermediate_result1 = Edge2DCleanup.eliminate_bsplines(edges, spline2arc_tol)
        intermediate_result2 = Edge2DCleanup.fix_coincidence(intermediate_result1, fuzz)
        result = Edge2DCleanup.merge_segmented_circles(intermediate_result2)
        return result


def build_graph_of_tangent_faces(shp: Part.Shape, root: int) -> nx.Graph:
    # Created a simple undirected graph object.
    graph_of_shape_faces = nx.Graph()
    # Track faces by their indices, because the underlying pointers
    # to faces may get changed around while building the graph.
    face_hashes = [f.hashCode() for f in shp.Faces]
    index_lookup = {h: i for i, h in enumerate(face_hashes)}
    # Get pairs of faces that share the same edge.
    candidates = [
        (i, shp.ancestorsOfType(e, Part.Face)) for i, e in enumerate(shp.Edges)
    ]
    # Filter to remove seams on cylinders or other faces that wrap back
    # onto themselves other than self-adjacent faces, edges should
    # always have 2 face ancestors this assumption is probably only
    # valid for watertight solids.
    saw_warning = False
    for edge_index, faces in filter(lambda c: len(c[1]) == 2, candidates):
        face_a, face_b = faces
        shared_edge = shp.Edges[edge_index]
        tangent_result, possible_geom_warning = TangentFaces.compare(
            face_a, face_b, shared_edge
        )
        saw_warning |= possible_geom_warning
        if tangent_result:
            graph_of_shape_faces.add_edge(
                index_lookup[face_a.hashCode()],
                index_lookup[face_b.hashCode()],
                # Store indexes in the label attr for debugging.
                label=edge_index,
            )
    # emit a warning if the shape has bends across unsupported face types
    if saw_warning:
        msg = (
            "This shape appears to have bends across surfaces that are not planes or cylinders."
            " Unfolding it may produce unexpected results.\n"
        )
        FreeCAD.Console.PrintWarning(msg)
    # graph_of_shape_faces should have at least three connected subgraphs
    # (top side, bottom side, and sheet edge sides of the sheetmetal part).
    # We only care about the subgraph that includes the selected root face.
    for c in nx.connected_components(graph_of_shape_faces):
        if root in c:
            return graph_of_shape_faces.subgraph(c).copy()
    # If there is nothing tangent to the root face, return a graph with
    # one node and no edges.
    # This is useful for dxf/svg export of flat plates for manufacturing.
    single_face_graph = nx.Graph()
    single_face_graph.add_node(root)
    return single_face_graph

class BendInfo:
    def __init__(self, bend_line: Part.Edge, bend_angle: float, bend_radius: float):
        self.line = bend_line
        self.angle = bend_angle
        self.radius = bend_radius

    def getLabelLocation(self, sketch_normal: Vector, distance: float) -> tuple[Vector, Vector]:
        center_parameter = 0.5 * (self.line.FirstParameter + self.line.LastParameter)
        center = self.line.valueAt(center_parameter)
        label_direction = self.line.tangentAt(center_parameter)
        perp_direction = sketch_normal.cross(label_direction).normalize()
        if perp_direction.y < 0:
            perp_direction =  perp_direction.negative()
            label_direction = label_direction.negative()
        label_position = center + perp_direction * distance

        return label_position, label_direction
    
    def getLineLength(self) -> float:
        return self.line.Length

def unroll_cylinder(
    cylindrical_face: Part.Face,
    refpos: UVRef,
    bac: BendAllowanceCalculator,
    thickness: float,
    seam_edges: set,
) -> tuple[list[Part.Edge], BendInfo]:
    """Given a cylindrical face and a reference corner,
    computes flattened versions of the face's non-seam edges,
    oriented with respect to the +x,+y quadrant of the 2D plane.
    """
    umin, umax, vmin, vmax = cylindrical_face.ParameterRange
    bend_angle = umax - umin
    radius = cylindrical_face.Surface.Radius
    bend_direction = BendDirection.from_face(cylindrical_face)
    bend_allowance = bac.get_bend_allowance(
        bend_direction, radius, thickness, bend_angle
    )
    overall_height = abs(vmax - vmin)
    y_scale_factor = bend_allowance / bend_angle
    flattened_edges = []
    for e in [
        edge for edge in cylindrical_face.Edges if edge.hashCode() not in seam_edges
    ]:
        edge_on_surface, e_param_min, e_param_max = cylindrical_face.curveOnSurface(e)
        if isinstance(edge_on_surface, (Part.Geom2d.Line2d, Part.Geom2d.Line2dSegment)):
            v1 = edge_on_surface.value(e_param_min)
            y1, x1 = v1.x - umin, v1.y - vmin
            v2 = edge_on_surface.value(e_param_max)
            y2, x2 = v2.x - umin, v2.y - vmin
            line = Part.makeLine(
                Vector(x1, y1 * y_scale_factor), Vector(x2, y2 * y_scale_factor)
            )
            flattened_edges.append(line)
        elif isinstance(edge_on_surface, Part.Geom2d.BSplineCurve2d):
            poles_and_weights = edge_on_surface.getPolesAndWeights()
            poles = [
                (v - vmin, (u - umin) * y_scale_factor, 0)
                for u, v, _ in poles_and_weights
            ]
            weights = [w for _, _, w in poles_and_weights]
            spline = Part.BSplineCurve()
            spline.buildFromPolesMultsKnots(poles=poles, weights=weights)
            flattened_edges.append(spline.toShape())
        else:
            errmsg = (
                f"Unhandled curve type when unfolding face: {type(edge_on_surface)}"
            )
            raise TypeError(errmsg)
    mirror_base_pos = Vector(overall_height / 2, bend_allowance / 2)
    # There are four possible orientations of the face corresponding to
    # four quadrants of the 2D plane. Whether flipping across the
    # x/y/both axis is required depends on the initial orientation and
    # the UV parameters. The correct flip conditions were figured out by
    # brute force (checking each possible permutation).
    if refpos == UVRef.BOTTOM_LEFT:
        pass
    elif refpos == UVRef.BOTTOM_RIGHT:
        flattened_edges = [
            x.mirror(mirror_base_pos, Vector(0, 1)) for x in flattened_edges
        ]
    elif refpos == UVRef.TOP_LEFT:
        flattened_edges = [
            x.mirror(mirror_base_pos, Vector(1, 0)) for x in flattened_edges
        ]
    elif refpos == UVRef.TOP_RIGHT:
        flattened_edges = [
            x.mirror(mirror_base_pos, Vector(0, 1)).mirror(
                mirror_base_pos, Vector(1, 0)
            )
            for x in flattened_edges
        ]
    half_bend_width = Vector((vmax - vmin) / 2, 0)
    bend_line = Part.makeLine(mirror_base_pos + half_bend_width, mirror_base_pos - half_bend_width)
    dir = 1 if bend_direction == BendDirection.UP else -1
    bend_info = BendInfo(bend_line, (180.0 - degrees(bend_angle)) * dir, radius)
    return flattened_edges, bend_info


def compute_unbend_transform(
    bent_face: Part.Face,
    base_edge: Part.Edge,
    thickness: float,
    bac: BendAllowanceCalculator,
) -> tuple[Matrix, Matrix, UVRef]:
    """Computes the position and orientation of a reference corner on a
    bent surface, as well as a transformation to flatten out subsequent
    faces to align with the pre-bend part of the shape.
    """
    # for cylindrical surfaces, the u-parameter corresponds to the radial
    # direction, and the u-period is the radial boundary of the cylindrical
    # patch. The v-period corresponds to the axial direction.
    umin, umax, vmin, vmax = bent_face.ParameterRange
    # The u period is always positive: 0.0 <= umin < umax <= 2*pi.
    bend_angle = umax - umin
    radius = bent_face.Surface.Radius
    # Disallow fully cylindrical bends. These can't be formed because
    # the opposite edge of the sheet will intersect the previous face.
    if bend_angle > radians(359.9):
        errmsg = "Bend angle must be less that 359.9 degrees"
        raise RuntimeError(errmsg)
    bend_direction = BendDirection.from_face(bent_face)
    # The reference edge should intersect with the bent cylindrical
    # surface at either opposite corner of surface's uv-parameter range.
    # We need to determine which of these possibilities is correct.
    first_corner_point = bent_face.valueAt(umin, vmin)
    second_corner_point = bent_face.valueAt(umax, vmin)
    # At least one of these points should be on the starting edge.
    dist1 = first_corner_point.distanceToLine(
        base_edge.Curve.Location, base_edge.Curve.Direction
    )
    dist2 = second_corner_point.distanceToLine(
        base_edge.Curve.Location, base_edge.Curve.Direction
    )
    # The x-axis of our desired reference is the tangent vector to a
    # radial line on the cylindrical surface, oriented away from the
    # previous face. We can compute candidates to choose from with
    # the .tangent() method.
    if dist1 < eps:  # "Forward" orientation.
        tangent_vector, binormal_vector = bent_face.Surface.tangent(umin, vmin)
        y_axis = tangent_vector
        # Use the normal of the face and not the surface here.
        # If the face is reverse oriented, the surface normal will be
        # flipped relative to the face normal.
        z_axis = bent_face.normalAt(umin, vmin)
        # Place the reference point such that the cylindrical face lies
        # in the (+x, +y) quadrant of the xy-plane of the reference
        # coordinate system.
        x_axis = y_axis.cross(z_axis)
        if x_axis.dot(corner_1 := bent_face.valueAt(umin, vmin)) < x_axis.dot(
            corner_2 := bent_face.valueAt(umin, vmax)
        ):
            lcs_base_point = corner_1
            uvref = UVRef.BOTTOM_LEFT
        else:
            lcs_base_point = corner_2
            uvref = UVRef.TOP_LEFT
    elif dist2 < eps:  # "Reverse" orientation.
        tangent_vector, binormal_vector = bent_face.Surface.tangent(umax, vmin)
        y_axis = tangent_vector.negative()
        z_axis = bent_face.normalAt(umax, vmin)
        x_axis = y_axis.cross(z_axis)
        if x_axis.dot(corner_3 := bent_face.valueAt(umax, vmin)) < x_axis.dot(
            corner_4 := bent_face.valueAt(umax, vmax)
        ):
            lcs_base_point = corner_3
            uvref = UVRef.BOTTOM_RIGHT
        else:
            lcs_base_point = corner_4
            uvref = UVRef.TOP_RIGHT
    else:
        errmsg = "No point on reference edge"
        raise RuntimeError(errmsg)
    # Note that the x-axis is ignored here based on the priority string.
    lcs_rotation = Rotation(x_axis, y_axis, z_axis, "ZYX")
    alignment_transform = Placement(lcs_base_point, lcs_rotation).toMatrix()
    # The actual unbend transformation is found by reversing the
    # rotation of a flat face after the bend due to the bending
    # operation, then pushing it forward according to the bend
    # allowance.
    bend_allowance = bac.get_bend_allowance(
        bend_direction, radius, thickness, bend_angle
    )
    # fmt: off
    allowance_transform = Matrix(
        1, 0, 0, 0,
        0, 1, 0, bend_allowance,
        0, 0, 1, 0,
        0, 0, 0, 1
    )
    rot = Rotation(
        Vector(1, 0, 0),
        (-1 if bend_direction == BendDirection.UP else 1) * degrees(bend_angle)
    ).toMatrix()
    translate = Matrix(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, (1 if bend_direction == BendDirection.UP else -1) * radius,
        0, 0, 0, 1
    )
    # fmt: on
    # Compose transformations to get the final matrix.
    overall_transform = Matrix()
    overall_transform.transform(Vector(), alignment_transform.inverse())
    overall_transform.transform(Vector(), translate * rot * translate.inverse())
    overall_transform.transform(Vector(), allowance_transform)
    overall_transform.transform(Vector(), alignment_transform)
    return alignment_transform, overall_transform, uvref

def unfold(
    shape: Part.Shape, root_face_index: int, bac: BendAllowanceCalculator
) -> tuple[list[Part.Edge], list[BendInfo]]:
    """Given a solid body of a sheet metal part and a reference face, computes
    a solid representation of the unbent object, as well as a compound object
    containing straight edges for each bend centerline.
    """
    graph_of_sheet_faces = build_graph_of_tangent_faces(shape, root_face_index)
    thickness = EstimateThickness.using_best_method(shape, root_face_index)
    # Also build a list of all seam edges, to be filtered out from the
    # unfolded shape.
    seam_edges_list = []
    for _, _, edata in graph_of_sheet_faces.edges(data=True):
        seam_edges_list.append(shape.Edges[edata["label"]].hashCode())
    seam_edges = set(seam_edges_list)
    # We could also get a random spanning tree here. Would that be
    # faster? Or is it better to take the opportunity to get a spanning
    # tree that meets some criteria for minimization?
    # I.E.: the shorter the longest path in the tree, the fewer nested
    # transformations we have to compute.
    spanning_tree = nx.minimum_spanning_tree(graph_of_sheet_faces, weight="label")
    # Convert to "directed tree", where every edge points away from the
    # selected face.
    dg = nx.DiGraph()
    for node in spanning_tree:
        dg.add_node(node)
    lengths = nx.all_pairs_shortest_path_length(spanning_tree)
    distances_to_root_face = {k: kv for k, kv in lengths}[root_face_index]
    for f1, f2, edata in spanning_tree.edges(data=True):
        if distances_to_root_face[f1] <= distances_to_root_face[f2]:
            dg.add_edge(f1, f2, label=edata["label"])
        else:
            dg.add_edge(f2, f1, label=edata["label"])
    # The digraph should now have everything we need to unfold the shape,
    # For every edge f1--e1-->f2 where f2 is a cylindrical face, feed f1
    # through our unbending functions with e1 as the stationary edge.
    for e in [
        e for e in dg.edges if shape.Faces[e[1]].Surface.TypeId == "Part::GeomCylinder"
    ]:
        # The bend face is the end-node of the directed edge.
        bend_part = shape.Faces[e[1]]
        # We stored the edge indices as the labels of the graph edges.
        edge_before_bend_index = dg.get_edge_data(e[0], e[1])["label"]
        # Check that we aren't trying to unfold across a non-linear
        # reference edge. This condition is reached if the user supplies
        # a part with complex formed features that have
        # unfoldable-but-tangent faces, for example.
        edge_before_bend = shape.Edges[edge_before_bend_index]
        if edge_before_bend.Curve.TypeId != "Part::GeomLine":
            errmsg = (
                "This shape appears to have bends across non-straight edges. "
                "Unfolding such a shape is not yet supported."
                f" (Edge{edge_before_bend_index + 1})"
            )
            raise RuntimeError(errmsg)
        # Compute the unbend transformation matrices.
        alignment_transform, overall_transform, uvref = compute_unbend_transform(
            bend_part, edge_before_bend, thickness, bac
        )
        # Determine the unbent face shape from the reference UV position.
        # Also get a bend line across the middle of the flattened face.
        dg.nodes[e[1]]["unbend_transform"] = overall_transform
        try:
            flattened_edges, bend_info = unroll_cylinder(
                bend_part, uvref, bac, thickness, seam_edges
            )
            # Add the transformation and unbend shape to the end node
            # of the edge as attributes.
            bend_info.line = bend_info.line.transformed(alignment_transform)
            dg.nodes[e[1]]["bend_info"] = bend_info
            dg.nodes[e[1]]["sketch_lines"] = [
                e.transformed(alignment_transform) for e in flattened_edges
            ]
        except Exception as E:
            msg = (
                f"failed to unroll a cylindrical face (Face{e[1] + 1})\n"
                + f"Original exception: {E}\n"
            )
            FreeCAD.Console.PrintWarning(msg)
    # Get a path from the root (stationary) face to each other face,
    # so we can combine transformations to position the final shape.
    # Apply the unbent transformation to all the flattened geometry to
    # bring it in-plane with the root face.
    list_of_sketch_lines = []
    list_of_bend_lines_info = []
    for face_id, path in nx.shortest_path(dg, source=root_face_index).items():
        # The path includes the root face itself, which we don't need.
        path_to_face = path[:-1]
        node_data = dg.nodes.data()
        # Accumulate transformations while traversing from the root face
        # to this face.
        list_of_matrices = [
            node_data[f]["unbend_transform"]
            for f in path_to_face
            if "unbend_transform" in node_data[f]
        ]
        # Use reduce() to do repeated matrix multiplication
        # Matrix() * M_1 * M_2 * ... * M_N for N matrices.
        final_mat = reduce(multiply_operator, list_of_matrices, Matrix())
        # Bent faces of the input shape are swapped for their unbent
        # versions.
        if "sketch_lines" in node_data[face_id]:
            list_of_sketch_lines.extend(
                [e.transformed(final_mat) for e in node_data[face_id]["sketch_lines"]]
            )
        # Planar faces of the input shape are returned aligned to the
        # root face, but otherwise unmodified.
        else:
            list_of_sketch_lines.extend(
                [
                    e.transformed(final_mat)
                    for e in shape.Faces[face_id].Edges
                    if e.hashCode() not in seam_edges
                ]
            )
        # Also combine all the bend lines into a list after positioning
        # them correctly.
        if "bend_info" in node_data[face_id]:
            bend_info = node_data[face_id]["bend_info"]
            bend_info.line = bend_info.line.transformed(final_mat)
            list_of_bend_lines_info.append(bend_info)
    # Extrude the 2d profile back into a flattened solid body.
    return list_of_sketch_lines, list_of_bend_lines_info

    
def getBendLabels(bend_infodata: list[BendInfo], bend_label_size: float = 2.0) -> Part.Compound:
    if not bend_infodata:
        return Part.makeCompound([])
    label_edges = []    
    for bend_info in bend_infodata:
        label_text = f"{bend_info.angle:.2f}".rstrip("0").rstrip(".") + "^"
        label_pos, label_dir = bend_info.getLabelLocation(Vector(0, 0, 1), distance=bend_label_size / 3.0)
        label_comp = SheetMetalText.smtGenerateText(label_text, label_pos, bend_label_size, label_dir)
        label_edges.extend(label_comp.Edges)
    return Part.makeCompound(label_edges)

def getUnfold(
    bac: BendAllowanceCalculator, solid: Part.Feature, facename: str
) -> tuple[Part.Face, Part.Shape, Part.Compound, Vector, list[BendInfo]]:
    object_placement = solid.Placement.toMatrix()
    shp = solid.Shape.transformed(object_placement.inverse())
    if hasattr(shp, "findSubShape"):
        # FreeCAD version >= 1.0
        subshape = shp.getElement(facename)
        root_face_index = shp.findSubShape(subshape)[1] - 1
    else:
        # FreeCAD version <= 0.21
        try:
            root_face_index = int(facename[4:]) - 1
        except ValueError:
            errmsg = f"Invalid shape name: {facename}"
            raise RuntimeError(errmsg)
    sketch_lines, bend_lines_info = unfold(shp, root_face_index, bac)
    sketch_align_transform = SketchExtraction.move_to_origin(
        Part.makeCompound(sketch_lines), shp.Faces[root_face_index]
    )
    for bend_info in bend_lines_info:
        bend_info.line = bend_info.line.transformed(sketch_align_transform)
    thickness = EstimateThickness.using_best_method(shp, root_face_index)
    sketch_lines = [e.transformed(sketch_align_transform) for e in sketch_lines]
    bend_lines = [bi.line for bi in bend_lines_info]
    #bend_infodata = [BendInfo(e, e.Tag / 100.0, 0) for e in bend_lines]
    sketch_wirelist = Edge2DCleanup.clean_and_structure_geometry(sketch_lines)
    root_normal = shp.Faces[root_face_index].normalAt(0, 0)
    face = Part.makeFace(sketch_wirelist, "Part::FaceMakerBullseye")
    unbent_solid = face.extrude(Vector(0.0, 0.0, -1 * thickness))
    inplace_unbend = face.transformed(sketch_align_transform.inverse()).extrude(
        root_normal.normalize() * -1 * thickness
    )
    bend_lines_compound = Part.makeCompound(bend_lines)
    trimmed_bend_lines = bend_lines_compound.common(
        unbent_solid.translated(Vector(0.0, 0.0, 0.5 * thickness))
    ).transformed(sketch_align_transform.inverse())
   
    return shp.Faces[root_face_index], inplace_unbend, trimmed_bend_lines, root_normal, bend_lines_info


def getUnfoldSketches(
    obj_label,
    selected_face: Part.Face,
    unfolded_shape: Part.Shape,
    bend_lines: Part.Compound,
    root_normal: Vector,
    existing_sketches: list[str],
    split_sketches: bool = False,
    sketch_color: str = "#000080",
    bend_sketch_color: str = "#c00000",
    internal_sketch_color: str = "#ff5733",
    bend_infodata: list[BendInfo] = [],
    bend_label_color: str = "#33c033",
    bend_label_size: float = 2.0,
) -> list[Part.Feature]:
    sketch_profile, inner_wires, hole_wires = SketchExtraction.extract_manually(
        unfolded_shape, root_normal
    )
    # Create transform to move the sketch profiles nicely to the origin.
    sketch_align_transform = SketchExtraction.move_to_origin(
        sketch_profile, selected_face
    )

    bend_labels = getBendLabels(bend_infodata, bend_label_size)

    if obj_label is None:
        obj_label = "Unfolded"

    if not split_sketches:
        bend_labels_transformed = bend_labels.transformed(sketch_align_transform.inverse()) 
        sketch_profile = Part.makeCompound(
            [sketch_profile, *inner_wires, *hole_wires, bend_lines, bend_labels_transformed]
        )
        inner_wires = None
        hole_wires = None
        bend_lines = None
    sketch_profile = sketch_profile.transformed(sketch_align_transform)
    # Organize the unfold sketch layers in a group.
    sketch_doc_obj = SketchExtraction.edges_to_sketch_object(
        sketch_profile.Edges, f"{obj_label}_Sketch", existing_sketches, sketch_color
    )
    sketch_objects_list = [sketch_doc_obj]
    # Bend lines are sometimes not present.
    if bend_lines and bend_lines.Edges:
        bend_lines = bend_lines.transformed(sketch_align_transform)
        bend_lines_doc_obj = SketchExtraction.edges_to_sketch_object(
            bend_lines.Edges,
            f"{obj_label}_Sketch_Bends",
            existing_sketches,
            bend_sketch_color,
        )
        bend_lines_doc_obj.ViewObject.DrawStyle = "Dashdot"
        sketch_objects_list.append(bend_lines_doc_obj)
    # Inner lines are sometimes not present.
    if inner_wires:
        inner_lines = Part.makeCompound(inner_wires).transformed(sketch_align_transform)
        inner_lines_doc_obj = SketchExtraction.edges_to_sketch_object(
            inner_lines.Edges,
            f"{obj_label}_Sketch_Internal",
            existing_sketches,
            internal_sketch_color,
        )
        sketch_objects_list.append(inner_lines_doc_obj)
    if hole_wires:
        hole_lines = Part.makeCompound(hole_wires).transformed(sketch_align_transform)
        hole_lines_doc_obj = SketchExtraction.edges_to_sketch_object(
            hole_lines.Edges,
            f"{obj_label}_Sketch_Holes",
            existing_sketches,
            internal_sketch_color,
        )
        sketch_objects_list.append(hole_lines_doc_obj)
    if bend_labels and bend_labels.Edges:
        bend_labels_doc_obj = SketchExtraction.edges_to_sketch_object(
            bend_labels.Edges,
            f"{obj_label}_Sketch_Bend_Labels",
            existing_sketches,
            bend_label_color,
        )
        bend_labels_doc_obj.ViewObject.PointSize = 0
        sketch_objects_list.append(bend_labels_doc_obj)
    return sketch_objects_list
