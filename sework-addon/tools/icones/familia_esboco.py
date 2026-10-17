# SPDX-License-Identifier: LGPL-2.1-or-later
# Ícones do Esboço (geometria, restrições e gestão) no estilo do guia ilustrado.
import math
from nucleo import (Icon, SOLID, COPY, BASE, CUT, YEL, OUT, NAVY, AMBER, AMBER_FILL, STEEL, RED,
                    SW, SW_GEO, PAPER_LINE)

ICONS = {}
LIGHT = "#8FAFD0"   # geometria de apoio nas restrições


def icon(*names):
    def deco(fn):
        for n in names:
            ICONS[n] = fn
        return fn
    return deco


def flat():
    g = Icon()
    g.fixed = (0, 0, 64, 64)
    return g


def pts(g, *ps, r=4.2):
    for q in ps:
        g.dot2(q, r)


def poly_closed(g, ps, color=NAVY, sw=SW_GEO, fill="none", dash=None):
    g.poly2(ps, fill=fill, stroke=color, sw=sw, dash=dash)


def regular(c, r, n, rot=-90):
    return [(c[0] + r * math.cos(math.radians(rot + 360 * i / n)), c[1] + r * math.sin(math.radians(rot + 360 * i / n)))
            for i in range(n)]


# ------------------------------------------------------------ geometria
@icon("Sketcher_CreateLine")
def line():
    g = flat()
    g.line2((12, 50), (52, 14))
    pts(g, (12, 50), (52, 14))
    return g


@icon("Sketcher_CreatePolyline")
def polyline():
    g = flat()
    P = [(8, 48), (22, 18), (38, 40), (56, 14)]
    g.path2(P)
    pts(g, *P)
    return g


@icon("Sketcher_CreatePoint")
def point():
    g = flat()
    g.line2((18, 32), (46, 32), color=LIGHT, sw=SW)
    g.line2((32, 18), (32, 46), color=LIGHT, sw=SW)
    pts(g, (32, 32), r=7)
    return g


@icon("Sketcher_CreateArc")
def arc_c():
    g = flat()
    g.path2(g.arc_pts2((30, 38), 22, 0, 150))
    pts(g, (30, 38), (52, 38), (30 + 22 * math.cos(math.radians(150)), 38 - 22 * math.sin(math.radians(150))))
    return g


@icon("Sketcher_Create3PointArc")
def arc_3p():
    g = flat()
    g.path2(g.arc_pts2((32, 40), 22, 10, 170))
    for a in (10, 90, 170):
        pts(g, (32 + 22 * math.cos(math.radians(a)), 40 - 22 * math.sin(math.radians(a))))
    return g


@icon("Sketcher_CreateCircle")
def circle_c():
    g = flat()
    g.circle2((32, 32), 21, "none", NAVY, SW_GEO)
    pts(g, (32, 32), (53, 32))
    g.line2((32, 32), (53, 32), color=AMBER, sw=SW, dash="3,3")
    return g


@icon("Sketcher_Create3PointCircle")
def circle_3p():
    g = flat()
    g.circle2((32, 32), 21, "none", NAVY, SW_GEO)
    for a in (30, 150, 270):
        pts(g, (32 + 21 * math.cos(math.radians(a)), 32 - 21 * math.sin(math.radians(a))))
    return g


def _ellipse(g, c, rx, ry, a0=0, a1=360, color=NAVY):
    g.path2([(c[0] + rx * math.cos(math.radians(a0 + (a1 - a0) * i / 48)),
              c[1] - ry * math.sin(math.radians(a0 + (a1 - a0) * i / 48))) for i in range(49)], color=color)


@icon("Sketcher_CreateEllipseByCenter", "Sketcher_Element_Ellipse_All", "Sketcher_CreateEllipse")
def ellipse_c():
    g = flat()
    _ellipse(g, (32, 32), 25, 15)
    pts(g, (32, 32), (57, 32), (32, 17))
    return g


