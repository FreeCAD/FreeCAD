# SPDX-License-Identifier: LGPL-2.1-or-later
# Ícones de recursos 3D (Peça e Sólidos) no estilo do guia ilustrado.
import math
from nucleo import Icon, SOLID, COPY, BASE, CUT, YEL, OUT, NAVY, AMBER, AMBER_FILL, SW, SW_GEO

ICONS = {}


def icon(*names):
    def deco(fn):
        for n in names:
            ICONS[n] = fn
        return fn
    return deco


# ------------------------------------------------------------ corpo / referências
@icon("PartDesign_Body")
def body():
    g = Icon()
    g.box(0, 9, 0, 6, 0, 3)
    g.box(0, 4, 0, 6, 3, 7)
    return g


@icon("PartDesign_Clone")
def clone():
    g = Icon()
    g.box(0, 6, 0, 5, 0, 5, pal=("none", "none", "none"), dash="4,3")
    g.box(4, 10, 4, 9, 0, 5)
    return g


@icon("PartDesign_ShapeBinder")
def binder():
    g = Icon()
    g.box(0, 8, 0, 6, 0, 5, pal=COPY)
    g.badge("link")
    return g


@icon("PartDesign_SubShapeBinder")
def subbinder():
    g = Icon()
    g.box(0, 8, 0, 6, 0, 5, pal=(YEL[0], COPY[1], COPY[2]))
    g.badge("link")
    return g


@icon("PartDesign_Plane", "Std_Plane")
def dplane():
    g = Icon()
    g.rect3(0, 10, 0, 8, 0, YEL[0], stroke=AMBER, sw=SW_GEO * 0.8, opacity=0.55)
    g.rect3(0, 10, 0, 8, 0, "none", stroke=OUT, sw=SW)
    return g


@icon("PartDesign_Line", "Std_Axis")
def dline():
    g = Icon()
    g.line((0, 0, 0), (0, 0, 10), color=AMBER, sw=SW_GEO * 1.3)
    g.dot2(g.p(0, 0, 0), 4)
    g.dot2(g.p(0, 0, 10), 4)
    g.line((-3, 3, 4), (3, -3, 4), color="#B0BCC9", sw=SW, dash="3,3")
    return g


@icon("PartDesign_Point", "Std_Point")
def dpoint():
    g = Icon()
    g.line((-4, 0, 0), (4, 0, 0), color="#B0BCC9", sw=SW)
    g.line((0, -4, 0), (0, 4, 0), color="#B0BCC9", sw=SW)
    g.line((0, 0, -4), (0, 0, 4), color="#B0BCC9", sw=SW)
    g.dot2(g.p(0, 0, 0), 7)
    return g


@icon("PartDesign_CoordinateSystem", "Std_CoordinateSystem")
def dcs():
    g = Icon()
    g.arrow3((0, 0, 0), (9, 0, 0), color="#B5523F")
    g.arrow3((0, 0, 0), (0, 9, 0), color="#2E8B6A")
    g.arrow3((0, 0, 0), (0, 0, 9), color=NAVY)
    g.dot2(g.p(0, 0, 0), 4)
    return g


# ------------------------------------------------------------ adicionar material
@icon("PartDesign_Pad")
def pad():
    g = Icon()
    g.box(0, 7, 0, 6, 0, 5)
    g.profile(0, 7, 0, 6, 0)
    g.arrow3((3.5, 3, 5), (3.5, 3, 11))
    return g


@icon("Part_Extrude")
def extrude():
    g = Icon()
    g.box(0, 7, 0, 6, 0, 6, pal=COPY, opacity=0.9)
    g.profile(0, 7, 0, 6, 0, fill=True)
    g.arrow3((3.5, 3, 0), (3.5, 3, 10))
    return g


@icon("PartDesign_Revolution", "Part_Revolve")
def revolution():
    g = Icon()
    g.cyl(0, 0, 6, 0, 6)
    g.rect3(0, 6, 0, 0.01, 6, "none", stroke=AMBER, sw=SW_GEO)
    g.line((0, 0, -2), (0, 0, 10), color=NAVY, sw=SW, dash="6,3,1,3")
    pts = [g.p(8 * math.cos(math.radians(a)), 8 * math.sin(math.radians(a)), 9) for a in range(-80, 200, 10)]
    g.path2(pts[:-1], color=NAVY, sw=SW_GEO)
    g.arrow2(pts[-2], pts[-1])
    return g


