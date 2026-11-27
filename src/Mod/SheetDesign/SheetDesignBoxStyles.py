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

"""Parametric blanks for the box styles of the International Fibreboard Case Code.

This is data of the Packaging domain. Each style function gives the pieces of one box style as
flat outlines with bend lines and angles, for the Refold operation. The styles follow the blank
drawings of the Fibre Box Handbook (Box Styles chapter). The drawings have no dimensions, thus
the proportions here are an interpretation.

A blank has layers: a flap that folds across a second flap has its score one thickness higher.
"""

import math

import FreeCAD
import Part

V = FreeCAD.Vector


class Panel:
    """A flat region of a blank with four corners.

    The corners a and b are on the side of the hinge. The corners c and d are on the far side.
    """

    def __init__(self, a, b, c, d):
        self.a, self.b, self.c, self.d = a, b, c, d

    def edge(self, name):
        """Return the two points of a side. The outward side is on the right of p to q."""
        return {
            "far": (self.d, self.c),
            "right": (self.c, self.b),
            "left": (self.a, self.d),
            "near": (self.b, self.a),
        }[name]

    @property
    def width(self):
        return self.a.distanceToPoint(self.b)

    @property
    def length(self):
        return self.a.distanceToPoint(self.d)


class Piece:
    """One piece of a box: its outline, its bend lines, and the angle of each line."""

    def __init__(self, name, face, lines, angles):
        self.name = name
        self.face = face
        self.lines = lines
        self.angles = angles


class Blank:
    """Builds the outline and the bend lines of one piece from panels and flaps."""

    def __init__(self, thickness, k_factor=0.5, radius=0.0):
        self.t = thickness
        self.k = k_factor
        self.r = radius
        self.faces = []
        self.holes = []
        self.lines = []
        self.angles = []

    def allowance(self, angle):
        return (self.r + self.k * self.t) * math.radians(abs(angle))

    def _add(self, points):
        # All faces must have the same turn direction, or they do not join into one outline.
        area = 0.0
        for i, p in enumerate(points):
            q = points[(i + 1) % len(points)]
            area += p.x * q.y - q.x * p.y
        if area < 0:
            points = points[::-1]
        self.faces.append(Part.Face(Part.makePolygon(points + [points[0]])))

    def panel(self, x0, y0, x1, y1):
        """Add the first panel: a rectangle.

        Its sides are: "far" at x1 (east), "near" at x0 (west), "right" at y1 (north), and
        "left" at y0 (south).
        """
        self._add([V(x0, y0, 0), V(x1, y0, 0), V(x1, y1, 0), V(x0, y1, 0)])
        return Panel(V(x0, y0, 0), V(x0, y1, 0), V(x1, y1, 0), V(x1, y0, 0))

    def polygon(self, points):
        """Add a panel of any outline. Returns its corner points."""
        points = [V(x, y, 0) for x, y in points]
        self._add(points)
        return points

    def flap(
        self,
        hinge,
        length,
        angle=90.0,
        offset=0.0,
        inset=(0.0, 0.0),
        widen=(0.0, 0.0),
        taper=(0.0, 0.0),
        round_corners=0.0,
    ):
        """Add a bend and a panel on a hinge.

        hinge: two points. The new panel is on the right of the direction from the first point
               to the second point.
        offset: a strip that makes the parent longer before the score (a layer offset).
        inset: the score is shorter than the hinge at its two ends.
        widen: the new panel is wider than the score at its two ends.
        taper: the far side of the new panel is shorter at its two ends.
        round_corners: radius of the two far corners of the new panel.
        """
        p, q = hinge
        span = p.distanceToPoint(q)
        u = (q - p) / span
        n = V(u.y, -u.x, 0)
        allowance = self.allowance(angle)

        def at(s, depth):
            return p + u * s + n * depth

        s0, s1 = inset[0], span - inset[1]
        if offset > 0:
            self._add([at(s0, 0), at(s1, 0), at(s1, offset), at(s0, offset)])
        near = offset + allowance
        far = near + length
        self._add([at(s0, offset), at(s1, offset), at(s1, near), at(s0, near)])
        a, b = at(s0 - widen[0], near), at(s1 + widen[1], near)
        c, d = at(s1 + widen[1] - taper[1], far), at(s0 - widen[0] + taper[0], far)
        if round_corners > 0:
            rho = min(round_corners, length)
            lo, hi = s0 - widen[0], s1 + widen[1]
            diagonal = rho * (1 - math.cos(math.pi / 4))
            # The sequence goes against the clock, as all other faces do.
            edges = [Part.makeLine(b, a)]
            if length - rho > 1e-9:
                edges.append(Part.makeLine(a, at(lo, far - rho)))
            edges.append(Part.Arc(at(lo, far - rho), at(lo + diagonal, far - diagonal), at(lo + rho, far)).toShape())
            edges.append(Part.makeLine(at(lo + rho, far), at(hi - rho, far)))
            edges.append(Part.Arc(at(hi - rho, far), at(hi - diagonal, far - diagonal), at(hi, far - rho)).toShape())
            if length - rho > 1e-9:
                edges.append(Part.makeLine(at(hi, far - rho), b))
            self.faces.append(Part.Face(Part.Wire(edges)))
            c, d = at(hi, far), at(lo, far)
        else:
            self._add([a, b, c, d])
        self.lines.append(Part.makeLine(at(s0, offset + allowance / 2), at(s1, offset + allowance / 2)))
        self.angles.append(float(angle))
        return Panel(a, b, c, d)

    def score(self, p, q, angle):
        """Add a bend line that is not the hinge of a flap, for example a diagonal."""
        self.lines.append(Part.makeLine(p, q))
        self.angles.append(float(angle))

    def hole(self, face):
        self.holes.append(face)

    def circle(self, x, y, radius):
        self.hole(Part.Face(Part.Wire(Part.makeCircle(radius, V(x, y, 0)))))

    def slot(self, x0, y0, x1, y1):
        self.hole(Part.Face(Part.makePolygon([V(x0, y0, 0), V(x1, y0, 0), V(x1, y1, 0), V(x0, y1, 0), V(x0, y0, 0)])))

    def piece(self, name):
        shape = self.faces[0] if len(self.faces) == 1 else self.faces[0].fuse(self.faces[1:]).removeSplitter()
        for hole in self.holes:
            shape = shape.cut(hole)
        if len(shape.Faces) != 1:
            raise ValueError(f"{name}: the blank has {len(shape.Faces)} faces")
        return Piece(name, shape.Faces[0], self.lines, self.angles)


# ---------------------------------------------------------------------------------- builders