@icon("Sketcher_CreateEllipse_3points")
def ellipse_3p():
    g = flat()
    _ellipse(g, (32, 32), 25, 15)
    pts(g, (7, 32), (57, 32), (32, 17))
    return g


@icon("Sketcher_CreateElliptical_Arc")
def ellipse_arc():
    g = flat()
    _ellipse(g, (32, 36), 25, 15, 0, 200)
    pts(g, (32, 36), (57, 36))
    return g


@icon("Sketcher_CreateHyperbolic_Arc")
def hyperbola():
    g = flat()
    g.path2([(32 + 8 * math.cosh(t / 10), 32 + 12 * math.sinh(t / 10)) for t in range(-16, 17)])
    g.path2([(32 - 8 * math.cosh(t / 10), 32 + 12 * math.sinh(t / 10)) for t in range(-16, 17)], color=LIGHT, dash="3,3", sw=SW)
    pts(g, (32, 32))
    return g


@icon("Sketcher_CreateParabolic_Arc")
def parabola():
    g = flat()
    g.path2([(32 + t, 52 - 0.07 * t * t) for t in range(-24, 25, 2)])
    pts(g, (32, 52), (32, 40))
    return g


@icon("Sketcher_CreateRectangle")
def rect():
    g = flat()
    P = [(10, 16), (54, 16), (54, 48), (10, 48)]
    poly_closed(g, P)
    pts(g, (10, 16), (54, 48))
    return g


@icon("Sketcher_CreateRectangle_Center")
def rect_c():
    g = flat()
    poly_closed(g, [(10, 16), (54, 16), (54, 48), (10, 48)])
    g.line2((32, 32), (54, 48), color=AMBER, sw=SW, dash="3,3")
    pts(g, (32, 32), (54, 48))
    return g


@icon("Sketcher_CreateOblong")
def oblong():
    g = flat()
    P = g.arc_pts2((20, 32), 14, 90, 270, 12) + g.arc_pts2((44, 32), 14, -90, 90, 12)
    poly_closed(g, P)
    pts(g, (8, 18), (56, 46), r=3.4)
    return g


def _ngon(n):
    def f():
        g = flat()
        poly_closed(g, regular((32, 33), 23, n))
        pts(g, (32, 33), regular((32, 33), 23, n)[0])
        return g
    return f


for n, name in ((3, "Sketcher_CreateTriangle"), (4, "Sketcher_CreateSquare"), (5, "Sketcher_CreatePentagon"),
                (6, "Sketcher_CreateHexagon"), (7, "Sketcher_CreateHeptagon"), (8, "Sketcher_CreateOctagon"),
                (6, "Sketcher_CreateRegularPolygon")):
    ICONS[name] = _ngon(n)


@icon("Sketcher_CreateSlot")
def slot():
    g = flat()
    P = g.arc_pts2((20, 32), 11, 90, 270, 12) + g.arc_pts2((44, 32), 11, -90, 90, 12)
    poly_closed(g, P)
    g.line2((20, 32), (44, 32), color=AMBER, sw=SW, dash="3,3")
    pts(g, (20, 32), (44, 32))
    return g


@icon("Sketcher_CreateArcSlot")
def arcslot():
    g = flat()
    outer = g.arc_pts2((32, 52), 34, 30, 150, 20)
    inner = g.arc_pts2((32, 52), 20, 150, 30, 20)
    poly_closed(g, outer + inner)
    pts(g, (32, 52))
    return g


def _bspline(g, ctrl, closed=False):
    g.path2(ctrl + ([ctrl[0]] if closed else []), color=AMBER, sw=SW, dash="3,3")
    # curva (De Casteljau simples por trechos)
    def bez(p0, p1, p2, p3, t):
        u = 1 - t
        return (u ** 3 * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t ** 3 * p3[0],
                u ** 3 * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t ** 3 * p3[1])
    c = ctrl
    curve = [bez(c[0], c[1], c[2], c[3], i / 30) for i in range(31)]
    g.path2(curve)
    pts(g, *ctrl, r=3.4)