@icon("PartDesign_Groove")
def groove():
    g = Icon()
    g.cyl(0, 0, 6, 0, 2.5)
    g.cyl(0, 0, 4.2, 2.5, 4.5, pal=CUT)
    g.cyl(0, 0, 6, 4.5, 7)
    g.line((0, 0, -2), (0, 0, 10), color=NAVY, sw=SW, dash="6,3,1,3")
    return g


def _loft(g, pal):
    pts_b = [(0, 0, 0), (8, 0, 0), (8, 8, 0), (0, 8, 0)]
    n = 36
    top = [(4 + 2.6 * math.cos(2 * math.pi * i / n), 4 + 2.6 * math.sin(2 * math.pi * i / n), 9) for i in range(n)]
    # silhueta frontal
    g.poly([(0, 8, 0), (8, 8, 0), (8, 0, 0), (6.6, 2.2, 9), (1.6, 6.4, 9)], pal[1])
    g.poly([(8, 8, 0), (8, 0, 0), (6.6, 2.2, 9), (5.8, 5.8, 9)], pal[2])
    g.poly2([g.p(*q) for q in top], pal[0], AMBER, SW_GEO * 0.8)
    g.poly2([g.p(*q) for q in pts_b], "none", AMBER, SW_GEO * 0.8)


@icon("PartDesign_AdditiveLoft", "Part_Loft")
def loft():
    g = Icon()
    _loft(g, SOLID)
    return g


@icon("PartDesign_SubtractiveLoft")
def sloft():
    g = Icon()
    g.box(-2, 10, -2, 10, -3, 0, pal=BASE)
    _loft(g, CUT)
    return g


def _pipe(g, pal):
    pts = [(0, 0, 0), (0, 0, 4), (1, 0, 7), (4, 0, 9), (9, 0, 9)]
    sm = []
    for i in range(len(pts) - 1):
        for t in range(6):
            a, b = pts[i], pts[i + 1]
            sm.append(g.p(*(a[j] + (b[j] - a[j]) * t / 6 for j in range(3))))
    sm.append(g.p(*pts[-1]))
    g.path2(sm, color=OUT, sw=17)
    g.path2(sm, color=pal[2], sw=14)
    g.path2(sm, color=pal[0], sw=6)
    g.ellipse_iso(0, 0, 2.2, 0, AMBER_FILL, AMBER, SW_GEO * 0.8)


@icon("PartDesign_AdditivePipe", "Part_Sweep")
def pipe():
    g = Icon()
    _pipe(g, SOLID)
    return g


@icon("PartDesign_SubtractivePipe")
def spipe():
    g = Icon()
    g.box(-3, 11, -3, 4, -2, 0, pal=BASE)
    _pipe(g, CUT)
    return g


def _helix(g, color):
    pts = [g.p(4 * math.cos(t / 8), 4 * math.sin(t / 8), t / 8 * 0.9) for t in range(0, 8 * 2 * 18, 2)]
    g.path2(pts, color=OUT, sw=9)
    g.path2(pts, color=color, sw=6)


@icon("PartDesign_AdditiveHelix")
def helix():
    g = Icon()
    _helix(g, SOLID[1])
    return g


@icon("PartDesign_SubtractiveHelix")
def shelix():
    g = Icon()
    g.cyl(0, 0, 5, -2, 0, pal=BASE)
    _helix(g, CUT[1])
    return g


# ------------------------------------------------------------ remover material
@icon("PartDesign_Pocket")
def pocket():
    g = Icon()
    g.box(0, 10, 0, 8, 0, 5, pal=BASE)
    hx0, hx1, hy0, hy1, top, zb = 2.5, 7, 2, 6, 5, 2
    g.rect3(hx0, hx1, hy0, hy1, top, CUT[2])
    g.poly([(hx0, hy0, top), (hx1, hy0, top), (hx1, hy0, zb), (hx0, hy0, zb)], CUT[1], sw=SW * 0.8)
    g.poly([(hx0, hy0, top), (hx0, hy1, top), (hx0, hy1, zb), (hx0, hy0, zb)], CUT[0], sw=SW * 0.8)
    g.rect3(hx0, hx1, hy0, hy1, top, "none", stroke=AMBER, sw=SW_GEO * 0.8)
    g.arrow3((4.7, 4, 11), (4.7, 4, 7.5))
    return g


