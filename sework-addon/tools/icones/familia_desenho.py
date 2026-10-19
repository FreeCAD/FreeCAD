# SPDX-License-Identifier: LGPL-2.1-or-later
# Desenho técnico (TechDraw): vistas, linhas de centro, cosméticos, cotas e roscas no estilo Sework.
import math
from nucleo import Icon, SOLID, COPY, YEL, OUT, NAVY, AMBER, RED, STEEL, PAPER, PAPER_LINE
from familia_geral import raw, tri, txt, sheet_landscape, mini_part, _dim, page

ICONS = {}
DASHDOT = "7,3,1.5,3"


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


def paper(g):
    raw(g, '<rect x="4" y="4" width="56" height="56" rx="4" fill="%s" stroke="%s" stroke-width="1.4"/>' % (PAPER, PAPER_LINE))


def line(g, x1, y1, x2, y2, color=OUT, w=3.2, dash=None, cap="round"):
    d = ' stroke-dasharray="%s"' % dash if dash else ""
    raw(g, '<path d="M %.1f %.1f L %.1f %.1f" stroke="%s" stroke-width="%.1f" stroke-linecap="%s"%s/>'
        % (x1, y1, x2, y2, color, w, cap, d))


def dot(g, x, y, r=4.2, fill=None):
    raw(g, '<circle cx="%.1f" cy="%.1f" r="%.1f" fill="%s" stroke="%s" stroke-width="1.8"/>' % (x, y, r, fill or YEL[0], OUT))


def badge(g, kind):
    g.badge(kind)


# ============================================================ vistas
@icon("actions/TechDraw_ProjectShape", "TechDraw_ProjectShape")
def project_shape():
    g = flat()
    sheet_landscape(g)
    mini_part(g, 22, 22, 1.1)
    raw(g, '<rect x="34" y="17" width="16" height="14" fill="none" stroke="%s" stroke-width="2.4"/>' % NAVY)
    line(g, 26, 25, 33, 24, AMBER, 2.2, "3,2")
    return g


@icon("actions/TechDraw_ProjectionGroup", "TechDraw_ProjectionGroup")
def projection_group():
    g = flat()
    paper(g)
    for x, y, w, h in ((24, 8, 16, 12), (24, 24, 16, 16), (8, 24, 12, 16), (44, 24, 12, 16)):
        raw(g, '<rect x="%d" y="%d" width="%d" height="%d" fill="%s" stroke="%s" stroke-width="2.4"/>'
            % (x, y, w, h, YEL[0] if (x, y) == (24, 24) else SOLID[0], OUT))
    mini_part(g, 32, 46, 0.9)
    return g


def _section(g, complex_=False):
    sheet_landscape(g)
    raw(g, '<rect x="12" y="16" width="26" height="22" fill="%s" stroke="%s" stroke-width="2.4"/>' % (SOLID[0], OUT))
    # hachura a 45° recortada à mão no retângulo 12..38 x 16..38 (o Qt ignora clip-path)
    x0, x1, y0, y1 = 12.0, 38.0, 16.0, 38.0
    for c in range(-20, 40, 5):          # linhas y = -x + k, k = x0 + y1 + c
        k = x0 + y1 + c
        pts = []
        for x in (x0, x1):
            y = k - x
            if y0 <= y <= y1:
                pts.append((x, y))
        for y in (y0, y1):
            x = k - y
            if x0 <= x <= x1:
                pts.append((x, y))
        pts = sorted(set(pts))
        if len(pts) >= 2:
            raw(g, '<path d="M %.1f %.1f L %.1f %.1f" stroke="%s" stroke-width="1.4"/>'
                % (pts[0][0], pts[0][1], pts[-1][0], pts[-1][1], AMBER))
    raw(g, '<rect x="12" y="16" width="26" height="22" fill="none" stroke="%s" stroke-width="2.4"/>' % OUT)
    if complex_:
        line(g, 25, 10, 25, 26, RED, 2.4, DASHDOT, "butt")
        line(g, 25, 26, 44, 26, RED, 2.4, DASHDOT, "butt")
        line(g, 44, 26, 44, 44, RED, 2.4, DASHDOT, "butt")
    else:
        line(g, 25, 10, 25, 46, RED, 2.4, DASHDOT, "butt")
    txt(g, 48, 22, "A", 11, NAVY)