@icon("Sketcher_CreateBSpline")
def bspline():
    g = flat()
    _bspline(g, [(8, 48), (18, 10), (44, 54), (56, 16)])
    return g


@icon("Sketcher_Create_Periodic_BSpline")
def bspline_p():
    g = flat()
    poly_closed(g, regular((32, 32), 24, 5), color=AMBER, sw=SW, dash="3,3")
    g.circle2((32, 32), 15, "none", NAVY, SW_GEO)
    pts(g, *regular((32, 32), 24, 5), r=3.4)
    return g


@icon("Sketcher_CreateBSplineByInterpolation")
def bspline_i():
    g = flat()
    P = [(8, 44), (24, 18), (40, 44), (56, 18)]
    g.path2([(8 + i, 31 + 13 * math.cos(math.pi * i / 16)) for i in range(0, 49, 2)])
    pts(g, *[(8 + i, 31 + 13 * math.cos(math.pi * i / 16)) for i in (0, 16, 32, 48)])
    return g


@icon("Sketcher_CreatePeriodicBSplineByInterpolation")
def bspline_pi():
    g = flat()
    g.circle2((32, 32), 20, "none", NAVY, SW_GEO)
    pts(g, *[(32 + 20 * math.cos(math.radians(a)), 32 + 20 * math.sin(math.radians(a))) for a in (0, 90, 180, 270)])
    return g


# ------------------------------------------------------------ edição
@icon("Sketcher_CreateFillet", "Sketcher_CompCreateFillets")
def sfillet():
    g = flat()
    g.path2([(10, 54), (10, 30)] + g.arc_pts2((30, 30), 20, 180, 90, 10) + [(54, 10)], color=NAVY)
    g.path2(g.arc_pts2((30, 30), 20, 180, 90, 10), color=AMBER, sw=SW_GEO)
    g.path2([(10, 30), (10, 10), (30, 10)], color=LIGHT, sw=SW, dash="3,3")
    return g


@icon("Sketcher_CreateChamfer")
def schamfer():
    g = flat()
    g.path2([(10, 54), (10, 28), (28, 10), (54, 10)])
    g.line2((10, 28), (28, 10), color=AMBER)
    g.path2([(10, 28), (10, 10), (28, 10)], color=LIGHT, sw=SW, dash="3,3")
    return g


@icon("Sketcher_Trimming")
def trim():
    g = flat()
    g.line2((32, 8), (32, 56), color=NAVY)
    g.line2((8, 32), (32, 32), color=NAVY)
    g.line2((32, 32), (56, 32), color=RED, sw=SW_GEO, dash="4,4")
    return g


@icon("Sketcher_Extend")
def extend():
    g = flat()
    g.line2((52, 8), (52, 56), color=NAVY)
    g.line2((8, 32), (30, 32), color=NAVY)
    g.line2((30, 32), (52, 32), color=AMBER, dash="4,4")
    pts(g, (52, 32))
    return g


@icon("Sketcher_Split")
def split():
    g = flat()
    g.line2((8, 44), (28, 30), color=NAVY)
    g.line2((36, 26), (56, 14), color=NAVY)
    pts(g, (28, 30), (36, 26))
    return g


@icon("Sketcher_ToggleConstruction")
def construction():
    g = flat()
    g.line2((10, 54), (54, 10), color=AMBER, dash="6,5")
    pts(g, (10, 54), (54, 10))
    return g


@icon("Sketcher_Projection", "Sketcher_External")
def projection():
    g = Icon()
    g.box(0, 8, 0, 6, 3, 7, pal=COPY)
    g.rect3(-2, 10, -2, 8, 0, "#EDF2F7", stroke=PAPER_LINE, sw=SW)
    g.rect3(0, 8, 0, 6, 0, "none", stroke=AMBER, sw=SW_GEO, dash="5,3")
    g.arrow3((4, 3, 3), (4, 3, 0.6), sw=SW)
    return g