@icon("PartDesign_Hole")
def hole():
    g = Icon()
    g.box(0, 10, 0, 8, 0, 5, pal=BASE)
    g.ellipse_iso(5, 4, 3.2, 5, CUT[2])
    n = 32
    inner = [g.p(5 + 3.2 * math.cos(math.radians(a)), 4 + 3.2 * math.sin(math.radians(a)), 5) for a in range(135, 315, 6)]
    low = [g.p(5 + 3.2 * math.cos(math.radians(a)), 4 + 3.2 * math.sin(math.radians(a)), 2.3) for a in range(135, 315, 6)]
    g.poly2(inner + low[::-1], CUT[1], OUT, SW * 0.7)
    g.ellipse_iso(5, 4, 3.2, 5, "none", AMBER, SW_GEO * 0.7)
    g.line((5, 4, 9), (5, 4, 0.5), color=NAVY, sw=SW, )
    return g


# ------------------------------------------------------------ acabamento
def _edge_block(g, kind):
    X0, X1, Y0, Y1, Z1 = 0, 9, 0, 6, 5
    c = 2.0
    if kind == "fillet":
        n = 8
        edge = [(Y1 - c + c * math.sin(math.pi / 2 * i / n), Z1 - c + c * math.cos(math.pi / 2 * i / n)) for i in range(n + 1)]
        zl = Z1 - c
    else:
        edge = [(Y1 - c, Z1), (Y1, Z1 - c)]
        zl = Z1 - c
    g.poly([(X0, Y1, 0), (X1, Y1, 0), (X1, Y1, zl), (X0, Y1, zl)], SOLID[1])
    g.poly([(X1, Y0, 0), (X1, Y1, 0)] + [(X1, y, z) for y, z in reversed(edge)] + [(X1, Y0, Z1)], SOLID[2])
    g.poly([(X0, Y0, Z1), (X1, Y0, Z1), (X1, Y1 - c, Z1), (X0, Y1 - c, Z1)], SOLID[0])
    g.poly([(X0, y, z) for y, z in edge] + [(X1, y, z) for y, z in reversed(edge)], YEL[0])


@icon("PartDesign_Fillet", "Part_Fillet")
def fillet():
    g = Icon()
    _edge_block(g, "fillet")
    return g


@icon("PartDesign_Chamfer", "Part_Chamfer")
def chamfer():
    g = Icon()
    _edge_block(g, "chamfer")
    return g


@icon("PartDesign_Draft")
def draft():
    g = Icon()
    k = 1.6
    g.poly([(0, 6, 0), (9, 6, 0), (9, 6 - k, 6), (0, 6 - k, 6)], YEL[0])
    g.poly([(9, 0, 0), (9, 6, 0), (9, 6 - k, 6), (9, k, 6)], SOLID[2])
    g.poly([(0, k, 6), (9, k, 6), (9, 6 - k, 6), (0, 6 - k, 6)], SOLID[0])
    g.line((0, 6, 0), (0, 6, 6.5), color=NAVY, sw=SW, dash="3,2")
    return g


@icon("PartDesign_Thickness", "Part_Thickness")
def thickness():
    g = Icon()
    X0, X1, Y0, Y1, Z1, t = 0, 9, 0, 7, 6, 1.2
    g.rect3(X0 + t, X1 - t, Y0 + t, Y1 - t, t, SOLID[0], sw=SW * 0.8)
    g.poly([(X0 + t, Y0 + t, Z1), (X1 - t, Y0 + t, Z1), (X1 - t, Y0 + t, t), (X0 + t, Y0 + t, t)], SOLID[2], sw=SW * 0.8)
    g.poly([(X0 + t, Y0 + t, Z1), (X0 + t, Y1 - t, Z1), (X0 + t, Y1 - t, t), (X0 + t, Y0 + t, t)], SOLID[1], sw=SW * 0.8)
    g.poly([(X0, Y1, 0), (X1, Y1, 0), (X1, Y1, Z1), (X0, Y1, Z1)], SOLID[1])
    g.poly([(X1, Y0, 0), (X1, Y1, 0), (X1, Y1, Z1), (X1, Y0, Z1)], SOLID[2])
    o = [g.p(X0, Y0, Z1), g.p(X1, Y0, Z1), g.p(X1, Y1, Z1), g.p(X0, Y1, Z1)]
    i = [g.p(X0 + t, Y0 + t, Z1), g.p(X0 + t, Y1 - t, Z1), g.p(X1 - t, Y1 - t, Z1), g.p(X1 - t, Y0 + t, Z1)]
    # anel (dois polígonos: externo amarelo, abertura branca por cima)
    g.poly2(o, YEL[0])
    g.poly2(i[::-1], "#FFFFFF", OUT, SW * 0.8)
    # recoloca as paredes internas visíveis
    g.poly([(X0 + t, Y0 + t, Z1), (X1 - t, Y0 + t, Z1), (X1 - t, Y0 + t, Z1 - 2.2), (X0 + t, Y0 + t, Z1 - 2.2)], SOLID[2], sw=SW * 0.8)
    g.poly([(X0 + t, Y0 + t, Z1), (X0 + t, Y1 - t, Z1), (X0 + t, Y1 - t, Z1 - 2.2), (X0 + t, Y0 + t, Z1 - 2.2)], SOLID[1], sw=SW * 0.8)
    return g