def tube(blank, widths, height, angles=90.0):
    """A row of panels with a score between each two. Returns the panels."""
    if not isinstance(angles, (list, tuple)):
        angles = [angles] * (len(widths) - 1)
    panels = [blank.panel(0, 0, widths[0], height)]
    for width, angle in zip(widths[1:], angles):
        panels.append(blank.flap(panels[-1].edge("far"), width, angle))
    return panels


#: The width of the manufacturer's joint strip.
JOINT = 35.0


def add_joint(blank, panel, width=JOINT, taper=0.0):
    """Add the manufacturer's joint: a strip on the free end of the first panel of a tube.

    The strip is as high as the panel, without the flaps. It goes on the outer side of the last
    panel, where the box is glued or stitched, thus its score is one thickness farther out.
    taper: the two free corners of the strip are cut at an angle by this size.
    """
    t = blank.t
    return blank.flap(panel.edge("near"), width, 90.0, offset=t, inset=(t, t), taper=(taper, taper))


def slotted(t, widths, depth, bottom=None, top=None, joint=JOINT, joint_taper=0.0):
    """A slotted box: a tube with flaps and a joint strip. A flap is (length, layer) or None.

    In the row of panels, "right" is the bottom side and "left" is the top side.
    """
    blank = Blank(t)
    panels = tube(blank, widths, depth)
    if joint:
        add_joint(blank, panels[0], joint, joint_taper)
    for side, flaps in (("right", bottom), ("left", top)):
        if not flaps:
            continue
        for panel, spec in zip(panels, flaps):
            if spec is None:
                continue
            length, layer = spec[0], spec[1]
            options = spec[2] if len(spec) > 2 else {}
            blank.flap(panel.edge(side), length, 90.0, offset=layer * t, **options)
    return blank


def tray(t, length, width, depth, corner=None, flap=None, blank=None):
    """A tray: a bottom and four walls. corner: None, "end" or "side" (who has the flaps).

    A corner flap is almost as long as the depth, as in the handbook drawings.
    """
    blank = blank or Blank(t)
    # Two flaps on one wall must not meet.
    along = length if corner == "end" else width
    flap = flap or min(depth - 2 * t, 0.45 * along)
    base = blank.panel(0, 0, length, width)
    walls = {}
    for name, key in (("east", "far"), ("west", "near"), ("north", "right"), ("south", "left")):
        ends = name in ("east", "west")
        # The walls with the corner flaps are wider by t on each side, thus their flaps are
        # outside the other walls.
        wide = (t, t) if (corner == "end" and ends) or (corner == "side" and not ends) else (0.0, 0.0)
        wall = blank.flap(base.edge(key), depth, 90.0, widen=wide)
        walls[name] = wall
        if wide[0] > 0:
            blank.flap(wall.edge("left"), flap, 90.0)
            blank.flap(wall.edge("right"), flap, 90.0)
    return blank, base, walls


# ------------------------------------------------------------------------------------ styles
# Each function gives a list of pieces. L, W, D are the inside length, width, and depth.


def _rsc(t, L, W, D, bottom, top):
    return [slotted(t, [L, W, L, W], D, bottom, top).piece("box")]


def style_0200(t, L, W, D):
    flaps = [(W / 2, 1), (W / 2, 0), (W / 2, 1), (W / 2, 0)]
    return _rsc(t, L, W, D, flaps, None)


def style_0201(t, L, W, D):
    flaps = [(W / 2, 1), (W / 2, 0), (W / 2, 1), (W / 2, 0)]
    return _rsc(t, L, W, D, flaps, flaps)


def style_0202(t, L, W, D):
    flaps = [(W / 2 + 15, 1), (W / 2 + 15, 0), (W / 2 + 15, 2), (W / 2 + 15, 0)]
    return _rsc(t, L, W, D, flaps, flaps)


def style_0203(t, L, W, D):
    flaps = [(W, 2), (W, 0), (W, 3), (W, 1)]
    return _rsc(t, L, W, D, flaps, flaps)


def style_0204(t, L, W, D):
    flaps = [(W / 2, 1), (L / 2, 0), (W / 2, 1), (L / 2, 0)]
    return _rsc(t, L, W, D, flaps, flaps)


def style_0205(t, L, W, D):
    flaps = [(L / 2, 1), (L / 2, 0), (L / 2, 2), (L / 2, 0)]
    return _rsc(t, L, W, D, flaps, flaps)


def style_0206(t, L, W, D):
    flaps = [(W, 1), (L / 2, 0), (W, 2), (L / 2, 0)]
    return _rsc(t, L, W, D, flaps, flaps)


def _frame(panel):
    """The direction along the hinge of a panel, and the direction to its far side."""
    u = (panel.b - panel.a) / panel.a.distanceToPoint(panel.b)
    return u, V(u.y, -u.x, 0)


def _tongue(blank, panel, length, hook=(0.0, 0.0), chamfer=0.0):
    """Add a tongue on the far side of a panel, in the plane of the panel.

    hook: the free end of the tongue is wider than its neck at the two ends (a barb).
    chamfer: the two free corners are cut at an angle.
    """
    u, n = _frame(panel)
    d, c = panel.d, panel.c
    neck = 0.4 * length
    points = [d, c]
    if hook[1] > 0:
        points += [c + n * neck, c + n * neck + u * hook[1]]
    hi, lo = c + u * hook[1], d - u * hook[0]
    points += [hi + n * (length - chamfer), hi + n * length - u * chamfer] if chamfer > 0 else [hi + n * length]
    points += [lo + n * length + u * chamfer, lo + n * (length - chamfer)] if chamfer > 0 else [lo + n * length]
    if hook[0] > 0:
        points += [d + n * neck - u * hook[0], d + n * neck]
    blank._add(points)


def _notch(blank, panel, width, depth):
    """Cut a rectangular notch in the middle of the far side of a panel."""
    u, n = _frame(panel)
    middle = (panel.c + panel.d) * 0.5
    p, q = middle - u * (width / 2), middle + u * (width / 2)
    blank.hole(Part.Face(Part.makePolygon([p + n, q + n, q - n * depth, p - n * depth, p + n])))


def _tab(blank, panel, position, width, length):
    """Add a rectangular tab on the far side of a panel. position: 0 to 1 along that side."""
    u, n = _frame(panel)
    middle = panel.d + (panel.c - panel.d) * position
    p, q = middle - u * (width / 2), middle + u * (width / 2)
    blank._add([p, q, q + n * length, p + n * length])


def _window(blank, panel, s0, s1, d0, d1):
    """Cut a rectangular hole in a panel. s is along the hinge from the corner a, and d is
    the distance from the hinge side."""
    u, n = _frame(panel)
    points = [panel.a + u * s + n * d for s, d in ((s0, d0), (s1, d0), (s1, d1), (s0, d1), (s0, d0))]
    blank.hole(Part.Face(Part.makePolygon(points)))


