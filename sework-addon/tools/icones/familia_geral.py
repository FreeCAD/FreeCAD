# SPDX-License-Identifier: LGPL-2.1-or-later
# Ícones comuns (arquivo, edição, vistas), Desenho técnico, Montagem e Planilha.
import math
from nucleo import (Icon, SOLID, COPY, BASE, CUT, YEL, OUT, NAVY, AMBER, AMBER_FILL, STEEL, RED, GREEN,
                    SW, SW_GEO, PAPER, PAPER_LINE)

ICONS = {}


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


def raw(g, s):
    g.items.append(("raw", s))


def page(g, x=10, y=4, w=40, h=54, fold=True, title=False):
    if fold:
        raw(g, '<path d="M %d %d h %d l 10 10 v %d h -%d z" fill="%s" stroke="%s" stroke-width="%.1f" '
               'stroke-linejoin="round"/>' % (x, y, w - 10, h - 10, w, PAPER, OUT, SW))
        raw(g, '<path d="M %d %d v 10 h 10 z" fill="%s" stroke="%s" stroke-width="%.1f" stroke-linejoin="round"/>'
            % (x + w - 10, y, YEL[0], OUT, SW))
    else:
        raw(g, '<rect x="%d" y="%d" width="%d" height="%d" rx="1.5" fill="%s" stroke="%s" stroke-width="%.1f"/>'
            % (x, y, w, h, PAPER, OUT, SW))
    if title:
        raw(g, '<rect x="%d" y="%d" width="%d" height="%d" fill="%s" stroke="%s" stroke-width="1.6"/>'
            % (x + w - 20, y + h - 11, 17, 8, YEL[0], OUT))


def sheet_landscape(g, x=4, y=10, w=56, h=42):
    raw(g, '<rect x="%d" y="%d" width="%d" height="%d" rx="1.5" fill="%s" stroke="%s" stroke-width="%.1f"/>'
        % (x, y, w, h, PAPER, OUT, SW))
    raw(g, '<rect x="%d" y="%d" width="%d" height="%d" fill="none" stroke="%s" stroke-width="1.2"/>'
        % (x + 3, y + 3, w - 6, h - 6, PAPER_LINE))
    raw(g, '<rect x="%d" y="%d" width="18" height="8" fill="%s" stroke="%s" stroke-width="1.4"/>'
        % (x + w - 21, y + h - 11, YEL[0], OUT))


def mini_part(g, x, y, s=1.0):
    """Pequena peça isométrica (azul) desenhada em coordenadas de tela."""
    c, k = 0.866 * 3.2 * s, 0.5 * 3.2 * s
    def P(a, b, z):
        return (x + (a - b) * c, y + (a + b) * k - z * 3.2 * s)
    def poly(ps, fill):
        raw(g, '<polygon points="%s" fill="%s" stroke="%s" stroke-width="1.8" stroke-linejoin="round"/>'
            % (" ".join("%.1f,%.1f" % P(*q) for q in ps), fill, OUT))
    X1, Y1, Z1 = 4, 3, 3
    poly([(0, Y1, 0), (X1, Y1, 0), (X1, Y1, Z1), (0, Y1, Z1)], SOLID[1])
    poly([(X1, 0, 0), (X1, Y1, 0), (X1, Y1, Z1), (X1, 0, Z1)], SOLID[2])
    poly([(0, 0, Z1), (X1, 0, Z1), (X1, Y1, Z1), (0, Y1, Z1)], SOLID[0])


def arrow_path(g, d, color=NAVY, sw=SW_GEO):
    raw(g, '<path d="%s" fill="none" stroke="%s" stroke-width="%.1f" stroke-linecap="round" stroke-linejoin="round"/>'
        % (d, color, sw))


def tri(g, pts, color=NAVY):
    raw(g, '<polygon points="%s" fill="%s"/>' % (" ".join("%.1f,%.1f" % q for q in pts), color))


def txt(g, x, y, s, size=14, color=NAVY, weight="800"):
    raw(g, '<text x="%.1f" y="%.1f" font-family="Segoe UI, Arial, sans-serif" font-size="%d" font-weight="%s" '
           'fill="%s" text-anchor="middle">%s</text>' % (x, y, size, weight, color, s))


# ============================================================ arquivo
@icon("document-new", "Std_New")
def doc_new():
    g = flat()
    page(g)
    g.badge("plus")
    return g


@icon("document-open", "Open", "Std_Open")
def doc_open():
    g = flat()
    raw(g, '<path d="M 6 16 h 16 l 5 5 h 25 v 33 h -46 z" fill="%s" stroke="%s" stroke-width="%.1f" '
           'stroke-linejoin="round"/>' % (SOLID[1], OUT, SW))
    raw(g, '<path d="M 6 54 l 8 -24 h 46 l -8 24 z" fill="%s" stroke="%s" stroke-width="%.1f" stroke-linejoin="round"/>'
        % (YEL[0], OUT, SW))
    return g


def _disk(g):
    raw(g, '<path d="M 8 8 h 40 l 8 8 v 40 h -48 z" fill="%s" stroke="%s" stroke-width="%.1f" stroke-linejoin="round"/>'
        % (SOLID[1], OUT, SW))
    raw(g, '<rect x="17" y="8" width="24" height="15" fill="%s" stroke="%s" stroke-width="1.8"/>' % (PAPER, OUT))
    raw(g, '<rect x="33" y="11" width="5" height="9" fill="%s"/>' % NAVY)
    raw(g, '<rect x="15" y="34" width="34" height="22" rx="1" fill="%s" stroke="%s" stroke-width="1.8"/>' % (YEL[0], OUT))


@icon("document-save", "Std_Save")
def doc_save():
    g = flat()
    _disk(g)
    return g


