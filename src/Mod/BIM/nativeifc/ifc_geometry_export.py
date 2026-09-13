# SPDX-License-Identifier: LGPL-2.1-or-later

"""FreeCAD geometry to IFC conversion boundary used by NativeIFC."""

import FreeCAD

from . import backend


class GeometryExporter:
    """Convert FreeCAD geometry with a NativeIFC-owned operation context."""

    def __init__(self, ifcfile):
        self.ifcfile = ifcfile
        self.ifcopenshell = backend.get_backend()

    def create_representation(self, context, obj, preferences, ifcclass=None):
        """Convert one FreeCAD object's geometry into an IFC representation."""

        direct = self._create_simple_wall(context, obj)
        if direct is not None:
            return self._styled(obj, direct)
        direct = self._create_rectangular_extrusion(context, obj, ifcclass)
        if direct is not None:
            return self._styled(obj, direct)
        direct = self._create_circular_extrusion(context, obj)
        if direct is not None:
            return self._styled(obj, direct)
        direct = self._create_planar_extrusion(context, obj)
        if direct is not None:
            return self._styled(obj, direct)
        direct = self._create_faceted_brep(context, obj)
        if direct is not None:
            return self._styled(obj, direct)
        direct = self._create_tessellated(context, obj)
        if direct is not None:
            return self._styled(obj, direct)
        placement = getattr(obj, "Placement", None)
        if placement is not None:
            return None, self._placement(self._global_placement(obj))
        return None, self._identity_placement()

    def _styled(self, obj, result):
        representation, placement = result
        colors = self._item_colors(obj, representation)
        if not colors:
            return result
        items = representation.Representations[0].Items
        for item, color in zip(items, colors):
            self.ifcopenshell.api.style.assign_item_style(
                self.ifcfile,
                item=item,
                style=self._surface_style(color),
            )
        return representation, placement

    def _item_colors(self, obj, representation):
        view = getattr(obj, "ViewObject", None)
        appearance = view if view and hasattr(view, "ShapeColor") else obj
        if not hasattr(appearance, "ShapeColor"):
            return []
        base = tuple(float(value) for value in appearance.ShapeColor[:3])
        transparency = float(getattr(appearance, "Transparency", 0.0)) / 100.0
        if transparency >= 1.0:
            transparency = 0.0
        items = representation.Representations[0].Items
        colors = [base + (transparency,)] * len(items)
        diffuse = getattr(appearance, "DiffuseColor", None)
        shape = getattr(obj, "Shape", None)
        if (
            diffuse
            and shape
            and len(diffuse) == len(shape.Faces)
            and len(items) == len(shape.Solids)
        ):
            colors = []
            face_index = 0
            for solid in shape.Solids:
                color = tuple(float(value) for value in diffuse[face_index][:3])
                colors.append(color + (transparency,))
                face_index += len(solid.Faces)
        return colors

    def _surface_style(self, color):
        color = tuple(min(1.0, max(0.0, value)) for value in color)
        name = "FreeCAD " + " ".join(f"{value:.6f}" for value in color)
        for style in self.ifcfile.by_type("IfcSurfaceStyle"):
            if style.Name == name:
                return style
        api_style = self.ifcopenshell.api.style
        style = api_style.add_style(self.ifcfile, name=name)
        api_style.add_surface_style(
            self.ifcfile,
            style=style,
            ifc_class="IfcSurfaceStyleShading",
            attributes={
                "SurfaceColour": {
                    "Name": None,
                    "Red": color[0],
                    "Green": color[1],
                    "Blue": color[2],
                },
                "Transparency": color[3],
            },
        )
        return style

    def _create_faceted_brep(self, context, obj):
        """Export arbitrary FreeCAD solids as schema-compatible faceted BReps."""

        if getattr(obj, "Subtractions", None):
            return None
        shape = getattr(obj, "Shape", None)
        if not shape or not shape.Solids:
            return None
        if len(shape.Faces) != sum(len(solid.Faces) for solid in shape.Solids):
            return None

        scale = self._coordinate_scale()
        breps = []
        for solid in shape.Solids:
            vertices, triangles = solid.tessellate(1.0)
            if not vertices or not triangles:
                return None
            points = [
                self.ifcfile.createIfcCartesianPoint(tuple(vertex.multiply(scale)))
                for vertex in vertices
            ]
            faces = []
            for triangle in triangles:
                if len(set(triangle)) != 3:
                    continue
                loop = self.ifcfile.createIfcPolyLoop([points[index] for index in triangle])
                bound = self.ifcfile.createIfcFaceOuterBound(loop, True)
                faces.append(self.ifcfile.createIfcFace([bound]))
            if not faces:
                return None
            shell = self.ifcfile.createIfcClosedShell(faces)
            breps.append(self.ifcfile.createIfcFacetedBrep(shell))

        shape_representation = self.ifcfile.createIfcShapeRepresentation(
            context,
            context.ContextIdentifier or "Body",
            "Brep",
            breps,
        )
        return self._product_shape(shape_representation), self._shape_coordinate_placement(obj)

    def _create_tessellated(self, context, obj):
        """Export arbitrary shape or mesh topology without the legacy exporter."""

        topology = self._mesh_topology(obj)
        if topology is None:
            shape = getattr(obj, "Shape", None)
            if not shape or not shape.Faces:
                return None
            vertices, triangles = shape.tessellate(1.0)
            placement = self._shape_coordinate_placement(obj)
        else:
            vertices, triangles = topology
            placement = self._placement(self._global_placement(obj))
        triangles = [
            tuple(index for index in triangle) for triangle in triangles if len(set(triangle)) == 3
        ]
        if not vertices or not triangles:
            return None

        scale = self._coordinate_scale()
        coordinates = [tuple(FreeCAD.Vector(vertex).multiply(scale)) for vertex in vertices]
        if self.ifcfile.wrapped_data.schema_name().startswith("IFC4"):
            point_list = self.ifcfile.createIfcCartesianPointList3D(coordinates)
            item = self.ifcfile.createIfcTriangulatedFaceSet(
                point_list,
                None,
                self._topology_is_closed(obj),
                [tuple(index + 1 for index in triangle) for triangle in triangles],
                None,
            )
            representation_type = "Tessellation"
        else:
            points = [self.ifcfile.createIfcCartesianPoint(point) for point in coordinates]
            faces = []
            for triangle in triangles:
                loop = self.ifcfile.createIfcPolyLoop([points[index] for index in triangle])
                faces.append(
                    self.ifcfile.createIfcFace([self.ifcfile.createIfcFaceOuterBound(loop, True)])
                )
            item = self.ifcfile.createIfcFaceBasedSurfaceModel(
                [self.ifcfile.createIfcConnectedFaceSet(faces)]
            )
            representation_type = "SurfaceModel"
        shape_representation = self.ifcfile.createIfcShapeRepresentation(
            context,
            context.ContextIdentifier or "Body",
            representation_type,
            [item],
        )
        return self._product_shape(shape_representation), placement

    def _mesh_topology(self, obj):
        mesh = getattr(obj, "Mesh", None)
        if not mesh or not getattr(mesh, "Facets", None):
            return None
        vertices = [point.Vector for point in mesh.Points]
        triangles = [tuple(facet.PointIndices) for facet in mesh.Facets]
        return vertices, triangles

    def _topology_is_closed(self, obj):
        mesh = getattr(obj, "Mesh", None)
        if mesh:
            return bool(mesh.isSolid())
        shape = getattr(obj, "Shape", None)
        return bool(shape and shape.isClosed())

    def _create_simple_wall(self, context, obj):
        """Export an unmodified rectangular Arch wall through IfcOpenShell's API."""

        if getattr(obj, "IfcType", None) != "Wall":
            return None
        if getattr(obj, "Base", None):
            return None
        if getattr(obj, "Additions", None):
            return None
        if not getattr(obj, "Shape", None) or len(obj.Shape.Solids) != 1:
            return None

        length = getattr(getattr(obj, "Length", None), "Value", 0.0)
        width = getattr(getattr(obj, "Width", None), "Value", 0.0)
        height = getattr(getattr(obj, "Height", None), "Value", 0.0)
        if min(length, width, height) <= 0.0:
            return None

        api_geometry = self.ifcopenshell.api.geometry
        shape_representation = api_geometry.add_wall_representation(
            self.ifcfile,
            context=context,
            length=length * 0.001,
            height=height * 0.001,
            thickness=width * 0.001,
        )
        representation = self._product_shape(shape_representation)
        return representation, self._placement_at_shape_minimum(obj)

    def _create_rectangular_extrusion(self, context, obj, ifcclass=None):
        """Export an axis-aligned rectangular prism through IfcOpenShell's API."""

        if ifcclass != "IfcOpeningElement" and getattr(obj, "IfcType", None) not in {
            "Beam",
            "Column",
            "Footing",
            "Member",
            "Plate",
            "Slab",
        }:
            return None
        if getattr(obj, "Additions", None) or getattr(obj, "Subtractions", None):
            return None
        shape = getattr(obj, "Shape", None)
        if not shape or len(shape.Solids) != 1 or len(shape.Faces) != 6:
            return None

        bounds = shape.BoundBox
        length, width, depth = bounds.XLength, bounds.YLength, bounds.ZLength
        box_volume = length * width * depth
        if min(length, width, depth) <= 0.0 or box_volume <= 0.0:
            return None
        if abs(shape.Volume - box_volume) > max(1e-6, box_volume * 1e-7):
            return None

        unit_scale = self.ifcopenshell.util.unit.calculate_unit_scale(self.ifcfile)
        coordinate_scale = 0.001 / unit_scale
        profile = self.ifcfile.createIfcRectangleProfileDef(
            "AREA",
            None,
            None,
            length * coordinate_scale,
            width * coordinate_scale,
        )
        shape_representation = self.ifcopenshell.api.geometry.add_profile_representation(
            self.ifcfile,
            context=context,
            profile=profile,
            depth=depth * 0.001,
            cardinal_point=5,
        )
        return self._product_shape(shape_representation), self._placement_at_shape_center_base(obj)

    def _create_circular_extrusion(self, context, obj):
        """Export circular and hollow-circular prisms as parameterized profiles."""

        if getattr(obj, "IfcType", None) not in {
            "Beam",
            "Column",
            "Footing",
            "Member",
            "Plate",
            "Slab",
        }:
            return None
        if getattr(obj, "Additions", None) or getattr(obj, "Subtractions", None):
            return None
        shape = getattr(obj, "Shape", None)
        if not shape or len(shape.Solids) != 1:
            return None

        extrusion = self._find_planar_extrusion(shape)
        if extrusion is None:
            return None
        face, direction, depth = extrusion
        circles = []
        for wire in face.Wires:
            if len(wire.Edges) != 1:
                return None
            curve = wire.Edges[0].Curve
            if getattr(curve, "TypeId", None) != "Part::GeomCircle":
                return None
            circles.append((wire, curve))
        if len(circles) not in (1, 2):
            return None

        outer_index = next(
            (index for index, item in enumerate(circles) if item[0].isSame(face.OuterWire)),
            None,
        )
        if outer_index is None:
            return None
        outer = circles[outer_index][1]
        center = outer.Location
        radius = outer.Radius
        scale = self._coordinate_scale()
        if len(circles) == 1:
            profile = self.ifcfile.createIfcCircleProfileDef("AREA", None, None, radius * scale)
        else:
            inner = circles[1 - outer_index][1]
            tolerance = max(1e-6, radius * 1e-7)
            if center.distanceToPoint(inner.Location) > tolerance:
                return None
            thickness = radius - inner.Radius
            if thickness <= tolerance:
                return None
            profile = self.ifcfile.createIfcCircleHollowProfileDef(
                "AREA", None, None, radius * scale, thickness * scale
            )

        shape_representation = self.ifcopenshell.api.geometry.add_profile_representation(
            self.ifcfile,
            context=context,
            profile=profile,
            depth=depth * 0.001,
            cardinal_point=None,
        )
        placement = self._placement_from_frame(obj, center, outer.XAxis, direction)
        return self._product_shape(shape_representation), placement

    def _create_planar_extrusion(self, context, obj):
        """Export a straight-edged planar profile, including voids."""

        if getattr(obj, "IfcType", None) not in {
            "Beam",
            "Column",
            "Footing",
            "Member",
            "Plate",
            "Slab",
        }:
            return None
        if getattr(obj, "Additions", None) or getattr(obj, "Subtractions", None):
            return None
        shape = getattr(obj, "Shape", None)
        if not shape or len(shape.Solids) != 1:
            return None

        extrusion = self._find_planar_extrusion(shape)
        if extrusion is None:
            return None
        face, direction, depth = extrusion
        frame = self._profile_frame(face, direction)
        if frame is None:
            return None
        origin, x_axis, y_axis = frame
        linear_loops = [self._wire_coordinates(wire, origin, x_axis, y_axis) for wire in face.Wires]

        outer_index = next(
            (index for index, wire in enumerate(face.Wires) if wire.isSame(face.OuterWire)),
            None,
        )
        if outer_index is None:
            return None
        structural = None
        if len(linear_loops) == 1 and linear_loops[0] is not None:
            structural = self._structural_profile(
                linear_loops[outer_index][:-1], origin, x_axis, y_axis
            )
        if structural is not None:
            profile, profile_origin, profile_x_axis = structural
            shape_representation = self.ifcopenshell.api.geometry.add_profile_representation(
                self.ifcfile,
                context=context,
                profile=profile,
                depth=depth * 0.001,
                cardinal_point=None,
            )
            placement = self._placement_from_frame(obj, profile_origin, profile_x_axis, direction)
            return self._product_shape(shape_representation), placement

        curves = [self._wire_curve(wire, origin, x_axis, y_axis) for wire in face.Wires]
        if any(curve is None for curve in curves):
            return None
        outer = curves[outer_index]
        inner = [curve for index, curve in enumerate(curves) if index != outer_index]
        if inner:
            profile = self.ifcfile.createIfcArbitraryProfileDefWithVoids("AREA", None, outer, inner)
        else:
            profile = self.ifcfile.createIfcArbitraryClosedProfileDef("AREA", None, outer)

        shape_representation = self.ifcopenshell.api.geometry.add_profile_representation(
            self.ifcfile,
            context=context,
            profile=profile,
            depth=depth * 0.001,
            cardinal_point=None,
        )
        placement = self._placement_from_frame(obj, origin, x_axis, direction)
        return self._product_shape(shape_representation), placement

    def _structural_profile(self, points, origin, x_axis, y_axis):
        """Recognize exact sharp-cornered I, T, U, and L structural profiles."""

        rotations = (
            (lambda x, y: (x, y), x_axis, y_axis),
            (lambda x, y: (y, -x), y_axis, FreeCAD.Vector(x_axis).negative()),
            (
                lambda x, y: (-x, -y),
                FreeCAD.Vector(x_axis).negative(),
                FreeCAD.Vector(y_axis).negative(),
            ),
            (lambda x, y: (-y, x), FreeCAD.Vector(y_axis).negative(), x_axis),
        )
        for transform, profile_x, profile_y in rotations:
            transformed = [transform(x, y) for x, y in points]
            minimum_x = min(x for x, _y in transformed)
            minimum_y = min(y for _x, y in transformed)
            normalized = [(x - minimum_x, y - minimum_y) for x, y in transformed]
            profile = self._match_structural_profile(normalized)
            if profile is None:
                continue
            maximum_x = max(x for x, _y in normalized)
            maximum_y = max(y for _x, y in normalized)
            scale = self._coordinate_scale()
            center = origin.add(
                FreeCAD.Vector(profile_x).multiply((minimum_x + maximum_x / 2.0) / scale)
            ).add(FreeCAD.Vector(profile_y).multiply((minimum_y + maximum_y / 2.0) / scale))
            return profile, center, profile_x
        return None

    def _match_structural_profile(self, points):
        xs = self._unique_coordinates(x for x, _y in points)
        ys = self._unique_coordinates(y for _x, y in points)
        if len(points) == 12 and len(xs) == 4 and len(ys) == 4:
            width, depth = xs[-1], ys[-1]
            web = xs[2] - xs[1]
            bottom_flange = ys[1]
            top_flange = depth - ys[2]
            expected = {
                (0, 0),
                (width, 0),
                (width, bottom_flange),
                (xs[2], bottom_flange),
                (xs[2], ys[2]),
                (width, ys[2]),
                (width, depth),
                (0, depth),
                (0, ys[2]),
                (xs[1], ys[2]),
                (xs[1], bottom_flange),
                (0, bottom_flange),
            }
            if self._same_points(points, expected) and self._close(bottom_flange, top_flange):
                return self.ifcfile.create_entity(
                    "IfcIShapeProfileDef",
                    ProfileType="AREA",
                    OverallWidth=width,
                    OverallDepth=depth,
                    WebThickness=web,
                    FlangeThickness=bottom_flange,
                )
        if len(points) == 8 and len(xs) == 4 and len(ys) == 3:
            width, depth = xs[-1], ys[-1]
            web = xs[2] - xs[1]
            flange = depth - ys[1]
            expected = {
                (0, depth),
                (width, depth),
                (width, ys[1]),
                (xs[2], ys[1]),
                (xs[2], 0),
                (xs[1], 0),
                (xs[1], ys[1]),
                (0, ys[1]),
            }
            if self._same_points(points, expected):
                return self.ifcfile.create_entity(
                    "IfcTShapeProfileDef",
                    ProfileType="AREA",
                    Depth=depth,
                    FlangeWidth=width,
                    WebThickness=web,
                    FlangeThickness=flange,
                )
        if len(points) == 8 and len(xs) == 3 and len(ys) == 4:
            width, depth = xs[-1], ys[-1]
            web = xs[1]
            bottom_flange = ys[1]
            top_flange = depth - ys[2]
            expected = {
                (0, 0),
                (width, 0),
                (width, bottom_flange),
                (web, bottom_flange),
                (web, ys[2]),
                (width, ys[2]),
                (width, depth),
                (0, depth),
            }
            if self._same_points(points, expected) and self._close(bottom_flange, top_flange):
                return self.ifcfile.create_entity(
                    "IfcUShapeProfileDef",
                    ProfileType="AREA",
                    Depth=depth,
                    FlangeWidth=width,
                    WebThickness=web,
                    FlangeThickness=bottom_flange,
                )
        if len(points) == 6 and len(xs) == 3 and len(ys) == 3:
            width, depth = xs[-1], ys[-1]
            vertical_thickness = xs[1]
            horizontal_thickness = ys[1]
            expected = {
                (0, 0),
                (width, 0),
                (width, horizontal_thickness),
                (vertical_thickness, horizontal_thickness),
                (vertical_thickness, depth),
                (0, depth),
            }
            if self._same_points(points, expected) and self._close(
                vertical_thickness, horizontal_thickness
            ):
                return self.ifcfile.create_entity(
                    "IfcLShapeProfileDef",
                    ProfileType="AREA",
                    Depth=depth,
                    Width=width,
                    Thickness=vertical_thickness,
                )
        return None

    def _unique_coordinates(self, coordinates):
        result = []
        for value in sorted(coordinates):
            if not result or not self._close(value, result[-1]):
                result.append(value)
        return result

    def _same_points(self, actual, expected):
        return len(actual) == len(expected) and all(
            any(self._close(x, ex) and self._close(y, ey) for ex, ey in expected) for x, y in actual
        )

    def _close(self, first, second):
        return abs(first - second) <= max(1e-9, max(abs(first), abs(second)) * 1e-7)

    def _find_planar_extrusion(self, shape):
        planar = [
            face
            for face in shape.Faces
            if getattr(face.Surface, "TypeId", None) == "Part::GeomPlane"
        ]
        for first_index, first in enumerate(planar):
            first_vertices = [vertex.Point for vertex in first.Vertexes]
            for second in planar[first_index + 1 :]:
                if len(first_vertices) != len(second.Vertexes):
                    continue
                offset = second.CenterOfMass.sub(first.CenterOfMass)
                depth = offset.Length
                if depth <= 1e-7:
                    continue
                direction = FreeCAD.Vector(offset).normalize()
                normal = first.normalAt(0, 0)
                if abs(abs(normal.dot(direction)) - 1.0) > 1e-7:
                    continue
                tolerance = max(1e-6, depth * 1e-7)
                translated = [point.add(offset) for point in first_vertices]
                second_vertices = [vertex.Point for vertex in second.Vertexes]
                if not all(
                    any(point.distanceToPoint(other) <= tolerance for other in second_vertices)
                    for point in translated
                ):
                    continue
                expected_volume = first.Area * depth
                if abs(shape.Volume - expected_volume) <= max(1e-5, expected_volume * 1e-7):
                    return first, direction, depth
        return None

    def _profile_frame(self, face, direction):
        outer = face.OuterWire
        if not outer.Edges:
            return None
        edge = next(
            (
                candidate
                for candidate in outer.Edges
                if getattr(candidate.Curve, "TypeId", None) == "Part::GeomLine"
            ),
            None,
        )
        if edge is None:
            return None
        origin = edge.Vertexes[0].Point
        x_axis = edge.Vertexes[-1].Point.sub(origin)
        if x_axis.Length <= 1e-7:
            return None
        x_axis.normalize()
        y_axis = direction.cross(x_axis)
        if y_axis.Length <= 1e-7:
            return None
        y_axis.normalize()
        return origin, x_axis, y_axis

    def _wire_coordinates(self, wire, origin, x_axis, y_axis):
        if any(getattr(edge.Curve, "TypeId", None) != "Part::GeomLine" for edge in wire.Edges):
            return None
        vertices = wire.OrderedVertexes
        if len(vertices) < 3:
            return None
        scale = self._coordinate_scale()
        coordinates = []
        for vertex in vertices:
            relative = vertex.Point.sub(origin)
            coordinates.append((relative.dot(x_axis) * scale, relative.dot(y_axis) * scale))
        coordinates.append(coordinates[0])
        return coordinates

    def _polyline(self, coordinates):
        points = [self.ifcfile.createIfcCartesianPoint(point) for point in coordinates]
        return self.ifcfile.createIfcPolyline(points)

    def _wire_curve(self, wire, origin, x_axis, y_axis):
        curve_types = {getattr(edge.Curve, "TypeId", None) for edge in wire.Edges}
        if not curve_types.issubset({"Part::GeomLine", "Part::GeomCircle"}):
            return None
        if curve_types == {"Part::GeomLine"}:
            return self._polyline(self._wire_coordinates(wire, origin, x_axis, y_axis))
        if not self.ifcfile.wrapped_data.schema_name().startswith("IFC4"):
            return None

        points = []
        segments = []
        current = None
        for edge in wire.OrderedEdges:
            start = edge.valueAt(edge.FirstParameter)
            end = edge.valueAt(edge.LastParameter)
            if current is not None and end.distanceToPoint(current) < start.distanceToPoint(
                current
            ):
                start, end = end, start
            start_index = len(points) + 1
            points.append(self._profile_coordinate(start, origin, x_axis, y_axis))
            if getattr(edge.Curve, "TypeId", None) == "Part::GeomCircle":
                middle = edge.valueAt((edge.FirstParameter + edge.LastParameter) / 2.0)
                points.append(self._profile_coordinate(middle, origin, x_axis, y_axis))
                points.append(self._profile_coordinate(end, origin, x_axis, y_axis))
                segments.append(
                    self.ifcfile.createIfcArcIndex((start_index, start_index + 1, start_index + 2))
                )
            else:
                points.append(self._profile_coordinate(end, origin, x_axis, y_axis))
                segments.append(self.ifcfile.createIfcLineIndex((start_index, start_index + 1)))
            current = end
        point_list = self.ifcfile.createIfcCartesianPointList2D(points)
        return self.ifcfile.createIfcIndexedPolyCurve(point_list, segments, False)

    def _profile_coordinate(self, point, origin, x_axis, y_axis):
        relative = point.sub(origin)
        scale = self._coordinate_scale()
        return relative.dot(x_axis) * scale, relative.dot(y_axis) * scale

    def _product_shape(self, shape_representation):
        return self.ifcfile.createIfcProductDefinitionShape(None, None, [shape_representation])

    def _placement_at_shape_minimum(self, obj):
        """Place API geometry at the lower corner of the existing FreeCAD shape."""

        unit_scale = self.ifcopenshell.util.unit.calculate_unit_scale(self.ifcfile)
        coordinate_scale = 0.001 / unit_scale
        bounds = obj.Shape.BoundBox
        local_origin = FreeCAD.Vector(bounds.XMin, bounds.YMin, bounds.ZMin)
        parent_placement = self._parent_placement(obj)
        origin = parent_placement.multVec(local_origin).multiply(coordinate_scale)
        x_axis = parent_placement.Rotation.multVec(FreeCAD.Vector(1, 0, 0))
        z_axis = parent_placement.Rotation.multVec(FreeCAD.Vector(0, 0, 1))

        point = self.ifcfile.createIfcCartesianPoint(tuple(origin))
        direction_z = self.ifcfile.createIfcDirection(tuple(z_axis))
        direction_x = self.ifcfile.createIfcDirection(tuple(x_axis))
        axis = self.ifcfile.createIfcAxis2Placement3D(point, direction_z, direction_x)
        return self.ifcfile.createIfcLocalPlacement(None, axis)

    def _placement_at_shape_center_base(self, obj):
        """Place centered profile geometry at the base of a FreeCAD shape."""

        bounds = obj.Shape.BoundBox
        local_origin = FreeCAD.Vector(
            (bounds.XMin + bounds.XMax) / 2.0,
            (bounds.YMin + bounds.YMax) / 2.0,
            bounds.ZMin,
        )
        parent_placement = self._parent_placement(obj)
        origin = parent_placement.multVec(local_origin).multiply(self._coordinate_scale())
        x_axis = parent_placement.Rotation.multVec(FreeCAD.Vector(1, 0, 0))
        z_axis = parent_placement.Rotation.multVec(FreeCAD.Vector(0, 0, 1))

        point = self.ifcfile.createIfcCartesianPoint(tuple(origin))
        direction_z = self.ifcfile.createIfcDirection(tuple(z_axis))
        direction_x = self.ifcfile.createIfcDirection(tuple(x_axis))
        axis = self.ifcfile.createIfcAxis2Placement3D(point, direction_z, direction_x)
        return self.ifcfile.createIfcLocalPlacement(None, axis)

    def _placement_from_frame(self, obj, origin, x_axis, z_axis):
        parent_placement = self._parent_placement(obj)
        global_origin = parent_placement.multVec(origin).multiply(self._coordinate_scale())
        global_x = parent_placement.Rotation.multVec(x_axis)
        global_z = parent_placement.Rotation.multVec(z_axis)
        point = self.ifcfile.createIfcCartesianPoint(tuple(global_origin))
        direction_z = self.ifcfile.createIfcDirection(tuple(global_z))
        direction_x = self.ifcfile.createIfcDirection(tuple(global_x))
        axis = self.ifcfile.createIfcAxis2Placement3D(point, direction_z, direction_x)
        return self.ifcfile.createIfcLocalPlacement(None, axis)

    def _shape_coordinate_placement(self, obj):
        """Place geometry whose coordinates already include obj.Placement."""

        parent = self._parent_placement(obj)
        origin = FreeCAD.Vector(parent.Base).multiply(self._coordinate_scale())
        x_axis = parent.Rotation.multVec(FreeCAD.Vector(1, 0, 0))
        z_axis = parent.Rotation.multVec(FreeCAD.Vector(0, 0, 1))
        point = self.ifcfile.createIfcCartesianPoint(tuple(origin))
        direction_z = self.ifcfile.createIfcDirection(tuple(z_axis))
        direction_x = self.ifcfile.createIfcDirection(tuple(x_axis))
        axis = self.ifcfile.createIfcAxis2Placement3D(point, direction_z, direction_x)
        return self.ifcfile.createIfcLocalPlacement(None, axis)

    def _coordinate_scale(self):
        unit_scale = self.ifcopenshell.util.unit.calculate_unit_scale(self.ifcfile)
        return 0.001 / unit_scale

    def _parent_placement(self, obj):
        """Return only container placement; Shape already includes obj.Placement."""

        return self._global_placement(obj).multiply(obj.Placement.inverse())

    def create_annotation(self, context, obj, history, preferences):
        """Convert a FreeCAD annotation directly to semantic IFC geometry."""

        annotation_type = getattr(getattr(obj, "Proxy", None), "Type", "")
        object_type = None
        identifier = "Annotation"
        representation_type = "Annotation2D"

        if annotation_type == "SectionPlane":
            items, placement = self._section_plane(obj)
            object_type = "DRAWING"
            identifier = "Body"
            representation_type = "CSG"
        elif annotation_type in ("Text", "DraftText") or obj.isDerivedFrom("App::Annotation"):
            items, placement = self._annotation_text(obj)
            object_type = "TEXT"
        elif annotation_type in ("Dimension", "LinearDimension", "AngularDimension"):
            items, placement = self._annotation_dimension(obj)
            object_type = "DIMENSION"
        elif obj.isDerivedFrom("Part::Feature"):
            items = self._annotation_shape(obj)
            placement = self._identity_placement()
            object_type = "AREA" if obj.Shape.Faces else "LINEWORK"
        else:
            return None

        shape_representation = self.ifcfile.createIfcShapeRepresentation(
            context,
            identifier,
            representation_type,
            items,
        )
        product_shape = self._product_shape(shape_representation)
        return self.ifcfile.createIfcAnnotation(
            self.ifcopenshell.guid.new(),
            history,
            obj.Label,
            getattr(obj, "Description", None),
            object_type,
            placement,
            product_shape,
        )

    def _annotation_shape(self, obj):
        curves = []
        included_edges = set()
        for wire in obj.Shape.Wires:
            curve = self._annotation_edges(wire.OrderedEdges)
            if curve:
                curves.append(curve)
            included_edges.update(edge.hashCode() for edge in wire.Edges)
        for edge in obj.Shape.Edges:
            if edge.hashCode() not in included_edges:
                curve = self._annotation_edges([edge])
                if curve:
                    curves.append(curve)
        return [self.ifcfile.createIfcGeometricCurveSet(curves)] if curves else []

    def _annotation_edges(self, edges):
        if not self.ifcfile.wrapped_data.schema_name().startswith("IFC4"):
            points = []
            for edge in edges:
                edge_points = edge.discretize(Deflection=0.1)
                if points and edge_points:
                    if points[-1].distanceToPoint(edge_points[-1]) < points[-1].distanceToPoint(
                        edge_points[0]
                    ):
                        edge_points.reverse()
                    if points[-1].distanceToPoint(edge_points[0]) < 1e-7:
                        edge_points = edge_points[1:]
                points.extend(edge_points)
            return self._polyline([self._annotation_coordinate(point) for point in points])

        points = []
        segments = []
        current = None
        for edge in edges:
            start = edge.valueAt(edge.FirstParameter)
            end = edge.valueAt(edge.LastParameter)
            if current is not None and end.distanceToPoint(current) < start.distanceToPoint(
                current
            ):
                start, end = end, start
            edge_points = [start]
            is_circle = getattr(edge.Curve, "TypeId", None) == "Part::GeomCircle"
            if is_circle:
                span = edge.LastParameter - edge.FirstParameter
                edge_points.append(edge.valueAt(edge.FirstParameter + span / 2.0))
                if start.distanceToPoint(end) < 1e-7:
                    edge_points.insert(1, edge.valueAt(edge.FirstParameter + span / 4.0))
                    edge_points.append(edge.valueAt(edge.FirstParameter + 3.0 * span / 4.0))
            elif getattr(edge.Curve, "TypeId", None) != "Part::GeomLine":
                edge_points = edge.discretize(Deflection=0.1)
            edge_points.append(end)
            start_index = len(points) + 1
            points.extend(self._annotation_coordinate(point) for point in edge_points)
            if is_circle:
                if len(edge_points) == 5:
                    segments.extend(
                        (
                            self.ifcfile.createIfcArcIndex(
                                (start_index, start_index + 1, start_index + 2)
                            ),
                            self.ifcfile.createIfcArcIndex(
                                (start_index + 2, start_index + 3, start_index + 4)
                            ),
                        )
                    )
                else:
                    segments.append(
                        self.ifcfile.createIfcArcIndex(
                            (start_index, start_index + 1, start_index + 2)
                        )
                    )
            else:
                segments.append(
                    self.ifcfile.createIfcLineIndex(
                        tuple(range(start_index, start_index + len(edge_points)))
                    )
                )
            current = end
        if not points:
            return None
        point_list = self.ifcfile.createIfcCartesianPointList3D(points)
        return self.ifcfile.createIfcIndexedPolyCurve(point_list, segments, False)

    def _annotation_coordinate(self, point):
        return tuple(FreeCAD.Vector(point).multiply(self._coordinate_scale()))

    def _annotation_text(self, obj):
        if obj.isDerivedFrom("App::Annotation"):
            text = ";".join(obj.LabelText)
            placement = FreeCAD.Placement(FreeCAD.Vector(obj.Position), FreeCAD.Rotation())
        else:
            text = ";".join(obj.Text)
            placement = self._global_placement(obj)
        literal_placement = self.ifcfile.createIfcAxis2Placement3D(
            self.ifcfile.createIfcCartesianPoint((0.0, 0.0, 0.0)), None, None
        )
        literal = self.ifcfile.createIfcTextLiteral(text, literal_placement, "LEFT")
        return [literal], self._placement(placement)

    def _annotation_dimension(self, obj):
        start = FreeCAD.Vector(obj.Start)
        end = FreeCAD.Vector(obj.End)
        dimline = FreeCAD.Vector(obj.Dimline)
        direction = end.sub(start)
        if direction.Length:
            direction.normalize()
            first = dimline.add(direction * start.sub(dimline).dot(direction))
            second = dimline.add(direction * end.sub(dimline).dot(direction))
        else:
            first, second = start, end
        curve = self._annotation_polycurve([first, second])
        curve_set = self.ifcfile.createIfcGeometricCurveSet([curve])
        midpoint = first.add(second).multiply(0.5)
        text_placement = self.ifcfile.createIfcAxis2Placement3D(
            self.ifcfile.createIfcCartesianPoint(self._annotation_coordinate(midpoint)), None, None
        )
        text = str(getattr(obj, "Distance", start.distanceToPoint(end)))
        literal = self.ifcfile.createIfcTextLiteral(text, text_placement, "LEFT")
        return [curve_set, literal], self._identity_placement()

    def _annotation_polycurve(self, points):
        coordinates = [self._annotation_coordinate(point) for point in points]
        if self.ifcfile.wrapped_data.schema_name().startswith("IFC4"):
            point_list = self.ifcfile.createIfcCartesianPointList3D(coordinates)
            return self.ifcfile.createIfcIndexedPolyCurve(point_list, None, False)
        return self._polyline(coordinates)

    def _section_plane(self, obj):
        length = getattr(getattr(obj, "ViewObject", None), "DisplayLength", 1000)
        height = getattr(getattr(obj, "ViewObject", None), "DisplayHeight", 1000)
        depth = getattr(obj, "Depth", 1000)
        dimensions = [
            float(getattr(value, "Value", value)) * self._coordinate_scale()
            for value in (length, height, depth)
        ]
        position = self.ifcfile.createIfcCartesianPoint(
            (-dimensions[0] / 2, -dimensions[1] / 2, -dimensions[2])
        )
        axis = self.ifcfile.createIfcAxis2Placement3D(position, None, None)
        block = self.ifcfile.createIfcBlock(axis, *dimensions)
        return [self.ifcfile.createIfcCsgSolid(block)], self._placement(self._global_placement(obj))

    def _identity_placement(self):
        return self._placement(FreeCAD.Placement())

    def _global_placement(self, obj):
        getter = getattr(obj, "getGlobalPlacement", None)
        return getter() if getter else obj.Placement

    def _placement(self, placement):
        origin = FreeCAD.Vector(placement.Base).multiply(self._coordinate_scale())
        z_axis = placement.Rotation.multVec(FreeCAD.Vector(0, 0, 1))
        x_axis = placement.Rotation.multVec(FreeCAD.Vector(1, 0, 0))
        axis = self.ifcfile.createIfcAxis2Placement3D(
            self.ifcfile.createIfcCartesianPoint(tuple(origin)),
            self.ifcfile.createIfcDirection(tuple(z_axis)),
            self.ifcfile.createIfcDirection(tuple(x_axis)),
        )
        return self.ifcfile.createIfcLocalPlacement(None, axis)
