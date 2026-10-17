# SPDX-License-Identifier: LGPL-2.1-or-later
# Sework Engenharia - núcleo do gerador de ícones (estilo do guia ilustrado).
"""Primitivas de desenho para ícones 64x64.

Tudo é desenhado em coordenadas de modelo e enquadrado automaticamente no quadro 64x64
ao final. A espessura dos traços é dada em pixels do ícone final (64 px), então todos os
ícones têm o mesmo peso de linha, independentemente da escala do desenho.
"""
import math

C30, S30 = math.cos(math.radians(30)), math.sin(math.radians(30))

# ---- Paleta Sework (a mesma do guia ilustrado)
SOLID = ("#A9C4E0", "#7FA3C9", "#5A80AA")      # topo, face esquerda, face direita
COPY = ("#D3E1EF", "#B9CDE2", "#9DB7D3")
BASE = ("#E4E9EF", "#C9D2DC", "#B0BCC9")
CUT = ("#E3A597", "#C9735F", "#A8584A")
YEL = ("#F7BE00", "#E5B000", "#C99A00")
OUT = "#1E2A38"
NAVY = "#003A70"
STEEL = "#5A80AA"
AMBER = "#E0A100"
AMBER_FILL = "#FBE7A6"
PAPER = "#FFFFFF"
PAPER_LINE = "#9FB1C4"
RED = "#B5523F"
GREEN = "#2E8B6A"

SW = 2.2        # traço padrão de contorno (px no ícone de 64)
SW_GEO = 3.6    # traço de geometria de esboço / setas


