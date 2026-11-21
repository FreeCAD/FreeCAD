# SPDX-License-Identifier: LGPL-2.1-or-later

"""Classic sections and shapes of handrail members.

A top rail takes one of a few traditional sections, drawn square to the
rail within its width and thickness. Posts and pickets can be stop-chamfered
or turned between the square blocks their joints are cut in, and a post's
top end can be worked to a classic finial.
"""

import math

import FreeCAD
import Part

RAIL_PROFILES = ("Square", "Circular", "Rounded", "Mopstick", "Oval", "Moulded")
MEMBER_SHAPES = ("Square", "Circular", "Chamfered", "Turned")
POST_CAPS = ("Flat", "Chamfered", "Pyramid", "Ball", "Acorn")

# A member is worked between its blocks only where that leaves a shaft at
# least this many times its size.
SHAFT_MINIMUM_RATIO = 3.0
# A turned member, from its lower block up: ``(height, radius)`` as shares of
# the turned length and of the largest radius its blocks allow. Three
# points stand for an arc through them, two for a straight line.
TURNED_PROFILE = (
    ((0.00, 0.95), (0.03, 0.95)),
    ((0.03, 0.95), (0.045, 0.72), (0.07, 0.62)),
    ((0.07, 0.62), (0.10, 0.88), (0.13, 0.62)),
    ((0.13, 0.62), (0.22, 0.97), (0.34, 0.86)),
    ((0.34, 0.86), (0.78, 0.45)),
    ((0.78, 0.45), (0.81, 0.40), (0.84, 0.45)),
    ((0.84, 0.45), (0.875, 0.70), (0.90, 0.60)),
    ((0.90, 0.60), (0.95, 0.74), (0.97, 0.95)),
    ((0.97, 0.95), (1.00, 0.95)),
)
# An acorn finial, from the post's top up: ``(height, radius)`` as shares of
# the post's size.
ACORN_PROFILE = (
    ((0.00, 0.20), (0.08, 0.20)),
    ((0.08, 0.20), (0.20, 0.36), (0.30, 0.42)),
    ((0.30, 0.42), (0.36, 0.42)),
    ((0.36, 0.42), (0.36, 0.38)),
    ((0.36, 0.38), (0.80, 0.30), (1.20, 0.00)),
)


def _outline_edges(segments, point):
    """Return the edges of ``segments``, each two points for a line or three
    for an arc through them, mapped to 3D by ``point``."""

    edges = []
    for segment in segments:
        ends = [point(*position) for position in segment]
        if len(ends) == 2:
            edges.append(Part.makeLine(ends[0], ends[1]))
        else:
            edges.append(Part.Arc(*ends).toShape())
    return edges


def rail_profile_face(profile, width, thickness, origin, across, down):
    """Return the section of a top rail as a face.

    ``origin`` is the middle of the rail's top, ``across`` and ``down`` the
    unit directions over its width and through its thickness, square to
    the rail. Every section fits ``width`` by ``thickness``:

    - ``Rounded``: a rectangle with its two upper edges rounded over;
    - ``Mopstick``: round, with a flat underside for the pickets;
    - ``Oval``: an ellipse;
    - ``Moulded``: a rounded top over a cove along each side for the
      fingers, the classic grip.

    Returns ``None`` for the sections built otherwise, ``Square`` and
    ``Circular``.
    """

    half = width / 2.0

    def point(y, v):
        return origin + across * y + down * v

    profile = str(profile)
    if profile == "Rounded":
        radius = min(width / 4.0, thickness / 2.0)
        inset = radius * (1.0 - math.sqrt(0.5))
        segments = (
            ((-half, thickness), (half, thickness)),
            ((half, thickness), (half, radius)),
            ((half, radius), (half - inset, inset), (half - radius, 0.0)),
            ((half - radius, 0.0), (-half + radius, 0.0)),
            ((-half + radius, 0.0), (-half + inset, inset), (-half, radius)),
            ((-half, radius), (-half, thickness)),
        )
    elif profile == "Mopstick":
        # Round as wide as the rail; its thickness sets where the flat is.
        height = min(max(thickness, 0.6 * width), 0.9 * width)
        chord = math.sqrt(max(half * half - (height - half) ** 2, 0.0))
        segments = (
            ((chord, height), (0.0, 0.0), (-chord, height)),
            ((-chord, height), (chord, height)),
        )
    elif profile == "Oval":
        centre = point(0.0, thickness / 2.0)
        side, under = point(half, thickness / 2.0), point(0.0, thickness)
        if abs(width - thickness) < 1e-6:
            edge = Part.makeCircle(half, centre, across.cross(down))
        elif width > thickness:
            edge = Part.Ellipse(side, under, centre).toShape()
        else:
            edge = Part.Ellipse(under, side, centre).toShape()
        return Part.Face(Part.Wire([edge]))
    elif profile == "Moulded":
        crown = 0.25 * thickness
        cove = min(width / 5.0, 0.3 * thickness)
        waist = 0.5 * thickness
        inset = cove * math.sqrt(0.5)
        segments = (
            ((-half, crown), (0.0, 0.0), (half, crown)),
            ((half, crown), (half, waist)),
            ((half, waist), (half - inset, waist + cove - inset), (half - cove, waist + cove)),
            ((half - cove, waist + cove), (half - cove, thickness)),
            ((half - cove, thickness), (-half + cove, thickness)),
            ((-half + cove, thickness), (-half + cove, waist + cove)),
            ((-half + cove, waist + cove), (-half + inset, waist + cove - inset), (-half, waist)),
            ((-half, waist), (-half, crown)),
        )
    else:
        return None
    return Part.Face(Part.Wire(_outline_edges(segments, point)))