@icon("actions/TechDraw_SectionView", "TechDraw_SectionView")
def section_view():
    g = flat()
    _section(g)
    return g


@icon("actions/TechDraw_ComplexSection", "TechDraw_ComplexSection")
def complex_section():
    g = flat()
    _section(g, True)
    return g


@icon("actions/TechDraw_ArchView", "TechDraw_ArchView")
def arch_view():
    g = flat()
    sheet_landscape(g)
    raw(g, '<path d="M 12 38 V 22 L 25 14 L 38 22 V 38 Z" fill="%s" stroke="%s" stroke-width="2.4" stroke-linejoin="round"/>' % (SOLID[0], OUT))
    raw(g, '<rect x="21" y="27" width="8" height="11" fill="%s" stroke="%s" stroke-width="1.6"/>' % (YEL[0], OUT))
    return g


@icon("actions/TechDraw_SpreadsheetView", "TechDraw_SpreadsheetView")
def spreadsheet_view():
    g = flat()
    sheet_landscape(g)
    raw(g, '<rect x="12" y="16" width="28" height="22" fill="#FFFFFF" stroke="%s" stroke-width="2"/>' % OUT)
    raw(g, '<rect x="12" y="16" width="28" height="6" fill="%s"/>' % SOLID[1])
    for x in (21, 30):
        line(g, x, 16, x, 38, PAPER_LINE, 1.2)
    for y in (27, 32):
        line(g, 12, y, 40, y, PAPER_LINE, 1.2)
    raw(g, '<rect x="21" y="27" width="9" height="5" fill="%s"/>' % YEL[0])
    return g


@icon("actions/TechDraw_Image", "TechDraw_Image")
def image_view():
    g = flat()
    sheet_landscape(g)
    raw(g, '<rect x="11" y="15" width="30" height="24" rx="2" fill="%s" stroke="%s" stroke-width="2"/>' % (SOLID[0], OUT))
    raw(g, '<path d="M 13 37 L 22 26 L 29 33 L 33 29 L 39 37 Z" fill="%s" stroke="%s" stroke-width="1.4" stroke-linejoin="round"/>' % (STEEL, OUT))
    raw(g, '<circle cx="33" cy="21" r="3" fill="%s"/>' % YEL[0])
    return g


@icon("actions/TechDraw_Symbol", "TechDraw_Symbol")
def symbol_view():
    g = flat()
    sheet_landscape(g)
    raw(g, '<polygon points="25,13 38,38 12,38" fill="%s" stroke="%s" stroke-width="2.4" stroke-linejoin="round"/>' % (YEL[0], OUT))
    txt(g, 25, 35, "!", 13, NAVY)
    return g


@icon("actions/TechDraw_MoveView.svg", "TechDraw_MoveView")
def move_view():
    g = flat()
    sheet_landscape(g)
    raw(g, '<rect x="11" y="17" width="14" height="12" fill="%s" stroke="%s" stroke-width="2" stroke-dasharray="3,2"/>' % (COPY[0], OUT))
    raw(g, '<rect x="28" y="24" width="14" height="12" fill="%s" stroke="%s" stroke-width="2"/>' % (SOLID[0], OUT))
    line(g, 19, 30, 26, 31, NAVY, 2.6)
    tri(g, [(30, 32), (24, 34), (25, 27)])
    return g


