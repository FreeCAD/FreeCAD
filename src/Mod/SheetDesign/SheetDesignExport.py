# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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

"""File export of a flat pattern: DXF and SVG.

The two formats have the same layers. A cutter must not cut a bend line, thus the outline, the
openings, the bend lines, and the texts are apart.

    OUTLINE     the outer contour of the part
    CUTOUT      the openings in the part
    BEND_UP     center lines of the bends to the top side, as dashed lines
    BEND_DOWN   center lines of the bends to the bottom side, as dashed lines
    BEND_TEXT   the direction, the angle, and the radius of each bend
    FORM        the outlines of the formed regions (a mark, not a cut)

The DXF file has the format R12 with only LINE, ARC, CIRCLE, POLYLINE, and TEXT. All programs
for cutters read this format.
"""

import math

import FreeCAD

OUTLINE, CUTOUT, BEND_UP, BEND_DOWN, BEND_TEXT = "OUTLINE", "CUTOUT", "BEND_UP", "BEND_DOWN", "BEND_TEXT"
FORM = "FORM"

#: Layer: (DXF color number, SVG color, dashed).
LAYERS = {
    OUTLINE: (7, "#000000", False),
    CUTOUT: (4, "#0080c0", False),
    BEND_UP: (3, "#00a000", True),
    BEND_DOWN: (6, "#c000c0", True),
    BEND_TEXT: (2, "#806000", False),
    FORM: (1, "#d00000", False),
}

#: The largest distance between a curve and the straight segments that replace it.
DEFLECTION = 0.01


def thickness_of(flat):
    """The thickness of the part of a flat pattern, from its sheet rule. None if not found."""
    feature = getattr(flat, "Source", None)
    for _step in range(1000):
        if feature is None:
            return None
        rule = getattr(feature, "Rule", None)
        if rule is not None and hasattr(rule, "Thickness"):
            return rule.Thickness.Value
        edge = getattr(feature, "Edge", None) or getattr(feature, "Edges", None)
        feature = getattr(feature, "Source", None) or (edge[0] if edge else None)
    return None


def _bend_lines(flat):
    """The bend lines of a flat pattern in the XY plane, and the outlines of its formed regions."""
    parts = flat.Shape.SubShapes
    to_xy = parts[0].Placement.inverse().toMatrix()
    lines, forms = [], []
    for part in parts[1:]:
        if part.ShapeType == "Edge":
            lines.append(part.transformed(to_xy, True))
        else:
            # The compound with the outlines of the formed regions.
            forms += part.transformed(to_xy, True).Wires
    return lines, forms


def group_scores(segments, thickness):
    """Put the bends that are on one straight line into one score.

    segments: (start, end) of each bend line. Two bend lines are one score if they are
    parallel, almost on one line, and end to end. A horizontal score of a box changes its
    position by some thicknesses from panel to panel, and a crossing score leaves a gap, thus
    the limits are a small number of thicknesses.

    Returns a list of scores. Each score is a list of bend indexes. The scores are in the
    sequence of their first bend.
    """
    across = 2.5 * thickness
    gap = 5.0 * thickness
    count = len(segments)

    def relation(i, k):
        """Returns (kind, distance): "line" for end to end, "beside" for side by side."""
        p, q = segments[i]
        r, s = segments[k]
        direction, other = q - p, s - r
        if direction.Length < 1e-9 or other.Length < 1e-9:
            return None, 0.0
        length = direction.Length
        direction = direction / length
        if abs(direction.cross(other / other.Length).z) > 0.01:
            return None, 0.0
        distance = max(abs((r - p).cross(direction).z), abs((s - p).cross(direction).z))
        if distance > across:
            return None, 0.0
        low, high = sorted(((r - p).dot(direction), (s - p).dot(direction)))
        overlap = min(length, high) - max(0.0, low)
        if overlap > thickness:
            return "beside", distance
        if -overlap > gap:
            return None, 0.0
        return "line", distance

    pairs = []
    beside = set()
    for i in range(count):
        for k in range(i + 1, count):
            kind, distance = relation(i, k)
            if kind == "line":
                pairs.append((distance, i, k))
            elif kind == "beside":
                beside.add((i, k))
    # Join the nearest lines first. Two lines that are side by side are never in one score.
    group_of = {i: [i] for i in range(count)}
    for _distance, i, k in sorted(pairs):
        first, second = group_of[i], group_of[k]
        if first is second:
            continue
        if any((min(m, n), max(m, n)) in beside for m in first for n in second):
            continue
        first.extend(second)
        for member in second:
            group_of[member] = first
    groups = []
    for i in range(count):
        if group_of[i] not in groups:
            groups.append(group_of[i])
    return sorted((sorted(group) for group in groups), key=lambda group: group[0])