class Icon:
    def __init__(self, k=1.0):
        self.k = k
        self.items = []
        self.defs = {}
        self.bb = [1e9, 1e9, -1e9, -1e9]
        self.fixed = None   # viewBox fixo opcional (x, y, w, h)

    # ---------------------------------------------------------------- base
    def _track(self, x, y):
        b = self.bb
        b[0], b[1], b[2], b[3] = min(b[0], x), min(b[1], y), max(b[2], x), max(b[3], y)

    def p(self, x, y, z=0.0):
        """Projeção isométrica (x para a direita-frente, y para a esquerda-frente, z para cima)."""
        return ((x - y) * C30 * self.k, (x + y) * S30 * self.k - z * self.k)

    def grad(self, c1, c2, vertical=True):
        key = (c1, c2, vertical)
        if key not in self.defs:
            gid = "g%d" % len(self.defs)
            x2, y2 = ("0", "1") if vertical else ("1", "0")
            self.defs[key] = (gid, '<linearGradient id="%s" x1="0" y1="0" x2="%s" y2="%s">'
                                   '<stop offset="0" stop-color="%s"/><stop offset="1" stop-color="%s"/>'
                                   '</linearGradient>' % (gid, x2, y2, c1, c2))
        return "url(#%s)" % self.defs[key][0]

    def poly2(self, pts, fill="none", stroke=OUT, sw=SW, dash=None, opacity=1.0, close=True, join="round"):
        for x, y in pts:
            self._track(x, y)
        self.items.append(("poly", pts, fill, stroke, sw, dash, opacity, close, join))

    def poly(self, pts3, fill, stroke=OUT, sw=SW, dash=None, opacity=1.0):
        self.poly2([self.p(*q) for q in pts3], fill, stroke, sw, dash, opacity)

    def line2(self, a, b, color=NAVY, sw=SW_GEO, dash=None, cap="round"):
        self._track(*a)
        self._track(*b)
        self.items.append(("line", a, b, color, sw, dash, cap))

    def line(self, a, b, color=NAVY, sw=SW_GEO, dash=None):
        self.line2(self.p(*a), self.p(*b), color, sw, dash)

    def circle2(self, c, r, fill="none", stroke=OUT, sw=SW, dash=None):
        self._track(c[0] - r, c[1] - r)
        self._track(c[0] + r, c[1] + r)
        self.items.append(("circle", c, r, fill, stroke, sw, dash))

    def text2(self, x, y, s, size, color=NAVY, weight="700", anchor="middle"):
        w = len(s) * size * 0.6
        self._track(x - w / 2, y - size * 0.8)
        self._track(x + w / 2, y + size * 0.2)
        self.items.append(("text", x, y, s, size, color, weight, anchor))

    # ---------------------------------------------------------------- 2D utilidades
    def path2(self, pts, color=NAVY, sw=SW_GEO, dash=None, fill="none"):
        self.poly2(pts, fill=fill, stroke=color, sw=sw, dash=dash, close=False)

    def arrow2(self, a, b, color=NAVY, sw=SW_GEO, head=1.0, both=False):
        self.line2(a, b, color, sw)
        self._head2(a, b, color, sw, head)
        if both:
            self._head2(b, a, color, sw, head)

    def _head2(self, a, b, color, sw, head):
        ang = math.atan2(b[1] - a[1], b[0] - a[0])
        L = (sw * 3.2) * head
        Wd = (sw * 1.9) * head
        # comprimentos em px finais: guardamos marcador para converter na escala depois
        self.items.append(("head", b, ang, L, Wd, color))
        self._track(*b)

    def arc_pts2(self, c, r, a0, a1, n=24, ry=None):
        ry = r if ry is None else ry
        return [(c[0] + r * math.cos(math.radians(a0 + (a1 - a0) * i / n)),
                 c[1] - ry * math.sin(math.radians(a0 + (a1 - a0) * i / n))) for i in range(n + 1)]

    def dot2(self, c, r=3.2, fill=AMBER, stroke=OUT):
        self.items.append(("dot", c, r, fill, stroke))
        self._track(c[0], c[1])

    # ---------------------------------------------------------------- 3D utilidades
    def box(self, x0, x1, y0, y1, z0, z1, pal=SOLID, sw=SW, opacity=1.0, dash=None):
        top, left, right = pal
        self.poly([(x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1)], left, sw=sw, opacity=opacity, dash=dash)
        self.poly([(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)], right, sw=sw, opacity=opacity, dash=dash)
        self.poly([(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)], top, sw=sw, opacity=opacity, dash=dash)

    def rect3(self, x0, x1, y0, y1, z, fill, stroke=OUT, sw=SW, dash=None, opacity=1.0):
        self.poly([(x0, y0, z), (x1, y0, z), (x1, y1, z), (x0, y1, z)], fill, stroke, sw, dash, opacity)

    def ring(self, cx, cy, r, z, n=36):
        return [self.p(cx + r * math.cos(2 * math.pi * i / n), cy + r * math.sin(2 * math.pi * i / n), z) for i in range(n)]

    def ellipse_iso(self, cx, cy, r, z, fill, stroke=OUT, sw=SW, dash=None, opacity=1.0):
        self.poly2(self.ring(cx, cy, r, z), fill, stroke, sw, dash, opacity)

    def cyl(self, cx, cy, r, z0, z1, pal=SOLID, sw=SW, hole=0.0):
        n = 48
        ts = [math.radians(-45 + 180 * i / n) for i in range(n + 1)]  # metade da frente
        top = [self.p(cx + r * math.cos(t), cy + r * math.sin(t), z1) for t in ts]
        bot = [self.p(cx + r * math.cos(t), cy + r * math.sin(t), z0) for t in ts]
        side = top + bot[::-1]
        self.poly2(side, self.grad(pal[1], pal[2], vertical=False), OUT, sw)
        self.ellipse_iso(cx, cy, r, z1, pal[0], OUT, sw)
        if hole:
            self.ellipse_iso(cx, cy, hole, z1, "#FFFFFF", OUT, sw * 0.9)

    def cone(self, cx, cy, r, z0, z1, pal=SOLID, sw=SW):
        n = 48
        ts = [math.radians(-45 + 180 * i / n) for i in range(n + 1)]
        bot = [self.p(cx + r * math.cos(t), cy + r * math.sin(t), z0) for t in ts]
        apex = self.p(cx, cy, z1)
        self.poly2([apex] + bot[::-1], self.grad(pal[1], pal[2], vertical=False), OUT, sw)

    def sphere2(self, c, r, pal=SOLID, sw=SW):
        self.circle2(c, r, self.grad(pal[0], pal[2]), OUT, sw)
        # brilho
        self.items.append(("ellipse", (c[0] - r * 0.32, c[1] - r * 0.38), r * 0.32, r * 0.2, "#FFFFFF", 0.65))

    def profile(self, x0, x1, y0, y1, z, fill=False):
        self.rect3(x0, x1, y0, y1, z, AMBER_FILL if fill else "none", stroke=AMBER, sw=SW_GEO * 0.9,
                   opacity=0.9 if fill else 1.0)

    def arrow3(self, a, b, color=NAVY, sw=SW_GEO, head=1.0, both=False):
        self.arrow2(self.p(*a), self.p(*b), color, sw, head, both)

    # ---------------------------------------------------------------- selos
    def badge(self, kind, color=YEL, where="br"):
        """Selo no canto (aplicado depois do enquadramento): plus, check, x, pencil, link."""
        self.items.append(("badge", kind, color, where))

    # ---------------------------------------------------------------- saída
    def svg(self, margin=3.0, size=64):
        if self.fixed:
            x0, y0, w, h = self.fixed
            s = size / max(w, h)
        else:
            x0, y0, x1, y1 = self.bb
            w, h = max(x1 - x0, 1e-6), max(y1 - y0, 1e-6)
            s = (size - 2 * margin) / max(w, h)
            # centraliza
            x0 -= ((size / s) - w) / 2
            y0 -= ((size / s) - h) / 2
        T = lambda x, y: ((x - x0) * s, (y - y0) * s)
        out = []
        for it in self.items:
            kind = it[0]
            if kind == "poly":
                _, pts, fill, stroke, sw, dash, op, close, join = it
                pp = " ".join("%.2f,%.2f" % T(x, y) for x, y in pts)
                tag = "polygon" if close else "polyline"
                d = ' stroke-dasharray="%s"' % dash if dash else ""
                out.append('<%s points="%s" fill="%s" fill-opacity="%.2f" stroke="%s" stroke-width="%.2f" '
                           'stroke-linejoin="%s" stroke-linecap="round"%s/>' % (tag, pp, fill, op, stroke, sw, join, d))
            elif kind == "line":
                _, a, b, color, sw, dash, cap = it
                (x1, y1), (x2, y2) = T(*a), T(*b)
                d = ' stroke-dasharray="%s"' % dash if dash else ""
                out.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" stroke="%s" stroke-width="%.2f" '
                           'stroke-linecap="%s"%s/>' % (x1, y1, x2, y2, color, sw, cap, d))
            elif kind == "head":
                _, b, ang, L, Wd, color = it
                x2, y2 = T(*b)
                bx, by = x2 - L * math.cos(ang), y2 - L * math.sin(ang)
                lx, ly = bx + Wd * math.sin(ang), by - Wd * math.cos(ang)
                rx, ry = bx - Wd * math.sin(ang), by + Wd * math.cos(ang)
                # recua a ponta um pouco para o traço arredondado não passar da seta
                out.append('<polygon points="%.2f,%.2f %.2f,%.2f %.2f,%.2f" fill="%s" stroke="%s" stroke-width="0.8" '
                           'stroke-linejoin="round"/>' % (x2, y2, lx, ly, rx, ry, color, color))
            elif kind == "circle":
                _, c, r, fill, stroke, sw, dash = it
                cx, cy = T(*c)
                d = ' stroke-dasharray="%s"' % dash if dash else ""
                out.append('<circle cx="%.2f" cy="%.2f" r="%.2f" fill="%s" stroke="%s" stroke-width="%.2f"%s/>'
                           % (cx, cy, r * s, fill, stroke, sw, d))
            elif kind == "ellipse":
                _, c, rx, ry, fill, op = it
                cx, cy = T(*c)
                out.append('<ellipse cx="%.2f" cy="%.2f" rx="%.2f" ry="%.2f" fill="%s" fill-opacity="%.2f"/>'
                           % (cx, cy, rx * s, ry * s, fill, op))
            elif kind == "dot":
                _, c, r, fill, stroke = it
                cx, cy = T(*c)
                out.append('<circle cx="%.2f" cy="%.2f" r="%.2f" fill="%s" stroke="%s" stroke-width="1.6"/>'
                           % (cx, cy, r, fill, stroke))
            elif kind == "text":
                _, x, y, txt, sz, color, weight, anchor = it
                tx, ty = T(x, y)
                out.append('<text x="%.2f" y="%.2f" font-family="Segoe UI, Arial, sans-serif" font-size="%.2f" '
                           'font-weight="%s" fill="%s" text-anchor="%s">%s</text>'
                           % (tx, ty, sz * s, weight, color, anchor, txt))
            elif kind == "badge":
                out.append(_badge(*it[1:]))
            elif kind == "raw":
                out.append(it[1])
        defs = "".join(v[1] for v in self.defs.values())
        return ('<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 64 64">'
                '<defs>%s</defs>%s</svg>' % (defs, "".join(out)))