# ------------------------------------------------------------ padrões
@icon("PartDesign_Mirrored", "Part_Mirror")
def mirrored():
    g = Icon()
    g.box(-8, -3, 0, 4, 0, 4)
    g.poly([(0, -2, -1), (0, 6, -1), (0, 6, 6), (0, -2, 6)], YEL[0], stroke=AMBER, sw=SW, opacity=0.45, dash="4,3")
    g.box(3, 8, 0, 4, 0, 4, pal=COPY)
    return g


@icon("PartDesign_LinearPattern")
def linear():
    g = Icon()
    for i in range(3):
        g.box(i * 5.5, i * 5.5 + 4, 0, 4, 0, 4, pal=SOLID if i == 0 else COPY)
    g.arrow3((0, 6.5, 0), (15, 6.5, 0))
    return g


@icon("PartDesign_PolarPattern")
def polar():
    g = Icon()
    R = 6.5
    order = sorted(range(5), key=lambda i: math.cos(2 * math.pi * i / 5 + 0.4) + math.sin(2 * math.pi * i / 5 + 0.4))
    g.ellipse_iso(0, 0, R, 0, "none", "#9FB1C4", SW, dash="3,3")
    for i in order:
        a = 2 * math.pi * i / 5 + 0.4
        x, y = R * math.cos(a), R * math.sin(a)
        g.box(x - 1.8, x + 1.8, y - 1.8, y + 1.8, 0, 3.4, pal=SOLID if i == 0 else COPY)
    g.line((0, 0, -2), (0, 0, 7), color=NAVY, sw=SW, dash="6,3,1,3")
    return g


@icon("PartDesign_MultiTransform")
def multi():
    g = Icon()
    for i in range(2):
        for j in range(2):
            g.box(i * 5, i * 5 + 3, j * 5, j * 5 + 3, 0, 3, pal=SOLID if i == j == 0 else COPY)
    g.arrow3((0, -2, 0), (9, -2, 0))
    g.arrow3((-2, 0, 0), (-2, 9, 0))
    return g


# ------------------------------------------------------------ sólidos (Part)
@icon("Part_Box_Parametric", "Part_Box")
def pbox():
    g = Icon()
    g.box(0, 8, 0, 8, 0, 8)
    return g


@icon("Part_Cylinder_Parametric", "Part_Cylinder")
def pcyl():
    g = Icon()
    g.cyl(0, 0, 5, 0, 9)
    return g


@icon("Part_Sphere_Parametric", "Part_Sphere")
def psph():
    g = Icon()
    g.sphere2((0, 0), 10)
    return g


@icon("Part_Cone_Parametric", "Part_Cone")
def pcone():
    g = Icon()
    g.cone(0, 0, 5.5, 0, 10)
    return g


@icon("Part_Torus_Parametric", "Part_Torus")
def ptorus():
    g = Icon()
    g.ellipse_iso(0, 0, 8, 0, g.grad(SOLID[0], SOLID[2]))
    g.ellipse_iso(0, 0, 3.4, 0.6, "#FFFFFF", OUT, SW)
    return g


@icon("Part_Tube_Parametric", "Part_Tube")
def ptube():
    g = Icon()
    g.cyl(0, 0, 5.5, 0, 9, hole=3.2)
    return g


@icon("Part_Primitives")
def pprims():
    g = Icon()
    g.box(0, 5, 3, 8, 0, 5)
    g.cone(8, 0, 3, 0, 7, pal=COPY)
    g.cyl(1, -3, 2.5, 0, 4)
    return g