@icon("document-save-as", "Std_SaveAs")
def doc_saveas():
    g = flat()
    _disk(g)
    g.badge("pencil")
    return g


@icon("document-print", "Std_Print", "actions/TechDraw_PrintAll")
def doc_print():
    g = flat()
    raw(g, '<rect x="16" y="6" width="32" height="18" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (PAPER, OUT, SW))
    raw(g, '<rect x="6" y="22" width="52" height="24" rx="4" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (SOLID[1], OUT, SW))
    raw(g, '<rect x="16" y="38" width="32" height="20" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (PAPER, OUT, SW))
    raw(g, '<path d="M 21 45 h 22 M 21 51 h 16" stroke="%s" stroke-width="2.4" stroke-linecap="round"/>' % PAPER_LINE)
    raw(g, '<circle cx="50" cy="30" r="2.6" fill="%s"/>' % YEL[0])
    return g


def _page_text(g, s, color=NAVY):
    page(g)
    raw(g, '<rect x="13" y="30" width="34" height="16" rx="3" fill="%s"/>' % color)
    txt(g, 30, 43, s, 13, "#FFFFFF")


@icon("Std_PrintPdf")
def pdf():
    g = flat()
    _page_text(g, "PDF", RED)
    return g


@icon("actions/TechDraw_ExportPageDXF")
def dxf():
    g = flat()
    _page_text(g, "DXF")
    return g


@icon("actions/TechDraw_ExportPageSVG")
def svgx():
    g = flat()
    _page_text(g, "SVG")
    return g


@icon("Std_Import")
def imp():
    g = flat()
    page(g, x=18)
    arrow_path(g, "M 2 34 H 30")
    tri(g, [(34, 34), (25, 27), (25, 41)])
    return g


@icon("Std_Export")
def exp():
    g = flat()
    page(g, x=4)
    arrow_path(g, "M 28 34 H 56")
    tri(g, [(62, 34), (53, 27), (53, 41)])
    return g


@icon("document-properties", "Std_ProjectInfo")
def props():
    g = flat()
    page(g)
    raw(g, '<path d="M 18 24 h 22 M 18 32 h 22 M 18 40 h 14" stroke="%s" stroke-width="3" stroke-linecap="round"/>' % STEEL)
    g.badge("pencil")
    return g


# ============================================================ edição
@icon("edit-undo", "Std_Undo")
def undo():
    g = flat()
    arrow_path(g, "M 18 22 H 38 A 14 14 0 0 1 38 50 H 22", sw=5)
    tri(g, [(8, 22), (20, 12), (20, 32)])
    return g


@icon("edit-redo", "Std_Redo")
def redo():
    g = flat()
    arrow_path(g, "M 46 22 H 26 A 14 14 0 0 0 26 50 H 42", sw=5)
    tri(g, [(56, 22), (44, 12), (44, 32)])
    return g


@icon("view-refresh", "Std_Refresh", "actions/TechDraw_RedrawPage")
def refresh():
    g = flat()
    arrow_path(g, "M 50 26 A 19 19 0 1 0 51 40", color=NAVY, sw=5)
    tri(g, [(54, 14), (56, 31), (40, 27)])
    raw(g, '<circle cx="32" cy="32" r="5" fill="%s" stroke="%s" stroke-width="1.6"/>' % (YEL[0], OUT))
    return g


@icon("edit-cut", "Std_Cut")
def cut():
    g = flat()
    raw(g, '<path d="M 22 8 L 38 40 M 42 8 L 26 40" stroke="%s" stroke-width="4" stroke-linecap="round"/>' % NAVY)
    raw(g, '<circle cx="22" cy="48" r="8" fill="%s" stroke="%s" stroke-width="3"/>' % (YEL[0], OUT))
    raw(g, '<circle cx="42" cy="48" r="8" fill="%s" stroke="%s" stroke-width="3"/>' % (YEL[0], OUT))
    return g


@icon("edit-copy", "Std_Copy")
def copy():
    g = flat()
    raw(g, '<rect x="8" y="6" width="32" height="40" rx="2" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (COPY[0], OUT, SW))
    raw(g, '<rect x="24" y="18" width="32" height="40" rx="2" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (PAPER, OUT, SW))
    raw(g, '<path d="M 30 30 h 20 M 30 38 h 20 M 30 46 h 12" stroke="%s" stroke-width="2.6" stroke-linecap="round"/>' % STEEL)
    return g


@icon("edit-paste", "Std_Paste")
def paste():
    g = flat()
    raw(g, '<rect x="8" y="10" width="38" height="48" rx="3" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (SOLID[1], OUT, SW))
    raw(g, '<rect x="18" y="5" width="18" height="10" rx="2" fill="%s" stroke="%s" stroke-width="2"/>' % (YEL[0], OUT))
    raw(g, '<rect x="26" y="24" width="30" height="36" rx="2" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (PAPER, OUT, SW))
    raw(g, '<path d="M 32 34 h 18 M 32 42 h 18 M 32 50 h 10" stroke="%s" stroke-width="2.6" stroke-linecap="round"/>' % STEEL)
    return g


@icon("edit-delete", "Std_Delete")
def delete():
    g = flat()
    raw(g, '<path d="M 14 18 h 36 l -4 40 h -28 z" fill="%s" stroke="%s" stroke-width="%.1f" stroke-linejoin="round"/>'
        % (CUT[1], OUT, SW))
    raw(g, '<rect x="10" y="10" width="44" height="8" rx="2" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (CUT[2], OUT, SW))
    raw(g, '<path d="M 26 26 v 24 M 38 26 v 24" stroke="#FFFFFF" stroke-width="3" stroke-linecap="round"/>')
    return g


@icon("edit-edit", "Std_Edit")
def edit():
    g = flat()
    raw(g, '<path d="M 12 52 l 4 -14 l 28 -28 l 10 10 l -28 28 z" fill="%s" stroke="%s" stroke-width="%.1f" '
           'stroke-linejoin="round"/>' % (YEL[0], OUT, SW))
    raw(g, '<path d="M 38 16 l 10 10" stroke="%s" stroke-width="%.1f"/>' % (OUT, SW))
    raw(g, '<path d="M 12 52 l 4 -14 l 10 10 z" fill="%s"/>' % NAVY)
    return g


@icon("Std_Placement")
def placement():
    g = Icon()
    g.box(0, 6, 0, 6, 0, 6, pal=COPY)
    g.arrow3((3, 3, 6), (3, 3, 12), color=NAVY, sw=SW_GEO)
    g.arrow3((6, 3, 3), (12, 3, 3), color=RED, sw=SW_GEO)
    g.arrow3((3, 6, 3), (3, 12, 3), color=GREEN, sw=SW_GEO)
    return g


# ============================================================ vistas
def _view_cube(face):
    def f():
        g = Icon()
        top = YEL[0] if face == "top" else SOLID[0]
        left = YEL[0] if face == "front" else SOLID[1]
        right = YEL[0] if face == "right" else SOLID[2]
        if face in ("rear", "left", "bottom"):
            # face oculta: desenhada atrás, com faces visíveis translúcidas
            hid = {"rear": [(0, 0, 0), (8, 0, 0), (8, 0, 8), (0, 0, 8)],
                   "left": [(0, 0, 0), (0, 8, 0), (0, 8, 8), (0, 0, 8)],
                   "bottom": [(0, 0, 0), (8, 0, 0), (8, 8, 0), (0, 8, 0)]}[face]
            g.poly(hid, YEL[0], stroke=AMBER, sw=SW)
            g.box(0, 8, 0, 8, 0, 8, opacity=0.35)
        else:
            g.box(0, 8, 0, 8, 0, 8, pal=(top, left, right))
        return g
    return f


for face, names in (("front", ("view-front", "Std_ViewFront")), ("top", ("view-top", "Std_ViewTop")),
                    ("right", ("view-right", "Std_ViewRight")), ("rear", ("view-rear", "Std_ViewRear")),
                    ("bottom", ("view-bottom", "Std_ViewBottom")), ("left", ("view-left", "Std_ViewLeft"))):
    for n in names:
        ICONS[n] = _view_cube(face)


@icon("view-isometric", "Std_ViewIsometric", "view-axonometric")
def iso():
    g = Icon()
    g.box(0, 8, 0, 8, 0, 8)
    g.line((8, 8, 0), (8, 8, -3), color=NAVY, sw=SW)
    g.line((0, 8, 0), (-2, 10, 0), color=NAVY, sw=SW)
    g.line((8, 0, 0), (10, -2, 0), color=NAVY, sw=SW)
    return g


@icon("zoom-all", "Std_ViewFitAll")
def zoomall():
    g = flat()
    raw(g, '<path d="M 6 18 V 6 H 18 M 46 6 H 58 V 18 M 58 46 V 58 H 46 M 18 58 H 6 V 46" fill="none" stroke="%s" '
           'stroke-width="4" stroke-linecap="round" stroke-linejoin="round"/>' % AMBER)
    mini_part(g, 30, 22, 1.6)
    return g


@icon("zoom-selection", "Std_ViewFitSelection")
def zoomsel():
    g = flat()
    raw(g, '<circle cx="26" cy="26" r="17" fill="%s" stroke="%s" stroke-width="4"/>' % (PAPER, NAVY))
    raw(g, '<path d="M 38 38 L 56 56" stroke="%s" stroke-width="7" stroke-linecap="round"/>' % NAVY)
    mini_part(g, 26, 19, 1.0)
    return g


@icon("align-to-selection", "Std_AlignToSelection")
def align():
    g = Icon()
    g.box(0, 8, 0, 8, 0, 5, pal=(YEL[0], SOLID[1], SOLID[2]))
    g.arrow3((4, 4, 12), (4, 4, 5.6))
    return g


@icon("DrawStyleAsIs", "Std_DrawStyle")
def drawstyle():
    g = Icon()
    g.box(0, 8, 0, 8, 0, 8, pal=("none", "none", "none"))
    g.poly([(0, 8, 0), (8, 8, 0), (8, 8, 8), (0, 8, 8)], SOLID[1])
    g.poly([(0, 0, 8), (8, 0, 8), (8, 8, 8), (0, 8, 8)], SOLID[0])
    return g


@icon("umf-measurement", "Std_Measure", "Std_MeasureDistance")
def measure():
    g = flat()
    raw(g, '<rect x="6" y="14" width="52" height="12" rx="2" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (YEL[0], OUT, SW))
    for i in range(10, 56, 6):
        raw(g, '<path d="M %d 14 v %d" stroke="%s" stroke-width="1.8"/>' % (i, 6 if (i - 10) % 12 else 9, OUT))
    raw(g, '<path d="M 10 26 v 30 l 8 -8 v -22 z" fill="%s" stroke="%s" stroke-width="%.1f" stroke-linejoin="round"/>'
        % (SOLID[1], OUT, SW))
    raw(g, '<path d="M 38 26 v 22 l 8 8 v -30 z" fill="%s" stroke="%s" stroke-width="%.1f" stroke-linejoin="round"/>'
        % (SOLID[2], OUT, SW))
    return g


@icon("preferences-system", "Std_DlgPreferences")
def prefs():
    g = flat()
    pts = []
    for i in range(16):
        a = math.radians(i * 22.5)
        r = 26 if i % 2 == 0 else 20
        pts.append((32 + r * math.cos(a), 32 + r * math.sin(a)))
    raw(g, '<polygon points="%s" fill="%s" stroke="%s" stroke-width="%.1f" stroke-linejoin="round"/>'
        % (" ".join("%.1f,%.1f" % q for q in pts), SOLID[1], OUT, SW))
    raw(g, '<circle cx="32" cy="32" r="9" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (YEL[0], OUT, SW))
    return g


@icon("accessories-text-editor", "Std_DlgMacroExecute")
def macros():
    g = flat()
    page(g)
    raw(g, '<path d="M 18 26 l -6 6 l 6 6 M 36 26 l 6 6 l -6 6" fill="none" stroke="%s" stroke-width="3.4" '
           'stroke-linecap="round" stroke-linejoin="round"/>' % NAVY)
    raw(g, '<path d="M 30 24 l -6 18" stroke="%s" stroke-width="3.4" stroke-linecap="round"/>' % AMBER)
    return g


@icon("Std_ToggleClipPlane")
def clip():
    g = Icon()
    g.box(0, 8, 0, 8, 0, 4)
    g.box(0, 8, 0, 8, 4, 8, pal=("none", "none", "none"), dash="3,3")
    g.rect3(-2, 10, -2, 10, 4, YEL[0], stroke=AMBER, sw=SW, opacity=0.55)
    return g


@icon("clear-selection", "Part_SelectFilter")
def selfilter():
    g = flat()
    raw(g, '<path d="M 6 8 h 52 l -20 24 v 20 l -12 6 v -26 z" fill="%s" stroke="%s" stroke-width="%.1f" '
           'stroke-linejoin="round"/>' % (YEL[0], OUT, SW))
    return g


@icon("folder", "Std_Group")
def folder():
    g = flat()
    raw(g, '<path d="M 6 14 h 18 l 5 6 h 29 v 34 h -52 z" fill="%s" stroke="%s" stroke-width="%.1f" '
           'stroke-linejoin="round"/>' % (YEL[0], OUT, SW))
    raw(g, '<path d="M 6 26 h 52" stroke="%s" stroke-width="1.8"/>' % OUT)
    return g


@icon("Geofeaturegroup", "Std_Part")
def stdpart():
    g = Icon()
    g.box(-1, 10, -1, 10, 0, 9, pal=(YEL[0], YEL[1], YEL[2]), opacity=0.25, dash="4,3")
    g.box(1.5, 7.5, 1.5, 7.5, 0, 5)
    return g


@icon("VarSet", "Std_VarSet")
def varset():
    g = flat()
    raw(g, '<rect x="6" y="8" width="52" height="48" rx="4" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (PAPER, OUT, SW))
    raw(g, '<rect x="6" y="8" width="52" height="12" rx="4" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (SOLID[1], OUT, SW))
    txt(g, 22, 44, "x", 22, NAVY)
    txt(g, 42, 44, "=", 22, AMBER)
    return g


# ============================================================ desenho técnico
@icon("actions/TechDraw_PageDefault", "actions/TechDraw_PageTemplate")
def td_page():
    g = flat()
    sheet_landscape(g)
    g.badge("plus")
    return g


@icon("actions/TechDraw_FillTemplateFields.svg")
def td_fill():
    g = flat()
    sheet_landscape(g)
    g.badge("pencil")
    return g


@icon("actions/TechDraw_ToggleFrame")
def td_frame():
    g = flat()
    sheet_landscape(g)
    raw(g, '<rect x="14" y="18" width="24" height="20" fill="none" stroke="%s" stroke-width="2" stroke-dasharray="4,3"/>' % NAVY)
    mini_part(g, 26, 26, 0.9)
    return g


@icon("actions/TechDraw_View")
def td_view():
    g = flat()
    sheet_landscape(g)
    mini_part(g, 28, 24, 1.2)
    return g


@icon("actions/TechDraw_ActiveView")
def td_active():
    g = flat()
    raw(g, '<rect x="6" y="8" width="52" height="36" rx="3" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (COPY[0], OUT, SW))
    raw(g, '<path d="M 24 44 v 8 h 16 v -8 M 18 54 h 28" fill="none" stroke="%s" stroke-width="%.1f"/>' % (OUT, SW))
    mini_part(g, 30, 22, 1.2)
    return g


@icon("actions/TechDraw_DraftView", "actions/TechDraw_ClipGroup")
def td_draft():
    g = flat()
    sheet_landscape(g)
    raw(g, '<rect x="12" y="16" width="26" height="22" fill="none" stroke="%s" stroke-width="2.6"/>' % NAVY)
    raw(g, '<circle cx="25" cy="27" r="6" fill="none" stroke="%s" stroke-width="2.6"/>' % AMBER)
    return g


@icon("actions/TechDraw_DetailView")
def td_detail():
    g = flat()
    sheet_landscape(g)
    raw(g, '<circle cx="24" cy="30" r="12" fill="%s" stroke="%s" stroke-width="3"/>' % (COPY[0], NAVY))
    raw(g, '<path d="M 18 34 h 12 v -8" fill="none" stroke="%s" stroke-width="2.6"/>' % OUT)
    raw(g, '<path d="M 33 39 l 8 8" stroke="%s" stroke-width="5" stroke-linecap="round"/>' % NAVY)
    return g


@icon("actions/TechDraw_BrokenView")
def td_broken():
    g = flat()
    sheet_landscape(g)
    raw(g, '<path d="M 10 24 h 14 l 3 -4 l 3 8 l 3 -4 h 1 M 10 34 h 14 l 3 -4 l 3 8 l 3 -4 h 1" fill="none" '
           'stroke="%s" stroke-width="2.4"/>' % NAVY)
    raw(g, '<path d="M 36 24 h 12 M 36 34 h 12" stroke="%s" stroke-width="2.4"/>' % NAVY)
    return g


def _dim(g, kind):
    raw(g, '<rect x="4" y="4" width="56" height="56" rx="4" fill="%s" stroke="%s" stroke-width="1.4"/>' % (PAPER, PAPER_LINE))
    k = NAVY
    if kind in ("h", "length", "chain", "coord", "extent_h", "chamfer_h"):
        raw(g, '<path d="M 12 46 v -26 M 52 46 v -26" stroke="%s" stroke-width="1.8"/>' % STEEL)
        raw(g, '<path d="M 14 28 H 50" stroke="%s" stroke-width="2.6"/>' % k)
        tri(g, [(12, 28), (20, 24), (20, 32)])
        tri(g, [(52, 28), (44, 24), (44, 32)])
        raw(g, '<rect x="24" y="12" width="16" height="9" rx="2" fill="%s"/>' % AMBER)
        if kind == "chain":
            raw(g, '<path d="M 32 46 v -22" stroke="%s" stroke-width="1.8"/>' % STEEL)
        if kind == "coord":
            raw(g, '<path d="M 14 40 H 40" stroke="%s" stroke-width="2.6"/>' % k)
            tri(g, [(42, 40), (34, 36), (34, 44)])
        raw(g, '<path d="M 12 50 H 52" stroke="%s" stroke-width="3.4"/>' % OUT)
    elif kind in ("v", "extent_v", "chamfer_v"):
        raw(g, '<path d="M 46 12 h -26 M 46 52 h -26" stroke="%s" stroke-width="1.8"/>' % STEEL)
        raw(g, '<path d="M 28 14 V 50" stroke="%s" stroke-width="2.6"/>' % k)
        tri(g, [(28, 12), (24, 20), (32, 20)])
        tri(g, [(28, 52), (24, 44), (32, 44)])
        raw(g, '<rect x="12" y="26" width="9" height="12" rx="2" fill="%s"/>' % AMBER)
        raw(g, '<path d="M 50 12 V 52" stroke="%s" stroke-width="3.4"/>' % OUT)
    elif kind == "radius":
        raw(g, '<path d="M 50 14 A 36 36 0 0 0 14 50" fill="none" stroke="%s" stroke-width="3.4"/>' % OUT)
        raw(g, '<path d="M 50 50 L 25 25" stroke="%s" stroke-width="2.6"/>' % k)
        tri(g, [(22, 22), (31, 25), (25, 31)])
        txt(g, 44, 34, "R", 14, AMBER)
    elif kind == "diameter":
        raw(g, '<circle cx="32" cy="32" r="20" fill="none" stroke="%s" stroke-width="3.4"/>' % OUT)
        raw(g, '<path d="M 18 46 L 46 18" stroke="%s" stroke-width="2.6"/>' % k)
        tri(g, [(16, 48), (19, 39), (25, 45)])
        tri(g, [(48, 16), (39, 19), (45, 25)])
    elif kind in ("angle", "3pt"):
        raw(g, '<path d="M 10 52 H 56 M 10 52 L 40 12" fill="none" stroke="%s" stroke-width="3.4"/>' % OUT)
        raw(g, '<path d="M 44 52 A 34 34 0 0 0 30 25" fill="none" stroke="%s" stroke-width="2.6"/>' % k)
        if kind == "3pt":
            for q in ((10, 52), (56, 52), (40, 12)):
                g.dot2(q, 4)
    elif kind == "area":
        raw(g, '<path d="M 12 50 L 18 16 L 50 12 L 54 46 Z" fill="%s" fill-opacity="0.55" stroke="%s" stroke-width="3"/>'
            % (YEL[0], OUT))
        txt(g, 33, 38, "A", 16, NAVY)
    elif kind == "arc":
        raw(g, '<path d="M 12 50 A 30 30 0 0 1 52 50" fill="none" stroke="%s" stroke-width="3.4"/>' % OUT)
        raw(g, '<path d="M 8 44 A 34 34 0 0 1 56 44" fill="none" stroke="%s" stroke-width="2.4"/>' % k)
        raw(g, '<rect x="24" y="8" width="16" height="9" rx="2" fill="%s"/>' % AMBER)
    elif kind == "oblique":
        raw(g, '<path d="M 10 46 L 46 14" stroke="%s" stroke-width="3.4"/>' % OUT)
        raw(g, '<path d="M 18 54 L 54 22" stroke="%s" stroke-width="2.4"/>' % k)
        tri(g, [(16, 56), (19, 47), (24, 52)])
        tri(g, [(56, 20), (48, 23), (53, 28)])


def _dimicon(kind):
    def f():
        g = flat()
        _dim(g, kind)
        return g
    return f


for kind, names in (
        ("h", ("TechDraw_HorizontalDimension", "TechDraw_Dimension", "TechDraw_CompDimensionTools")),
        ("v", ("TechDraw_VerticalDimension",)),
        ("oblique", ("TechDraw_LengthDimension", "actions/TechDraw_AxoLengthDimension.svg")),
        ("radius", ("TechDraw_RadiusDimension",)),
        ("diameter", ("TechDraw_DiameterDimension",)),
        ("angle", ("TechDraw_AngleDimension",)),
        ("3pt", ("TechDraw_3PtAngleDimension",)),
        ("area", ("TechDraw_AreaDimension",)),
        ("extent_h", ("TechDraw_HorizontalExtentDimension",)),
        ("extent_v", ("TechDraw_VerticalExtentDimension",)),
        ("chain", ("TechDraw_ExtensionCreateHorizChainDimension", "TechDraw_ExtensionCreateObliqueChainDimension")),
        ("v", ("TechDraw_ExtensionCreateVertChainDimension", "TechDraw_ExtensionCreateVertCoordDimension",
               "TechDraw_ExtensionCreateVertChamferDimension")),
        ("coord", ("TechDraw_ExtensionCreateHorizCoordDimension", "TechDraw_ExtensionCreateObliqueCoordDimension",
                   "TechDraw_ExtensionCreateHorizChamferDimension")),
        ("arc", ("TechDraw_ExtensionCreateLengthArc",))):
    for n in names:
        ICONS[n] = _dimicon(kind)


@icon("TechDraw_DimensionRepair")
def td_repair():
    g = flat()
    _dim(g, "h")
    g.badge("check")
    return g


@icon("TechDraw_ExtensionLockUnlockView")
def td_lock():
    g = flat()
    sheet_landscape(g)
    raw(g, '<rect x="20" y="28" width="20" height="14" rx="2" fill="%s" stroke="%s" stroke-width="2"/>' % (YEL[0], OUT))
    raw(g, '<path d="M 24 28 v -5 a 6 6 0 0 1 12 0 v 5" fill="none" stroke="%s" stroke-width="3"/>' % OUT)
    return g


@icon("TechDraw_Balloon")
def td_balloon():
    g = flat()
    raw(g, '<path d="M 12 54 L 30 32" stroke="%s" stroke-width="2.6"/>' % NAVY)
    g.dot2((12, 54), 4)
    raw(g, '<circle cx="38" cy="24" r="16" fill="%s" stroke="%s" stroke-width="3"/>' % (PAPER, NAVY))
    txt(g, 38, 31, "1", 18, NAVY)
    return g


@icon("actions/TechDraw_Annotation")
def td_text():
    g = flat()
    raw(g, '<rect x="6" y="8" width="52" height="48" rx="3" fill="%s" stroke="%s" stroke-width="1.6"/>' % (PAPER, PAPER_LINE))
    txt(g, 24, 42, "A", 30, NAVY)
    raw(g, '<path d="M 40 22 h 12 M 40 32 h 12 M 40 42 h 8" stroke="%s" stroke-width="3" stroke-linecap="round"/>' % AMBER)
    return g


@icon("actions/TechDraw_LeaderLine", "actions/TechDraw_Line2Points")
def td_leader():
    g = flat()
    raw(g, '<path d="M 10 52 L 34 22 H 56" fill="none" stroke="%s" stroke-width="3" stroke-linejoin="round"/>' % NAVY)
    tri(g, [(8, 55), (11, 44), (18, 50)])
    raw(g, '<path d="M 38 16 h 16" stroke="%s" stroke-width="3" stroke-linecap="round"/>' % AMBER)
    return g


@icon("actions/TechDraw_WeldSymbol")
def td_weld():
    g = flat()
    raw(g, '<path d="M 8 52 L 26 30 H 58" fill="none" stroke="%s" stroke-width="3" stroke-linejoin="round"/>' % NAVY)
    tri(g, [(6, 55), (9, 44), (16, 50)])
    raw(g, '<polygon points="34,30 46,30 34,18" fill="%s" stroke="%s" stroke-width="2"/>' % (YEL[0], OUT))
    return g


@icon("actions/TechDraw_SurfaceFinishSymbols")
def td_rough():
    g = flat()
    raw(g, '<path d="M 10 32 L 20 50 L 40 12 H 58" fill="none" stroke="%s" stroke-width="3.4" stroke-linejoin="round"/>' % NAVY)
    raw(g, '<rect x="40" y="18" width="16" height="8" rx="2" fill="%s"/>' % AMBER)
    return g


@icon("actions/TechDraw_HoleShaftFit.svg")
def td_fit():
    g = flat()
    raw(g, '<circle cx="32" cy="32" r="22" fill="%s" stroke="%s" stroke-width="3"/>' % (BASE[1], OUT))
    raw(g, '<circle cx="32" cy="32" r="13" fill="%s" stroke="%s" stroke-width="3"/>' % (SOLID[1], OUT))
    txt(g, 32, 38, "H7", 14, "#FFFFFF")
    return g


@icon("actions/TechDraw_Hatch", "actions/TechDraw_GeometricHatch")
def td_hatch():
    g = flat()
    raw(g, '<rect x="8" y="8" width="48" height="48" rx="2" fill="%s" stroke="%s" stroke-width="3"/>' % (PAPER, OUT))
    for i in range(-40, 50, 9):
        x1, y1, x2, y2 = 8 + i, 56, 8 + i + 48, 8
        # recorta à mão no quadrado
        xa, ya = max(8, x1), 56 - (max(8, x1) - x1)
        xb, yb = min(56, x2), 8 + (x2 - min(56, x2))
        if xa < xb:
            raw(g, '<path d="M %.1f %.1f L %.1f %.1f" stroke="%s" stroke-width="2"/>' % (xa, ya, xb, yb, AMBER))
    return g


@icon("actions/TechDraw_DecorateLine")
def td_decorate():
    g = flat()
    raw(g, '<path d="M 8 18 H 56" stroke="%s" stroke-width="4"/>' % NAVY)
    raw(g, '<path d="M 8 32 H 56" stroke="%s" stroke-width="3" stroke-dasharray="8,5"/>' % NAVY)
    raw(g, '<path d="M 8 46 H 56" stroke="%s" stroke-width="2.6" stroke-dasharray="14,4,3,4"/>' % AMBER)
    return g


@icon("actions/TechDraw_ShowAll")
def td_showall():
    g = flat()
    raw(g, '<path d="M 4 32 Q 32 6 60 32 Q 32 58 4 32 Z" fill="%s" stroke="%s" stroke-width="3"/>' % (PAPER, NAVY))
    raw(g, '<circle cx="32" cy="32" r="10" fill="%s" stroke="%s" stroke-width="2.4"/>' % (YEL[0], OUT))
    raw(g, '<circle cx="32" cy="32" r="4" fill="%s"/>' % OUT)
    return g


# ============================================================ montagem
def _asm(g, symbol):
    g.box(0, 6, 0, 6, 0, 5)
    g.box(7, 13, 0, 6, 0, 5, pal=COPY)
    symbol(g)


def _asm_icon(sym_fn):
    def f():
        g = Icon()
        _asm(g, sym_fn)
        return g
    return f


def _sym_badge(kind):
    def s(g):
        g.badge(kind)
    return s


@icon("Geoassembly", "Assembly_CreateAssembly")
def asm_new():
    g = Icon()
    g.box(0, 6, 0, 6, 0, 5)
    g.box(4, 10, 4, 10, 5, 9, pal=COPY)
    return g


@icon("Assembly_InsertLink", "Assembly_Insert")
def asm_insert():
    g = Icon()
    g.box(0, 7, 0, 7, 0, 6, pal=COPY)
    g.badge("plus")
    return g


@icon("Assembly_SolveAssembly")
def asm_solve():
    g = Icon()
    _asm(g, lambda g: None)
    g.badge("check")
    return g


@icon("Assembly_ExplodedView")
def asm_exploded():
    g = Icon()
    g.box(0, 6, 0, 6, 0, 4)
    g.box(0, 6, 0, 6, 8, 12, pal=COPY)
    g.arrow3((3, 3, 4.5), (3, 3, 7.5), sw=SW)
    return g


@icon("Assembly_CreateSimulation")
def asm_sim():
    g = flat()
    raw(g, '<circle cx="32" cy="32" r="26" fill="%s" stroke="%s" stroke-width="3"/>' % (SOLID[1], OUT))
    raw(g, '<polygon points="25,18 25,46 48,32" fill="%s" stroke="%s" stroke-width="2" stroke-linejoin="round"/>' % (YEL[0], OUT))
    return g


@icon("Assembly_BillOfMaterials", "Assembly_CreateBom")
def asm_bom():
    g = flat()
    raw(g, '<rect x="8" y="6" width="48" height="52" rx="3" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (PAPER, OUT, SW))
    for i, y in enumerate((16, 28, 40, 52)):
        raw(g, '<rect x="14" y="%d" width="7" height="7" fill="%s"/>' % (y - 5, YEL[0] if i == 0 else SOLID[1]))
        raw(g, '<path d="M 26 %d h 24" stroke="%s" stroke-width="3" stroke-linecap="round"/>' % (y - 1.5, STEEL))
    return g


@icon("Assembly_ToggleGrounded")
def asm_ground():
    g = Icon()
    g.box(0, 8, 0, 8, 3, 8)
    g.rect3(-2, 10, -2, 10, 0, YEL[0], stroke=AMBER, sw=SW)
    for i in range(0, 12, 3):
        g.line((-2 + i, 10, 0), (-2 + i - 1.5, 11.5, -1.5), color=OUT, sw=SW)
    return g


def _joint(sym):
    def f():
        g = Icon()
        g.box(0, 6, 0, 6, 0, 5)
        g.box(8, 14, 0, 6, 0, 5, pal=COPY)
        sym(g)
        return g
    return f


def j_fixed(g):
    g.badge("link")


def j_rev(g):
    pts = [g.p(7 + 3.2 * math.cos(math.radians(a)), 3 + 3.2 * math.sin(math.radians(a)), 7) for a in range(-60, 230, 12)]
    g.path2(pts[:-1], color=NAVY)
    g.arrow2(pts[-2], pts[-1])
    g.line((7, 3, 3), (7, 3, 11), color=NAVY, sw=SW, dash="5,3")


def j_cyl(g):
    j_rev(g)
    g.arrow3((5, 3, 12), (9, 3, 12), sw=SW, both=True)


def j_slider(g):
    g.arrow3((2, 3, 8), (12, 3, 8), both=True)


def j_ball(g):
    x, y = g.p(7, 3, 8)
    g.sphere2((x, y), 3.4, pal=(YEL[0], YEL[1], YEL[2]))


def j_dist(g):
    g.arrow3((6, -1, 6), (8, -1, 6), sw=SW, both=True)
    g.line((6, -1, 5), (6, -1, 7.5), color=NAVY, sw=SW)
    g.line((8, -1, 5), (8, -1, 7.5), color=NAVY, sw=SW)


def j_par(g):
    g.line((1, 3, 8), (5, 3, 11), color=AMBER)
    g.line((9, 3, 8), (13, 3, 11), color=AMBER)


def j_perp(g):
    g.line((3, 3, 7), (11, 3, 7), color=AMBER)
    g.line((7, 3, 7), (7, 3, 12), color=AMBER)


def j_angle(g):
    g.line((3, 3, 7), (11, 3, 7), color=AMBER)
    g.line((3, 3, 7), (9, 3, 12), color=AMBER)


def j_screw(g):
    pts = [g.p(7 + 1.6 * math.cos(t / 3), 3 + 1.6 * math.sin(t / 3), 6 + t / 8) for t in range(0, 45)]
    g.path2(pts, color=NAVY, sw=SW_GEO * 0.8)


def j_gear(g):
    x, y = g.p(7, 3, 9)
    ps = []
    for i in range(16):
        a = math.radians(i * 22.5)
        r = 4.2 if i % 2 == 0 else 3.2
        ps.append((x + r * math.cos(a), y + r * math.sin(a) * 0.8))
    g.poly2(ps, YEL[0], OUT, SW)


def j_rack(g):
    j_gear(g)
    g.line((1, 3, 6.2), (13, 3, 6.2), color=AMBER)


for sym, names in ((j_fixed, ("Assembly_CreateJointFixed",)), (j_rev, ("Assembly_CreateJointRevolute",)),
                   (j_cyl, ("Assembly_CreateJointCylindrical",)), (j_slider, ("Assembly_CreateJointSlider",)),
                   (j_ball, ("Assembly_CreateJointBall",)), (j_dist, ("Assembly_CreateJointDistance",)),
                   (j_par, ("Assembly_CreateJointParallel",)), (j_perp, ("Assembly_CreateJointPerpendicular",)),
                   (j_angle, ("Assembly_CreateJointAngle",)), (j_screw, ("Assembly_CreateJointScrew",)),
                   (j_gear, ("Assembly_CreateJointGears", "Assembly_CreateJointGearBelt")),
                   (j_rack, ("Assembly_CreateJointRackPinion",))):
    for n in names:
        ICONS[n] = _joint(sym)


# ============================================================ planilha
def grid(g, hl=None):
    raw(g, '<rect x="6" y="8" width="52" height="48" rx="3" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (PAPER, OUT, SW))
    raw(g, '<rect x="6" y="8" width="52" height="11" rx="3" fill="%s"/>' % SOLID[1])
    for x in (23, 40):
        raw(g, '<path d="M %d 8 v 48" stroke="%s" stroke-width="1.6"/>' % (x, PAPER_LINE))
    for y in (19, 31, 43):
        raw(g, '<path d="M 6 %d h 52" stroke="%s" stroke-width="1.6"/>' % (y, PAPER_LINE))
    if hl:
        x, y, w, h = hl
        raw(g, '<rect x="%d" y="%d" width="%d" height="%d" fill="%s" stroke="%s" stroke-width="2"/>' % (x, y, w, h, YEL[0], OUT))
    raw(g, '<rect x="6" y="8" width="52" height="48" rx="3" fill="none" stroke="%s" stroke-width="%.1f"/>' % (OUT, SW))


@icon("Spreadsheet", "Spreadsheet_CreateSheet")
def sp_new():
    g = flat()
    grid(g, (23, 31, 17, 12))
    return g


@icon("SpreadsheetAlias", "Spreadsheet_SetAlias")
def sp_alias():
    g = flat()
    grid(g)
    raw(g, '<path d="M 26 30 h 22 l 8 8 l -8 8 h -22 z" fill="%s" stroke="%s" stroke-width="2"/>' % (YEL[0], OUT))
    return g


@icon("SpreadsheetMergeCells", "Spreadsheet_MergeCells")
def sp_merge():
    g = flat()
    grid(g, (6, 31, 52, 12))
    return g


@icon("SpreadsheetSplitCell", "Spreadsheet_SplitCell")
def sp_split():
    g = flat()
    grid(g, (23, 31, 17, 12))
    raw(g, '<path d="M 31.5 29 v 16" stroke="%s" stroke-width="3" stroke-dasharray="3,2"/>' % RED)
    return g


@icon("SpreadsheetImport", "Spreadsheet_Import")
def sp_imp():
    g = flat()
    grid(g)
    g.badge("arrow", where="bl")
    return g


@icon("SpreadsheetExport", "Spreadsheet_Export")
def sp_exp():
    g = flat()
    grid(g)
    g.badge("arrow")
    return g


def _style(letter, extra=""):
    def f():
        g = flat()
        raw(g, '<rect x="8" y="8" width="48" height="48" rx="6" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (PAPER, OUT, SW))
        raw(g, '<text x="32" y="45" font-family="Georgia, serif" font-size="36" font-weight="%s" font-style="%s" '
               'fill="%s" text-anchor="middle">%s</text>' % ("800" if letter == "B" else "500",
                                                             "italic" if letter == "I" else "normal", NAVY, letter))
        if letter == "U":
            raw(g, '<path d="M 18 50 h 28" stroke="%s" stroke-width="3.4" stroke-linecap="round"/>' % AMBER)
        return g
    return f


ICONS["SpreadsheetStyleBold"] = ICONS["Spreadsheet_StyleBold"] = _style("B")
ICONS["SpreadsheetStyleItalic"] = ICONS["Spreadsheet_StyleItalic"] = _style("I")
ICONS["SpreadsheetStyleUnderline"] = ICONS["Spreadsheet_StyleUnderline"] = _style("U")


def _align(kind):
    def f():
        g = flat()
        raw(g, '<rect x="8" y="8" width="48" height="48" rx="4" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (PAPER, OUT, SW))
        if kind in ("left", "center", "right"):
            gx = {"left": 14, "center": 32, "right": 50}[kind]
            raw(g, '<path d="M %d 13 V 51" stroke="%s" stroke-width="2.4" stroke-dasharray="3,2"/>' % (gx, AMBER))
            for i, w in enumerate((30, 18, 24)):
                x = {"left": 14, "center": 32 - w / 2, "right": 50 - w}[kind]
                raw(g, '<path d="M %.1f %d h %d" stroke="%s" stroke-width="5" stroke-linecap="butt"/>'
                    % (x, 20 + i * 12, w, NAVY))
        else:
            gy = {"top": 14, "vcenter": 32, "bottom": 50}[kind]
            raw(g, '<path d="M 13 %d H 51" stroke="%s" stroke-width="2.4" stroke-dasharray="3,2"/>' % (gy, AMBER))
            for i, h in enumerate((30, 18, 24)):
                y = {"top": 14, "vcenter": 32 - h / 2, "bottom": 50 - h}[kind]
                raw(g, '<path d="M %d %.1f v %d" stroke="%s" stroke-width="5" stroke-linecap="butt"/>'
                    % (20 + i * 12, y, h, NAVY))
        return g
    return f


for kind, names in (("left", ("SpreadsheetAlignLeft", "Spreadsheet_AlignLeft")),
                    ("center", ("SpreadsheetAlignCenter", "Spreadsheet_AlignCenter")),
                    ("right", ("SpreadsheetAlignRight", "Spreadsheet_AlignRight")),
                    ("top", ("SpreadsheetAlignTop", "Spreadsheet_AlignTop")),
                    ("vcenter", ("SpreadsheetAlignVCenter", "Spreadsheet_AlignVCenter")),
                    ("bottom", ("SpreadsheetAlignBottom", "Spreadsheet_AlignBottom"))):
    for n in names:
        ICONS[n] = _align(kind)
