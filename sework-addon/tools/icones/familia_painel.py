# SPDX-License-Identifier: LGPL-2.1-or-later
# Ícones que aparecem no painel de tarefas: elementos do esboço, restrições da lista,
# cabeçalhos das caixas e botões auxiliares.
#
# Elementos: o FreeCAD recolore por pixel para gerar as variantes de construção/externa:
#   pontos vermelhos (matiz < 30°, saturação > 50 %) e bordas brancas mudam de cor.
# Por isso: pontos em RED (tijolo), arestas brancas com contorno, parte destacada em amarelo.
import math
from nucleo import Icon, YEL, OUT, NAVY, AMBER, RED, SOLID, COPY, STEEL, PAPER_LINE
import familia_esboco as E

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


# ============================================================ elementos do esboço
def edge(g, d, hl=False):
    raw(g, '<path d="%s" fill="none" stroke="%s" stroke-width="8.5" stroke-linecap="round" stroke-linejoin="round"/>'
        % (d, OUT))
    raw(g, '<path d="%s" fill="none" stroke="%s" stroke-width="4.6" stroke-linecap="round" stroke-linejoin="round"/>'
        % (d, YEL[0] if hl else "#FFFFFF"))


def pt(g, x, y, hl=False, r=5.6):
    raw(g, '<circle cx="%.1f" cy="%.1f" r="%.1f" fill="%s" stroke="%s" stroke-width="2"/>'
        % (x, y, r + (0.8 if hl else 0), YEL[0] if hl else RED, OUT))


def element(draw, points, hl):
    """draw(g, hl_edge) desenha as arestas; points: dict nome->(x,y); hl: 'edge' ou nome do ponto."""
    def f():
        g = flat()
        draw(g, hl == "edge")
        for name, (x, y) in points.items():
            pt(g, x, y, hl == name)
        return g
    return f


def reg(prefix, draw, points, variants):
    for suffix, hl in variants:
        ICONS["Sketcher_Element_%s_%s" % (prefix, suffix)] = element(draw, points, hl)


# linha
LINE = "M 12 52 L 52 12"
reg("Line", lambda g, h: edge(g, LINE, h), {"s": (12, 52), "e": (52, 12)},
    (("Edge", "edge"), ("StartingPoint", "s"), ("EndPoint", "e")))

# arco de circunferência (centro mostrado)
ARC = "M 10 22 A 32 32 0 0 1 42 54"
reg("Arc", lambda g, h: edge(g, ARC, h), {"s": (42, 54), "e": (10, 22), "m": (10, 54)},
    (("Edge", "edge"), ("StartingPoint", "s"), ("EndPoint", "e"), ("MidPoint", "m")))

# circunferência
reg("Circle", lambda g, h: edge(g, "M 32 9 A 23 23 0 1 1 31.9 9 Z", h), {"m": (32, 32)},
    (("Edge", "edge"), ("MidPoint", "m")))

# elipse
ELL = '<ellipse cx="32" cy="32" rx="26" ry="13" transform="rotate(-35 32 32)" fill="none" stroke="%s" stroke-width="%s"/>'
F1 = (32 - 21 * math.cos(math.radians(35)), 32 + 21 * math.sin(math.radians(35)))
F2 = (32 + 21 * math.cos(math.radians(35)), 32 - 21 * math.sin(math.radians(35)))


def ell(g, hl):
    raw(g, ELL % (OUT, 8.5))
    raw(g, ELL % (YEL[0] if hl else "#FFFFFF", 4.6))


def axis(g, which, hl=True):
    a = math.radians(-35 if which == "major" else 55)
    r = 26 if which == "major" else 13
    x1, y1 = 32 - r * math.cos(a), 32 - r * math.sin(a)
    x2, y2 = 32 + r * math.cos(a), 32 + r * math.sin(a)
    edge(g, "M %.1f %.1f L %.1f %.1f" % (x1, y1, x2, y2), hl)


