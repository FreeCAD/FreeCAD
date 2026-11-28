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

"""A TechDraw drawing of a flat pattern: the view, the bend notes, and the bend table.

The drawing has these objects:

    the view     a TechDraw part view of the outline, thus dimensions are possible
    the notes    a symbol on the view with the bend lines and the number of each bend
    the table    a symbol with the direction, the angle, and the radius of each bend

The notes and the table read the flat pattern again at each recompute. They follow a change
of the part.
"""

import os

import FreeCAD

import SheetDesignExport

#: The scales that the drawing selects from, largest first.
SCALES = (2.0, 1.0, 0.5, 0.4, 0.2, 0.1, 0.05, 0.02)

UP_COLOR = "#008000"
DOWN_COLOR = "#a000a0"
TEXT_COLOR = "#000000"


def _svg(width, height, min_x, min_y, body):
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" version="1.1" width="{width:.3f}mm" height="{height:.3f}mm" '
        f'viewBox="{min_x:.3f} {min_y:.3f} {width:.3f} {height:.3f}">\n' + "\n".join(body) + "\n</svg>\n"
    )


def notes_svg(flat, scale):
    """The bend lines and the bend numbers, in the position of the view. The unit is mm on paper."""
    layout = SheetDesignExport.Layout(flat)
    box = layout.box
    center_x, center_y = (box.XMin + box.XMax) / 2, (box.YMin + box.YMax) / 2
    margin = 12.0
    half_x = box.XLength * scale / 2 + margin
    half_y = box.YLength * scale / 2 + margin

    def paper(point):
        return (point.x - center_x) * scale, -(point.y - center_y) * scale

    body = []
    radius = 3.2
    for bend in layout.bends:
        color = UP_COLOR if bend.angle >= 0 else DOWN_COLOR
        x1, y1 = paper(bend.start)
        x2, y2 = paper(bend.end)
        body.append(
            f'<line x1="{x1:.3f}" y1="{y1:.3f}" x2="{x2:.3f}" y2="{y2:.3f}" stroke="{color}" '
            f'stroke-width="0.35" stroke-dasharray="6 2 1 2" fill="none"/>'
        )
    # One number for each score, in a circle on the middle of its longest bend.
    for score in layout.scores:
        bend = score.main
        color = UP_COLOR if bend.angle >= 0 else DOWN_COLOR
        x1, y1 = paper(bend.start)
        x2, y2 = paper(bend.end)
        mx, my = (x1 + x2) / 2, (y1 + y2) / 2
        body.append(f'<circle cx="{mx:.3f}" cy="{my:.3f}" r="{radius}" fill="#ffffff" stroke="{color}" stroke-width="0.35"/>')
        body.append(
            f'<text x="{mx:.3f}" y="{my + 1.3:.3f}" font-size="3.5" font-family="sans-serif" text-anchor="middle" '
            f'fill="{TEXT_COLOR}">{bend.number}</text>'
        )
    # A rectangle with no color keeps the middle of the symbol on the middle of the view.
    body.insert(0, f'<rect x="{-half_x:.3f}" y="{-half_y:.3f}" width="{2 * half_x:.3f}" height="{2 * half_y:.3f}" fill="none" stroke="none"/>')
    return _svg(2 * half_x, 2 * half_y, -half_x, -half_y, body)


def table_rows(flat):
    """The rows of the bend table: one row for each score, in the bend sequence.

    A row is (number, direction, angle, radius). A score with bends of different angles shows
    each value."""
    layout = SheetDesignExport.Layout(flat)
    rows = []
    for score in layout.scores:
        values = score.angles()
        directions = []
        for direction, _angle in values:
            if direction.capitalize() not in directions:
                directions.append(direction.capitalize())
        rows.append(
            (
                score.number,
                "/".join(directions),
                "/".join(f"{angle:g}°" for _direction, angle in values),
                "/".join(f"{radius:g}" for radius in score.radii()),
            )
        )
    return sorted(rows), layout


def scale_text(scale):
    return f"{scale:g}:1" if scale >= 1.0 else f"1:{1.0 / scale:g}"


def table_svg(flat, scale=None):
    """The bend table with the flat size. The unit is mm on paper."""
    rows, layout = table_rows(flat)
    columns = (("Bend", 14.0), ("Direction", 22.0), ("Angle", 18.0), ("Radius", 18.0))
    width = sum(size for _title, size in columns)
    row = 6.0
    lines = [f"Flat size: {layout.box.XLength:.2f} x {layout.box.YLength:.2f} mm"]
    thickness = SheetDesignExport.thickness_of(flat)
    if thickness is not None:
        lines.append(f"Thickness: {thickness:g} mm")
    if scale is not None:
        lines.append(f"Scale of the view: {scale_text(scale)}")
    height = row * (len(rows) + 1) + row * len(lines) + 2.0
    body = []
    text = 'font-size="3.5" font-family="sans-serif" fill="#000000"'
    stroke = 'stroke="#000000" stroke-width="0.25" fill="none"'
    table_height = row * (len(rows) + 1)
    body.append(f'<rect x="0" y="0" width="{width}" height="{table_height}" {stroke}/>')
    x = 0.0
    for index, (title, size) in enumerate(columns):
        if index:
            body.append(f'<line x1="{x}" y1="0" x2="{x}" y2="{table_height}" {stroke}/>')
        body.append(f'<text x="{x + size / 2}" y="{row - 1.8}" text-anchor="middle" font-weight="bold" {text}>{title}</text>')
        x += size
    for number, values in enumerate(rows):
        y = row * (number + 1)
        body.append(f'<line x1="0" y1="{y}" x2="{width}" y2="{y}" {stroke}/>')
        x = 0.0
        for value, (_title, size) in zip(values, columns):
            body.append(f'<text x="{x + size / 2}" y="{y + row - 1.8}" text-anchor="middle" {text}>{value}</text>')
            x += size
    for number, line in enumerate(lines):
        body.append(f'<text x="0" y="{table_height + row * (number + 1) - 1.0}" {text}>{line}</text>')
    return _svg(width + 0.5, height, -0.25, -0.25, body)