def scores_of(flat, count=None):
    """The scores of a flat pattern: (groups, numbers).

    groups: for each score, the indexes of its bends. numbers: the number of each score in the
    bend sequence. The property BendOrder gives the numbers, if all bends of a score have the
    same number and each number from 1 is used one time. If not, the numbers are 1, 2, 3.
    """
    if count is None:
        count = len(getattr(flat, "BendAngles", []))
    groups = [[index] for index in range(count)]
    try:
        lines = _bend_lines(flat)[0]
    except (AttributeError, IndexError):
        lines = []
    if len(lines) == count and count:
        segments = [(line.Vertexes[0].Point, line.Vertexes[-1].Point) for line in lines]
        groups = group_scores(segments, thickness_of(flat) or 1.0)
    order = [int(value) for value in getattr(flat, "BendOrder", [])]
    if len(order) == count:
        values = [{order[index] for index in group} for group in groups]
        if all(len(value) == 1 for value in values):
            numbers = [next(iter(value)) for value in values]
            if sorted(numbers) == list(range(1, len(groups) + 1)):
                return groups, numbers
    return groups, list(range(1, len(groups) + 1))


def bend_numbers(flat, count=None):
    """The sequence number of each bend: the number of its score."""
    groups, numbers = scores_of(flat, count)
    result = [0] * sum(len(group) for group in groups)
    for group, number in zip(groups, numbers):
        for index in group:
            result[index] = number
    return result


class Bend:
    """One bend of the model: the part of a score between two panels."""

    def __init__(self, edge, angle, radius, number=0):
        self.start = edge.Vertexes[0].Point
        self.end = edge.Vertexes[-1].Point
        self.angle = angle
        self.radius = radius
        self.number = number

    @property
    def length(self):
        return (self.end - self.start).Length

    @property
    def layer(self):
        return BEND_UP if self.angle >= 0 else BEND_DOWN

    @property
    def direction(self):
        return "UP" if self.angle >= 0 else "DOWN"


class Score:
    """One straight line that folds in one action: one bend or more."""

    def __init__(self, number, bends):
        self.number = number
        self.bends = bends
        self.main = max(bends, key=lambda bend: bend.length)

    def angles(self):
        """The different (direction, angle) values of the bends, in their sequence."""
        values = []
        for bend in self.bends:
            value = (bend.direction, round(abs(bend.angle), 6))
            if value not in values:
                values.append(value)
        return values

    def radii(self):
        values = []
        for bend in self.bends:
            if round(bend.radius, 6) not in values:
                values.append(round(bend.radius, 6))
        return values

    def text(self, degree="°"):
        angles = "/".join(f"{direction} {angle:g}{degree}" for direction, angle in self.angles())
        radii = "/".join(f"{radius:g}" for radius in self.radii())
        return f"{self.number} {angles} R{radii}"

    def text_position(self, height):
        """The start point and the rotation of the text, beside the middle of the longest bend."""
        start, end = self.main.start, self.main.end
        direction = end - start
        rotation = math.degrees(math.atan2(direction.y, direction.x))
        if rotation > 90 or rotation <= -90:
            rotation += 180 if rotation < 0 else -180
        radians = math.radians(rotation)
        along = FreeCAD.Vector(math.cos(radians), math.sin(radians), 0)
        normal = FreeCAD.Vector(-along.y, along.x, 0)
        middle = (start + end) * 0.5
        length = 0.6 * height * len(self.text())
        return middle - along * (length / 2) + normal * (0.4 * height), rotation


class Layout:
    """The parts of a flat pattern in the XY plane, by layer."""

    def __init__(self, flat):
        parts = flat.Shape.SubShapes
        if not parts or not parts[0].Faces:
            raise ValueError("The object has no flat pattern")
        # The flat pattern is in the plane of the root panel. Move it to the XY plane.
        to_xy = parts[0].Placement.inverse().toMatrix()
        face = parts[0].transformed(to_xy, True).Faces[0]
        self.outline = face.OuterWire
        self.cutouts = [wire for wire in face.Wires if not wire.isSame(face.OuterWire)]
        angles = list(getattr(flat, "BendAngles", []))
        radii = list(getattr(flat, "BendRadii", []))
        lines, self.forms = _bend_lines(flat)
        numbers = bend_numbers(flat, len(angles))
        self.bends = []
        for index, edge in enumerate(lines):
            angle = angles[index] if index < len(angles) else 0.0
            radius = radii[index] if index < len(radii) else 0.0
            self.bends.append(Bend(edge, angle, radius, numbers[index] if index < len(numbers) else 0))
        self.scores = []
        if len(lines) == len(angles):
            groups, score_numbers = scores_of(flat, len(angles))
            self.scores = [Score(number, [self.bends[i] for i in group]) for group, number in zip(groups, score_numbers)]
        self.box = face.BoundBox

    def wires(self):
        yield OUTLINE, self.outline
        for wire in self.cutouts:
            yield CUTOUT, wire
        for wire in self.forms:
            yield FORM, wire

    def text_height(self):
        return max(2.0, min(5.0, self.box.DiagonalLength / 60.0))