def ellipse_icon(hl):
    def f():
        g = flat()
        ell(g, hl in ("edge", "edge1"))
        if hl == "major":
            axis(g, "major")
        if hl == "minor":
            axis(g, "minor")
        if hl == "all":
            axis(g, "major", False)
            axis(g, "minor", False)
        if hl in ("f1", "f2", "all"):
            pt(g, F1[0], F1[1], hl == "f1", 4.6)
            pt(g, F2[0], F2[1], hl == "f2", 4.6)
        pt(g, 32, 32, hl == "c")
        return g
    return f


for suf, hl in (("Edge_1", "edge1"), ("Edge_2", "edge"), ("CentrePoint", "c"), ("Focus1", "f1"), ("Focus2", "f2"),
                ("MajorAxis", "major"), ("MinorAxis", "minor"), ("All", "all")):
    ICONS["Sketcher_Element_Ellipse_" + suf] = ellipse_icon(hl)

# arco de elipse (meia elipse em U), centro abaixo
EARC = "M 12 14 C 8 40 20 54 32 54 C 44 54 56 40 52 14"
for prefix, d, c in (("Elliptical_Arc", EARC, (32, 30)),
                     ("Hyperbolic_Arc", "M 10 12 Q 32 50 54 12", (32, 54)),
                     ("Parabolic_Arc", "M 12 12 Q 32 72 52 12", (32, 26))):
    reg(prefix, (lambda dd: lambda g, h: edge(g, dd, h))(d), {"s": (12 if prefix != "Hyperbolic_Arc" else 10, 12 if prefix != "Elliptical_Arc" else 14),
                                                              "e": (52 if prefix != "Hyperbolic_Arc" else 54, 12 if prefix != "Elliptical_Arc" else 14),
                                                              "c": c},
        (("Edge", "edge"), ("Start_Point", "s"), ("End_Point", "e"), ("Centre_Point", "c")))

# B-spline
BSP = "M 10 50 C 18 18 30 18 32 32 C 34 46 46 46 54 14"
reg("BSpline", lambda g, h: edge(g, BSP, h), {"s": (10, 50), "e": (54, 14)},
    (("Edge", "edge"), ("StartPoint", "s"), ("EndPoint", "e")))


@icon("Sketcher_Element_Point_StartingPoint")
def el_point():
    g = flat()
    pt(g, 32, 32, False, 9)
    return g


@icon("Sketcher_Element_SelectionTypeInvalid")
def el_invalid():
    g = flat()
    raw(g, '<path d="M 14 32 H 50" stroke="#8C99A6" stroke-width="5" stroke-linecap="round"/>')
    return g


def text_el(hl):
    def f():
        g = flat()
        edge(g, "M 14 50 L 32 12 L 50 50 M 21 36 H 43")
        edge(g, "M 8 56 H 56")
        pt(g, 8, 56, hl == "s")
        pt(g, 56, 56, hl == "e")
        return g
    return f


ICONS["Sketcher_Element_Text_StartPoint"] = text_el("s")
ICONS["Sketcher_Element_Text_EndPoint"] = text_el("e")


