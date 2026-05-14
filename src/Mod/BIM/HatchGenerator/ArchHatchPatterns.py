# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                          *
# *   Copyright (c) 2026 Regis Benoit Brice Nde Tene <regisndetene@gmail.com>*
# *                                                                          *
# *   This file is part of FreeCAD.                                         *
# *                                                                          *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                          *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                          *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                          *
# ***************************************************************************

import FreeCAD
import Part
import math
import random


def make_rectangle(width, height):
    """Creates a Face covering width x height in the XY plane."""
    p0 = FreeCAD.Vector(0, 0, 0)
    p1 = FreeCAD.Vector(width, 0, 0)
    p2 = FreeCAD.Vector(width, height, 0)
    p3 = FreeCAD.Vector(0, height, 0)
    polygon = Part.makePolygon([p0, p1, p2, p3, p0])
    face = Part.Face(polygon)
    return face


def generate_built_in_pattern_shape(pattern_type):
    """Generate built-in patterns for demonstration."""
    grid_size = 10
    hex_y = math.sqrt(3)
    tile_size = 2.5

    base_rect = make_rectangle(10, 10)

    if pattern_type == "SolidFill":
        return base_rect

    elif pattern_type == "HorizontalLines":
        components = []
        for y in range(0, grid_size + 1, 2):
            components.append(
                Part.makeLine(FreeCAD.Vector(0, y, 0), FreeCAD.Vector(grid_size, y, 0))
            )
        return Part.makeCompound(components)

    elif pattern_type == "VerticalLines":
        components = []
        for x in range(0, grid_size + 1, 2):
            components.append(
                Part.makeLine(FreeCAD.Vector(x, 0, 0), FreeCAD.Vector(x, grid_size, 0))
            )
        return Part.makeCompound(components)

    elif pattern_type == "Crosshatch":
        horizontal = generate_built_in_pattern_shape("HorizontalLines")
        vertical = generate_built_in_pattern_shape("VerticalLines")
        return horizontal.fuse(vertical)

    elif pattern_type == "Herringbone":
        components = []
        for row in range(0, grid_size, 2):
            line1 = Part.makeLine(FreeCAD.Vector(0, row, 0), FreeCAD.Vector(grid_size, row + 2, 0))
            line2 = Part.makeLine(FreeCAD.Vector(grid_size, row, 0), FreeCAD.Vector(0, row + 2, 0))
            components.extend([line1, line2])
        return Part.makeCompound(components)

    elif pattern_type == "BrickPattern":
        components = []
        for y in [0, 5, 10]:
            components.append(
                Part.makeLine(FreeCAD.Vector(0, y, 0), FreeCAD.Vector(grid_size, y, 0))
            )
        for x in [0, 5, 10]:
            components.append(Part.makeLine(FreeCAD.Vector(x, 0, 0), FreeCAD.Vector(x, 5, 0)))
        for x in [2.5, 7.5]:
            components.append(Part.makeLine(FreeCAD.Vector(x, 5, 0), FreeCAD.Vector(x, 10, 0)))
        return Part.makeCompound(components)

    elif pattern_type == "RandomDots":
        components = []
        for _ in range(10):
            x = random.uniform(0, grid_size)
            y = random.uniform(0, grid_size)
            components.append(Part.makeCircle(0.5, FreeCAD.Vector(x, y, 0)))
        return Part.makeCompound(components)

    elif pattern_type == "OverlappingSquares":
        components = []
        tile_size_int = int(tile_size)
        for x in range(0, grid_size, tile_size_int):
            for y in range(0, grid_size, tile_size_int):
                if ((x // tile_size_int) + (y // tile_size_int)) % 2 == 0:
                    square = make_rectangle(tile_size, tile_size)
                    matrix = FreeCAD.Matrix()
                    matrix.move(FreeCAD.Vector(x, y, 0))
                    shifted = square.copy()
                    shifted.transformShape(matrix)
                    components.append(shifted)
        return Part.makeCompound(components)

    elif pattern_type == "Checkerboard":
        components = []
        tile_size_val = float(tile_size)
        num_tiles = int(grid_size / tile_size_val)

        for row in range(num_tiles):
            for col in range(num_tiles):
                if (row + col) % 2 == 0:
                    square = make_rectangle(tile_size_val, tile_size_val)
                    placement = FreeCAD.Placement()
                    x_shift = col * tile_size_val + tile_size_val / 2
                    y_shift = row * tile_size_val + tile_size_val / 2
                    placement.move(FreeCAD.Vector(x_shift, y_shift, 0))
                    square.Placement = placement
                    components.append(square)
        return Part.makeCompound(components)

    elif pattern_type == "CheckerboardCircles":
        components = []
        tile_size_val = 3.0
        for x in range(0, grid_size, int(tile_size_val)):
            for y in range(0, grid_size, int(tile_size_val)):
                if (x // tile_size_val + y // tile_size_val) % 2 == 0:
                    components.append(
                        Part.makeCircle(
                            tile_size_val / 3,
                            FreeCAD.Vector(x + tile_size_val / 2, y + tile_size_val / 2, 0),
                        )
                    )
        return Part.makeCompound(components)

    elif pattern_type == "RotatingHexagons":

        def create_hexagon(center, size, rotation):
            points = []
            for i in range(7):
                angle = math.radians(60 * i + rotation)
                point = center + FreeCAD.Vector(size * math.cos(angle), size * math.sin(angle), 0)
                points.append(point)
            return Part.makePolygon(points)

        shapes = []
        spacing = grid_size / 6
        size = spacing / 2

        for row in range(7):
            offset = spacing / 2 if row % 2 else 0
            for col in range(7):
                center = FreeCAD.Vector(col * spacing + offset, row * spacing * 0.866, 0)
                rotation = 30 * ((row + col) % 4)
                shapes.append(create_hexagon(center, size, rotation))

        return Part.makeCompound(shapes)

    elif pattern_type == "NestedTriangles":

        def create_nested_triangle(center, size, levels):
            shapes = []
            for i in range(levels):
                scale = 1 - (i * 0.25)
                current_size = size * scale
                points = []
                for j in range(4):
                    angle = math.radians(120 * j + 30)
                    point = center + FreeCAD.Vector(
                        current_size * math.cos(angle), current_size * math.sin(angle), 0
                    )
                    points.append(point)
                shapes.append(Part.makePolygon(points))
            return shapes

        patterns = []
        spacing = grid_size / 4
        size = spacing / 2

        for row in range(5):
            offset = spacing / 2 if row % 2 else 0
            for col in range(5):
                center = FreeCAD.Vector(col * spacing + offset, row * spacing, 0)
                patterns.extend(create_nested_triangle(center, size, 3))

        return Part.makeCompound(patterns)

    elif pattern_type == "InterlockingCircles":

        def create_circle_pattern(center, radius):
            shapes = []
            steps = 32

            points = []
            for i in range(steps + 1):
                angle = 2 * math.pi * i / steps
                point = center + FreeCAD.Vector(
                    radius * math.cos(angle), radius * math.sin(angle), 0
                )
                points.append(point)
            shapes.append(Part.makePolygon(points))

            for i in range(6):
                arc_points = []
                angle_offset = i * math.pi / 3
                arc_center = center + FreeCAD.Vector(
                    radius * math.cos(angle_offset), radius * math.sin(angle_offset), 0
                )

                for j in range(steps // 3 + 1):
                    angle = angle_offset + math.pi / 2 + (math.pi * j / (steps // 3))
                    point = arc_center + FreeCAD.Vector(
                        radius * math.cos(angle), radius * math.sin(angle), 0
                    )
                    arc_points.append(point)
                shapes.append(Part.makePolygon(arc_points))

            return shapes

        patterns = []
        spacing = grid_size / 3
        radius = spacing / 2

        for row in range(4):
            offset = spacing / 2 if row % 2 else 0
            for col in range(4):
                center = FreeCAD.Vector(col * spacing + offset, row * spacing, 0)
                patterns.extend(create_circle_pattern(center, radius))

        return Part.makeCompound(patterns)

    elif pattern_type == "RecursiveSquares":

        def create_recursive_square(center, size, depth):
            if depth == 0:
                return []

            shapes = []
            points = []

            for i in range(5):
                angle = math.radians(90 * i + 45)
                point = center + FreeCAD.Vector(size * math.cos(angle), size * math.sin(angle), 0)
                points.append(point)
            shapes.append(Part.makePolygon(points))

            new_size = size * 0.4
            for i in range(4):
                angle = math.radians(90 * i + 45)
                new_center = center + FreeCAD.Vector(
                    size * 0.7 * math.cos(angle), size * 0.7 * math.sin(angle), 0
                )
                shapes.extend(create_recursive_square(new_center, new_size, depth - 1))

            return shapes

        patterns = []
        spacing = grid_size / 3
        size = spacing / 2

        for row in range(3):
            for col in range(3):
                center = FreeCAD.Vector(col * spacing + spacing / 2, row * spacing + spacing / 2, 0)
                patterns.extend(create_recursive_square(center, size, 3))

        return Part.makeCompound(patterns)

    elif pattern_type == "FlowerOfLife":

        def create_flower_circle(center, radius, num_petals):
            shapes = []
            steps = 32

            points = []
            for i in range(steps + 1):
                angle = 2 * math.pi * i / steps
                point = center + FreeCAD.Vector(
                    radius * math.cos(angle), radius * math.sin(angle), 0
                )
                points.append(point)
            shapes.append(Part.makePolygon(points))

            for i in range(num_petals):
                petal_points = []
                angle = 2 * math.pi * i / num_petals
                petal_center = center + FreeCAD.Vector(
                    radius * math.cos(angle), radius * math.sin(angle), 0
                )

                for j in range(steps + 1):
                    angle = 2 * math.pi * j / steps
                    point = petal_center + FreeCAD.Vector(
                        radius * math.cos(angle), radius * math.sin(angle), 0
                    )
                    petal_points.append(point)
                shapes.append(Part.makePolygon(petal_points))

            return shapes

        patterns = []
        radius = grid_size / 8
        levels = 3

        for level in range(levels):
            center = FreeCAD.Vector(grid_size / 2, grid_size / 2, 0)
            patterns.extend(create_flower_circle(center, radius * (level + 1), 6 * (level + 1)))

        return Part.makeCompound(patterns)

    elif pattern_type == "VoronoiMesh":

        def create_cell(center, points):
            cell_points = []
            num_segments = len(points)

            for i in range(num_segments):
                p1 = points[i]
                p2 = points[(i + 1) % num_segments]
                mid = (p1 + p2) * 0.5

                diff = p2 - p1
                perp = FreeCAD.Vector(-diff.y, diff.x, 0).normalize()

                cell_points.append(mid + perp * grid_size / 4)
                cell_points.append(mid - perp * grid_size / 4)

            return Part.makePolygon(cell_points)

        cells = []
        num_points = 12
        points = []

        for _ in range(num_points):
            point = FreeCAD.Vector(
                random.uniform(grid_size / 4, 3 * grid_size / 4),
                random.uniform(grid_size / 4, 3 * grid_size / 4),
                0,
            )
            points.append(point)

        for i, center in enumerate(points):
            nearby_points = sorted(points, key=lambda p: (p - center).Length)[1:6]
            cells.append(create_cell(center, nearby_points))

        return Part.makeCompound(cells)

    elif pattern_type == "OffsetChecker":
        components = []
        tile_size_val = 2.5
        offset = False
        for y in range(0, grid_size * 2, int(tile_size_val)):
            offset = not offset
            for x in range(-int(tile_size_val), grid_size * 2, int(tile_size_val)):
                shift = tile_size_val / 2 if offset else 0
                if (x // tile_size_val + y // tile_size_val) % 2 == 0:
                    square = make_rectangle(tile_size_val, tile_size_val)
                    square.translate(FreeCAD.Vector(x + shift, y - shift, 0))
                    components.append(square)
        return Part.makeCompound(components)

    elif pattern_type == "ZigZag":
        components = []
        for y in range(0, grid_size + 1, 2):
            for x in range(0, grid_size, 4):
                line1 = Part.makeLine(FreeCAD.Vector(x, y, 0), FreeCAD.Vector(x + 2, y + 2, 0))
                line2 = Part.makeLine(FreeCAD.Vector(x + 2, y + 2, 0), FreeCAD.Vector(x + 4, y, 0))
                components.extend([line1, line2])
        return Part.makeCompound(components)

    elif pattern_type in ("HexagonalHoriz", "HexagonalVerti", "HexagonalPattern"):
        hex_size = 2.0
        points = []
        if pattern_type == "HexagonalHoriz":
            start_angle = 30
        else:
            start_angle = 0
        for angle in range(start_angle, start_angle + 360, 60):
            rad = math.radians(angle)
            px = hex_size * math.cos(rad)
            py = hex_size * math.sin(rad)
            points.append(FreeCAD.Vector(px, py, 0))
        points.append(points[0])
        wire = Part.makePolygon(points)
        try:
            face = Part.Face(wire)
            return face
        except Exception:
            return Part.makeCompound([])

    elif pattern_type == "TrianglesGrid":
        components = []
        for y in range(0, grid_size + 1, 3):
            for x in range(0, grid_size + 1, 3):
                tri1 = Part.makePolygon(
                    [
                        FreeCAD.Vector(x, y, 0),
                        FreeCAD.Vector(x + 3, y, 0),
                        FreeCAD.Vector(x + 1.5, y + hex_y, 0),
                        FreeCAD.Vector(x, y, 0),
                    ]
                )
                tri2 = Part.makePolygon(
                    [
                        FreeCAD.Vector(x + 3, y, 0),
                        FreeCAD.Vector(x + 1.5, y + hex_y, 0),
                        FreeCAD.Vector(x + 3, y + hex_y * 2, 0),
                        FreeCAD.Vector(x + 3, y, 0),
                    ]
                )
                components.extend([tri1, tri2])
        return Part.makeCompound(components)

    elif pattern_type == "MidEastMosaic":
        components = []
        triangle_size_val = 3.0
        triangle_height = (math.sqrt(3) / 2) * triangle_size_val
        x_step = triangle_size_val
        y_step = triangle_height
        y = 0.0
        row = 0
        while y <= grid_size:
            if row % 2 == 1:
                x_offset = triangle_size_val / 2
            else:
                x_offset = 0.0
            x = x_offset
            while x <= grid_size:
                points_up = [
                    FreeCAD.Vector(x, y, 0),
                    FreeCAD.Vector(x + triangle_size_val / 2, y + triangle_height, 0),
                    FreeCAD.Vector(x - triangle_size_val / 2, y + triangle_height, 0),
                    FreeCAD.Vector(x, y, 0),
                ]
                try:
                    wire_up = Part.makePolygon(points_up)
                    face_up = Part.Face(wire_up)
                    components.append(face_up)
                except Exception:
                    pass

                points_down = [
                    FreeCAD.Vector(x, y + triangle_height, 0),
                    FreeCAD.Vector(x + triangle_size_val / 2, y, 0),
                    FreeCAD.Vector(x - triangle_size_val / 2, y, 0),
                    FreeCAD.Vector(x, y + triangle_height, 0),
                ]
                try:
                    wire_down = Part.makePolygon(points_down)
                    face_down = Part.Face(wire_down)
                    components.append(face_down)
                except Exception:
                    pass

                x += x_step
            y += y_step
            row += 1
        return Part.makeCompound(components)

    elif pattern_type == "StarGridPattern":
        components = []
        star_size = 3.0
        spacing_x = 10.0
        spacing_y = 10.0
        rotation_angle = 0
        rows = int(grid_size / spacing_y) + 1
        cols = int(grid_size / spacing_x) + 1
        for row in range(rows):
            y = row * spacing_y
            for col in range(cols):
                x = col * spacing_x
                points = []
                for i in range(5):
                    angle_deg = rotation_angle + i * 72
                    angle_rad = math.radians(angle_deg)
                    outer_x = x + star_size * math.cos(angle_rad)
                    outer_y = y + star_size * math.sin(angle_rad)
                    points.append(FreeCAD.Vector(outer_x, outer_y, 0))
                    angle_deg = rotation_angle + (i * 72) + 36
                    angle_rad = math.radians(angle_deg)
                    inner_size = star_size / 2
                    inner_x = x + inner_size * math.cos(angle_rad)
                    inner_y = y + inner_size * math.sin(angle_rad)
                    points.append(FreeCAD.Vector(inner_x, inner_y, 0))
                points.append(points[0])
                try:
                    wire = Part.makePolygon(points)
                    face = Part.Face(wire)
                    components.append(face)
                except Exception:
                    pass
        return Part.makeCompound(components)

    elif pattern_type == "BasketWeave":
        components = []
        for x in range(0, grid_size, 2):
            for y in range(0, grid_size, 4):
                line1 = Part.makeLine(FreeCAD.Vector(x, y, 0), FreeCAD.Vector(x, y + 2, 0))
                line2 = Part.makeLine(
                    FreeCAD.Vector(x + 2, y + 2, 0), FreeCAD.Vector(x + 2, y + 4, 0)
                )
                components.extend([line1, line2])
        for y in range(0, grid_size, 2):
            for x in range(0, grid_size, 4):
                line1 = Part.makeLine(FreeCAD.Vector(x, y, 0), FreeCAD.Vector(x + 2, y, 0))
                line2 = Part.makeLine(
                    FreeCAD.Vector(x + 2, y + 2, 0), FreeCAD.Vector(x + 4, y + 2, 0)
                )
                components.extend([line1, line2])
        return Part.makeCompound(components)

    elif pattern_type == "Honeycomb":
        components = []
        hex_size = 1.5
        hex_height = math.sqrt(3) * hex_size
        x_step = 3 * hex_size
        y_step = hex_height
        y = 0.0
        while y <= grid_size:
            row_number = int(y / y_step)
            if row_number % 2 == 0:
                x_offset = 1.5 * hex_size
            else:
                x_offset = 0.0
            x = 0.0
            while x <= grid_size:
                center_x = x + x_offset
                center_y = y
                points = []
                for angle_deg in range(0, 360, 60):
                    angle_rad = math.radians(angle_deg)
                    px = center_x + hex_size * math.cos(angle_rad)
                    py = center_y + hex_size * math.sin(angle_rad)
                    points.append(FreeCAD.Vector(px, py, 0))
                points.append(points[0])
                try:
                    wire = Part.makePolygon(points)
                    face = Part.Face(wire)
                    components.append(face)
                except Exception:
                    pass
                x += x_step
            y += y_step
        return Part.makeCompound(components)

    elif pattern_type == "SineWave":
        components = []
        amp = 1.5
        freq = 2
        points = []
        for x in range(0, 100):
            x_val = x / 10.0
            y_val = amp * math.sin(math.radians(x_val * freq * 36))
            points.append(FreeCAD.Vector(x_val, grid_size / 2 + y_val, 0))
        components.append(Part.makePolygon(points))
        return Part.makeCompound(components)

    elif pattern_type == "SpaceFrame":
        components = []
        node_dist = 2.5
        for x in range(0, grid_size, int(node_dist)):
            for y in range(0, grid_size, int(node_dist)):
                for dx in [-node_dist, 0, node_dist]:
                    for dy in [-node_dist, 0, node_dist]:
                        if dx == dy == 0:
                            continue
                        if 0 <= x + dx <= grid_size and 0 <= y + dy <= grid_size:
                            components.append(
                                Part.makeLine(
                                    FreeCAD.Vector(x, y, 0), FreeCAD.Vector(x + dx, y + dy, 0)
                                )
                            )
        return Part.makeCompound(components)

    elif pattern_type == "HoneycombDual":
        components = []
        hex_size = 1.2
        hex_height = math.sqrt(3) * hex_size

        for y in range(0, grid_size * 2, int(hex_height)):
            x_offset = int(hex_size) if (y // int(hex_height)) % 2 else 0
            for x in range(x_offset, grid_size * 2, int(hex_size * 2)):
                points = []
                for angle in range(0, 360, 60):
                    rad = math.radians(angle + 30)
                    px = x + hex_size * math.cos(rad)
                    py = y + hex_size * math.sin(rad)
                    points.append(FreeCAD.Vector(px, py, 0))
                points.append(points[0])
                components.append(Part.makePolygon(points))
                components.append(Part.makeCircle(hex_size / 2, FreeCAD.Vector(x, y, 0)))

        return Part.makeCompound(components)

    elif pattern_type == "ArtDeco":
        components = []
        unit = 3.0
        for x in range(0, grid_size, int(unit)):
            for y in range(0, grid_size, int(unit)):
                components.append(
                    Part.makePolygon(
                        [
                            FreeCAD.Vector(x, y, 0),
                            FreeCAD.Vector(x + unit, y + unit, 0),
                            FreeCAD.Vector(x, y + unit * 2, 0),
                            FreeCAD.Vector(x, y, 0),
                        ]
                    )
                )
                for a in range(0, 360, 45):
                    rad = math.radians(a)
                    components.append(
                        Part.makeLine(
                            FreeCAD.Vector(x + unit / 2, y + unit / 2, 0),
                            FreeCAD.Vector(
                                x + unit / 2 + math.cos(rad) * unit / 2,
                                y + unit / 2 + math.sin(rad) * unit / 2,
                                0,
                            ),
                        )
                    )
        return Part.makeCompound(components)

    elif pattern_type == "StainedGlass":
        components = []
        tile = 2.5
        for x in range(0, grid_size, int(tile)):
            for y in range(0, grid_size, int(tile)):
                components.append(
                    Part.makeLine(
                        FreeCAD.Vector(x + tile / 2, y, 0),
                        FreeCAD.Vector(x + tile / 2, y + tile, 0),
                    )
                )
                components.append(
                    Part.makeLine(
                        FreeCAD.Vector(x, y + tile / 2, 0),
                        FreeCAD.Vector(x + tile, y + tile / 2, 0),
                    )
                )
                components.append(
                    Part.makeCircle(tile / 3, FreeCAD.Vector(x + tile / 2, y + tile / 2, 0))
                )
        return Part.makeCompound(components)

    elif pattern_type == "PenroseTriangle":
        components = []
        size = 2.0
        for i in range(0, grid_size, int(size * 2)):
            points = [
                FreeCAD.Vector(i, 0, 0),
                FreeCAD.Vector(i + size, 0, 0),
                FreeCAD.Vector(i + size / 2, size * math.sqrt(3) / 2, 0),
            ]
            components.append(Part.makePolygon(points + [points[0]]))
            points = [
                FreeCAD.Vector(i + size / 2, size * math.sqrt(3) / 2, 0),
                FreeCAD.Vector(i + size, 0, 0),
                FreeCAD.Vector(i + size * 1.5, size * math.sqrt(3) / 2, 0),
            ]
            components.append(Part.makePolygon(points + [points[0]]))
        return Part.makeCompound(components)

    elif pattern_type == "GreekKey":
        components = []
        unit_size = 2.0
        for x in range(0, grid_size, int(unit_size * 2)):
            for y in range(0, grid_size, int(unit_size * 2)):
                points = [
                    FreeCAD.Vector(x, y, 0),
                    FreeCAD.Vector(x + unit_size, y, 0),
                    FreeCAD.Vector(x + unit_size, y + unit_size, 0),
                    FreeCAD.Vector(x + unit_size * 0.75, y + unit_size, 0),
                    FreeCAD.Vector(x + unit_size * 0.75, y + unit_size * 0.75, 0),
                    FreeCAD.Vector(x + unit_size * 1.25, y + unit_size * 0.75, 0),
                    FreeCAD.Vector(x + unit_size * 1.25, y + unit_size, 0),
                    FreeCAD.Vector(x + unit_size * 1.5, y + unit_size, 0),
                    FreeCAD.Vector(x + unit_size * 1.5, y, 0),
                    FreeCAD.Vector(x + unit_size * 2, y, 0),
                ]
                components.append(Part.makePolygon(points))
        return Part.makeCompound(components)

    elif pattern_type == "ChainLinks":
        components = []
        link_width = 1.5
        for y in range(0, grid_size, int(link_width * 2)):
            for x in range(0, grid_size, int(link_width * 3)):
                components.append(
                    Part.makeCircle(
                        link_width / 2, FreeCAD.Vector(x + link_width * 1.5, y + link_width, 0)
                    )
                )
                components.append(
                    Part.makeCircle(
                        link_width / 2, FreeCAD.Vector(x + link_width * 3, y + link_width * 2, 0)
                    )
                )
        return Part.makeCompound(components)

    elif pattern_type == "TriangleForest":
        components = []
        chevron_width = 3.0
        chevron_height = 3.0
        spacing_x = chevron_width * 2
        spacing_y = chevron_height
        rows = int(grid_size / spacing_y) + 2
        for row in range(rows):
            y = row * spacing_y
            if row % 2 == 0:
                x_offset = chevron_width
            else:
                x_offset = 0.0
            cols = int(grid_size / spacing_x) + 2
            for col in range(cols):
                x = col * spacing_x + x_offset
                points = [
                    FreeCAD.Vector(x - chevron_width / 2, y, 0),
                    FreeCAD.Vector(x + chevron_width / 2, y, 0),
                    FreeCAD.Vector(x, y + chevron_height, 0),
                    FreeCAD.Vector(x - chevron_width / 2, y, 0),
                ]
                try:
                    wire = Part.makePolygon(points)
                    face = Part.Face(wire)
                    components.append(face)
                except Exception:
                    pass
        return Part.makeCompound(components)

    elif pattern_type == "CeramicTile":
        components = []
        hex_size = 1.0
        hex_height = math.sqrt(3) * hex_size
        spacing_x = 3 * hex_size
        spacing_y = hex_height
        rows = int(grid_size / spacing_y) + 2
        cols = int(grid_size / spacing_x) + 2
        for row in range(rows):
            y = row * spacing_y
            if row % 2 == 1:
                x_offset = 1.5 * hex_size
            else:
                x_offset = 0.0
            for col in range(cols):
                x = col * spacing_x + x_offset
                points = []
                for angle_deg in range(0, 360, 60):
                    angle_rad = math.radians(angle_deg)
                    px = x + hex_size * math.cos(angle_rad)
                    py = y + hex_size * math.sin(angle_rad)
                    points.append(FreeCAD.Vector(px, py, 0))
                points.append(points[0])
                try:
                    wire = Part.makePolygon(points)
                    face = Part.Face(wire)
                    components.append(face)
                except Exception:
                    pass
        return Part.makeCompound(components)

    elif pattern_type == "CirclesGrid":
        components = []
        for x in range(1, grid_size, 3):
            for y in range(1, grid_size, 3):
                components.append(Part.makeCircle(1.0, FreeCAD.Vector(x, y, 0)))
        return Part.makeCompound(components)

    elif pattern_type == "PlusSigns":
        components = []
        for x in range(1, grid_size, 3):
            for y in range(1, grid_size, 3):
                horizontal_line = Part.makeLine(
                    FreeCAD.Vector(x - 0.5, y, 0), FreeCAD.Vector(x + 0.5, y, 0)
                )
                vertical_line = Part.makeLine(
                    FreeCAD.Vector(x, y - 0.5, 0), FreeCAD.Vector(x, y + 0.5, 0)
                )
                components.extend([horizontal_line, vertical_line])
        return Part.makeCompound(components)

    elif pattern_type == "WavesPattern":
        components = []
        amplitude = 1.0
        wavelength = 10.0
        frequency = 1.0
        wave_spacing = 5.0
        points_per_wave = 200
        num_waves = int(grid_size / wave_spacing) + 2
        for wave_num in range(num_waves):
            y_base = wave_num * wave_spacing
            points = []
            for i in range(points_per_wave + 1):
                x = i * (wavelength / points_per_wave)
                y = y_base + amplitude * math.sin(2 * math.pi * frequency * x / wavelength)
                points.append(FreeCAD.Vector(x, y, 0))
            try:
                wire = Part.makePolygon(points)
                components.append(wire)
            except Exception:
                pass
        return Part.makeCompound(components)

    elif pattern_type == "GalaxyStarsPattern":
        components = []
        star_size = 1.0
        inner_ratio = 0.5
        spacing_x = 3.0
        spacing_y = 3.0
        rotation_angle = 0
        inner_radius = star_size * inner_ratio
        rows = int(grid_size / spacing_y) + 2
        cols = int(grid_size / spacing_x) + 2
        for row in range(rows):
            y = row * spacing_y
            if row % 2 == 1:
                x_offset = spacing_x / 2
            else:
                x_offset = 0.0
            for col in range(cols):
                x = col * spacing_x + x_offset
                points = []
                for i in range(10):
                    angle_deg = rotation_angle + i * 36
                    angle_rad = math.radians(angle_deg)
                    if i % 2 == 0:
                        px = x + star_size * math.cos(angle_rad)
                        py = y + star_size * math.sin(angle_rad)
                    else:
                        px = x + inner_radius * math.cos(angle_rad)
                        py = y + inner_radius * math.sin(angle_rad)
                    points.append(FreeCAD.Vector(px, py, 0))
                points.append(points[0])
                try:
                    wire = Part.makePolygon(points)
                    face = Part.Face(wire)
                    components.append(face)
                except Exception:
                    pass
        return Part.makeCompound(components)

    elif pattern_type == "GridDots":
        components = []
        for x in range(1, grid_size, 2):
            for y in range(1, grid_size, 2):
                components.append(Part.makeCircle(0.2, FreeCAD.Vector(x, y, 0)))
        return Part.makeCompound(components)

    elif pattern_type == "HexDots":
        components = []
        hex_size = 1.5
        dot_radius = hex_size / 3
        hex_height = math.sqrt(3) * hex_size

        for row in range(int(math.ceil(grid_size / hex_height)) + 1):
            y = row * hex_height
            x_offset = hex_size if row % 2 == 1 else 0
            for col in range(int(math.ceil(grid_size / (hex_size * 2))) + 1):
                x = col * hex_size * 2 + x_offset
                if x < grid_size and y < grid_size:
                    components.append(Part.makeCircle(dot_radius, FreeCAD.Vector(x, y, 0)))

        return Part.makeCompound(components)

    elif pattern_type == "FractalTree":
        components = []

        def draw_branch(start, direction, depth):
            if depth > 5:
                return
            end = start + direction
            components.append(Part.makeLine(start, end))

            for ang in [-30, 0, 30]:
                new_dir = direction * 0.7
                rot_angle = math.radians(ang)
                rotation_matrix = FreeCAD.Matrix()
                rotation_matrix.rotateZ(rot_angle)
                new_dir = rotation_matrix.multVec(new_dir)
                draw_branch(end, new_dir, depth + 1)

        start_point = FreeCAD.Vector(grid_size / 2, 0, 0)
        initial_direction = FreeCAD.Vector(0, 3, 0)
        draw_branch(start_point, initial_direction, 0)

        return Part.makeCompound(components)

    elif pattern_type == "Voronoi":
        components = []

        points = [
            FreeCAD.Vector(random.uniform(0, grid_size), random.uniform(0, grid_size), 0)
            for _ in range(15)
        ]

        try:
            from scipy.spatial import Voronoi
            import numpy as np

            point_array = np.array([[pt.x, pt.y] for pt in points])
            vor = Voronoi(point_array)

            for region_index in vor.point_region:
                region = vor.regions[region_index]
                if -1 in region or len(region) == 0:
                    continue

                vertices = [vor.vertices[vertex_index] for vertex_index in region]
                freecad_vertices = [FreeCAD.Vector(v[0], v[1], 0) for v in vertices]

                try:
                    wire = Part.makePolygon(freecad_vertices + [freecad_vertices[0]])
                    face = Part.Face(wire)
                    components.append(face)
                except Exception as e:
                    FreeCAD.Console.PrintWarning(f"Failed to create Voronoi face: {str(e)}\n")

        except ImportError:
            FreeCAD.Console.PrintError("scipy is required for Voronoi pattern generation.\n")
            return None

        return Part.makeCompound(components)

    elif pattern_type == "FractalBranches":

        def create_branch(start, length, angle, depth):
            if depth == 0:
                return []

            end = start + FreeCAD.Vector(length * math.cos(angle), length * math.sin(angle), 0)

            shapes = [Part.makeLine(start, end)]

            phi = (1 + math.sqrt(5)) / 2
            new_length = length / phi
            angle_var = math.radians(random.uniform(-20, 20))

            angles = [
                angle + math.pi / 4 + angle_var,
                angle - math.pi / 4 - angle_var,
                angle + angle_var,
            ]

            for new_angle in angles:
                shapes.extend(create_branch(end, new_length, new_angle, depth - 1))

            return shapes

        branches = []
        start_points = [
            FreeCAD.Vector(grid_size / 2, 0, 0),
            FreeCAD.Vector(grid_size / 4, 0, 0),
            FreeCAD.Vector(3 * grid_size / 4, 0, 0),
        ]

        for start in start_points:
            branches.extend(create_branch(start, grid_size / 4, math.pi / 2, 6))

        return Part.makeCompound(branches)

    elif pattern_type == "OrganicMaze":

        def create_maze_segment(start, end, complexity):
            points = [start]
            current = start
            target = end

            while (current - target).Length > grid_size / 20:
                direction = (target - current).normalize()
                perp = FreeCAD.Vector(-direction.y, direction.x, 0)
                random_offset = perp * random.uniform(-complexity, complexity)
                step = direction * grid_size / 20 + random_offset
                current = current + step
                points.append(current)

            points.append(target)
            return Part.makePolygon(points)

        segments = []
        num_segments = 15
        points = []

        for _ in range(num_segments):
            point = FreeCAD.Vector(random.uniform(0, grid_size), random.uniform(0, grid_size), 0)
            points.append(point)

        for i in range(len(points)):
            for j in range(i + 1, len(points)):
                if random.random() < 0.3:
                    segments.append(create_maze_segment(points[i], points[j], grid_size / 10))

        return Part.makeCompound(segments)

    elif pattern_type == "BiomorphicCells":

        def create_cell_membrane(center, size):
            points = []
            num_points = random.randint(12, 18)
            base_radius = size * (1 + random.uniform(-0.2, 0.2))

            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                radius = base_radius * (
                    1
                    + 0.3 * math.sin(3 * angle)
                    + 0.2 * math.sin(5 * angle)
                    + random.uniform(-0.1, 0.1)
                )
                point = center + FreeCAD.Vector(
                    radius * math.cos(angle), radius * math.sin(angle), 0
                )
                points.append(point)

            return Part.makePolygon(points)

        cells = []
        num_cells = 12
        min_size = grid_size / 8
        max_size = grid_size / 4

        for _ in range(num_cells):
            center = FreeCAD.Vector(
                random.uniform(grid_size / 4, 3 * grid_size / 4),
                random.uniform(grid_size / 4, 3 * grid_size / 4),
                0,
            )
            size = random.uniform(min_size, max_size)
            cells.append(create_cell_membrane(center, size))

            for _ in range(random.randint(2, 4)):
                offset = FreeCAD.Vector(
                    random.uniform(-size / 2, size / 2), random.uniform(-size / 2, size / 2), 0
                )
                organelle_size = size * random.uniform(0.2, 0.4)
                cells.append(create_cell_membrane(center + offset, organelle_size))

        return Part.makeCompound(cells)

    elif pattern_type in ("RadialSunburst", "Sunburst"):
        components = []
        if pattern_type == "RadialSunburst":
            spokes = 16
            inner_radius = 4.0
            outer_radius = 4.0
        else:
            spokes = 24
            inner_radius = 2.0
            outer_radius = 4.0

        for i in range(spokes):
            angle = math.radians(i * (360 / spokes))
            if pattern_type == "Sunburst":
                if i % 2 == 0:
                    inner_radius = 2.0
                    outer_radius = 4.0
                else:
                    inner_radius = 3.5
                    outer_radius = 4.5
            x1 = grid_size / 2 + inner_radius * math.cos(angle)
            y1 = grid_size / 2 + inner_radius * math.sin(angle)
            x2 = grid_size / 2 + outer_radius * math.cos(angle)
            y2 = grid_size / 2 + outer_radius * math.sin(angle)
            components.append(Part.makeLine(FreeCAD.Vector(x1, y1, 0), FreeCAD.Vector(x2, y2, 0)))
        return Part.makeCompound(components)

    elif pattern_type == "Ziggurat":
        components = []
        step_size = 1.5
        for x in range(0, grid_size, int(step_size * 3)):
            for y in range(0, grid_size, int(step_size * 3)):
                for i in range(3):
                    components.append(
                        Part.makePolygon(
                            [
                                FreeCAD.Vector(x + i * step_size, y + i * step_size, 0),
                                FreeCAD.Vector(x + (i + 1) * step_size, y + i * step_size, 0),
                                FreeCAD.Vector(x + (i + 1) * step_size, y + (i + 1) * step_size, 0),
                                FreeCAD.Vector(x + i * step_size, y + (i + 1) * step_size, 0),
                                FreeCAD.Vector(x + i * step_size, y + i * step_size, 0),
                            ]
                        )
                    )
        return Part.makeCompound(components)

    elif pattern_type == "SpiralPattern":

        def create_spiral(center, max_radius, turns, points_per_turn):
            points = []
            total_points = int(turns * points_per_turn)

            for i in range(total_points + 1):
                t = i / points_per_turn
                radius = max_radius * t / turns
                angle = 2 * math.pi * t
                point = center + FreeCAD.Vector(
                    radius * math.cos(angle), radius * math.sin(angle), 0
                )
                points.append(point)

            edges = []
            for i in range(len(points) - 1):
                edges.append(Part.makeLine(points[i], points[i + 1]))

            return edges

        radius = min(grid_size / 2.5, 2)
        center = FreeCAD.Vector(grid_size / 2, grid_size / 2, 0)

        spirals = []
        for i in range(3):
            phase = 2 * math.pi * i / 3
            spiral_center = center + FreeCAD.Vector(
                0.2 * radius * math.cos(phase), 0.2 * radius * math.sin(phase), 0
            )
            spirals.extend(create_spiral(spiral_center, radius, 4, 50))

        return Part.makeCompound(spirals)

    elif pattern_type == "PentaflakeFractal":

        def pentaflake(center, size, depth):
            if depth == 0:
                points = []
                for i in range(5):
                    angle = math.radians(72 * i - 18)
                    points.append(
                        center + FreeCAD.Vector(size * math.cos(angle), size * math.sin(angle), 0)
                    )
                points.append(points[0])
                return [Part.makePolygon(points)]

            shapes = []
            shapes.extend(pentaflake(center, size / 3, depth - 1))

            for i in range(5):
                angle = math.radians(72 * i - 18)
                new_center = center + FreeCAD.Vector(
                    size * 0.618 * math.cos(angle), size * 0.618 * math.sin(angle), 0
                )
                shapes.extend(pentaflake(new_center, size / 3, depth - 1))

            return shapes

        size = min(grid_size / 3, 2)
        center = FreeCAD.Vector(grid_size / 2, grid_size / 2, 0)

        return Part.makeCompound(pentaflake(center, size, 3))

    elif pattern_type == "HilbertCurve":

        def hilbert(x, y, xi, xj, yi, yj, n):
            if n <= 0:
                X = x + (xi + yi) / 2
                Y = y + (xj + yj) / 2
                return [FreeCAD.Vector(X, Y, 0)]

            points = []
            points.extend(hilbert(x, y, yi / 2, yj / 2, xi / 2, xj / 2, n - 1))
            points.extend(hilbert(x + xi / 2, y + xj / 2, xi / 2, xj / 2, yi / 2, yj / 2, n - 1))
            points.extend(
                hilbert(
                    x + xi / 2 + yi / 2, y + xj / 2 + yj / 2, xi / 2, xj / 2, yi / 2, yj / 2, n - 1
                )
            )
            points.extend(
                hilbert(x + xi + yi / 2, y + xj + yj / 2, -yi / 2, -yj / 2, -xi / 2, -xj / 2, n - 1)
            )
            return points

        size = min(grid_size * 0.8, 4)
        offset = (grid_size - size) / 2
        points = hilbert(offset, offset, size, 0, 0, size, 4)

        edges = []
        for i in range(len(points) - 1):
            edges.append(Part.makeLine(points[i], points[i + 1]))

        return Part.makeCompound(edges)

    elif pattern_type == "SierpinskiTriangle":

        def sierpinski(p1, p2, p3, depth):
            if depth == 0:
                return [Part.makePolygon([p1, p2, p3, p1])]

            mid1 = (p1 + p2) * 0.5
            mid2 = (p2 + p3) * 0.5
            mid3 = (p3 + p1) * 0.5

            return (
                sierpinski(p1, mid1, mid3, depth - 1)
                + sierpinski(mid1, p2, mid2, depth - 1)
                + sierpinski(mid3, mid2, p3, depth - 1)
            )

        size = min(grid_size / 2, 3)
        center = FreeCAD.Vector(grid_size / 2, grid_size / 2, 0)
        height = size * math.sqrt(3) / 2

        points = [
            center + FreeCAD.Vector(0, height, 0),
            center + FreeCAD.Vector(-size, -height, 0),
            center + FreeCAD.Vector(size, -height, 0),
        ]

        return Part.makeCompound(sierpinski(points[0], points[1], points[2], 4))

    elif pattern_type == "PenroseTiling":

        def create_rhombus(center, size, angle):
            points = []
            for i in [0, 72, 144, 216, 288]:
                a = math.radians(angle + i)
                points.append(center + FreeCAD.Vector(size * math.cos(a), size * math.sin(a), 0))
            return Part.makePolygon(points[:4])

        components = []
        phi = (1 + math.sqrt(5)) / 2
        sizes = [2.0, 2.0 / phi]
        for i in range(10):
            center = FreeCAD.Vector(random.uniform(2, 8), random.uniform(2, 8), 0)
            components.append(create_rhombus(center, sizes[i % 2], random.choice([0, 36])))
        return Part.makeCompound(components)

    elif pattern_type == "EinsteinMonotile":

        def einstein_tile(center):
            angles = [0, 60, 120, 180, 240, 300]
            points = []
            for i, ang in enumerate(angles):
                r = 1.0 if i % 2 else 1.732
                x = center.x + r * math.cos(math.radians(ang))
                y = center.y + r * math.sin(math.radians(ang))
                points.append(FreeCAD.Vector(x, y, 0))
            points.append(points[0])
            return Part.makePolygon(points)

        components = []
        for x in range(0, grid_size * 2, 3):
            for y in range(0, grid_size * 2, 3):
                components.append(einstein_tile(FreeCAD.Vector(x, y, 0)))
        return Part.makeCompound(components)

    elif pattern_type == "LeafVeins":
        components = []
        midrib = Part.makeLine(FreeCAD.Vector(5, 1, 0), FreeCAD.Vector(5, 9, 0))
        components.append(midrib)
        for y in range(1, 10, 2):
            for side in [-1, 1]:
                angle = math.radians(45 * (1 - abs(y - 5) / 5))
                points = [
                    FreeCAD.Vector(5, y, 0),
                    FreeCAD.Vector(5 + side * 3 * math.cos(angle), y + 3 * math.sin(angle), 0),
                ]
                components.append(Part.makePolygon(points))
        return Part.makeCompound(components)

    elif pattern_type == "WoodPlanks":
        components = []
        for y in range(0, grid_size + 1, 2):
            components.append(
                Part.makeLine(FreeCAD.Vector(0, y, 0), FreeCAD.Vector(grid_size, y, 0))
            )
            if y % 4 == 0:
                for x in [3, 7]:
                    vertical_line = Part.makeLine(
                        FreeCAD.Vector(x, y, 0), FreeCAD.Vector(x, y + 1, 0)
                    )
                    components.append(vertical_line)
        return Part.makeCompound(components)

    elif pattern_type == "ParquetHerringbone":
        components = []
        plank_width = 1.0
        plank_length = 4.0
        angle = 45

        for i in range(-grid_size, grid_size * 2, int(plank_length / math.sqrt(2))):
            for j in range(-grid_size, grid_size * 2, int(plank_width)):
                points1 = [
                    FreeCAD.Vector(j, i, 0),
                    FreeCAD.Vector(
                        j + plank_length * math.cos(math.radians(angle)),
                        i + plank_length * math.sin(math.radians(angle)),
                        0,
                    ),
                    FreeCAD.Vector(
                        j
                        + plank_length * math.cos(math.radians(angle))
                        - plank_width * math.sin(math.radians(angle)),
                        i
                        + plank_length * math.sin(math.radians(angle))
                        + plank_width * math.cos(math.radians(angle)),
                        0,
                    ),
                    FreeCAD.Vector(
                        j - plank_width * math.sin(math.radians(angle)),
                        i + plank_width * math.cos(math.radians(angle)),
                        0,
                    ),
                ]
                components.append(Part.makePolygon(points1))

                points2 = [
                    FreeCAD.Vector(j + plank_length * math.cos(math.radians(angle)), i, 0),
                    FreeCAD.Vector(
                        j
                        + plank_length * math.cos(math.radians(angle))
                        - plank_length * math.cos(math.radians(angle)),
                        i + plank_length * math.sin(math.radians(angle)),
                        0,
                    ),
                    FreeCAD.Vector(
                        j
                        + plank_length * math.cos(math.radians(angle))
                        - plank_length * math.cos(math.radians(angle))
                        - plank_width * math.sin(math.radians(angle)),
                        i
                        + plank_length * math.sin(math.radians(angle))
                        + plank_width * math.cos(math.radians(angle)),
                        0,
                    ),
                    FreeCAD.Vector(
                        j
                        + plank_length * math.cos(math.radians(angle))
                        - plank_width * math.sin(math.radians(angle)),
                        i + plank_width * math.cos(math.radians(angle)),
                        0,
                    ),
                ]
                components.append(Part.makePolygon(points2))

        return Part.makeCompound(components)

    elif pattern_type == "WoodGrain":
        components = []
        num_grains = 50
        max_deviation = 0.5

        for _ in range(num_grains):
            x_start = random.uniform(0, grid_size)
            y_start = random.uniform(0, grid_size)
            amp = random.uniform(0.1, 0.5)
            freq = random.uniform(0.1, 0.3)

            points = []
            for t in range(0, grid_size * 2, 2):
                x_val = x_start + t * 0.1
                y_val = (
                    y_start
                    + amp * math.sin(freq * x_val)
                    + random.uniform(-max_deviation, max_deviation)
                )
                points.append(FreeCAD.Vector(x_val, y_val, 0))

            components.append(Part.makePolygon(points))

        return Part.makeCompound(components)

    elif pattern_type in (
        "DrywallOrangePeel",
        "StuccoSandFloat",
        "ConcreteSaltFinish",
        "ConcreteSandblastPattern",
        "ConcreteAggregatePattern",
    ):
        components = []
        size = grid_size

        if pattern_type == "DrywallOrangePeel":
            num_spots = 400
            spot_size = size / 60
            num_points_range = (6, 8)
        elif pattern_type == "StuccoSandFloat":
            num_spots = 1000
            spot_size = size / 120
            num_points_range = (4, 6)
        elif pattern_type == "ConcreteSaltFinish":
            num_spots = 300
            spot_size = size / 100
            num_points_range = (4, 6)
        elif pattern_type == "ConcreteSandblastPattern":
            num_spots = 200
            spot_size = size / 60
            num_points_range = (8, 12)
        else:
            num_spots = 50
            spot_size = size / 30
            num_points_range = (5, 8)

        for _ in range(num_spots):
            center = FreeCAD.Vector(random.uniform(0, size), random.uniform(0, size), 0)
            num_points = random.randint(num_points_range[0], num_points_range[1])
            points = []
            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                radius = spot_size * (1 + random.uniform(-0.3, 0.3))
                point = center + FreeCAD.Vector(
                    radius * math.cos(angle), radius * math.sin(angle), 0
                )
                points.append(point)
            components.append(Part.makePolygon(points))

        if pattern_type == "ConcreteSandblastPattern":
            for _ in range(20):
                center = FreeCAD.Vector(random.uniform(0, size), random.uniform(0, size), 0)
                num_points = random.randint(num_points_range[0], num_points_range[1])
                points = []
                for i in range(num_points + 1):
                    angle = 2 * math.pi * i / num_points
                    radius = spot_size * 3 * (1 + random.uniform(-0.5, 0.5))
                    point = center + FreeCAD.Vector(
                        radius * math.cos(angle), radius * math.sin(angle), 0
                    )
                    points.append(point)
                components.append(Part.makePolygon(points))

        return Part.makeCompound(components)

    elif pattern_type in ("DrywallKnockdown", "StuccoDash"):
        components = []
        size = grid_size

        if pattern_type == "DrywallKnockdown":
            num_splatters = 200
            splatter_size = size / 40
            num_points_range = (8, 12)
        else:
            num_splatters = 300
            splatter_size = size / 50
            num_points_range = (7, 10)

        for _ in range(num_splatters):
            center = FreeCAD.Vector(random.uniform(0, size), random.uniform(0, size), 0)
            num_points = random.randint(num_points_range[0], num_points_range[1])
            points = []
            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                radius = splatter_size * (1 + random.uniform(-0.4, 0.4))
                if pattern_type == "DrywallKnockdown":
                    radius *= 1.5 if i % 2 == 0 else 0.7
                else:
                    radius *= 2 if i % 2 == 0 else 0.5
                point = center + FreeCAD.Vector(
                    radius * math.cos(angle), radius * math.sin(angle), 0
                )
                points.append(point)
            components.append(Part.makePolygon(points))

        return Part.makeCompound(components)

    elif pattern_type == "DrywallSkipTrowel":

        def create_skip_mark(center, size, angle):
            points = []
            length = size * random.uniform(0.8, 1.2)
            width = size * random.uniform(0.2, 0.4)

            steps = 20
            for i in range(steps + 1):
                t = i / steps
                r = width * math.sqrt(1 - (2 * t - 1) ** 2)
                x = length * (t - 0.5)
                point = center + FreeCAD.Vector(
                    x * math.cos(angle) - r * math.sin(angle),
                    x * math.sin(angle) + r * math.cos(angle),
                    0,
                )
                points.append(point)

            return Part.makePolygon(points)

        marks = []
        size = grid_size
        num_marks = 200
        mark_size = size / 15

        for _ in range(num_marks):
            center = FreeCAD.Vector(random.uniform(0, size), random.uniform(0, size), 0)
            angle = random.uniform(0, math.pi)
            marks.append(create_skip_mark(center, mark_size, angle))

        return Part.makeCompound(marks)

    elif pattern_type == "Concrete":
        components = []
        for _ in range(20):
            x1 = random.uniform(0, grid_size)
            y1 = random.uniform(0, grid_size)
            angle = math.radians(random.uniform(0, 360))
            x2 = x1 + random.uniform(1, 3) * math.cos(angle)
            y2 = y1 + random.uniform(1, 3) * math.sin(angle)
            components.append(Part.makeLine(FreeCAD.Vector(x1, y1, 0), FreeCAD.Vector(x2, y2, 0)))
        return Part.makeCompound(components)

    elif pattern_type == "ConcreteStampedPattern":

        def create_cobblestone(center, size):
            num_points = random.randint(6, 8)
            points = []
            base_radius = size / 2

            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                radius = base_radius * (1 + random.uniform(-0.2, 0.2))
                point = center + FreeCAD.Vector(
                    radius * math.cos(angle), radius * math.sin(angle), 0
                )
                points.append(point)

            return Part.makePolygon(points)

        stones = []
        size = grid_size
        stone_size = size / 8
        offset = stone_size * 0.1

        for row in range(int(size / stone_size) + 1):
            row_offset = offset * (row % 2)
            for col in range(int(size / stone_size) + 1):
                center = FreeCAD.Vector(
                    col * stone_size + row_offset + random.uniform(-offset, offset),
                    row * stone_size + random.uniform(-offset, offset),
                    0,
                )
                stones.append(create_cobblestone(center, stone_size))

        return Part.makeCompound(stones)

    elif pattern_type == "ConcreteFormTiePattern":

        def create_tie_hole(center, radius):
            points = []
            num_points = 16

            for i in range(num_points + 1):
                angle = 2 * math.pi * i / num_points
                point = center + FreeCAD.Vector(
                    radius * math.cos(angle), radius * math.sin(angle), 0
                )
                points.append(point)

            return Part.makePolygon(points)

        holes = []
        size = grid_size
        spacing_x = size / 4
        spacing_y = size / 3
        hole_radius = size / 40

        for x in range(1, 4):
            for y in range(1, 3):
                center = FreeCAD.Vector(x * spacing_x, y * spacing_y, 0)
                holes.append(create_tie_hole(center, hole_radius))

        return Part.makeCompound(holes)

    elif pattern_type in ("ConcreteControlJoint", "ConcreteGridPattern"):
        components = []
        size = grid_size
        joint_width = size / 50

        if pattern_type == "ConcreteControlJoint":
            for x in [size / 3, 2 * size / 3]:
                start = FreeCAD.Vector(x, 0, 0)
                end = FreeCAD.Vector(x, size, 0)
                direction = end.sub(start)
                perp = FreeCAD.Vector(-direction.y, direction.x, 0).normalize()
                length = direction.Length
                steps = int(length / (joint_width * 2))
                points = []
                for i in range(steps):
                    t = i / steps
                    mid = start + direction.multiply(t)
                    points.append(mid + perp.multiply(joint_width / 2))
                    points.append(mid + direction.multiply(1 / (2 * steps)))
                    points.append(mid + perp.multiply(-joint_width / 2))
                    points.append(mid + direction.multiply(1 / steps))
                components.append(Part.makePolygon(points))

            for y in [size / 3, 2 * size / 3]:
                start = FreeCAD.Vector(0, y, 0)
                end = FreeCAD.Vector(size, y, 0)
                direction = end.sub(start)
                perp = FreeCAD.Vector(-direction.y, direction.x, 0).normalize()
                length = direction.Length
                steps = int(length / (joint_width * 2))
                points = []
                for i in range(steps):
                    t = i / steps
                    mid = start + direction.multiply(t)
                    points.append(mid + perp.multiply(joint_width / 2))
                    points.append(mid + direction.multiply(1 / (2 * steps)))
                    points.append(mid + perp.multiply(-joint_width / 2))
                    points.append(mid + direction.multiply(1 / steps))
                components.append(Part.makePolygon(points))
        else:
            spacing = size / 4
            for y in range(1, 4):
                start = FreeCAD.Vector(0, y * spacing, 0)
                end = FreeCAD.Vector(size, y * spacing, 0)
                direction = end.sub(start)
                perp = FreeCAD.Vector(-direction.y, direction.x, 0).normalize()
                points = [
                    start + perp.multiply(joint_width / 2),
                    end + perp.multiply(joint_width / 2),
                    end - perp.multiply(joint_width / 2),
                    start - perp.multiply(joint_width / 2),
                    start + perp.multiply(joint_width / 2),
                ]
                components.append(Part.makePolygon(points))

            for x in range(1, 4):
                start = FreeCAD.Vector(x * spacing, 0, 0)
                end = FreeCAD.Vector(x * spacing, size, 0)
                direction = end.sub(start)
                perp = FreeCAD.Vector(-direction.y, direction.x, 0).normalize()
                points = [
                    start + perp.multiply(joint_width / 2),
                    end + perp.multiply(joint_width / 2),
                    end - perp.multiply(joint_width / 2),
                    start - perp.multiply(joint_width / 2),
                    start + perp.multiply(joint_width / 2),
                ]
                components.append(Part.makePolygon(points))

        return Part.makeCompound(components)

    elif pattern_type == "WoodKnotPattern":

        def create_knot(center, radius):
            shapes = []
            for r in range(3, int(radius * 10), 2):
                r_val = r / 10
                points = []
                steps = int(2 * math.pi * r_val * 10)
                for i in range(steps + 1):
                    angle = 2 * math.pi * i / steps
                    current_radius = r_val * (1 + random.uniform(-0.1, 0.1))
                    point = center + FreeCAD.Vector(
                        current_radius * math.cos(angle), current_radius * math.sin(angle), 0
                    )
                    points.append(point)
                shapes.append(Part.makePolygon(points))
            return shapes

        knots = []
        size = grid_size
        num_knots = 3

        for _ in range(num_knots):
            center = FreeCAD.Vector(
                random.uniform(size / 4, 3 * size / 4), random.uniform(size / 4, 3 * size / 4), 0
            )
            radius = random.uniform(size / 15, size / 10)
            knots.extend(create_knot(center, radius))

        return Part.makeCompound(knots)

    elif pattern_type == "BrushedConcrete":
        components = []
        for _ in range(15):
            x1 = random.uniform(0, grid_size)
            y1 = random.uniform(0, grid_size)
            angle = math.radians(random.choice([30, 45, 60]))
            length = random.uniform(3, 6)
            x2 = x1 + length * math.cos(angle)
            y2 = y1 + length * math.sin(angle)
            components.append(Part.makeLine(FreeCAD.Vector(x1, y1, 0), FreeCAD.Vector(x2, y2, 0)))
        return Part.makeCompound(components)

    elif pattern_type == "PebbleConcrete":
        components = []
        for _ in range(25):
            x = random.uniform(0, grid_size)
            y = random.uniform(0, grid_size)
            components.append(Part.makeCircle(random.uniform(0.2, 0.5), FreeCAD.Vector(x, y, 0)))
        return Part.makeCompound(components)

    elif pattern_type == "CrackedConcrete":
        components = []
        for _ in range(3):
            x1 = random.uniform(0, grid_size)
            y1 = random.uniform(0, grid_size)
            angle = math.radians(random.uniform(0, 360))
            length = random.uniform(4, 8)
            x2 = x1 + length * math.cos(angle)
            y2 = y1 + length * math.sin(angle)
            components.append(Part.makeLine(FreeCAD.Vector(x1, y1, 0), FreeCAD.Vector(x2, y2, 0)))

            for _ in range(3):
                branch_angle = angle + math.radians(random.uniform(-30, 30))
                branch_length = random.uniform(1, 2)
                x3 = x2 + branch_length * math.cos(branch_angle)
                y3 = y2 + branch_length * math.sin(branch_angle)
                components.append(
                    Part.makeLine(FreeCAD.Vector(x2, y2, 0), FreeCAD.Vector(x3, y3, 0))
                )
        return Part.makeCompound(components)

    elif pattern_type == "AggregateConcrete":
        components = []
        for _ in range(5):
            x1 = random.uniform(0, grid_size)
            y1 = random.uniform(0, grid_size)
            components.append(
                Part.makeLine(
                    FreeCAD.Vector(x1, y1, 0),
                    FreeCAD.Vector(x1 + random.uniform(-1, 1), y1 + random.uniform(-1, 1), 0),
                )
            )
        for _ in range(20):
            x = random.uniform(0, grid_size)
            y = random.uniform(0, grid_size)
            components.append(Part.makeCircle(random.uniform(0.2, 0.5), FreeCAD.Vector(x, y, 0)))
        return Part.makeCompound(components)

    elif pattern_type == "StampedConcrete":
        components = []
        for x in range(0, grid_size, 3):
            for y in range(0, grid_size, 3):
                if (x // 3 + y // 3) % 2 == 0:
                    components.append(Part.makeCircle(1.0, FreeCAD.Vector(x + 1.5, y + 1.5, 0)))
                else:
                    components.append(
                        Part.makePolygon(
                            [
                                FreeCAD.Vector(x, y, 0),
                                FreeCAD.Vector(x + 3, y, 0),
                                FreeCAD.Vector(x + 1.5, y + 3, 0),
                                FreeCAD.Vector(x, y, 0),
                            ]
                        )
                    )
        return Part.makeCompound(components)

    elif pattern_type == "Insulation":
        components = []
        for y in range(0, grid_size + 1, 2):
            for x in range(0, grid_size, 4):
                line1 = Part.makeLine(FreeCAD.Vector(x, y, 0), FreeCAD.Vector(x + 2, y + 1, 0))
                line2 = Part.makeLine(FreeCAD.Vector(x + 2, y + 1, 0), FreeCAD.Vector(x + 4, y, 0))
                components.extend([line1, line2])
        return Part.makeCompound(components)

    elif pattern_type == "Rebar":
        components = []
        for x in range(0, grid_size + 1, 2):
            for y in range(0, grid_size + 1, 2):
                vertical_line = Part.makeLine(
                    FreeCAD.Vector(x, 0, 0), FreeCAD.Vector(x, grid_size, 0)
                )
                horizontal_line = Part.makeLine(
                    FreeCAD.Vector(0, y, 0), FreeCAD.Vector(grid_size, y, 0)
                )
                circle = Part.makeCircle(0.3, FreeCAD.Vector(x, y, 0))
                components.extend([vertical_line, horizontal_line, circle])
        return Part.makeCompound(components)

    elif pattern_type == "RoofTiles":
        components = []
        for y in range(0, grid_size + 1, 3):
            for x in range(0, grid_size + 1, 3):
                arc = Part.ArcOfCircle(
                    Part.Circle(FreeCAD.Vector(x + 1.5, y + 1.5, 0), FreeCAD.Vector(0, 0, 1), 1.5),
                    0,
                    math.pi,
                ).toShape()
                components.append(arc)
        return Part.makeCompound(components)

    else:
        FreeCAD.Console.PrintWarning(f"Unknown built-in pattern: {pattern_type}\n")
        return base_rect