def _ends(edge):
    """The start and the end of an edge in the direction of its wire."""
    start, end = edge.valueAt(edge.FirstParameter), edge.valueAt(edge.LastParameter)
    return (end, start) if edge.Orientation == "Reversed" else (start, end)


def _middle(edge):
    return edge.valueAt((edge.FirstParameter + edge.LastParameter) / 2)


def _is_counterclockwise(start, middle, end):
    return (middle - start).cross(end - middle).z > 0


def _points(edge):
    points = edge.discretize(Deflection=DEFLECTION)
    return points[::-1] if edge.Orientation == "Reversed" else points


# --------------------------------------------------------------------------------------- DXF


def _dxf(*pairs):
    return "".join(f"{code}\n{value}\n" for code, value in pairs)


def _dxf_edge(edge, layer):
    curve = edge.Curve.TypeId
    start, end = _ends(edge)
    if curve == "Part::GeomLine":
        return _dxf((0, "LINE"), (8, layer), (10, f"{start.x:.6f}"), (20, f"{start.y:.6f}"), (11, f"{end.x:.6f}"), (21, f"{end.y:.6f}"))
    if curve == "Part::GeomCircle":
        center, radius = edge.Curve.Center, edge.Curve.Radius
        if edge.isClosed():
            return _dxf((0, "CIRCLE"), (8, layer), (10, f"{center.x:.6f}"), (20, f"{center.y:.6f}"), (40, f"{radius:.6f}"))
        # An ARC goes against the clock from its start angle to its end angle.
        if not _is_counterclockwise(start, _middle(edge), end):
            start, end = end, start
        a0 = math.degrees(math.atan2(start.y - center.y, start.x - center.x))
        a1 = math.degrees(math.atan2(end.y - center.y, end.x - center.x))
        return _dxf((0, "ARC"), (8, layer), (10, f"{center.x:.6f}"), (20, f"{center.y:.6f}"), (40, f"{radius:.6f}"), (50, f"{a0:.6f}"), (51, f"{a1:.6f}"))
    # Other curves: straight segments.
    text = _dxf((0, "POLYLINE"), (8, layer), (66, 1), (70, 0))
    for point in _points(edge):
        text += _dxf((0, "VERTEX"), (8, layer), (10, f"{point.x:.6f}"), (20, f"{point.y:.6f}"))
    return text + _dxf((0, "SEQEND"))


def dxf_text(flat, labels=True):
    """Return the DXF file of a flat pattern as text."""
    layout = Layout(flat)
    text = _dxf((0, "SECTION"), (2, "HEADER"), (9, "$INSUNITS"), (70, 4), (0, "ENDSEC"))
    text += _dxf((0, "SECTION"), (2, "TABLES"))
    text += _dxf((0, "TABLE"), (2, "LTYPE"), (70, 2))
    text += _dxf((0, "LTYPE"), (2, "CONTINUOUS"), (70, 0), (3, "Solid line"), (72, 65), (73, 0), (40, "0.0"))
    text += _dxf((0, "LTYPE"), (2, "DASHED"), (70, 0), (3, "Dashed"), (72, 65), (73, 2), (40, "6.0"), (49, "4.0"), (49, "-2.0"))
    text += _dxf((0, "ENDTAB"), (0, "TABLE"), (2, "LAYER"), (70, len(LAYERS)))
    for name, (color, _svg, dashed) in LAYERS.items():
        text += _dxf((0, "LAYER"), (2, name), (70, 0), (62, color), (6, "DASHED" if dashed else "CONTINUOUS"))
    text += _dxf((0, "ENDTAB"), (0, "ENDSEC"), (0, "SECTION"), (2, "ENTITIES"))
    for layer, wire in layout.wires():
        for edge in wire.OrderedEdges:
            text += _dxf_edge(edge, layer)
    height = layout.text_height()
    for bend in layout.bends:
        text += _dxf((0, "LINE"), (8, bend.layer), (10, f"{bend.start.x:.6f}"), (20, f"{bend.start.y:.6f}"), (11, f"{bend.end.x:.6f}"), (21, f"{bend.end.y:.6f}"))
    # One label for each score. A score can have more than one bend.
    for bend in layout.scores:
        if labels:
            position, rotation = bend.text_position(height)
            text += _dxf((0, "TEXT"), (8, BEND_TEXT), (10, f"{position.x:.6f}"), (20, f"{position.y:.6f}"), (40, f"{height:.3f}"), (1, bend.text("%%d")), (50, f"{rotation:.3f}"))
    return text + _dxf((0, "ENDSEC"), (0, "EOF"))