def _section_wire(point, tangent, normal, width, thickness, elevation, circular=False, inset=0.0):
    """Return a post's plan section at ``elevation``, ``inset`` all round."""

    centre = FreeCAD.Vector(point[0], point[1], elevation)
    if circular:
        return Part.Wire(
            [Part.makeCircle(max(width / 2.0 - inset, 0.01), centre, FreeCAD.Vector(0.0, 0.0, 1.0))]
        )
    across = FreeCAD.Vector(normal[0], normal[1], 0.0) * max(width / 2.0 - inset, 0.01)
    along = FreeCAD.Vector(tangent[0], tangent[1], 0.0) * max(thickness / 2.0 - inset, 0.01)
    corners = [
        centre - across - along,
        centre + across - along,
        centre + across + along,
        centre - across + along,
    ]
    return Part.makePolygon(corners + corners[:1])


def _revolved(segments, point, axis_point, scale_height, scale_radius, base, normal):
    """Return the solid turned from ``segments`` about the plumb axis."""

    radial = FreeCAD.Vector(normal[0], normal[1], 0.0)
    foot = FreeCAD.Vector(point[0], point[1], base)

    def position(height, radius):
        return (
            foot
            + radial * (radius * scale_radius)
            + FreeCAD.Vector(0.0, 0.0, height * scale_height)
        )

    edges = _outline_edges(segments, position)
    first = edges[0].Vertexes[0].Point
    last = edges[-1].Vertexes[-1].Point
    low = FreeCAD.Vector(foot.x, foot.y, first.z)
    high = FreeCAD.Vector(foot.x, foot.y, last.z)
    closing = [Part.makeLine(high, low)]
    if (first - low).Length > 1e-7:
        closing.append(Part.makeLine(low, first))
    if (last - high).Length > 1e-7:
        closing.insert(0, Part.makeLine(last, high))
    face = Part.Face(Part.Wire(edges + closing))
    return face.revolve(axis_point, FreeCAD.Vector(0.0, 0.0, 1.0), 360.0)


def _stop_chamfers(point, tangent, normal, width, thickness, low, high):
    """Return the four wedges a stop chamfer removes from a square member
    between ``low`` and ``high``: each runs out to nothing on the arris."""

    size = min(width, thickness) / 5.0
    across = FreeCAD.Vector(normal[0], normal[1], 0.0)
    along = FreeCAD.Vector(tangent[0], tangent[1], 0.0)
    wedges = []
    for side in (-1.0, 1.0):
        for end in (-1.0, 1.0):
            corner = across * (side * width / 2.0) + along * (end * thickness / 2.0)
            first = corner - across * (side * size)
            second = corner - along * (end * size)
            # Reaching past the faces and the arris, so that no face of a
            # wedge lies on one of the member's.
            outward = corner - (first + second) / 2.0
            reach = (first - second) * 0.25
            triangle = [first + reach, second - reach, corner + outward * 0.5]

            def at(position, elevation):
                return FreeCAD.Vector(point[0] + position.x, point[1] + position.y, elevation)

            tip = corner + outward * 0.02
            sections = [
                Part.Vertex(at(tip, low)),
                Part.makePolygon([at(p, low + size) for p in triangle + triangle[:1]]),
                Part.makePolygon([at(p, high - size) for p in triangle + triangle[:1]]),
                Part.Vertex(at(tip, high)),
            ]
            wedges.append(Part.makeLoft(sections, True, True))
    return wedges