@icon("Sketcher_Intersection")
def intersection():
    g = Icon()
    g.rect3(-3, 10, -3, 9, 3, "#EDF2F7", stroke=PAPER_LINE, sw=SW, opacity=0.8)
    g.cyl(3.5, 3, 3.5, 0, 6, pal=COPY)
    g.ellipse_iso(3.5, 3, 3.5, 3, "none", AMBER, SW_GEO)
    return g


@icon("Sketcher_CarbonCopy")
def carbon():
    g = flat()
    poly_closed(g, [(6, 10), (40, 10), (40, 36), (6, 36)], color=LIGHT, sw=SW_GEO, dash="4,3")
    poly_closed(g, [(22, 26), (56, 26), (56, 52), (22, 52)])
    return g


@icon("Sketcher_Offset")
def offset():
    g = flat()
    poly_closed(g, [(18, 20), (46, 20), (46, 44), (18, 44)])
    poly_closed(g, [(8, 10), (56, 10), (56, 54), (8, 54)], color=AMBER, sw=SW_GEO, dash="5,3")
    return g


@icon("Sketcher_Translate")
def translate():
    g = flat()
    poly_closed(g, [(8, 34), (28, 34), (28, 54), (8, 54)], color=LIGHT, sw=SW_GEO, dash="4,3")
    poly_closed(g, [(36, 10), (56, 10), (56, 30), (36, 30)])
    g.arrow2((24, 38), (38, 26), color=AMBER)
    return g


@icon("Sketcher_Rotate")
def rotate():
    g = flat()
    poly_closed(g, [(22, 22), (42, 22), (42, 42), (22, 42)], color=LIGHT, sw=SW_GEO, dash="4,3")
    c, r = (32, 32), 14
    q = [(c[0] + r * math.cos(math.radians(a + 25)) * 1.0 - 0, c[1] + r * math.sin(math.radians(a + 25))) for a in (225, 315, 45, 135)]
    poly_closed(g, [(32 + 14 * math.cos(math.radians(a)), 32 + 14 * math.sin(math.radians(a))) for a in (205, 295, 25, 115)])
    arc = g.arc_pts2(c, 26, 20, 100, 12)
    g.path2(arc[:-1], color=AMBER)
    g.arrow2(arc[-2], arc[-1], color=AMBER)
    pts(g, c, r=3.2)
    return g


@icon("Sketcher_Scale")
def sscale():
    g = flat()
    poly_closed(g, [(8, 30), (30, 30), (30, 52), (8, 52)])
    poly_closed(g, [(8, 10), (52, 10), (52, 52), (8, 52)], color=AMBER, sw=SW_GEO, dash="5,3")
    g.arrow2((30, 30), (48, 14), color=NAVY, sw=SW)
    return g


@icon("Sketcher_Symmetry", "Sketcher_MirrorSketch")
def symmetry():
    g = flat()
    g.line2((32, 6), (32, 58), color=NAVY, sw=SW, dash="6,3,1,3")
    poly_closed(g, [(8, 18), (24, 18), (24, 46)], color=NAVY)
    poly_closed(g, [(56, 18), (40, 18), (40, 46)], color=AMBER, dash="4,3")
    return g


@icon("Sketcher_JoinCurves")
def join():
    g = flat()
    g.path2(g.arc_pts2((20, 44), 16, 180, 0, 12), color=NAVY)
    g.path2([(36, 44), (56, 14)], color=NAVY)
    pts(g, (36, 44), r=5.5)
    return g


@icon("Sketcher_RemoveAxesAlignment")
def rmaxes():
    g = flat()
    g.line2((8, 50), (56, 50), color=LIGHT, sw=SW)
    g.line2((14, 56), (14, 8), color=LIGHT, sw=SW)
    poly_closed(g, [(24, 36), (40, 20), (54, 34), (38, 50)])
    g.badge("x", where="tr")
    return g