# ============================================================ restrições da lista
def glyph_extra(g, kind):
    E.glyph(g, "horizontal")       # cria o selo; o símbolo é trocado abaixo
    g.items.pop()                  # remove o traço horizontal
    cx, cy = 46, 46
    k = NAVY
    if kind == "pointonobject":
        raw(g, '<path d="M %d %d l 18 -12" stroke="%s" stroke-width="2.6" stroke-linecap="round"/>' % (cx - 9, cy + 6, k))
        raw(g, '<circle cx="%d" cy="%d" r="3.6" fill="%s"/>' % (cx, cy, k))
    elif kind == "group":
        for dx, dy in ((-6, -5), (2, -5), (-6, 3), (2, 3)):
            raw(g, '<rect x="%d" y="%d" width="6" height="6" rx="1" fill="%s"/>' % (cx + dx, cy + dy, k))
    elif kind == "text":
        raw(g, '<text x="%d" y="%d" font-family="Segoe UI, Arial" font-size="17" font-weight="800" fill="%s" '
               'text-anchor="middle">T</text>' % (cx, cy + 6, k))
    elif kind == "snell":
        raw(g, '<path d="M %d %d H %d" stroke="%s" stroke-width="1.6" stroke-dasharray="2,2"/>' % (cx - 10, cy, cx + 10, k))
        raw(g, '<path d="M %d %d L %d %d L %d %d" fill="none" stroke="%s" stroke-width="2.6" stroke-linejoin="round"/>'
            % (cx - 8, cy - 9, cx, cy, cx + 4, cy + 10, k))
    elif kind == "align":
        raw(g, '<ellipse cx="%d" cy="%d" rx="9" ry="5" fill="none" stroke="%s" stroke-width="2.4"/>' % (cx, cy, k))
        raw(g, '<path d="M %d %d H %d" stroke="%s" stroke-width="2.4"/>' % (cx - 9, cy, cx + 9, k))


@icon("Constraint_PointOnObject", "Sketcher_ConstrainPointOnObject")
def c_poo():
    g = flat()
    g.line2((6, 40), (42, 8), color=E.LIGHT)
    E.pts(g, (24, 24), r=5)
    glyph_extra(g, "pointonobject")
    return g


@icon("Constraint_Group")
def c_group():
    g = flat()
    g.line2((6, 30), (40, 30), color=E.LIGHT)
    g.line2((10, 8), (10, 40), color=E.LIGHT)
    glyph_extra(g, "group")
    return g


@icon("Constraint_Text")
def c_text():
    g = flat()
    g.line2((6, 36), (40, 10), color=E.LIGHT)
    glyph_extra(g, "text")
    return g


@icon("Constraint_SnellsLaw", "Sketcher_ConstrainSnellsLaw")
def c_snell():
    g = flat()
    g.line2((4, 24), (44, 24), color=E.LIGHT)
    g.path2([(8, 6), (24, 24), (30, 42)])
    E.pts(g, (24, 24), r=4.6)
    glyph_extra(g, "snell")
    return g


def _int_align(extra):
    def f():
        g = flat()
        raw(g, '<ellipse cx="24" cy="24" rx="20" ry="11" transform="rotate(-30 24 24)" fill="none" stroke="%s" '
               'stroke-width="3.6"/>' % E.LIGHT)
        extra(g)
        glyph_extra(g, "align")
        return g
    return f


def _ax(a, r):
    def d(g):
        rad = math.radians(a)
        g.line2((24 - r * math.cos(rad), 24 - r * math.sin(rad)), (24 + r * math.cos(rad), 24 + r * math.sin(rad)), color=NAVY)
    return d


def _foc(s):
    def d(g):
        rad = math.radians(-30)
        E.pts(g, (24 + s * 15 * math.cos(rad), 24 + s * 15 * math.sin(rad)), r=4.4)
    return d


ICONS["Constraint_InternalAlignment"] = _int_align(lambda g: E.pts(g, (24, 24), r=4.4))
ICONS["Constraint_InternalAlignment_Ellipse_MajorAxis"] = _int_align(_ax(-30, 20))
ICONS["Constraint_InternalAlignment_Ellipse_MinorAxis"] = _int_align(_ax(60, 11))
ICONS["Constraint_InternalAlignment_Ellipse_Focus1"] = _int_align(_foc(-1))
ICONS["Constraint_InternalAlignment_Ellipse_Focus2"] = _int_align(_foc(1))

ICONS["Constraint_Coincident"] = E.ICONS["Constraint_PointOnPoint"]

# Restrições de referência (driven): mesmo desenho, selo azul-claro em vez de amarelo
DRIVEN = "#BFD3E8"


def driven(fn):
    def f():
        g = fn()
        tag = 'rx="6" fill="%s"' % YEL[0]
        new = []
        for it in g.items:
            if it[0] == "raw" and tag in it[1]:
                it = ("raw", it[1].replace(tag, 'rx="6" fill="%s"' % DRIVEN))
            new.append(it)
        g.items = new
        return g
    return f