def make_member_shape(
    point,
    tangent,
    bottom,
    top,
    shape,
    width,
    thickness,
    blocks=None,
    cap="Flat",
):
    """Create one plumb post or picket.

    ``shape`` is its section, ``Square`` or ``Circular``, or how a square
    member is worked between its blocks: ``Chamfered``, with stop chamfers
    along its four arrises, or ``Turned`` to a classic vase. ``blocks`` are
    the heights its lower block ends and its upper block begins at: the
    joints are cut in those and they stay square. A member too short for a
    shaft between them stays square throughout. ``cap`` works a post's top
    end: ``Chamfered`` and ``Pyramid`` within its length, a ``Ball`` or an
    ``Acorn`` standing on it.
    """

    bottom = float(bottom)
    top = max(float(top), bottom + 0.01)
    width = max(float(width), 0.01)
    thickness = max(float(thickness), 0.01)
    shape = str(shape)
    cap = str(cap)
    circular = shape == "Circular"
    size = width if circular else min(width, thickness)
    tangent_length = max(math.hypot(*tangent), 1e-9)
    tangent = (tangent[0] / tangent_length, tangent[1] / tangent_length)
    normal = (-tangent[1], tangent[0])
    axis_point = FreeCAD.Vector(point[0], point[1], 0.0)
    plumb = FreeCAD.Vector(0.0, 0.0, 1.0)

    def prism(low, high):
        return Part.Face(
            _section_wire(point, tangent, normal, width, thickness, low, circular)
        ).extrude(plumb * (high - low))

    try:
        # The top end worked within the member's own length.
        finial = None
        body_top = top
        if cap == "Chamfered" and top - bottom > size:
            body_top = top - size / 6.0
            finial = Part.makeLoft(
                [
                    _section_wire(point, tangent, normal, width, thickness, body_top, circular),
                    _section_wire(
                        point, tangent, normal, width, thickness, top, circular, size / 6.0
                    ),
                ],
                True,
                True,
            )
        elif cap == "Pyramid" and top - bottom > size:
            body_top = top - size / 3.0
            finial = Part.makeLoft(
                [
                    _section_wire(point, tangent, normal, width, thickness, body_top, circular),
                    Part.Vertex(FreeCAD.Vector(point[0], point[1], top)),
                ],
                True,
                True,
            )
        elif cap == "Ball":
            diameter = 0.9 * size
            neck = Part.makeCylinder(
                0.22 * diameter, 0.2 * diameter, FreeCAD.Vector(point[0], point[1], top)
            )
            ball = Part.makeSphere(
                diameter / 2.0, FreeCAD.Vector(point[0], point[1], top + 0.6 * diameter)
            )
            finial = neck.fuse(ball)
        elif cap == "Acorn":
            finial = _revolved(ACORN_PROFILE, point, axis_point, size, size, top, normal)

        low, high = blocks if blocks is not None else (bottom, body_top)
        low = min(max(float(low), bottom), body_top)
        high = min(max(float(high), low), body_top)
        worked = shape in ("Chamfered", "Turned") and (high - low >= SHAFT_MINIMUM_RATIO * size)
        if worked and shape == "Chamfered":
            result = prism(bottom, body_top).cut(
                _stop_chamfers(point, tangent, normal, width, thickness, low, high)
            )
        elif worked:
            pieces = [
                _revolved(TURNED_PROFILE, point, axis_point, high - low, size / 2.0, low, normal)
            ]
            if low - bottom > 1e-6:
                pieces.append(prism(bottom, low))
            if body_top - high > 1e-6:
                pieces.append(prism(high, body_top))
            result = pieces[0].multiFuse(pieces[1:]) if len(pieces) > 1 else pieces[0]
        else:
            result = prism(bottom, body_top)
        if finial is not None:
            result = result.fuse(finial)
        if shape not in ("Square", "Circular") or finial is not None:
            result = result.removeSplitter()
        if result.isNull() or not result.isValid() or len(result.Solids) != 1:
            return prism(bottom, top)
        return result
    except (Part.OCCError, ValueError):
        try:
            return prism(bottom, top)
        except Part.OCCError:
            return Part.Shape()