@icon("actions/TechDraw_ShareView.svg", "TechDraw_ShareView")
def share_view():
    g = flat()
    for x, y, fill in ((6, 8, PAPER), (22, 22, PAPER)):
        raw(g, '<rect x="%d" y="%d" width="36" height="28" rx="2" fill="%s" stroke="%s" stroke-width="2.2"/>' % (x, y, fill, OUT))
    mini_part(g, 40, 34, 0.9)
    badge(g, "link")
    return g


def _clip(sign):
    def f():
        g = flat()
        sheet_landscape(g)
        raw(g, '<rect x="11" y="15" width="28" height="22" fill="none" stroke="%s" stroke-width="2.4" stroke-dasharray="4,2"/>' % NAVY)
        mini_part(g, 25, 23, 1.0)
        badge(g, sign)
        return g
    return f


ICONS["actions/TechDraw_ClipGroupAdd"] = ICONS["TechDraw_ClipGroupAdd"] = _clip("plus")
ICONS["actions/TechDraw_ClipGroupRemove"] = ICONS["TechDraw_ClipGroupRemove"] = _clip("minus")


def _stack(kind):
    def f():
        g = flat()
        for i, y in enumerate((36, 26, 16)):
            hl = (kind in ("top", "up") and i == 2) or (kind in ("bottom", "down") and i == 0)
            raw(g, '<polygon points="%d,%d %d,%d %d,%d %d,%d" fill="%s" stroke="%s" stroke-width="2.2" stroke-linejoin="round"/>'
                % (6, y, 26, y - 8, 46, y, 26, y + 8, YEL[0] if hl else SOLID[i], OUT))
        up = kind in ("top", "up")
        x = 54
        if up:
            line(g, x, 46, x, 18, NAVY, 3.4)
            tri(g, [(x, 10), (x - 6, 20), (x + 6, 20)])
        else:
            line(g, x, 10, x, 38, NAVY, 3.4)
            tri(g, [(x, 46), (x - 6, 36), (x + 6, 36)])
        if kind in ("top", "bottom"):
            yy = 7 if up else 50
            line(g, x - 7, yy, x + 7, yy, NAVY, 3.4)
        return g
    return f


for k, n in (("top", "StackTop"), ("bottom", "StackBottom"), ("up", "StackUp"), ("down", "StackDown")):
    ICONS["actions/TechDraw_" + n] = ICONS["TechDraw_" + n] = _stack(k)


@icon("actions/TechDraw_ExportPagePDF", "TechDraw_ExportPagePDF")
def export_pdf():
    g = flat()
    page(g)
    raw(g, '<rect x="13" y="30" width="34" height="16" rx="3" fill="%s"/>' % RED)
    txt(g, 30, 43, "PDF", 13, "#FFFFFF")
    return g


# ============================================================ linhas de centro
def _cl_base(g):
    paper(g)


@icon("actions/TechDraw_FaceCenterLine", "TechDraw_FaceCenterLine")
def face_cl():
    g = flat()
    paper(g)
    raw(g, '<rect x="12" y="16" width="40" height="32" fill="%s" stroke="%s" stroke-width="2.6"/>' % (SOLID[0], OUT))
    line(g, 32, 8, 32, 56, RED, 2.4, DASHDOT, "butt")
    line(g, 6, 32, 58, 32, RED, 2.4, DASHDOT, "butt")
    return g


@icon("actions/TechDraw_2LineCenterline", "TechDraw_2LineCenterLine")
def two_line_cl():
    g = flat()
    paper(g)
    line(g, 16, 10, 16, 54, OUT, 3.2)
    line(g, 48, 10, 48, 54, OUT, 3.2)
    line(g, 32, 6, 32, 58, RED, 2.4, DASHDOT, "butt")
    return g


@icon("actions/TechDraw_2PointCenterline", "TechDraw_2PointCenterLine")
def two_point_cl():
    g = flat()
    paper(g)
    line(g, 10, 52, 54, 12, RED, 2.4, DASHDOT, "butt")
    dot(g, 16, 47)
    dot(g, 48, 17)
    return g