for base in ("HorizontalDistance", "VerticalDistance", "Length", "Radius", "Diameter", "InternalAngle"):
    ICONS["Constraint_%s_Driven" % base] = driven(E.ICONS["Constraint_" + base])
ICONS["Constraint_SnellsLaw_Driven"] = driven(c_snell)


# ============================================================ cabeçalhos e botões
@icon("Sketcher_Sketch")
def sketch_obj():
    g = Icon()
    E._sheet(g)
    return g


@icon("Sketcher_CreateLineAngleLength")
def line_angle():
    g = flat()
    g.line2((8, 52), (56, 52), color=E.LIGHT, sw=2.6)
    g.line2((8, 52), (48, 14))
    raw(g, '<path d="M 30 52 A 22 22 0 0 0 24 37" fill="none" stroke="%s" stroke-width="3"/>' % AMBER)
    E.pts(g, (8, 52), (48, 14))
    return g


@icon("Std_ToggleVisibility")
def eye():
    g = flat()
    raw(g, '<path d="M 4 32 Q 32 6 60 32 Q 32 58 4 32 Z" fill="#FFFFFF" stroke="%s" stroke-width="3.4" '
           'stroke-linejoin="round"/>' % NAVY)
    raw(g, '<circle cx="32" cy="32" r="11" fill="%s" stroke="%s" stroke-width="2.4"/>' % (YEL[0], OUT))
    raw(g, '<circle cx="32" cy="32" r="4.5" fill="%s"/>' % OUT)
    return g


@icon("Sketcher_Settings", "preferences-general")
def gear_small():
    g = flat()
    ps = []
    for i in range(16):
        a = math.radians(i * 22.5)
        r = 27 if i % 2 == 0 else 20
        ps.append("%.1f,%.1f" % (32 + r * math.cos(a), 32 + r * math.sin(a)))
    raw(g, '<polygon points="%s" fill="%s" stroke="%s" stroke-width="2.4" stroke-linejoin="round"/>'
        % (" ".join(ps), SOLID[1], OUT))
    raw(g, '<circle cx="32" cy="32" r="9" fill="%s" stroke="%s" stroke-width="2.4"/>' % (YEL[0], OUT))
    return g


# ============================================================ árvore do modelo
@icon("TreeItemVisible")
def tree_visible():
    g = flat()
    raw(g, '<path d="M 4 32 Q 32 4 60 32 Q 32 60 4 32 Z" fill="#FFFFFF" stroke="%s" stroke-width="5" '
           'stroke-linejoin="round"/>' % NAVY)
    raw(g, '<circle cx="32" cy="32" r="12" fill="%s" stroke="%s" stroke-width="3"/>' % (YEL[0], OUT))
    raw(g, '<circle cx="32" cy="32" r="5" fill="%s"/>' % OUT)
    return g


@icon("TreeItemInvisible")
def tree_invisible():
    g = flat()
    raw(g, '<path d="M 4 32 Q 32 4 60 32 Q 32 60 4 32 Z" fill="#F3F5F8" stroke="#9AA7B4" stroke-width="5" '
           'stroke-linejoin="round"/>')
    raw(g, '<circle cx="32" cy="32" r="11" fill="#D9E0E8" stroke="#9AA7B4" stroke-width="3"/>')
    raw(g, '<path d="M 10 54 L 54 10" stroke="#7B8794" stroke-width="6" stroke-linecap="round"/>')
    return g