def _badge(kind, color, where):
    cx, cy = {"br": (50, 50), "tr": (50, 14), "bl": (14, 50), "tl": (14, 14)}[where]
    r = 12
    fill = {"plus": YEL[0], "check": GREEN, "x": RED, "pencil": YEL[0], "link": YEL[0],
            "minus": RED, "arrow": YEL[0], "eye": YEL[0]}.get(kind, color)
    s = ['<circle cx="%d" cy="%d" r="%d" fill="%s" stroke="%s" stroke-width="2"/>' % (cx, cy, r, fill, OUT)]
    w = 3.4
    ink = OUT if fill == YEL[0] else "#FFFFFF"
    if kind == "plus":
        s.append('<path d="M %d %d h 12 M %d %d v 12" stroke="%s" stroke-width="%.1f" stroke-linecap="round"/>'
                 % (cx - 6, cy, cx, cy - 6, ink, w))
    elif kind == "minus":
        s.append('<path d="M %d %d h 12" stroke="%s" stroke-width="%.1f" stroke-linecap="round"/>' % (cx - 6, cy, ink, w))
    elif kind == "check":
        s.append('<path d="M %d %d l 4 4 l 8 -9" fill="none" stroke="%s" stroke-width="%.1f" stroke-linecap="round" '
                 'stroke-linejoin="round"/>' % (cx - 6, cy, ink, w))
    elif kind == "x":
        s.append('<path d="M %d %d l 10 10 M %d %d l -10 10" stroke="%s" stroke-width="%.1f" stroke-linecap="round"/>'
                 % (cx - 5, cy - 5, cx + 5, cy - 5, ink, w))
    elif kind == "pencil":
        s.append('<path d="M %d %d l 9 -9 l 3 3 l -9 9 l -4 1 z" fill="#FFFFFF" stroke="%s" stroke-width="1.8" '
                 'stroke-linejoin="round"/>' % (cx - 6, cy + 4, ink))
    elif kind == "link":
        s.append('<rect x="%d" y="%d" width="11" height="6.5" rx="3.2" fill="none" stroke="%s" stroke-width="2.4" '
                 'transform="rotate(-35 %d %d)"/>' % (cx - 9, cy - 1, ink, cx - 3.5, cy + 2))
        s.append('<rect x="%d" y="%d" width="11" height="6.5" rx="3.2" fill="none" stroke="%s" stroke-width="2.4" '
                 'transform="rotate(-35 %d %d)"/>' % (cx - 2, cy - 6, ink, cx + 3.5, cy - 3))
    elif kind == "arrow":
        s.append('<path d="M %d %d h 10 m -4 -4 l 4 4 l -4 4" fill="none" stroke="%s" stroke-width="%.1f" '
                 'stroke-linecap="round" stroke-linejoin="round"/>' % (cx - 6, cy, ink, w))
    elif kind == "eye":
        s.append('<path d="M %d %d q 8 -8 16 0 q -8 8 -16 0 z" fill="#FFFFFF" stroke="%s" stroke-width="1.8"/>'
                 '<circle cx="%d" cy="%d" r="2.6" fill="%s"/>' % (cx - 8, cy, ink, cx, cy, ink))
    return "".join(s)