@icon("TechDraw_ExtensionCircleCenterLines")
def circle_cl():
    g = flat()
    paper(g)
    raw(g, '<circle cx="32" cy="32" r="16" fill="%s" stroke="%s" stroke-width="2.8"/>' % (SOLID[0], OUT))
    line(g, 32, 8, 32, 56, RED, 2.4, DASHDOT, "butt")
    line(g, 8, 32, 56, 32, RED, 2.4, DASHDOT, "butt")
    return g


@icon("TechDraw_ExtensionHoleCircle")
def hole_circle():
    g = flat()
    paper(g)
    raw(g, '<circle cx="32" cy="32" r="18" fill="none" stroke="%s" stroke-width="2" stroke-dasharray="%s"/>' % (RED, DASHDOT))
    for i in range(6):
        a = math.radians(i * 60 - 90)
        raw(g, '<circle cx="%.1f" cy="%.1f" r="5" fill="%s" stroke="%s" stroke-width="2"/>'
            % (32 + 18 * math.cos(a), 32 + 18 * math.sin(a), YEL[0] if i == 0 else SOLID[0], OUT))
    return g


# ============================================================ cosméticos
@icon("actions/TechDraw_CosmeticVertex", "TechDraw_CosmeticVertex")
def cos_vertex():
    g = flat()
    paper(g)
    line(g, 12, 50, 52, 14, PAPER_LINE, 2.4)
    dot(g, 32, 32, 7)
    return g


@icon("actions/TechDraw_Midpoints", "TechDraw_Midpoints")
def midpoints():
    g = flat()
    paper(g)
    line(g, 10, 50, 54, 14, OUT, 3.2)
    dot(g, 32, 32, 5.5)
    return g


@icon("actions/TechDraw_Quadrants", "TechDraw_Quadrants")
def quadrants():
    g = flat()
    paper(g)
    raw(g, '<circle cx="32" cy="32" r="18" fill="none" stroke="%s" stroke-width="3"/>' % OUT)
    for x, y in ((32, 14), (50, 32), (32, 50), (14, 32)):
        dot(g, x, y, 4.5)
    return g


@icon("actions/TechDraw_CosmeticCircle", "TechDraw_CosmeticCircle", "TechDraw_ExtensionDrawCosmCircle")
def cos_circle():
    g = flat()
    paper(g)
    raw(g, '<circle cx="32" cy="32" r="18" fill="none" stroke="%s" stroke-width="3.2"/>' % NAVY)
    dot(g, 32, 32, 3.5)
    return g


@icon("TechDraw_ExtensionDrawCosmCircle3Points")
def cos_circle3():
    g = flat()
    paper(g)
    raw(g, '<circle cx="32" cy="32" r="18" fill="none" stroke="%s" stroke-width="3.2"/>' % NAVY)
    for a in (200, 300, 60):
        r = math.radians(a)
        dot(g, 32 + 18 * math.cos(r), 32 + 18 * math.sin(r), 4)
    return g


@icon("TechDraw_ExtensionDrawCosmArc")
def cos_arc():
    g = flat()
    paper(g)
    raw(g, '<path d="M 14 44 A 20 20 0 0 1 50 40" fill="none" stroke="%s" stroke-width="3.2"/>' % NAVY)
    dot(g, 32, 42, 3.5)
    dot(g, 14, 44, 4)
    dot(g, 50, 40, 4)
    return g


@icon("actions/TechDraw_CosmeticEraser", "TechDraw_CosmeticEraser")
def cos_eraser():
    g = flat()
    paper(g)
    line(g, 8, 46, 56, 46, PAPER_LINE, 2.4, "4,3")
    raw(g, '<path d="M 16 40 L 36 14 L 52 26 L 34 50 H 24 Z" fill="%s" stroke="%s" stroke-width="2.4" stroke-linejoin="round"/>' % (RED, OUT))
    raw(g, '<path d="M 16 40 L 26 28 L 40 40 L 34 50 H 24 Z" fill="#FFFFFF" stroke="%s" stroke-width="2.4" stroke-linejoin="round"/>' % OUT)
    return g