@icon("Part_Shapebuilder")
def pbuilder():
    g = Icon()
    g.box(0, 8, 0, 8, 0, 8, pal=("none", "none", "none"), dash="3,3")
    g.poly([(0, 8, 0), (8, 8, 0), (8, 8, 8), (0, 8, 8)], YEL[0])
    g.poly([(8, 0, 0), (8, 8, 0), (8, 8, 8), (8, 0, 8)], SOLID[2])
    return g


def _two_boxes(g, mode):
    A = (0, 7, 0, 7, 0, 6)
    B = (4, 11, -3, 4, 2, 8)
    if mode == "cut":
        g.box(*A)
        g.box(4, 7, 0, 4, 2, 6, pal=CUT)
        g.box(*B, pal=("none", "none", "none"), dash="3,3")
    elif mode == "fuse":
        g.box(*A)
        g.box(*B)
    elif mode == "common":
        g.box(*A, pal=("none", "none", "none"), dash="3,3")
        g.box(*B, pal=("none", "none", "none"), dash="3,3")
        g.box(4, 7, 0, 4, 2, 6)
    else:
        g.box(*A)
        g.box(*B, pal=COPY, opacity=0.85)


@icon("Part_Cut")
def pcut():
    g = Icon()
    _two_boxes(g, "cut")
    return g


@icon("Part_Fuse")
def pfuse():
    g = Icon()
    _two_boxes(g, "fuse")
    return g


@icon("Part_Common")
def pcommon():
    g = Icon()
    _two_boxes(g, "common")
    return g


@icon("Part_Booleans", "PartDesign_Boolean")
def pbool():
    g = Icon()
    _two_boxes(g, "both")
    return g


@icon("Part_Scale")
def pscale():
    g = Icon()
    g.box(0, 9, 0, 9, 0, 9, pal=("none", "none", "none"), dash="3,3")
    g.box(0, 5, 4, 9, 0, 5)
    g.arrow3((5.5, 3.5, 5.5), (8.6, 0.4, 8.6))
    return g


@icon("Part_Section")
def psection():
    g = Icon()
    g.box(0, 8, 0, 8, 0, 6, pal=COPY)
    g.cyl(4, 4, 3.5, -2, 9, pal=COPY)
    g.ellipse_iso(4, 4, 3.5, 6, "none", YEL[1], SW_GEO * 1.1)
    return g


@icon("Part_CrossSections")
def pcross():
    g = Icon()
    g.box(0, 8, 0, 6, 0, 9, pal=COPY)
    for z in (2, 4.5, 7):
        g.rect3(-1.5, 9.5, -1.5, 7.5, z, YEL[0], stroke=AMBER, sw=SW * 0.9, opacity=0.5)
    return g


@icon("Part_MakeFace")
def pface():
    g = Icon()
    g.rect3(0, 10, 0, 8, 0, YEL[0], stroke=AMBER, sw=SW_GEO, opacity=0.6)
    for q in ((0, 0), (10, 0), (10, 8), (0, 8)):
        g.dot2(g.p(q[0], q[1], 0), 3.6)
    return g


@icon("Part_RuledSurface")
def pruled():
    g = Icon()
    a = [(0, 0, 8), (10, 0, 6)]
    b = [(0, 8, 0), (10, 8, 2)]
    g.poly([a[0], a[1], b[1], b[0]], YEL[0], stroke=OUT, sw=SW, opacity=0.6)
    for i in range(1, 5):
        t = i / 5
        g.line(tuple(a[0][j] + (a[1][j] - a[0][j]) * t for j in range(3)),
               tuple(b[0][j] + (b[1][j] - b[0][j]) * t for j in range(3)), color=AMBER, sw=SW)
    g.line(a[0], a[1], color=NAVY)
    g.line(b[0], b[1], color=NAVY)
    return g


@icon("Part_ProjectionOnSurface")
def pproj():
    g = Icon()
    g.box(0, 10, 0, 8, 0, 3)
    pts = [g.p(2 + i * 0.6, 2 + 4 * math.sin(i / 2.0) * 0.5 + 2, 3) for i in range(11)]
    g.path2(pts, color=AMBER, sw=SW_GEO)
    g.arrow3((5, 4, 10), (5, 4, 4))
    return g


@icon("Part_Defeaturing")
def pdefeat():
    g = Icon()
    g.box(0, 10, 0, 8, 0, 4)
    g.box(6, 9, 1, 3, 4, 6.5, pal=CUT, dash="3,2")
    g.badge("minus")
    return g


@icon("Part_CheckGeometry")
def pcheck():
    g = Icon()
    g.box(0, 9, 0, 7, 0, 6)
    g.badge("check")
    return g