def _slit(blank, panel, points, width=1.0):
    """Cut a slit along a line of points (s, d) in a panel. See _window for s and d."""
    u, n = _frame(panel)
    for (s0, d0), (s1, d1) in zip(points, points[1:]):
        p, q = panel.a + u * s0 + n * d0, panel.a + u * s1 + n * d1
        along = (q - p) / p.distanceToPoint(q)
        side = V(along.y, -along.x, 0) * (width / 2)
        p, q = p - along * (width / 2), q + along * (width / 2)
        blank.hole(Part.Face(Part.makePolygon([p - side, q - side, q + side, p + side, p - side])))


def _snap_bottom(blank, panels, L, W, hooks=False):
    """The bottom of a snap lock box: four die-cut flaps of the same depth.

    The first long panel has the largest flap, with a notch. Each end panel has a flap with
    one diagonal side and a tongue beside the second long panel. The second long panel has
    a flap with two diagonal sides and a tongue in the middle.
    hooks: the tongues of the end panels have a barb, and the middle tongue has cut corners.
    """
    t = blank.t
    depth = 0.75 * W
    neck, tip = 0.7 * depth, 0.3 * depth
    large = blank.flap(panels[0].edge("right"), depth, 90.0)
    _notch(blank, large, 0.4 * L, tip)
    barb = 0.12 * W if hooks else 0.0
    # On the edge "right", the first end of a flap is at the side of the next panel.
    end = blank.flap(panels[1].edge("right"), neck, 90.0, offset=2 * t, taper=(0.0, 0.57 * W))
    _tongue(blank, end, tip, hook=(0.0, barb))
    middle = blank.flap(panels[2].edge("right"), neck, 90.0, offset=t, taper=(0.32 * L, 0.32 * L))
    _tongue(blank, middle, tip, chamfer=0.25 * tip if hooks else 0.0)
    end = blank.flap(panels[3].edge("right"), neck, 90.0, offset=3 * t, taper=(0.57 * W, 0.0))
    _tongue(blank, end, tip, hook=(barb, 0.0))


def _tuck_top(t, L, W, D, blank, panels):
    """A lid on the first panel with a tuck flap, and dust flaps on the end panels."""
    lid = blank.flap(panels[0].edge("left"), W - t, 90.0, offset=t)
    blank.flap(lid.edge("far"), 0.2 * W, 90.0, inset=(t, t), round_corners=0.1 * W)
    for panel in (panels[1], panels[3]):
        # The dust flaps are narrower than the box, thus the tuck flap of the lid has its space.
        blank.flap(panel.edge("left"), W / 2, 90.0, inset=(2 * t, 2 * t))


def _snap_box(t, L, W, D, hooks):
    blank = Blank(t)
    panels = tube(blank, [L, W, L, W], D)
    add_joint(blank, panels[0])
    _snap_bottom(blank, panels, L, W, hooks)
    return blank, panels


def style_0215(t, L, W, D):
    """A snap lock bottom, and a tuck top on the panel of the largest bottom flap."""
    blank, panels = _snap_box(t, L, W, D, hooks=False)
    _tuck_top(t, L, W, D, blank, panels)
    return [blank.piece("box")]


def style_0216(t, L, W, D):
    """A snap lock bottom with hooks, and the top of an RSC."""
    blank, panels = _snap_box(t, L, W, D, hooks=True)
    for panel, layer in zip(panels, (1, 0, 1, 0)):
        blank.flap(panel.edge("left"), W / 2, 90.0, offset=layer * t)
    return [blank.piece("box")]


def style_0225(t, L, W, D):
    """A full bottom file box. The row is W, L, W, L with the joint on the last panel.

    The first long panel has a full bottom flap with a wing on each side. The wings fold up
    on the inner side of the end panels. The second long panel has a second full bottom flap.
    The end panels have no bottom flap.
    """
    blank = Blank(t)
    panels = tube(blank, [W, L, W, L], D)
    blank.flap(panels[-1].edge("far"), JOINT, 90.0, offset=t, inset=(t, t), taper=(JOINT / 3, JOINT / 3))
    for panel, layer in zip(panels, (0, 1, 0, 1)):
        blank.flap(panel.edge("left"), W / 2, 90.0, offset=layer * t)
    # The bottom flap with its wings is narrower than the box by the two wings.
    bottom = blank.flap(panels[1].edge("right"), W - 2 * t, 90.0, inset=(t, t))
    wing = min(0.75 * W, D - 4 * t)
    blank.flap(bottom.edge("left"), wing, 90.0)
    blank.flap(bottom.edge("right"), wing, 90.0)
    blank.flap(panels[3].edge("right"), W - 4 * t, 90.0, offset=t, inset=(2 * t, 2 * t))
    return [blank.piece("box")]