@icon("Document")
def tree_document():
    g = flat()
    raw(g, '<path d="M 10 4 h 30 l 14 14 v 42 h -44 z" fill="#FFFFFF" stroke="%s" stroke-width="3" '
           'stroke-linejoin="round"/>' % OUT)
    raw(g, '<path d="M 40 4 v 14 h 14 z" fill="%s" stroke="%s" stroke-width="3" stroke-linejoin="round"/>' % (YEL[0], OUT))
    c, s = 0.866 * 3, 0.5 * 3
    def P(x, y, z):
        return "%.1f,%.1f" % (31 + (x - y) * c, 34 + (x + y) * s - z * 3)
    for f, col in (([(0, 5, 0), (6, 5, 0), (6, 5, 4), (0, 5, 4)], SOLID[1]),
                   ([(6, 0, 0), (6, 5, 0), (6, 5, 4), (6, 0, 4)], SOLID[2]),
                   ([(0, 0, 4), (6, 0, 4), (6, 5, 4), (0, 5, 4)], SOLID[0])):
        raw(g, '<polygon points="%s" fill="%s" stroke="%s" stroke-width="2.4" stroke-linejoin="round"/>'
            % (" ".join(P(*v) for v in f), col, OUT))
    return g


@icon("Warning")
def tree_warning():
    g = flat()
    raw(g, '<path d="M 32 4 L 61 56 H 3 Z" fill="%s" stroke="%s" stroke-width="4" stroke-linejoin="round"/>' % (YEL[0], OUT))
    raw(g, '<path d="M 32 22 V 40" stroke="%s" stroke-width="7" stroke-linecap="round"/>' % OUT)
    raw(g, '<circle cx="32" cy="49" r="4" fill="%s"/>' % OUT)
    return g


@icon("overlay_error")
def tree_error():
    g = flat()
    raw(g, '<circle cx="32" cy="32" r="28" fill="%s" stroke="#FFFFFF" stroke-width="4"/>' % RED)
    raw(g, '<path d="M 32 16 V 36" stroke="#FFFFFF" stroke-width="8" stroke-linecap="round"/>')
    raw(g, '<circle cx="32" cy="47" r="5" fill="#FFFFFF"/>')
    return g


@icon("overlay_recompute")
def tree_recompute():
    g = flat()
    raw(g, '<circle cx="32" cy="32" r="28" fill="%s" stroke="#FFFFFF" stroke-width="4"/>' % YEL[0])
    raw(g, '<path d="M 46 26 A 15 15 0 1 0 47 38" fill="none" stroke="%s" stroke-width="6" stroke-linecap="round"/>' % NAVY)
    raw(g, '<polygon points="50,14 51,30 37,27" fill="%s"/>' % NAVY)
    return g


@icon("LinkOverlay", "LinkOverlayReadOnly")
def tree_link():
    g = flat()
    raw(g, '<rect x="4" y="4" width="56" height="56" rx="14" fill="#FFFFFF" stroke="%s" stroke-width="4"/>' % NAVY)
    raw(g, '<path d="M 20 44 L 42 22 M 28 20 H 44 V 36" fill="none" stroke="%s" stroke-width="7" '
           'stroke-linecap="round" stroke-linejoin="round"/>' % NAVY)
    return g


# ============================================================ categorias das Preferências
import familia_3d as _F3
import familia_geral as _FG


def _alias(src, name):
    return src.ICONS[name]


@icon("preferences-general")
def pref_general():
    return _FG.ICONS["preferences-system"]()


@icon("preferences-display")
def pref_display():
    g = flat()
    raw(g, '<rect x="4" y="8" width="56" height="38" rx="5" fill="%s" stroke="%s" stroke-width="3"/>' % (SOLID[0], OUT))
    raw(g, '<rect x="10" y="14" width="44" height="26" rx="2" fill="#EEF0F3"/>')
    raw(g, '<path d="M 24 46 v 8 h 16 v -8 M 16 56 h 32" fill="none" stroke="%s" stroke-width="3" stroke-linecap="round"/>' % OUT)
    raw(g, '<polygon points="32,18 44,25 32,32 20,25" fill="%s" stroke="%s" stroke-width="2"/>' % (YEL[0], OUT))
    raw(g, '<polygon points="20,25 32,32 32,38 20,31" fill="%s" stroke="%s" stroke-width="2"/>' % (SOLID[1], OUT))
    raw(g, '<polygon points="44,25 32,32 32,38 44,31" fill="%s" stroke="%s" stroke-width="2"/>' % (SOLID[2], OUT))
    return g