@icon("actions/TechDraw_AddOffsetVertex.svg", "TechDraw_CommandAddOffsetVertex")
def offset_vertex():
    g = flat()
    paper(g)
    dot(g, 16, 46, 4.5, SOLID[1])
    line(g, 18, 44, 40, 22, NAVY, 2.4, "3,2")
    tri(g, [(44, 18), (35, 22), (40, 27)])
    dot(g, 46, 16, 5)
    return g


@icon("TechDraw_ExtensionVertexAtIntersection", "TechDraw_CommandVertexCreationGroup")
def vertex_intersection():
    g = flat()
    paper(g)
    line(g, 10, 50, 54, 14, OUT, 3)
    line(g, 10, 14, 54, 50, OUT, 3)
    dot(g, 32, 32, 6)
    return g


@icon("TechDraw_ExtensionLineParallel")
def line_parallel():
    g = flat()
    paper(g)
    line(g, 10, 44, 42, 12, OUT, 3.2)
    line(g, 22, 54, 54, 22, NAVY, 3.2, "5,3")
    return g


@icon("TechDraw_ExtensionLinePerpendicular")
def line_perp():
    g = flat()
    paper(g)
    line(g, 8, 48, 56, 48, OUT, 3.2)
    line(g, 32, 48, 32, 10, NAVY, 3.2, "5,3")
    raw(g, '<path d="M 32 40 H 40 V 48" fill="none" stroke="%s" stroke-width="2"/>' % AMBER)
    return g


@icon("TechDraw_ExtensionExtendLine")
def extend_line():
    g = flat()
    paper(g)
    line(g, 10, 32, 38, 32, OUT, 3.4)
    line(g, 38, 32, 50, 32, NAVY, 3.4, "4,3")
    tri(g, [(58, 32), (48, 26), (48, 38)])
    return g


@icon("TechDraw_ExtensionShortenLine")
def shorten_line():
    g = flat()
    paper(g)
    line(g, 10, 32, 36, 32, OUT, 3.4)
    line(g, 40, 32, 56, 32, PAPER_LINE, 3.4, "4,3")
    tri(g, [(34, 32), (44, 26), (44, 38)])
    return g


@icon("TechDraw_ExtensionChangeLineAttributes")
def change_line_attr():
    g = flat()
    paper(g)
    line(g, 10, 18, 54, 18, OUT, 4.2)
    line(g, 10, 32, 54, 32, OUT, 2.6, "6,4")
    line(g, 10, 46, 40, 46, AMBER, 2.6, DASHDOT, "butt")
    badge(g, "pencil")
    return g


@icon("TechDraw_ExtensionSelectLineAttributes")
def select_line_attr():
    g = flat()
    paper(g)
    line(g, 10, 18, 54, 18, OUT, 4.2)
    line(g, 10, 32, 54, 32, OUT, 2.6, "6,4")
    line(g, 10, 46, 40, 46, AMBER, 2.6, DASHDOT, "butt")
    badge(g, "check")
    return g