def style_0226(t, L, W, D, joint=JOINT):
    """A bellows top and bottom: the flaps of all four panels are one band with no slot.

    The blank is one rectangle: the manufacturer's joint strip, then the panels L, W, L, W.

    Each flap of an end panel has two diagonal scores. Its center triangle folds in from the end
    wall. Its two side triangles fold flat under the flaps of the long panels, thus they are
    hidden when the box is closed. At each corner, the long wall, the end wall, the long flap,
    the side triangle, and the center triangle make a closed loop of panels.

    The end walls are lower by two thicknesses at the top and at the bottom. This gives the
    space for the two layers of triangles below the long flaps.

    The joint strip is on the first long panel, and it has the full height of the blank. It goes
    on the outer side of the last end panel. Its two flap parts have the same diagonal score as
    the corner of that end panel, and they fold together with that corner. The board is thus
    double in a small zone at the joint corner. A model of stiff panels cannot show this without
    material in the same space: the folded model has an Overlap there, as large as the two flap
    parts of the strip. The last end panel is complete: its outer side triangle has no long
    flap to hold it, and folds flat on its center triangle.
    """
    blank = Blank(t)
    a = blank.allowance(90.0)
    half = W / 2
    spans = []
    x = 0.0
    for width in (L, W, L, W):
        spans.append((x, x + width))
        x += width + a
    right = spans[-1][1]
    # The strip is the outer layer, thus the first long panel is longer by one thickness.
    start = -t
    spans[0] = (start, spans[0][1])
    left = start - a - joint
    blank.faces.append(
        Part.Face(
            Part.makePolygon(
                [V(left, -half, 0), V(right, -half, 0), V(right, D + half, 0), V(left, D + half, 0), V(left, -half, 0)]
            )
        )
    )
    # The scores between the walls. They stop below the layers of the triangles.
    for x0, x1 in spans[:-1]:
        blank.score(V(x1 + a / 2, 2 * t, 0), V(x1 + a / 2, D - 2 * t, 0), 90.0)
    blank.score(V(start - a / 2, 2 * t, 0), V(start - a / 2, D - 2 * t, 0), 90.0)  # The strip.

    def corner(edge, toward, base, up, kind="full"):
        """One corner. edge: the X of the end of the long panel. toward: +1 if the end panel
        is at a larger X. base: the Y of the top or the bottom of the long walls. up: +1 for
        the top flaps, -1 for the bottom flaps.

        kind "full": a long flap, a side triangle, and a center triangle.
        kind "strip": the same on the joint strip. The diagonal ends on the edge of the strip.
        kind "free": the outer corner of the last end panel. Only the diagonal.
        """

        def at(u, v):
            return V(edge + toward * u, base + up * v, 0)

        def relief(p, q):
            blank.slot(min(p.x, q.x), min(p.y, q.y), max(p.x, q.x), max(p.y, q.y))

        if kind == "free":
            relief(at(a, -2 * t), at(2 * a, a))
            blank.score(at(2 * a, a), at(a + half, half), 180.0)  # Side to center triangle.
            return
        relief(at(0, -2 * t), at(a, a))
        blank.score(at(a, a), at(a, half), 180.0)  # Long flap to side triangle.
        if kind == "strip":
            blank.score(at(2 * a, a), at(a + joint, joint), 180.0)
            y = base + up * (-2 * t + a / 2)
            blank.score(V(edge + toward * a, y, 0), V(edge + toward * (a + joint), y, 0), 90.0)
        else:
            blank.score(at(2 * a, a), at(a + half, half), 180.0)  # Side to center triangle.

    for base, up in ((D, 1), (0.0, -1)):
        for index, (x0, x1) in enumerate(spans):
            if index % 2 == 0:
                # A long panel: the score of its flap, and a corner at each end.
                blank.score(V(x0, base + up * a / 2, 0), V(x1, base + up * a / 2, 0), 90.0)
                corner(x1, 1, base, up)
                corner(x0, -1, base, up, "full" if index > 0 else "strip")
            else:
                # An end panel: the score of its center triangle, two thicknesses lower.
                y = base + up * (-2 * t + a / 2)
                blank.score(V(x0, y, 0), V(x1, y, 0), 90.0)
        corner(right + a, -1, base, up, "free")
    return [blank.piece("box")]


def style_0228(t, L, W, D):
    """An RSC with a divider. The row is: joint, divider, a half long panel, end panel, long
    panel, end panel, and a second half long panel.

    The two half panels make one long side. The second half is on the outer side of the
    first. The divider goes across the box, and its joint strip is on the long panel. Each
    panel but the divider has a top flap and a bottom flap.
    """
    blank = Blank(t)
    panels = tube(blank, [W - 3 * t, L / 2, W, L, W + t, 0.59 * L], D)
    blank.flap(panels[0].edge("near"), JOINT, 90.0, inset=(t, t), taper=(JOINT / 3, JOINT / 3))
    # The layer of each pair of flaps. The second half panel is one layer out.
    for panel, layer in zip(panels[1:], (1, 0, 1, 0, 2)):
        for side in ("left", "right"):
            blank.flap(panel.edge(side), W / 2 - (t if layer == 0 else 0), 90.0, offset=layer * t)
    return [blank.piece("box")]


def style_0301(t, L, W, D):
    """The two trays of the handbook. Side slotted (SS): the slots are in the side panels, and
    the corner flaps are on the end panels. End slotted (ES): the flaps are on the side panels."""
    return [
        tray(t, L, W, D / 2, corner="end")[0].piece("tray, side slotted"),
        tray(t, L, W, D / 2, corner="side")[0].piece("tray, end slotted"),
    ]



def _cover(t, L, W, depth, name="cover", corner="side"):
    """A cover for a body. In the handbook, the cover of a design style box is end slotted."""
    return tray(t, L + 2 * t + 3, W + 2 * t + 3, depth, corner=corner)[0].piece(name)


def style_0301_ftd(t, L, W, D):
    return [tray(t, L, W, D, corner="end")[0].piece("body"), _cover(t, L, W, D)]


def style_0306(t, L, W, D):
    return [tray(t, L, W, D, corner="end")[0].piece("body"), _cover(t, L, W, D / 4)]


def style_0310(t, L, W, D):
    body = Blank(t)
    add_joint(body, tube(body, [L, W, L, W], D)[0])
    # The covers of this style are the side slotted type.
    covers = [_cover(t, L, W, D / 4, name, corner="end") for name in ("cover 1", "cover 2")]
    return [body.piece("tube")] + covers


def style_0320(t, L, W, D):
    flaps = [(W / 2, 1), (W / 2, 0), (W / 2, 1), (W / 2, 0)]
    outer = slotted(t, [L + 2 * t + 3, W + 2 * t + 3, L + 2 * t + 3, W + 2 * t + 3], D, flaps, None)
    return [slotted(t, [L, W, L, W], D, flaps, None).piece("inner"), outer.piece("outer")]



def _flange_cover(t, L, W, depth, name):
    """A cover with no corner flaps: each flange has two sections. The second section folds
    in below the flange of the tube."""
    blank, _base, walls = tray(t, L + 2 * t + 3, W + 2 * t + 3, depth, corner=None)
    for wall in walls.values():
        blank.flap(wall.edge("far"), 0.6 * depth, 90.0, inset=(depth, depth))
    return blank.piece(name)


def style_0325(t, L, W, D):
    """A tube with flanges that fold to the outer side, and two covers that lock on them."""
    body = Blank(t)
    panels = tube(body, [L, W, L, W], D)
    add_joint(body, panels[0])
    for panel in panels:
        for side in ("left", "right"):
            body.flap(panel.edge(side), 30.0, -90.0)
    return [body.piece("tube"), _flange_cover(t, L, W, D / 4, "cover 1"), _flange_cover(t, L, W, D / 4, "cover 2")]


def _octagon_cover(t, side, depth, name):
    """An octagonal bottom with eight walls."""
    blank = Blank(t)
    radius = side / (2 * math.sin(math.pi / 8))
    points = [
        (radius * math.cos(math.pi / 8 + i * math.pi / 4), radius * math.sin(math.pi / 8 + i * math.pi / 4))
        for i in range(8)
    ]
    corners = blank.polygon(points)
    for i in range(8):
        # The polygon goes against the clock, thus the outer side is on the right of p to q.
        blank.flap((corners[i], corners[(i + 1) % 8]), depth, 90.0)
    return blank.piece(name)