class _Symbol:
    """Base of the proxies: a TechDraw symbol that is made again from a flat pattern."""

    def __init__(self, obj):
        obj.addProperty("App::PropertyLink", "Source", "Sheet Design", "The flat pattern")
        obj.Proxy = self

    def execute(self, obj):
        flat = obj.Source
        if flat is not None and flat.isValid() and not flat.Shape.isNull():
            text = self.make(obj, flat)
            if obj.Symbol != text:
                obj.Symbol = text
        return False  # The TechDraw code of the symbol runs after this.

    def dumps(self):
        return None

    def loads(self, _state):
        return None


class BendNotes(_Symbol):
    """The bend lines and the bend numbers on the view of the flat pattern."""

    def __init__(self, obj):
        super().__init__(obj)
        obj.addProperty("App::PropertyLink", "View", "Sheet Design", "The view of the flat pattern")

    def make(self, obj, flat):
        view = obj.View
        scale = view.getScale() if view is not None else 1.0
        if view is not None:
            # The notes stay on the view.
            for name in ("X", "Y"):
                if abs(getattr(obj, name).Value - getattr(view, name).Value) > 1e-6:
                    setattr(obj, name, getattr(view, name).Value)
        return notes_svg(flat, scale)


class BendTable(_Symbol):
    """The table of the bends, in the bend sequence."""

    def __init__(self, obj):
        super().__init__(obj)
        obj.addProperty("App::PropertyLink", "View", "Sheet Design", "The view of the flat pattern")

    def make(self, obj, flat):
        return table_svg(flat, obj.View.getScale() if obj.View is not None else None)


def _template_file():
    folder = os.path.join(FreeCAD.getResourceDir(), "Mod", "TechDraw", "Templates")
    for name in (os.path.join("ISO", "A3_Landscape_blank.svg"), "Default_Template_A4_Landscape.svg"):
        path = os.path.join(folder, name)
        if os.path.exists(path):
            return path
    raise FileNotFoundError("No TechDraw template was found")


def make_drawing(flat, template=None):
    """Make a drawing page for a flat pattern. Returns (page, view, notes, table)."""
    import TechDraw  # noqa: F401  Registers the object types.

    doc = flat.Document
    layout = SheetDesignExport.Layout(flat)
    page = doc.addObject("TechDraw::DrawPage", "FlatPatternPage")
    sheet = doc.addObject("TechDraw::DrawSVGTemplate", "Template")
    sheet.Template = template or _template_file()
    page.Template = sheet
    width, height = sheet.Width.Value, sheet.Height.Value

    # The view shows the outline only. The notes draw the bend lines, with a line type.
    outline = doc.addObject("SheetDesign::FlatPattern", flat.Name + "Outline")
    outline.Source = flat.Source
    outline.BendLines = False
    outline.Placement = flat.Placement
    if outline.ViewObject is not None:
        outline.ViewObject.hide()

    area_x, area_y = width * 0.55, height * 0.60
    scale = next(
        (s for s in SCALES if layout.box.XLength * s <= area_x and layout.box.YLength * s <= area_y), SCALES[-1]
    )
    rotation = flat.Shape.SubShapes[0].Placement.Rotation
    view = doc.addObject("TechDraw::DrawViewPart", "FlatPatternView")
    page.addView(view)
    view.Source = [outline]
    view.Direction = rotation.multVec(FreeCAD.Vector(0, 0, 1))
    view.XDirection = rotation.multVec(FreeCAD.Vector(1, 0, 0))
    view.ScaleType = "Custom"
    view.Scale = scale
    view.X = width * 0.36
    view.Y = height * 0.56

    notes = doc.addObject("TechDraw::DrawViewSymbolPython", "BendNotes")
    BendNotes(notes)
    page.addView(notes)
    notes.Source = flat
    notes.View = view
    notes.X, notes.Y = view.X, view.Y

    table = doc.addObject("TechDraw::DrawViewSymbolPython", "BendTable")
    BendTable(table)
    page.addView(table)
    table.Source = flat
    table.View = view
    table.X = width * 0.80
    table.Y = height * 0.62
    doc.recompute()
    return page, view, notes, table