# ============================================================ cotas: cadeia, cascata e formato
def _chain(orient, cascade=False):
    def f():
        g = flat()
        paper(g)
        if orient == "h":
            for x in (10, 30, 54):
                line(g, x, 54, x, 22, STEEL, 1.6)
            ys = (28, 38) if cascade else (28, 28)
            line(g, 12, ys[0], 28, ys[0], NAVY, 2.4)
            line(g, (12 if cascade else 32), ys[1], 52, ys[1], NAVY, 2.4)
            tri(g, [(10, ys[0]), (16, ys[0] - 3), (16, ys[0] + 3)])
            tri(g, [(30, ys[0]), (24, ys[0] - 3), (24, ys[0] + 3)])
            tri(g, [(54, ys[1]), (48, ys[1] - 3), (48, ys[1] + 3)])
            line(g, 8, 56, 56, 56, OUT, 3.2)
        elif orient == "v":
            for y in (10, 30, 54):
                line(g, 10, y, 42, y, STEEL, 1.6)
            xs = (36, 26) if cascade else (36, 36)
            line(g, xs[0], 12, xs[0], 28, NAVY, 2.4)
            line(g, xs[1], (12 if cascade else 32), xs[1], 52, NAVY, 2.4)
            tri(g, [(xs[0], 10), (xs[0] - 3, 16), (xs[0] + 3, 16)])
            tri(g, [(xs[0], 30), (xs[0] - 3, 24), (xs[0] + 3, 24)])
            tri(g, [(xs[1], 54), (xs[1] - 3, 48), (xs[1] + 3, 48)])
            line(g, 52, 8, 52, 56, OUT, 3.2)
        else:
            line(g, 8, 52, 50, 10, OUT, 3.2)
            off = (0, 8) if cascade else (0, 0)
            line(g, 16 + 0, 56 - 0, 32, 40, NAVY, 2.4)
            line(g, 34 + off[1] * 0, 38 - off[1] * 0, 54, 18, NAVY, 2.4)
            tri(g, [(14, 58), (16, 50), (21, 54)])
            tri(g, [(56, 16), (49, 18), (53, 23)])
            if cascade:
                line(g, 22, 58, 60, 20, AMBER, 2, "3,2")
        raw(g, '<rect x="20" y="10" width="14" height="7" rx="2" fill="%s"/>' % AMBER) if orient == "h" else None
        return g
    return f


for n, o, c in (("PosHorizChainDimension", "h", False), ("PosVertChainDimension", "v", False),
                ("PosObliqueChainDimension", "o", False), ("CascadeHorizDimension", "h", True),
                ("CascadeVertDimension", "v", True), ("CascadeObliqueDimension", "o", True)):
    ICONS["TechDraw_Extension" + n] = _chain(o, c)


def _fmt(label, sign=None, strike=False):
    def f():
        g = flat()
        _dim(g, "h")
        raw(g, '<rect x="10" y="34" width="44" height="18" rx="3" fill="#FFFFFF" stroke="%s" stroke-width="2"/>' % OUT)
        txt(g, 32, 48, label, 13, NAVY)
        if strike:
            line(g, 14, 50, 50, 36, RED, 3)
        if sign:
            g.badge(sign)
        return g
    return f


ICONS["TechDraw_ExtensionIncreaseDecimal"] = _fmt("0,00", "plus")
ICONS["TechDraw_ExtensionDecreaseDecimal"] = _fmt("0,0", "minus")
ICONS["TechDraw_ExtensionInsertDiameter"] = _fmt("Ø20")
ICONS["TechDraw_ExtensionInsertSquare"] = _fmt("□20")
ICONS["TechDraw_ExtensionInsertRepetition"] = _fmt("4×")
ICONS["TechDraw_ExtensionRemovePrefixChar"] = _fmt("Ø", strike=True)
ICONS["TechDraw_ExtensionCustomizeFormat"] = _fmt("%.2f")


@icon("TechDraw_ExtensionArcLengthAnnotation")
def arc_len_annot():
    g = flat()
    paper(g)
    raw(g, '<path d="M 12 48 A 24 24 0 0 1 52 48" fill="none" stroke="%s" stroke-width="3.2"/>' % OUT)
    raw(g, '<rect x="18" y="12" width="28" height="12" rx="3" fill="%s"/>' % AMBER)
    txt(g, 32, 22, "⌒ L", 9, "#FFFFFF")
    return g


