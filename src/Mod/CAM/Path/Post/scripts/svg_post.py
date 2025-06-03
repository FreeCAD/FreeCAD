# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2024 Ondsel <development@ondsel.com>                    *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

from Path.Post.Processor import PostProcessor
import Path
import FreeCAD
import math

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate

debug = False
if debug:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

# Define colors for the layers
LAYER_COLORS = {"CUT": "red", "ENGRAVE": "blue", "FILL": "green", "DEFAULT": "black"}


class Svg(PostProcessor):
    def __init__(self, job):

        super().__init__(
            job,
            tooltip=translate("CAM", "SVG post processor"),
            tooltipargs=[],
            units="mm",
        )
        Path.Log.debug("SVG post processor initialized")

    def export(self):
        Path.Log.debug("Exporting the job")

        use_layers = "--layers" in self._job.PostProcessorArgs

        postables = self._buildPostList()
        Path.Log.debug(f"postables count: {len(postables)}")

        svg_strings = []
        for idx, section in enumerate(postables):
            svg_content = self.create_svg_section(section, idx, use_layers)
            svg_strings.append((idx, svg_content))

        return svg_strings

    def create_svg_section(self, section, idx, use_layers):
        Path.Log.track()
        partname, sublist = section

        # Initialize bounding box
        xmin, ymin, xmax, ymax = self.calculate_bounding_box(sublist)

        if xmin is None or ymin is None or xmax is None or ymax is None:
            Path.Log.debug("No wires found, skipping section")
            return ""

        width = xmax - xmin
        height = ymax - ymin

        # Create the SVG header with a normalized viewBox
        svg_content = (
            f'<svg xmlns="http://www.w3.org/2000/svg" '
            f'xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" '
            f'width="{width}mm" height="{height}mm" viewBox="0 0 {width} {height}">\n'
        )

        # Convert each wire to an SVG path
        for obj_idx, obj in enumerate(sublist):

            strokestyle = self.get_stroke_style(obj)
            color = strokestyle["color"]
            width = strokestyle["width"]
            pathtype = strokestyle["label"]
            Path.Log.debug(pathtype)

            wires = Path.Geom.wiresForPath(obj.Path)
            if not wires:
                continue  # Skip objects that do not produce wires

            if use_layers:
                layer_id = f"layer{idx}_{obj_idx}"
                svg_content += (
                    f'<g id="{layer_id}" inkscape:label="{obj.Label}" inkscape:groupmode="layer">\n'
                )

            for wire in wires:
                path_data = self.wire_to_svg_path(wire, width, height, xmin, ymin)
                if pathtype == "FILL":
                    svg_content += f'  <path d="{path_data}" stroke="none" fill="{color}" />\n'
                else:
                    svg_content += f'  <path d="{path_data}" stroke="{color}" stroke-width="{width}" fill="none" />\n'

            if use_layers:
                svg_content += "</g>\n"

        # Close the SVG tag
        svg_content += "</svg>"

        return svg_content

    def wire_to_svg_path(self, wire, width, height, xmin, ymin):
        """Convert FreeCAD wire to SVG path data with y-axis inversion and limited precision"""
        path_data = ""
        is_first_point = True
        vertices_info = []

        def format_coord(value):
            return f"{value:.4f}"

        for edge in wire.Edges:
            start_point = edge.Vertexes[0].Point
            end_point = edge.Vertexes[-1].Point
            Path.Log.debug(f"Edge Type: {edge.Curve.TypeId}")
            Path.Log.debug(
                f"Start Point: ({format_coord(start_point.x)}, {format_coord(start_point.y)})"
            )
            Path.Log.debug(f"End Point: ({format_coord(end_point.x)}, {format_coord(end_point.y)})")

            # Check if the edge is vertical (should be skipped)
            if start_point.x == end_point.x and start_point.y == end_point.y:
                Path.Log.debug("Skipping vertical edge")
                continue

            if is_first_point:
                path_data += f"M {format_coord(start_point.x - xmin)} {format_coord(height - (start_point.y - ymin))} "
                vertices_info.append(
                    f"M {format_coord(start_point.x - xmin)} {format_coord(height - (start_point.y - ymin))}"
                )
                is_first_point = False

            if edge.Curve.TypeId in ["Part::GeomLineSegment", "Part::GeomLine"]:
                # Handle line segment without discretization
                path_data += f"L {format_coord(end_point.x - xmin)} {format_coord(height - (end_point.y - ymin))} "
                vertices_info.append(
                    f"L {format_coord(end_point.x - xmin)} {format_coord(height - (end_point.y - ymin))}"
                )
                Path.Log.debug(
                    f"Line segment from ({format_coord(start_point.x)}, {format_coord(start_point.y)}) to ({format_coord(end_point.x)}, {format_coord(end_point.y)})"
                )
            elif edge.Curve.TypeId in ["Part::GeomCircle", "Part::GeomArcOfCircle"]:
                # Handle circular arc using 'A' command
                radius = edge.Curve.Radius
                center = edge.Curve.Center
                start_angle = math.atan2(start_point.y - center.y, start_point.x - center.x)
                end_angle = math.atan2(end_point.y - center.y, end_point.x - center.x)

                # Calculate the angle difference and normalize to [-π, π]
                angle_diff = (end_angle - start_angle) % (2 * math.pi)
                if angle_diff > math.pi:
                    angle_diff -= 2 * math.pi

                Path.Log.debug(f"Angle difference: {angle_diff:.2f} radians")

                # Determine the large_arc_flag and sweep_flag
                large_arc_flag = 1 if abs(angle_diff) > (math.pi) else 0
                sweep_flag = 0 if angle_diff > 0 else 1

                start_x = format_coord(start_point.x - xmin)
                start_y = format_coord(height - (start_point.y - ymin))
                end_x = format_coord(end_point.x - xmin)
                end_y = format_coord(height - (end_point.y - ymin))

                path_data += f"A {format_coord(radius)} {format_coord(radius)} 0 {large_arc_flag} {sweep_flag} {end_x} {end_y} "
                vertices_info.append(
                    f"A {format_coord(radius)} {format_coord(radius)} 0 {large_arc_flag} {sweep_flag} {end_x} {end_y}"
                )
                Path.Log.debug(
                    f"Circular arc with radius {format_coord(radius)} from ({start_x}, {start_y}) to ({end_x}, {end_y}) (large_arc_flag: {large_arc_flag}, sweep_flag: {sweep_flag})"
                )
                Path.Log.debug(path_data)
            else:
                # Discretize other types of edges into 100 segments
                vertices = edge.discretize(100)
                if len(vertices) < 2:
                    continue
                if all(vertices[0].x == v.x and vertices[0].y == v.y for v in vertices[1:]):
                    continue
                for vertex in vertices:
                    path_data += f"L {format_coord(vertex.x - xmin)} {format_coord(height - (vertex.y - ymin))} "
                    vertices_info.append(
                        f"L {format_coord(vertex.x - xmin)} {format_coord(height - (vertex.y - ymin))}"
                    )
                Path.Log.debug(f"Discretized edge with {len(vertices)} points")

        path_data += "Z "  # Close the path for filled edges
        vertices_info.append("Z")
        Path.Log.debug(f"SVG Path: {path_data.strip()}")
        Path.Log.debug(f"Vertices Info: {vertices_info}")
        return path_data.strip()

    def calculate_bounding_box(self, sublist):
        xmin, ymin, xmax, ymax = None, None, None, None
        for obj in sublist:
            wires = Path.Geom.wiresForPath(obj.Path)
            for wire in wires:
                for vertex in wire.Vertexes:
                    x, y = vertex.X, vertex.Y
                    if xmin is None or x < xmin:
                        xmin = x
                    if xmax is None or x > xmax:
                        xmax = x
                    if ymin is None or y < ymin:
                        ymin = y
                    if ymax is None or y > ymax:
                        ymax = y
        return xmin, ymin, xmax, ymax

    def get_stroke_style(self, obj):
        strokewidth = 0.1
        color = LAYER_COLORS["DEFAULT"]
        label = None

        if hasattr(obj, "ToolController"):
            tc = obj.ToolController
            if hasattr(tc, "Tool") and hasattr(tc.Tool, "Diameter"):
                strokewidth = tc.Tool.Diameter
            if hasattr(tc, "Label"):
                for key in LAYER_COLORS:
                    if key in tc.Label:
                        label = key
                        color = LAYER_COLORS[key]
                        break

        return {"width": strokewidth, "color": color, "label": label}

    @property
    def tooltip(self):
        tooltip = """
        This is the Ondsel SVG CAM post processor.
        It will export a CAM job to an SVG file with colors and layers.

        Tool Controllers determine how the svg will be created and, thus, how the
        laser will behave.  They must have a label containing a string from the list at the top of the post file.
        (e.g. CUT, FILL, ENGRAVE). SVG Paths will be color coded similarly

        Actual laser behavior will depend on how the laser controller is configured
        to process the colors / layers.

        Step-downs in operations will result in multiple passes.


        """
        return tooltip

    @property
    def tooltipArgs(self):
        argtooltip = """
        --layers: Output will be written to different layers. Layer names are taken from the operation label
        """
        return argtooltip

    @property
    def preferredExtension(self):
        return "svg"