def style_0351(t, L, W, D):
    side = 0.4 * L
    body = Blank(t)
    add_joint(body, tube(body, [side] * 8, D, 45.0)[0])
    cover_side = side + 2 * t * math.tan(math.pi / 8) + 2
    return [
        body.piece("tube"),
        _octagon_cover(t, cover_side, D / 4, "cover 1"),
        _octagon_cover(t, cover_side, D / 4, "cover 2"),
    ]



def _folder(t, L, W, depth, ends="wall", dust=None):
    """A bottom, two side walls with a top half each, and two ends.

    ends: "strip" = an end wall with a narrow strip that folds in below the top panels.
          "buffer" = an end with four sections: outer wall, top, inner wall, and a foot.
    dust: None, "side" (dust flaps on the ends of the side walls), or "end" (dust flaps on the
          sides of the end walls, and a tuck flap on each end of the top halves).
    """
    blank = Blank(t)
    base = blank.panel(0, 0, W, L)
    for key in ("far", "near"):
        wall = blank.flap(base.edge(key), depth, 90.0)
        # With tuck flaps, a top half is shorter, thus its flaps are on the inner side of the ends.
        top = blank.flap(wall.edge("far"), W / 2 - t, 90.0, inset=(2 * t, 2 * t) if dust == "end" else (0.0, 0.0))
        if dust == "side":
            for end in ("left", "right"):
                blank.flap(wall.edge(end), 0.9 * depth, 90.0)
        if dust == "end":
            for end in ("left", "right"):
                blank.flap(top.edge(end), depth - 4 * t, 90.0, taper=(t, t))
    for key in ("right", "left"):
        if ends == "buffer":
            # The rolled end is lower by one thickness, thus it is below the top panels.
            outer = blank.flap(base.edge(key), depth - t, 90.0)
            top = blank.flap(outer.edge("far"), 4 * t, 90.0)
            inner = blank.flap(top.edge("far"), depth - 3 * t - 1.0, 90.0)
            blank.flap(inner.edge("far"), 0.6 * depth, -90.0)
            continue
        # The end wall is one layer out if dust flaps are behind it, and lower by one
        # thickness if a strip folds in below the top panels.
        out = t if dust == "side" else 0.0
        low = t if ends == "strip" else 0.0
        inset = (t, t) if dust == "end" else (0.0, 0.0)
        wall = blank.flap(base.edge(key), depth - out - low, 90.0, offset=out, inset=inset)
        if ends == "strip":
            # The strip is between the dust flaps, if the folder has them.
            clear = 0.9 * depth + 2 * t if dust == "side" else 3 * t
            blank.flap(wall.edge("far"), 0.6 * depth, 90.0, inset=(clear, clear))
        if dust == "end":
            for end in ("left", "right"):
                blank.flap(wall.edge(end), 0.9 * depth, 90.0, taper=(0.0, 0.3 * depth) if end == "left" else (0.3 * depth, 0.0))
    return blank



def style_0401(t, L, W, D):
    return [_folder(t, L, W, D / 2.5, ends="strip").piece("folder")]



def style_0403(t, L, W, D):
    return [_folder(t, L, W, D / 2.5, ends="buffer").piece("folder")]



def style_0406(t, L, W, D):
    """A wrap around blank: the tube goes around the length, the flaps close the ends. The
    sides of the flaps taper."""
    side = {"taper": (D / 8, D / 8)}
    short = min(D / 2, W / 2 - t)
    flaps = [(D / 2, 1, side), (short, 0, side), (D / 2, 1, side), (short, 0, side)]
    return [slotted(t, [W, D, W, D], L, flaps, flaps, joint_taper=JOINT / 3).piece("wrap")]



def _panel_folder(t, L, widths, flaps):
    """A row of panels around the length, with end flaps.

    flaps: (length, layer) or (length, layer, inset) or None, for each panel."""
    blank = Blank(t)
    panels = tube(blank, widths, L)
    for panel, spec in zip(panels, flaps):
        if spec is None:
            continue
        for side in ("left", "right"):
            inset = spec[2] if len(spec) > 2 else (0.0, 0.0)
            blank.flap(panel.edge(side), spec[0], 90.0, offset=spec[1] * t, inset=inset)
    return blank


def style_0410(t, L, W, D):
    """A five panel folder: the fifth panel is as wide as the depth, and it covers a side
    panel. The other four panels have end flaps."""
    depth = D / 2.5
    widths = [depth - 2 * t, W - t, depth, W, depth + t]
    # A flap of a narrow panel is between the panels that hold the outer flaps.
    narrow = (W / 2 - 2 * t, 0, (t, t))
    flaps = [None, (depth - 2 * t, 1), narrow, (depth - 3 * t, 2), narrow]
    return [_panel_folder(t, L, widths, flaps).piece("folder")]



def style_0411(t, L, W, D):
    """A five panel folder with the seam in the middle of a wide panel. All panels have end
    flaps."""
    depth = D / 2.5
    widths = [W / 2 - t, depth, W, depth, W / 2 - t]
    flaps = [(depth - 3 * t, 2), (W / 2 - 2 * t, 0), (depth - 2 * t, 1), (W / 2 - 2 * t, 0), (depth - 3 * t, 2)]
    return [_panel_folder(t, L, widths, flaps).piece("folder")]



def style_0415(t, L, W, D):
    return [_folder(t, L, W, D / 2.5, ends="strip", dust="side").piece("folder")]



def style_0416(t, L, W, D):
    return [_folder(t, L, W, D / 2.5, dust="end").piece("folder")]