@icon("Sketcher_ArcOverlay")
def arcoverlay():
    g = flat()
    g.circle2((32, 34), 20, "none", AMBER, SW, dash="3,3")
    g.path2(g.arc_pts2((32, 34), 20, 20, 160))
    pts(g, (32, 34))
    return g


@icon("Sketcher_SwitchVirtualSpace")
def vspace():
    g = Icon()
    g.rect3(0, 10, 0, 8, 0, "#EDF2F7", stroke=PAPER_LINE, sw=SW)
    g.rect3(0, 10, 0, 8, 4, YEL[0], stroke=AMBER, sw=SW, opacity=0.5)
    g.arrow3((11, -1, 0.5), (11, -1, 4), sw=SW, both=True)
    return g


# ------------------------------------------------------------ restrições
def cbase(geom):
    g = flat()
    geom(g)
    return g


def glyph(g, kind):
    """Selo amarelo com o símbolo da restrição (canto inferior direito)."""
    cx, cy, r = 46, 46, 15
    g.items.append(("raw", '<rect x="%d" y="%d" width="%d" height="%d" rx="6" fill="%s" stroke="%s" stroke-width="2"/>'
                    % (cx - r, cy - r, 2 * r, 2 * r, YEL[0], OUT)))
    s = []
    k = NAVY
    w = 3.2
    if kind == "coincident":
        s.append('<circle cx="%d" cy="%d" r="6" fill="none" stroke="%s" stroke-width="%.1f"/>' % (cx, cy, k, w))
        s.append('<circle cx="%d" cy="%d" r="2.4" fill="%s"/>' % (cx, cy, k))
    elif kind == "horizontal":
        s.append('<path d="M %d %d h 18" stroke="%s" stroke-width="%.1f" stroke-linecap="round"/>' % (cx - 9, cy, k, w))
    elif kind == "vertical":
        s.append('<path d="M %d %d v 18" stroke="%s" stroke-width="%.1f" stroke-linecap="round"/>' % (cx, cy - 9, k, w))
    elif kind == "horver":
        s.append('<path d="M %d %d v 14 h 14" fill="none" stroke="%s" stroke-width="%.1f" stroke-linecap="round" '
                 'stroke-linejoin="round"/>' % (cx - 7, cy - 7, k, w))
    elif kind == "parallel":
        s.append('<path d="M %d %d l 6 -14 M %d %d l 6 -14" stroke="%s" stroke-width="%.1f" stroke-linecap="round"/>'
                 % (cx - 7, cy + 7, cx + 1, cy + 7, k, w))
    elif kind == "perpendicular":
        s.append('<path d="M %d %d h 16 M %d %d v -15" stroke="%s" stroke-width="%.1f" stroke-linecap="round"/>'
                 % (cx - 8, cy + 7, cx, cy + 7, k, w))
    elif kind == "tangent":
        s.append('<circle cx="%d" cy="%d" r="6" fill="none" stroke="%s" stroke-width="2.4"/>' % (cx, cy + 2, k))
        s.append('<path d="M %d %d h 18" stroke="%s" stroke-width="%.1f" stroke-linecap="round"/>' % (cx - 9, cy - 4, k, w))
    elif kind == "equal":
        s.append('<path d="M %d %d h 16 M %d %d h 16" stroke="%s" stroke-width="%.1f" stroke-linecap="round"/>'
                 % (cx - 8, cy - 4, cx - 8, cy + 4, k, w))
    elif kind == "symmetric":
        s.append('<path d="M %d %d v 18" stroke="%s" stroke-width="2" stroke-dasharray="3,2"/>' % (cx, cy - 9, k))
        s.append('<path d="M %d %d l -6 5 l 6 5 M %d %d l 6 5 l -6 5" fill="none" stroke="%s" stroke-width="2.6" '
                 'stroke-linejoin="round"/>' % (cx - 3, cy - 5, cx + 3, cy - 5, k))
    elif kind == "lock":
        s.append('<rect x="%d" y="%d" width="14" height="10" rx="2" fill="%s"/>' % (cx - 7, cy - 2, k))
        s.append('<path d="M %d %d v -4 a 4 4 0 0 1 8 0 v 4" fill="none" stroke="%s" stroke-width="2.6"/>' % (cx - 4, cy - 2, k))
    elif kind == "dimension":
        s.append('<path d="M %d %d h 18" stroke="%s" stroke-width="2.4"/>' % (cx - 9, cy, k))
        s.append('<path d="M %d %d l 5 -3.5 v 7 z M %d %d l -5 -3.5 v 7 z" fill="%s"/>' % (cx - 9, cy, cx + 9, cy, k))
    elif kind == "radius":
        s.append('<text x="%d" y="%d" font-family="Segoe UI, Arial" font-size="17" font-weight="800" fill="%s" '
                 'text-anchor="middle">R</text>' % (cx, cy + 6, k))
    elif kind == "diameter":
        s.append('<circle cx="%d" cy="%d" r="7" fill="none" stroke="%s" stroke-width="2.6"/>' % (cx, cy, k))
        s.append('<path d="M %d %d l 14 -14" stroke="%s" stroke-width="2.6"/>' % (cx - 7, cy + 7, k))
    elif kind == "angle":
        s.append('<path d="M %d %d h 16 M %d %d l 12 -12" stroke="%s" stroke-width="%.1f" stroke-linecap="round"/>'
                 % (cx - 8, cy + 7, cx - 8, cy + 7, k, w))
        s.append('<path d="M %d %d a 10 10 0 0 0 -3 -7" fill="none" stroke="%s" stroke-width="2"/>' % (cx + 3, cy + 7, k))
    elif kind == "toggle":
        s.append('<rect x="%d" y="%d" width="20" height="10" rx="5" fill="#FFFFFF" stroke="%s" stroke-width="2"/>'
                 % (cx - 10, cy - 5, k))
        s.append('<circle cx="%d" cy="%d" r="4" fill="%s"/>' % (cx + 5, cy, k))
    for x in s:
        g.items.append(("raw", x))