@icon("TechDraw_ExtensionAreaAnnotation")
def area_annot():
    g = flat()
    paper(g)
    raw(g, '<path d="M 12 50 L 16 20 L 48 14 L 52 46 Z" fill="%s" fill-opacity="0.55" stroke="%s" stroke-width="2.6"/>' % (YEL[0], OUT))
    raw(g, '<rect x="18" y="26" width="28" height="13" rx="3" fill="#FFFFFF" stroke="%s" stroke-width="1.6"/>' % OUT)
    txt(g, 32, 36, "A=", 9, NAVY)
    return g


@icon("TechDraw_ExtensionPositionSectionView.svg", "TechDraw_ExtensionPositionSectionView")
def position_section():
    g = flat()
    sheet_landscape(g)
    raw(g, '<rect x="10" y="18" width="14" height="16" fill="%s" stroke="%s" stroke-width="2"/>' % (SOLID[0], OUT))
    raw(g, '<rect x="30" y="18" width="14" height="16" fill="%s" stroke="%s" stroke-width="2"/>' % (YEL[0], OUT))
    line(g, 25, 26, 29, 26, NAVY, 2.4)
    line(g, 8, 26, 47, 26, RED, 1.4, DASHDOT, "butt")
    return g


# ============================================================ roscas
def _thread(kind, side):
    def f():
        g = flat()
        paper(g)
        if side:
            if kind == "bolt":
                raw(g, '<rect x="14" y="10" width="36" height="44" fill="%s" stroke="%s" stroke-width="2.6"/>' % (SOLID[0], OUT))
                line(g, 19, 10, 19, 46, OUT, 1.6)
                line(g, 45, 10, 45, 46, OUT, 1.6)
            else:
                raw(g, '<rect x="8" y="10" width="48" height="44" fill="%s" stroke="%s" stroke-width="2.6"/>' % (SOLID[0], OUT))
                raw(g, '<rect x="20" y="10" width="24" height="34" fill="#FFFFFF" stroke="%s" stroke-width="2.4"/>' % OUT)
                line(g, 16, 10, 16, 44, OUT, 1.6)
                line(g, 48, 10, 48, 44, OUT, 1.6)
            line(g, 32, 6, 32, 58, RED, 2, DASHDOT, "butt")
        else:
            raw(g, '<circle cx="32" cy="32" r="20" fill="%s" stroke="%s" stroke-width="2.6"/>' % (SOLID[0] if kind == "bolt" else "#FFFFFF", OUT))
            raw(g, '<path d="M 32 %d A %d %d 0 1 1 %d 32" fill="none" stroke="%s" stroke-width="1.8"/>'
                % (32 - (15 if kind == "bolt" else 25), 15 if kind == "bolt" else 25, 15 if kind == "bolt" else 25,
                   32 - (15 if kind == "bolt" else 25), OUT))
            line(g, 32, 6, 32, 58, RED, 2, DASHDOT, "butt")
            line(g, 6, 32, 58, 32, RED, 2, DASHDOT, "butt")
        return g
    return f


for n, k, s in (("ThreadBoltSide", "bolt", True), ("ThreadBoltBottom", "bolt", False),
                ("ThreadHoleSide", "hole", True), ("ThreadHoleBottom", "hole", False)):
    ICONS["TechDraw_Extension" + n] = _thread(k, s)


@icon("actions/TechDraw_RichTextAnnotation", "TechDraw_RichTextAnnotation")
def rich_text():
    g = flat()
    paper(g)
    txt(g, 20, 34, "A", 24, NAVY)
    raw(g, '<path d="M 34 18 h 18 M 34 28 h 18" stroke="%s" stroke-width="3.4" stroke-linecap="round"/>' % NAVY)
    raw(g, '<path d="M 12 44 h 40" stroke="%s" stroke-width="3" stroke-linecap="round"/>' % AMBER)
    raw(g, '<path d="M 12 52 h 28" stroke="%s" stroke-width="3" stroke-linecap="round" stroke-dasharray="1,0"/>' % STEEL)
    return g