# --------------------------------------------------------------------------------------- SVG


def _svg_path(wire, flip):
    """The path data of a closed wire. SVG has Y down, thus Y is turned."""
    commands = []
    for index, edge in enumerate(wire.OrderedEdges):
        start, end = _ends(edge)
        if index == 0:
            commands.append(f"M {start.x:.4f} {flip(start.y):.4f}")
        curve = edge.Curve.TypeId
        if curve == "Part::GeomLine":
            commands.append(f"L {end.x:.4f} {flip(end.y):.4f}")
        elif curve == "Part::GeomCircle":
            radius = edge.Curve.Radius
            if edge.isClosed():
                # A full circle: two half circles.
                opposite = edge.valueAt((edge.FirstParameter + edge.LastParameter) / 2)
                commands.append(f"A {radius:.4f} {radius:.4f} 0 1 0 {opposite.x:.4f} {flip(opposite.y):.4f}")
                commands.append(f"A {radius:.4f} {radius:.4f} 0 1 0 {end.x:.4f} {flip(end.y):.4f}")
            else:
                middle = _middle(edge)
                # The drawing looks the same as the part with Y up. The sweep flag 1 is a turn
                # with the clock on the screen, thus a turn against the clock has the flag 0.
                sweep = 0 if _is_counterclockwise(start, middle, end) else 1
                swept = abs(edge.LastParameter - edge.FirstParameter)
                large = 1 if swept > math.pi else 0
                commands.append(f"A {radius:.4f} {radius:.4f} 0 {large} {sweep} {end.x:.4f} {flip(end.y):.4f}")
        else:
            for point in _points(edge)[1:]:
                commands.append(f"L {point.x:.4f} {flip(point.y):.4f}")
    return " ".join(commands) + " Z"


def svg_text(flat, labels=True):
    """Return the SVG file of a flat pattern as text. The unit is the millimeter."""
    layout = Layout(flat)
    box = layout.box
    margin = 5.0
    width, height = box.XLength + 2 * margin, box.YLength + 2 * margin

    def flip(y):
        return box.YMax - y + margin

    groups = {name: [] for name in LAYERS}
    for layer, wire in layout.wires():
        data = _svg_path(wire, flip)
        groups[layer].append(f'<path d="{data}"/>')
    text_height = layout.text_height()
    for bend in layout.bends:
        groups[bend.layer].append(
            f'<line x1="{bend.start.x:.4f}" y1="{flip(bend.start.y):.4f}" x2="{bend.end.x:.4f}" y2="{flip(bend.end.y):.4f}"/>'
        )
    for bend in layout.scores:
        if labels:
            position, rotation = bend.text_position(text_height)
            x, y = position.x, flip(position.y)
            groups[BEND_TEXT].append(
                f'<text x="{x:.4f}" y="{y:.4f}" font-size="{text_height:.2f}" transform="rotate({-rotation:.3f} {x:.4f} {y:.4f})">{bend.text()}</text>'
            )
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" '
        f'width="{width:.3f}mm" height="{height:.3f}mm" viewBox="{box.XMin - margin:.3f} 0 {width:.3f} {height:.3f}">',
    ]
    for name, (_color, color, dashed) in LAYERS.items():
        if not groups[name]:
            continue
        if name == BEND_TEXT:
            style = f'fill="{color}" stroke="none" font-family="sans-serif"'
        else:
            style = f'fill="none" stroke="{color}" stroke-width="0.25"' + (' stroke-dasharray="4 2"' if dashed else "")
        lines.append(f'  <g id="{name}" inkscape:groupmode="layer" inkscape:label="{name}" {style}>')
        lines += ["    " + item for item in groups[name]]
        lines.append("  </g>")
    lines.append("</svg>")
    return "\n".join(lines) + "\n"


def export(flat, path, labels=True):
    """Write a flat pattern to a file. The extension selects the format: .dxf or .svg."""
    lower = path.lower()
    if lower.endswith(".svg"):
        text = svg_text(flat, labels)
    elif lower.endswith(".dxf"):
        text = dxf_text(flat, labels)
    else:
        raise ValueError("The file name must end with .dxf or .svg")
    with open(path, "w", encoding="utf-8") as handle:
        handle.write(text)