def _two_lines(g, a, b, c, d, color=LIGHT):
    g.line2(a, b, color=color)
    g.line2(c, d, color=color)


@icon("Constraint_PointOnPoint", "Sketcher_ConstrainCoincident", "Sketcher_ConstrainCoincidentUnified")
def c_coinc():
    g = flat()
    _two_lines(g, (6, 40), (28, 22), (28, 22), (40, 6))
    pts(g, (28, 22), r=5)
    glyph(g, "coincident")
    return g


@icon("Constraint_Horizontal")
def c_h():
    g = flat()
    g.line2((6, 20), (44, 20), color=LIGHT)
    pts(g, (6, 20), (44, 20))
    glyph(g, "horizontal")
    return g


@icon("Constraint_Vertical")
def c_v():
    g = flat()
    g.line2((20, 4), (20, 42), color=LIGHT)
    pts(g, (20, 4), (20, 42))
    glyph(g, "vertical")
    return g


@icon("Constraint_HorVer")
def c_hv():
    g = flat()
    g.path2([(8, 6), (8, 30), (36, 30)], color=LIGHT)
    glyph(g, "horver")
    return g


@icon("Constraint_Parallel")
def c_par():
    g = flat()
    _two_lines(g, (4, 30), (24, 4), (16, 40), (36, 14))
    glyph(g, "parallel")
    return g


@icon("Constraint_Perpendicular")
def c_perp():
    g = flat()
    _two_lines(g, (6, 34), (40, 34), (20, 34), (20, 4))
    glyph(g, "perpendicular")
    return g


@icon("Constraint_Tangent")
def c_tan():
    g = flat()
    g.circle2((22, 22), 13, "none", LIGHT, SW_GEO)
    g.line2((2, 35), (44, 35), color=LIGHT)
    glyph(g, "tangent")
    return g