def _roll_end_tray(t, L, W, depth, tabs=2, cover=False):
    """A tray with side walls and rolled end walls: outer wall, top strip, inner wall.

    tabs: the number of locking tabs on the free edge of each inner wall. Each tab goes into
          a slot in the bottom.
    cover: a locking cover on the rear wall: a side flap on each end, a front flap, and an
           ear on each end of the front flap.
    """
    blank = Blank(t)
    base = blank.panel(0, 0, L, W)
    places = (0.2, 0.8) if tabs == 2 else (0.5,)
    size = 0.18 * W if tabs == 2 else 0.25 * W
    for key in ("far", "near"):
        outer = blank.flap(base.edge(key), depth, 90.0, offset=t)
        top = blank.flap(outer.edge("far"), 3 * t, 90.0)
        inner = blank.flap(top.edge("far"), depth - 2 * t, 90.0)
        for place in places:
            _tab(blank, inner, place, size, 1.5 * t)
            x0, x1 = (L - 2.5 * t, L - 0.25 * t) if key == "far" else (0.25 * t, 2.5 * t)
            blank.slot(x0, place * W - size / 2 - 2.0, x1, place * W + size / 2 + 2.0)
    back = blank.flap(base.edge("right"), depth, 90.0)
    front = blank.flap(base.edge("left"), depth, 90.0)
    for wall in (back, front):
        # Flaps on the side walls. They are between the outer and the inner end walls.
        blank.flap(wall.edge("left"), 0.8 * depth, 90.0, inset=(0.0, t))
        blank.flap(wall.edge("right"), 0.8 * depth, 90.0, inset=(t, 0.0))
    if cover:
        # The cover is longer than the tray, thus its side flaps are outside the end walls.
        lid = blank.flap(back.edge("far"), W + 2 * t, 90.0, offset=t, widen=(3 * t, 3 * t))
        for end in ("left", "right"):
            blank.flap(lid.edge(end), 0.9 * depth, 90.0, inset=(2 * t, 2 * t), taper=(0.15 * W, 0.15 * W))
        # The ears are one layer out from the side flaps.
        flap = blank.flap(lid.edge("far"), 0.87 * depth, 90.0, widen=(1.5 * t, 1.5 * t))
        for end in ("left", "right"):
            blank.flap(flap.edge(end), 0.8 * depth, 90.0, round_corners=0.3 * depth)
    return blank


def style_0422(t, L, W, D):
    return [_roll_end_tray(t, L, W, D / 2.5, tabs=2).piece("tray")]


def style_0427(t, L, W, D):
    return [_roll_end_tray(t, L, W, D / 2.5, tabs=1, cover=True).piece("tray")]


def style_0457(t, L, W, D):
    """A self locking tray. Each end wall has a hook flap on each end. The hook flaps fold on
    the outer side of the side walls, and each head goes into a slit of the side wall."""
    depth = D / 2.5
    blank = Blank(t)
    base = blank.panel(0, 0, L, W)
    for key in ("right", "left"):
        wall = blank.flap(base.edge(key), depth, 90.0)
        # A slit near each end: a part along the wall, and a diagonal part.
        for a, b, c in ((0.34, 0.27, 0.20), (0.66, 0.73, 0.80)):
            _slit(blank, wall, [(a * L, 0.6 * depth), (b * L, 0.6 * depth), (c * L, 0.4 * depth)])
    for key in ("far", "near"):
        wall = blank.flap(base.edge(key), depth, 90.0, widen=(t, t))
        # The side of a hook flap at the bottom is a diagonal. The head is at the far end.
        for end, taper in (("left", (0.45 * depth, 0.1 * depth)), ("right", (0.1 * depth, 0.45 * depth))):
            hook = blank.flap(wall.edge(end), 0.55 * depth, 90.0, taper=taper)
            _tongue(blank, hook, 0.35 * depth, chamfer=0.12 * depth)
    return [blank.piece("tray")]


def style_0460(t, L, W, D):
    """A display tray: a bottom and two high walls. Each end has three flaps on one score: a
    low wall on the bottom, and a flap on each high wall that folds in behind it."""
    blank = Blank(t)
    base = blank.panel(0, 0, W, L)
    low = 0.4 * D
    for key in ("far", "near"):
        wall = blank.flap(base.edge(key), D, 90.0)
        blank.flap(wall.edge("left"), low, 90.0, taper=(0.0, low / 2))
        blank.flap(wall.edge("right"), low, 90.0, taper=(low / 2, 0.0))
    for key in ("right", "left"):
        blank.flap(base.edge(key), low, 90.0, offset=t)
    return [blank.piece("tray")]



def _open_cover(blank, wall, width):
    """A cover on the far edge of a wall, in the plane of the wall: the box is open. The
    score of the cover has the angle zero."""
    p, q = wall.edge("far")
    u = (q - p) / p.distanceToPoint(q)
    n = V(u.y, -u.x, 0)
    blank._add([p, q, q + n * width, p + n * width])
    blank.score(p, q, 0.0)
    return Panel(p, q, q + n * width, p + n * width)


def style_0470(t, L, W, D):
    """A roll end tray with a tuck top: the row is cover, rear wall, bottom, front wall.

    Each end is on the rear wall: the outer end wall, a flap on the bottom, and a part that
    rolls in from the top. The front wall has a flap on each end, between the two layers of
    the end. The cover is open.
    """
    depth = D / 2
    blank = Blank(t)
    base = blank.panel(0, 0, W, L)
    rear = blank.flap(base.edge("near"), depth, 90.0)
    front = blank.flap(base.edge("far"), depth, 90.0, inset=(t, t))
    cover = _open_cover(blank, rear, W)
    blank.flap(cover.edge("far"), 0.3 * depth, 90.0, inset=(3 * t, 3 * t), round_corners=0.15 * depth)
    for end in ("left", "right"):
        # The flap of the front wall starts above the flap on the bottom.
        blank.flap(front.edge(end), W - 6 * t, 90.0, inset=(2 * t, 0.0) if end == "left" else (0.0, 2 * t))
        low, high = ("left", "right") if end == "left" else ("right", "left")
        # The end wall starts one thickness above the bottom, thus its flap is on the bottom.
        outer = blank.flap(rear.edge(end), W - t, 90.0, inset=(t, 0.0) if end == "left" else (0.0, t))
        foot = blank.flap(outer.edge(low), 0.4 * L, 90.0, inset=(2 * t, 2 * t))
        top = blank.flap(outer.edge(high), 4 * t, 90.0)
        inner = blank.flap(top.edge("far"), depth - 6 * t, 90.0, inset=(2 * t, 2 * t))
        # The locking tab of the inner wall goes into a slot of the flap on the bottom.
        size = 0.25 * W
        _tab(blank, inner, 0.5, size, 2 * t)
        _window(blank, foot, foot.width / 2 - size / 2 - 4.0, foot.width / 2 + size / 2 + 4.0, 0.25 * t, 3.5 * t)
    return [blank.piece("tray")]


def _flat_strip(blank, edge, width):
    """A strip on an edge, in the plane of its panel. Its score has the angle zero."""
    p, q = edge
    u = (q - p) / p.distanceToPoint(q)
    n = V(u.y, -u.x, 0)
    blank._add([p, q, q + n * width, p + n * width])
    blank.score(p, q, 0.0)
    return Panel(p, q, q + n * width, p + n * width)