@icon("preferences-workbenches")
def pref_workbenches():
    g = flat()
    for i, (x, y) in enumerate(((6, 6), (34, 6), (6, 34), (34, 34))):
        raw(g, '<rect x="%d" y="%d" width="24" height="24" rx="5" fill="%s" stroke="%s" stroke-width="3"/>'
            % (x, y, YEL[0] if i == 0 else SOLID[i % 3], OUT))
    return g


@icon("preferences-import-export")
def pref_importexport():
    g = flat()
    raw(g, '<path d="M 6 22 H 44" stroke="%s" stroke-width="6" stroke-linecap="round"/>' % NAVY)
    raw(g, '<polygon points="58,22 44,12 44,32" fill="%s"/>' % NAVY)
    raw(g, '<path d="M 58 44 H 20" stroke="%s" stroke-width="6" stroke-linecap="round"/>' % AMBER)
    raw(g, '<polygon points="6,44 20,34 20,54" fill="%s"/>' % AMBER)
    return g


@icon("preferences-python")
def pref_python():
    g = flat()
    raw(g, '<rect x="4" y="8" width="56" height="48" rx="6" fill="#FFFFFF" stroke="%s" stroke-width="3"/>' % OUT)
    raw(g, '<rect x="4" y="8" width="56" height="11" rx="6" fill="%s"/>' % NAVY)
    raw(g, '<path d="M 14 30 l 8 6 l -8 6" fill="none" stroke="%s" stroke-width="4" stroke-linecap="round" '
           'stroke-linejoin="round"/>' % NAVY)
    raw(g, '<path d="M 28 44 H 46" stroke="%s" stroke-width="4" stroke-linecap="round"/>' % AMBER)
    return g


@icon("preferences-addon_manager", "AddonManager")
def pref_addons():
    g = Icon()
    g.box(0, 8, 0, 8, 0, 6, pal=COPY)
    g.badge("plus")
    return g


@icon("preferences-material")
def pref_material():
    g = flat()
    grad = ('<defs><radialGradient id="m" cx="0.35" cy="0.3" r="0.8"><stop offset="0" stop-color="#FFFFFF"/>'
            '<stop offset="0.45" stop-color="%s"/><stop offset="1" stop-color="%s"/></radialGradient></defs>' % (SOLID[0], SOLID[2]))
    raw(g, grad)
    raw(g, '<circle cx="32" cy="32" r="26" fill="url(#m)" stroke="%s" stroke-width="3"/>' % OUT)
    raw(g, '<path d="M 10 40 Q 32 52 54 40" fill="none" stroke="%s" stroke-width="3"/>' % YEL[0])
    return g


@icon("preferences-start")
def pref_start():
    g = flat()
    raw(g, '<path d="M 8 30 L 32 8 L 56 30 V 56 H 8 Z" fill="%s" stroke="%s" stroke-width="3" stroke-linejoin="round"/>' % (SOLID[0], OUT))
    raw(g, '<rect x="24" y="36" width="16" height="20" fill="%s" stroke="%s" stroke-width="3"/>' % (YEL[0], OUT))
    return g


ICONS["preferences-measure"] = _FG.ICONS["umf-measurement"]
ICONS["preferences-spreadsheet"] = _FG.ICONS["Spreadsheet"]
ICONS["preferences-techdraw"] = _FG.ICONS["actions/TechDraw_PageDefault"]
ICONS["preferences-assembly"] = _FG.ICONS["Geoassembly"]
ICONS["preferences-sketcher"] = sketch_obj
ICONS["preferences-part_design"] = _F3.ICONS["PartDesign_Body"]
# "Part/Part Design" vira o caminho preferences-part/part_design.svg
ICONS["SUB:preferences-part/part_design"] = _F3.ICONS["PartDesign_Body"]