@icon("Constraint_EqualLength")
def c_eq():
    g = flat()
    _two_lines(g, (6, 10), (36, 10), (6, 26), (36, 26))
    glyph(g, "equal")
    return g


@icon("Constraint_Symmetric")
def c_sym():
    g = flat()
    g.line2((22, 2), (22, 42), color=NAVY, sw=SW, dash="5,3,1,3")
    pts(g, (8, 22), (36, 22))
    glyph(g, "symmetric")
    return g


@icon("Constraint_Block")
def c_block():
    g = flat()
    poly_closed(g, [(6, 8), (36, 8), (36, 32), (6, 32)], color=LIGHT)
    glyph(g, "lock")
    return g


@icon("Constraint_Lock")
def c_lock():
    g = flat()
    g.line2((6, 20), (30, 20), color=LIGHT, sw=SW, dash="3,3")
    g.line2((18, 8), (18, 32), color=LIGHT, sw=SW, dash="3,3")
    pts(g, (18, 20), r=5.5)
    glyph(g, "lock")
    return g


def _dimgeo(g, horiz=True, vert=False):
    g.line2((6, 30), (36, 12), color=LIGHT)
    pts(g, (6, 30), (36, 12))


@icon("Constraint_Dimension", "Sketcher_CompDimensionTools", "Constraint_Length")
def c_dim():
    g = flat()
    _dimgeo(g)
    glyph(g, "dimension")
    return g


@icon("Constraint_HorizontalDistance")
def c_hdist():
    g = flat()
    pts(g, (6, 26), (38, 10))
    g.arrow2((6, 36), (30, 36), color=NAVY, sw=SW, both=True)
    glyph(g, "horizontal")
    return g


@icon("Constraint_VerticalDistance")
def c_vdist():
    g = flat()
    pts(g, (10, 6), (26, 34))
    g.arrow2((38, 8), (38, 32), color=NAVY, sw=SW, both=True)
    glyph(g, "vertical")
    return g


@icon("Constraint_Radius")
def c_rad():
    g = flat()
    g.path2(g.arc_pts2((24, 28), 18, 30, 210), color=LIGHT)
    g.arrow2((24, 28), (24 + 18 * math.cos(math.radians(120)), 28 - 18 * math.sin(math.radians(120))), color=NAVY, sw=SW)
    glyph(g, "radius")
    return g


@icon("Constraint_Diameter", "Constraint_Radiam")
def c_dia():
    g = flat()
    g.circle2((22, 22), 16, "none", LIGHT, SW_GEO)
    g.arrow2((12, 33), (32, 11), color=NAVY, sw=SW, both=True)
    glyph(g, "diameter")
    return g


@icon("Constraint_InternalAngle")
def c_ang():
    g = flat()
    _two_lines(g, (6, 36), (40, 36), (6, 36), (30, 6))
    g.path2(g.arc_pts2((6, 36), 18, 0, 52, 10), color=NAVY, sw=SW)
    glyph(g, "angle")
    return g


@icon("Sketcher_ToggleConstraint", "Sketcher_ToggleActiveConstraint", "Sketcher_CompToggleConstraints")
def c_toggle():
    g = flat()
    _dimgeo(g)
    glyph(g, "toggle")
    return g


@icon("Sketcher_SelectConstraints")
def sel_cons():
    g = flat()
    g.items.append(("raw", '<path d="M 8 6 L 8 34 L 15 27 L 20 38 L 25 36 L 20 25 L 29 25 Z" fill="#FFFFFF" '
                           'stroke="%s" stroke-width="2.2" stroke-linejoin="round"/>' % OUT))
    glyph(g, "dimension")
    return g