def _bliss_body(t, L, W, D, flaps):
    """The body of a Bliss box: a bottom, two side walls, and two top halves.

    flaps: "three" = joint flaps on the two ends of the bottom and of the side walls.
           "five" = also on the two ends of the top halves.
    The joint flaps fold in on the end pieces.
    """
    size = 30.0
    blank = Blank(t)
    base = blank.panel(0, 0, W, L)
    for end in ("right", "left"):
        # The flap of the bottom is narrower than the bottom: the flaps of the walls are
        # beside it in the plane of the end.
        blank.flap(base.edge(end), size, 90.0, inset=(size + t, size + t))
    for key in ("far", "near"):
        wall = blank.flap(base.edge(key), D, 90.0)
        top = blank.flap(wall.edge("far"), W / 2 - t, 90.0)
        for end in ("left", "right"):
            blank.flap(wall.edge(end), size, 90.0)
            if flaps == "five":
                # The flap of a top half is at its free side, and a little longer.
                clear = size + 2 * t
                blank.flap(top.edge(end), 1.2 * size, 90.0, inset=(clear, 0.0) if end == "left" else (0.0, clear))
    return blank.piece("body")


def _bliss_end(t, W, D, flap, legs):
    """An end piece of a Bliss box. flap: a flap of the full width on one long edge.
    legs: a strip on each short edge, in the plane of the piece."""
    blank = Blank(t)
    base = blank.panel(0, 0, W, D)
    if flap:
        blank.flap(base.edge("right"), 0.4 * D, 90.0)
    if legs:
        for key in ("far", "near"):
            _flat_strip(blank, base.edge(key), 0.2 * W)
    return blank


def _bliss(t, L, W, D, body_flaps, end_flap, legs):
    return [
        _bliss_body(t, L, W, D, body_flaps),
        _bliss_end(t, W, D, end_flap, legs).piece("end 1"),
        _bliss_end(t, W, D, end_flap, legs).piece("end 2"),
    ]


def style_0601a(t, L, W, D):
    return _bliss(t, L, W, D, "three", True, False)


def style_0601b(t, L, W, D):
    return _bliss(t, L, W, D, "three", True, True)


def style_0606a(t, L, W, D):
    return _bliss(t, L, W, D, "five", False, False)


def style_0606b(t, L, W, D):
    return _bliss(t, L, W, D, "five", False, True)


def style_0711(t, L, W, D):
    """An auto bottom with RSC top flaps. Each bottom flap of a long panel has a diagonal
    score, where the flap of the adjacent end panel is glued. The diagonal scores are flat
    when the box is set up, thus they have the angle zero."""
    blank = Blank(t)
    panels = tube(blank, [L, W, L, W], D)
    add_joint(blank, panels[0], taper=JOINT / 3)
    for panel, layer in zip(panels, (1, 0, 1, 0)):
        blank.flap(panel.edge("left"), W / 2, 90.0, offset=layer * t)
    depth = 0.75 * W
    allowance = blank.allowance(90.0)
    for panel, layer in ((panels[0], 0), (panels[2], 1)):
        flap = blank.flap(panel.edge("right"), depth, 90.0, offset=layer * t)
        _notch(blank, flap, 0.15 * L, 0.15 * depth)
        # On the edge "right", the first end is at the side of the next panel.
        u, n = _frame(flap)
        blank.score(flap.a + u * 5.0 + n * 2.0, flap.a + u * (0.7 * depth) + n * (0.7 * depth), 0.0)
    for panel, layer in ((panels[1], 2), (panels[3], 3)):
        blank.flap(panel.edge("right"), W / 2, 90.0, offset=layer * t, taper=(W / 4, W / 4))
    return [blank.piece("box")]


def style_0760(t, L, W, D):
    """A self-erecting six corner tray: the row is side wall, bottom, rear wall, cover, front
    flap. Each of the five panels has a flap on each end.

    The end flaps of the bottom are the end walls. The end flaps of the walls fold in on them,
    where a glue line holds them. The diagonal scores are for the collapse of the glued tray:
    they are flat when the tray is erected, thus their angle is zero. The cover is open.
    """
    depth = D / 2
    blank = Blank(t)
    base = blank.panel(0, 0, W, L)
    side = blank.flap(base.edge("near"), depth, 90.0, inset=(t, t))
    rear = blank.flap(base.edge("far"), depth, 90.0, inset=(t, t))
    cover = _open_cover(blank, rear, W)
    front = blank.flap(cover.edge("far"), depth, 90.0, inset=(t, t))
    corner = min(depth - 2 * t, W / 2 - 2 * t)  # Two flaps on one end wall must not meet.
    for end in ("left", "right"):
        wall = blank.flap(base.edge(end), depth, 90.0)
        blank.flap(side.edge(end), corner, 90.0)
        # A slot is between the flap of the rear wall and the flap of the cover.
        blank.flap(rear.edge(end), corner, 90.0, inset=(0.0, 2 * t) if end == "left" else (2 * t, 0.0))
        lid = blank.flap(cover.edge(end), depth, 90.0)
        blank.flap(front.edge(end), corner, 90.0)
        # The collapse scores: two on the end wall of the bottom, one on the flap of the cover.
        # On the flap of the cover, the score starts at the side of the front flap.
        for panel, corners in ((wall, ("a", "b")), (lid, ("b",) if end == "left" else ("a",))):
            for name in corners:
                start = getattr(panel, name)
                along = (panel.b - panel.a) if name == "a" else (panel.a - panel.b)
                along = along / along.Length
                up = (panel.d - panel.a) / panel.a.distanceToPoint(panel.d)
                blank.score(start, start + along * depth + up * depth, 0.0)
    return [blank.piece("tray")]


def _tubes(t, D, widths, angles=90.0):
    blank = Blank(t)
    tube(blank, widths, D, angles)
    return [blank.piece("tube")]


def style_0900(t, L, W, D):
    blank = Blank(t)
    blank.panel(0, 0, L, W)
    return [blank.piece("pad")]



def style_0904(t, L, W, D):
    return _tubes(t, D, [0.85 * W, L, W, L - t])



def style_0905(t, L, W, D):
    return _tubes(t, D, [0.94 * W, L, W, 0.92 * L])



def style_0906(t, L, W, D):
    return _tubes(t, D, [L, W, L, W - 2 * t])


def style_0907(t, L, W, D):
    return _tubes(t, D, [L / 3, W, L, W, L / 3])



def style_0908(t, L, W, D):
    return _tubes(t, D, [L / 4, W, L, W, L / 4])



def style_0909(t, L, W, D):
    return _tubes(t, D, [0.36 * W, L, W, L, 0.36 * W])



def style_0910(t, L, W, D):
    return _tubes(t, D, [W / 2 - t, L, W, L, W / 2 - t])



def style_0914(t, L, W, D):
    """A square tube of five panels: the fifth panel is on the first, as a second layer."""
    return _tubes(t, D, [W, W, W, W - t, W - 3 * t])