@icon("Sketcher_SelectElementsAssociatedWithConstraints")
def sel_elems():
    g = flat()
    g.line2((24, 8), (40, 26), color=AMBER)
    g.items.append(("raw", '<path d="M 6 14 L 6 42 L 13 35 L 18 46 L 23 44 L 18 33 L 27 33 Z" fill="#FFFFFF" '
                           'stroke="%s" stroke-width="2.2" stroke-linejoin="round"/>' % OUT))
    glyph(g, "coincident")
    return g


# ------------------------------------------------------------ gestão do esboço
def _sheet(g, z=0, prof=True):
    g.rect3(0, 10, 0, 8, z, "#EDF2F7", stroke=PAPER_LINE, sw=SW)
    if prof:
        P = [(2.5, 2, z), (7.5, 2, z), (7.5, 6, z), (4.5, 6, z), (2.5, 4.5, z)]
        g.poly(P, AMBER_FILL, stroke=AMBER, sw=SW_GEO * 0.85)


@icon("Sketcher_NewSketch", "PartDesign_NewSketch")
def newsk():
    g = Icon()
    _sheet(g)
    g.badge("plus")
    return g


@icon("Sketcher_EditSketch")
def editsk():
    g = Icon()
    _sheet(g)
    g.badge("pencil")
    return g


@icon("Sketcher_LeaveSketch")
def leavesk():
    g = Icon()
    _sheet(g)
    g.badge("check")
    return g


@icon("Sketcher_ValidateSketch")
def validsk():
    g = Icon()
    _sheet(g)
    g.badge("eye")
    return g


@icon("Sketcher_MapSketch")
def mapsk():
    g = Icon()
    g.box(0, 10, 0, 8, -4, 0, pal=COPY)
    _sheet(g, 0.01)
    g.badge("link")
    return g


@icon("Sketcher_ReorientSketch")
def reorient():
    g = Icon()
    g.poly([(0, 0, 0), (10, 0, 0), (10, 0, 8), (0, 0, 8)], "#EDF2F7", stroke=PAPER_LINE, sw=SW)
    g.poly([(2.5, 0, 2), (7.5, 0, 2), (7.5, 0, 6), (2.5, 0, 6)], AMBER_FILL, stroke=AMBER, sw=SW_GEO * 0.85)
    arc = [g.p(11 * math.cos(math.radians(a)), 11 * math.sin(math.radians(a)), 0) for a in range(-40, 50, 8)]
    g.path2(arc[:-1], color=NAVY)
    g.arrow2(arc[-2], arc[-1])
    return g


@icon("Sketcher_MergeSketch")
def mergesk():
    g = Icon()
    g.rect3(0, 8, 0, 6, 0, "#EDF2F7", stroke=PAPER_LINE, sw=SW)
    g.rect3(1.5, 4.5, 1.5, 4.5, 0, "none", stroke=AMBER, sw=SW_GEO * 0.8)
    g.rect3(3, 11, 3, 9, 5, "#EDF2F7", stroke=PAPER_LINE, sw=SW, opacity=0.9)
    g.ellipse_iso(8, 6, 1.8, 5, "none", AMBER, SW_GEO * 0.8)
    g.arrow3((9.5, 1, 5), (7.5, -0.5, 1), sw=SW)
    return g


@icon("Sketcher_ViewSketch")
def viewsk():
    g = flat()
    g.items.append(("raw", '<rect x="8" y="8" width="48" height="48" rx="3" fill="#EDF2F7" stroke="%s" stroke-width="2"/>' % PAPER_LINE))
    poly_closed(g, [(18, 20), (46, 20), (46, 44), (30, 44), (18, 34)], color=AMBER, fill=AMBER_FILL)
    g.badge("eye")
    return g


@icon("Sketcher_ViewSection")
def viewsec():
    g = Icon()
    g.box(0, 9, 0, 7, 0, 6, pal=COPY)
    g.poly([(4.5, -2, -1), (4.5, 9, -1), (4.5, 9, 8), (4.5, -2, 8)], YEL[0], stroke=AMBER, sw=SW, opacity=0.5)
    return g