def style_0920(t, L, W, D):
    """A scored partition for two cells: a wide rear panel, and at each end a side, a front,
    and a return panel."""
    a = W / 2
    return _tubes(t, D, [a - 3 * t, a, a, 2 * a + 4 * t, a, a, a - 3 * t], 90.0)


def style_0921(t, L, W, D):
    a = W / 2
    return _tubes(t, D, [a, a, a, a, a, a, a - 2 * t], [90, 90, -90, -90, 90, 90])



def _card(t, length, height, slots, name, depth=None):
    """A partition card. Each slot goes from one long edge to the depth (half of the height)."""
    depth = height / 2 if depth is None else depth
    blank = Blank(t)
    blank.panel(0, 0, length, height)
    for x in slots:
        blank.slot(x - t / 2 - 0.5, height - depth, x + t / 2 + 0.5, height + 1)
    return blank.piece(name)


def style_0930(t, L, W, D):
    return [_card(t, L, D, [L / 2], "card 1"), _card(t, L, D, [L / 2], "card 2")]


def style_0931(t, L, W, D):
    return [_card(t, L, D, [L / 3, 2 * L / 3], f"card {i + 1}") for i in range(4)]


def style_0933(t, L, W, D):
    long_cards = [_card(t, L, D / 2, [L / 6, L / 2, 5 * L / 6], f"long card {i + 1}") for i in range(2)]
    short_cards = [_card(t, W, D / 2, [W / 4, 3 * W / 4], f"short card {i + 1}") for i in range(3)]
    return long_cards + short_cards



def style_0935(t, L, W, D):
    cards = [_card(t, L, D / 2, [L / 6, L / 2, 5 * L / 6], f"long card {i + 1}") for i in range(2)]
    cards += [_card(t, W, D / 2, [W / 4, 3 * W / 4], f"short card {i + 1}") for i in range(2)]
    # The slots of the high card are as deep as those of the short cards.
    cards.append(_card(t, W, D, [W / 4, 3 * W / 4], "high card", depth=D / 4))
    return cards


def style_0965(t, L, W, D):
    return _tubes(t, D, [W / 2] * 5, [180, -180, 180, -180])



def style_0966(t, L, W, D):
    """Five panels, as 0965. The handbook drawing does not show how its fold is different."""
    return _tubes(t, D, [W / 2] * 5, [180, -180, 180, -180])


def style_0967(t, L, W, D):
    """A pad of eight equal panels with a low part and a high part.

    The first panel folds on the second: the low part, with two layers. The second and the
    third panel are the base, with a flat score between them. The other panels fold on the
    third in a zigzag: the high part, with six layers. This fold is an interpretation of the
    handbook sketch.
    """
    return _tubes(t, D, [W / 2 - 2 * t] + [W / 2] * 7, [180, 0, 180, -180, 180, -180, 180])


def style_die_cut_pad(t, L, W, D):
    blank = Blank(t)
    blank.panel(0, 0, L, W)
    blank.circle(L / 2, W / 2, W / 3)
    return [blank.piece("pad")]


#: All styles of the Box Styles chapter. A value of None is a style with no blank yet.
STYLES = {
    "0200 Half Slotted Container": style_0200,
    "0201 Regular Slotted Container": style_0201,
    "0202 Overlap Slotted Container": style_0202,
    "0203 Full Overlap Slotted Container": style_0203,
    "0204 Center Special Slotted Container": style_0204,
    "0205 Center Special Overlap Slotted Container": style_0205,
    "0206 Center Special Full Overlap Slotted Container": style_0206,
    "0215 Snap Bottom Container with Tuck Top": style_0215,
    "0216 Snap Bottom Container with RSC Top": style_0216,
    "0225 Full Bottom File Box": style_0225,
    "0226 Bellows Style Top and Bottom Container": style_0226,
    "0228 Integral Divider Container": style_0228,
    "0301 Trays, Design Style": style_0301,
    "0301 Full Telescope Design Style Container": style_0301_ftd,
    "0306 Design Style Container with Cover": style_0306,
    "0310 Double Cover Container": style_0310,
    "0320 Full Telescope Half Slotted Container": style_0320,
    "0325 Interlocking Double Cover Container": style_0325,
    "0351 Octagonal Double Cover Container": style_0351,
    "0401 One Piece Folder": style_0401,
    "0403 One Piece Folder with End Buffers": style_0403,
    "0406 Wrap Around Blank": style_0406,
    "0410 Five Panel Folder": style_0410,
    "0411 Center Seam Five Panel Folder": style_0411,
    "0415 One Piece Folder with Dust Flaps": style_0415,
    "0416 One Piece Folder, Die Cut": style_0416,
    "0422 Roll End Tray": style_0422,
    "0427 Roll End Tray with Locking Cover": style_0427,
    "0457 Self Locking Tray": style_0457,
    "0460 Display Tray": style_0460,
    "0470 Roll End Tray with Tuck Top": style_0470,
    "0601A Bliss Style Container with End Flaps": style_0601a,
    "0601B Bliss Style Container with End Flaps and Legs": style_0601b,
    "0606A Bliss Style Container": style_0606a,
    "0606B Bliss Style Container with End Panel Legs": style_0606b,
    "0711 Pre-glued Auto Bottom with RSC Top Flaps": style_0711,
    "0760 Self-Erecting Six Corner Tray": style_0760,
    "0900 Pads": style_0900,
    "0904 Tube": style_0904,
    "0905 Tube": style_0905,
    "0906 Tube": style_0906,
    "0907 Tube": style_0907,
    "0908 Tube": style_0908,
    "0909 Tube": style_0909,
    "0910 Tube": style_0910,
    "0914 Tube": style_0914,
    "0920 Partition": style_0920,
    "0921 Partition": style_0921,
    "0930 Partition": style_0930,
    "0931 Partition": style_0931,
    "0933 Partition": style_0933,
    "0935 Partition": style_0935,
    "0965 Inner Packing Piece": style_0965,
    "0966 Inner Packing Piece": style_0966,
    "0967 Inner Packing Piece": style_0967,
    "Die-Cut Support Pad": style_die_cut_pad,
}


def fold(doc, rule, piece, name="Folded"):
    """Add the document objects for one piece. Returns the folded feature."""
    profile = doc.addObject("Part::Feature", name + "Blank")
    profile.Shape = piece.face
    if not piece.lines:
        feature = doc.addObject("SheetDesign::BasePanel", name)
        feature.Profile = profile
        feature.Rule = rule
        return feature
    scores = doc.addObject("Part::Feature", name + "Scores")
    scores.Shape = Part.makeCompound(piece.lines)
    feature = doc.addObject("SheetDesign::Refold", name)
    feature.Profile = profile
    feature.BendLines = scores
    feature.Angles = piece.angles
    feature.Rule = rule
    return feature
