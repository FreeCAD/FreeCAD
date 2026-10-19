# SPDX-License-Identifier: LGPL-2.1-or-later
# Sework Engenharia - ilustrações isométricas para o painel de tarefas.
"""Gera SVGs explicativos (estilo isométrico, paleta Sework) para cada recurso e modo.

Cada função recebe parâmetros simples e devolve o texto SVG. Nada aqui depende do
FreeCAD, então dá para gerar e conferir os desenhos fora do programa.
"""
import math

W, H = 300, 160
C30, S30 = math.cos(math.radians(30)), math.sin(math.radians(30))

# Paleta
SOLID = ("#A9C4E0", "#7FA3C9", "#5A80AA")      # topo, face esquerda, face direita (azul-aço)
BASE = ("#E4E9EF", "#C9D2DC", "#B0BCC9")       # material existente (cinza frio)
CUT = ("#D99A8C", "#C2715F", "#A8584A")        # material removido (tijolo)
GHOST = ("#F1F4F8", "#E6EBF1", "#DCE3EA")
OUT = "#1E2A38"
NAVY = "#003A70"
AMBER = "#E0A100"
AMBER_FILL = "#FBE7A6"
TARGET = "#F7BE00"
PLANE = "#EDF2F7"
PLANE_LINE = "#A9B9CA"
TXT = "#1E2A38"


class Iso:
    def __init__(self, scale=9.0, ox=150, oy=105):
        self.k, self.ox, self.oy = scale, ox, oy
        self.parts = []
        self.bb = [1e9, 1e9, -1e9, -1e9]

    def track(self, x, y):
        b = self.bb
        b[0], b[1], b[2], b[3] = min(b[0], x), min(b[1], y), max(b[2], x), max(b[3], y)

    def p(self, x, y, z):
        sx = (x - y) * C30 * self.k + self.ox
        sy = (x + y) * S30 * self.k - z * self.k + self.oy
        self.track(sx, sy)
        return sx, sy

    def poly(self, pts3, fill, stroke=OUT, sw=1.2, dash=None, opacity=1.0):
        pts = " ".join("%.1f,%.1f" % self.p(*q) for q in pts3)
        d = ' stroke-dasharray="%s"' % dash if dash else ""
        self.parts.append('<polygon points="%s" fill="%s" fill-opacity="%.2f" stroke="%s" stroke-width="%.1f" '
                          'stroke-linejoin="round"%s/>' % (pts, fill, opacity, stroke, sw, d))

    def line(self, a, b, color=NAVY, sw=1.6, dash=None):
        (x1, y1), (x2, y2) = self.p(*a), self.p(*b)
        d = ' stroke-dasharray="%s"' % dash if dash else ""
        self.parts.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" stroke-width="%.1f" '
                          'stroke-linecap="round"%s/>' % (x1, y1, x2, y2, color, sw, d))

    def arrow(self, a, b, color=NAVY, sw=1.8, both=False):
        self.line(a, b, color, sw)
        self._head(a, b, color)
        if both:
            self._head(b, a, color)

    def _head(self, a, b, color):
        (x1, y1), (x2, y2) = self.p(*a), self.p(*b)
        ang = math.atan2(y2 - y1, x2 - x1)
        L, Wd = 8, 4
        bx, by = x2 - L * math.cos(ang), y2 - L * math.sin(ang)
        lx, ly = bx + Wd * math.sin(ang), by - Wd * math.cos(ang)
        rx, ry = bx - Wd * math.sin(ang), by + Wd * math.cos(ang)
        self.parts.append('<polygon points="%.1f,%.1f %.1f,%.1f %.1f,%.1f" fill="%s"/>' % (x2, y2, lx, ly, rx, ry, color))

    def text(self, x, y, s, size=11, color=TXT, anchor="start", weight="600"):
        w = len(s) * size * 0.56
        x0 = x - (w if anchor == "end" else w / 2 if anchor == "middle" else 0)
        self.track(x0, y - size)
        self.track(x0 + w, y + 2)
        self.parts.append('<text x="%.1f" y="%.1f" font-family="Segoe UI, Arial, sans-serif" font-size="%d" '
                          'font-weight="%s" fill="%s" text-anchor="%s">%s</text>' % (x, y, size, weight, color, anchor, s))

    def text3(self, q, s, dx=6, dy=4, **kw):
        x, y = self.p(*q)
        self.text(x + dx, y + dy, s, **kw)

    def box(self, x0, x1, y0, y1, z0, z1, pal=SOLID, sw=1.2, dash=None, opacity=1.0):
        top, left, right = pal
        # face +y (esquerda, frente), face +x (direita, frente), topo
        self.poly([(x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1)], left, sw=sw, dash=dash, opacity=opacity)
        self.poly([(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)], right, sw=sw, dash=dash, opacity=opacity)
        self.poly([(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)], top, sw=sw, dash=dash, opacity=opacity)

    def plane(self, x0, x1, y0, y1, z):
        self.poly([(x0, y0, z), (x1, y0, z), (x1, y1, z), (x0, y1, z)], PLANE, stroke=PLANE_LINE, sw=1.0)

    def profile(self, x0, x1, y0, y1, z, fill=True):
        self.poly([(x0, y0, z), (x1, y0, z), (x1, y1, z), (x0, y1, z)], AMBER_FILL if fill else "none",
                  stroke=AMBER, sw=2.4, opacity=0.85 if fill else 0)

    def svg(self, pad=10):
        x0, y0, x1, y1 = self.bb
        if x0 > x1:
            x0, y0, x1, y1 = 0, 0, W, H
        x0, y0, x1, y1 = x0 - pad, y0 - pad, x1 + pad, y1 + pad
        w, h = x1 - x0, y1 - y0
        # mantém a proporção W:H
        if w / h > W / H:
            nh = w * H / W
            y0 -= (nh - h) / 2
            h = nh
        else:
            nw = h * W / H
            x0 -= (nw - w) / 2
            w = nw
        return ('<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="%.1f %.1f %.1f %.1f">'
                '%s</svg>' % (W, H, x0, y0, w, h, "".join(self.parts)))


def _legend(g, items):
    """A legenda é mostrada pelo painel (fora do desenho)."""
    g.legend = items
    return
    y = 14
    for color, label in items:
        g.parts.append('<rect x="8" y="%d" width="10" height="10" rx="2" fill="%s" stroke="%s" stroke-width="0.8"/>'
                       % (y - 9, color, OUT))
        g.text(22, y, label, size=10, weight="500")
        y += 14


# --------------------------------------------------------------------------- utilidades
TARGET_PAL = (TARGET, "#E5B000", "#C99A00")
COPY_PAL = ("#C9D9EA", "#AFC5DD", "#93AFCC")


def _raw(g, svg, *pts):
    g.parts.append(svg)
    for x, y in pts:
        g.track(x, y)


def _dim(g, z0, z1, x, y, label, both=False, sub=None):
    """Cota vertical ao lado direito da peça (fora das faces)."""
    g.line((x, y, z0), (x - 0.9, y + 0.9, z0), color=NAVY, sw=0.8)
    g.line((x, y, z1), (x - 0.9, y + 0.9, z1), color=NAVY, sw=0.8)
    g.arrow((x, y, z0), (x, y, z1), both=both)
    g.text3((x, y, (z0 + z1) / 2), label, dx=7, dy=4)
    if sub:
        g.text3((x, y, (z0 + z1) / 2), sub, dx=7, dy=17, size=9, weight="400")


# --------------------------------------------------------------------------- Extrusão
def pad(mode="Length", midplane=False, reversed_=False):
    g = Iso(scale=9)
    X0, X1, Y0, Y1 = 0, 6, 0, 5
    ax, ay = X1 + 2.2, Y0 - 1.2
    g.plane(-2, 8, -2, 7, 0)
    if mode == "TwoLengths":
        g.box(X0, X1, Y0, Y1, -3.5, 5.5)
        g.profile(X0, X1, Y0, Y1, 0, fill=False)
        _dim(g, 0, 5.5, ax, ay, "Comprimento 1")
        _dim(g, 0, -3.5, ax, ay, "Comprimento 2")
    elif midplane:
        g.box(X0, X1, Y0, Y1, -3, 3)
        g.profile(X0, X1, Y0, Y1, 0, fill=False)
        _dim(g, -3, 3, ax, ay, "Comprimento", both=True, sub="metade para cada lado")
    elif mode in ("UpToFace", "UpToShape"):
        g.box(X0, X1, Y0, Y1, 0, 7.5)
        g.box(-2, 9, -2, 7, 7.5, 8.3, pal=TARGET_PAL)
        g.profile(X0, X1, Y0, Y1, 0, fill=False)
        g.text3((9, -2, 8.3), "Face de destino" if mode == "UpToFace" else "Forma de destino", dx=6, dy=-2)
        _dim(g, 0, 7.5, ax + 1.5, ay, "até a face")
    elif mode in ("UpToFirst", "UpToLast"):
        first = mode == "UpToFirst"
        top = 4.5 if first else 8
        g.box(X0, X1, Y0, Y1, 0, top)
        g.box(-1.5, 7.5, -1.5, 6.5, 4.5, 5.4, pal=TARGET_PAL if first else GHOST, opacity=1 if first else 0.7)
        g.box(-1.5, 7.5, -1.5, 6.5, 8, 8.9, pal=GHOST if first else TARGET_PAL, opacity=0.7 if first else 1)
        g.profile(X0, X1, Y0, Y1, 0, fill=False)
        g.text3((7.5, -1.5, top + 0.9), "para na 1ª face" if first else "vai até a última face", dx=6, dy=-2)
    else:
        z1 = -6 if reversed_ else 6
        g.box(X0, X1, Y0, Y1, min(0, z1), max(0, z1))
        g.profile(X0, X1, Y0, Y1, 0, fill=False)
        _dim(g, 0, z1, ax, ay, "Comprimento")
    _legend(g, [(AMBER_FILL, "Esboço (perfil)"), (SOLID[0], "Sólido criado")] +
            ([(TARGET, "Face de destino")] if mode.startswith("UpTo") else []))
    return g


# --------------------------------------------------------------------------- Cavidade
def pocket(mode="Length", midplane=False):
    g = Iso(scale=8.5)
    B = (-3, 9, -3, 8)
    hx0, hx1, hy0, hy1 = 1, 5, 1, 4
    top = 6
    depth = {"ThroughAll": 6, "UpToFirst": 3.5, "UpToFace": 3.5, "UpToShape": 3.5}.get(mode, 3)
    zb = top - depth
    g.box(B[0], B[1], B[2], B[3], 0, top, pal=BASE)
    if mode in ("UpToFace", "UpToShape"):
        g.poly([(B[0], B[2], zb), (B[1], B[2], zb), (B[1], B[3], zb), (B[0], B[3], zb)], TARGET, stroke="#C99A00",
               sw=1.0, opacity=0.35, dash="4,3")
    g.poly([(hx0, hy0, top), (hx1, hy0, top), (hx1, hy1, top), (hx0, hy1, top)], CUT[2], sw=1.0)
    if mode != "ThroughAll":
        g.poly([(hx0, hy0, zb), (hx1, hy0, zb), (hx1, hy1, zb), (hx0, hy1, zb)], CUT[2], sw=1.0)
    g.poly([(hx0, hy0, top), (hx1, hy0, top), (hx1, hy0, zb), (hx0, hy0, zb)], CUT[1], sw=1.0)
    g.poly([(hx0, hy0, top), (hx0, hy1, top), (hx0, hy1, zb), (hx0, hy0, zb)], CUT[0], sw=1.0)
    # bordas frontais da abertura (por cima das paredes)
    g.line((hx0, hy1, top), (hx1, hy1, top), color=OUT, sw=1.2)
    g.line((hx1, hy0, top), (hx1, hy1, top), color=OUT, sw=1.2)
    g.profile(hx0, hx1, hy0, hy1, top, fill=False)
    ax, ay = B[1] + 2, B[2] - 1
    if mode == "ThroughAll":
        _dim(g, top, 0, ax, ay, "Atravessa tudo")
    elif mode in ("UpToFace", "UpToShape"):
        _dim(g, top, zb, ax, ay, "até a face" if mode == "UpToFace" else "até a forma")
    elif mode == "UpToFirst":
        _dim(g, top, zb, ax, ay, "até a 1ª face")
    elif midplane:
        _dim(g, top + 2, zb, ax, ay, "Profundidade", both=True, sub="simétrica ao esboço")
    elif mode == "TwoLengths":
        _dim(g, top + 2, zb, ax, ay, "Prof. 1 + 2", both=True)
    else:
        _dim(g, top, zb, ax, ay, "Profundidade")
    _legend(g, [(AMBER_FILL, "Esboço (perfil)"), (CUT[1], "Material removido")])
    return g


# --------------------------------------------------------------------------- Revolução / Ranhura
def revolution(mode="Angle", groove=False):
    g = Iso()
    cx = 150
    body = CUT if groove else SOLID
    r = 46
    _raw(g, '<path d="M %d 60 A %d 14 0 0 0 %d 60 L %d 120 A %d 14 0 0 0 %d 120 Z" fill="%s" stroke="%s" stroke-width="1.2"/>'
         % (cx - r, r, cx + r, cx + r, r, cx - r, body[1], OUT), (cx - r, 46), (cx + r, 134))
    _raw(g, '<ellipse cx="%d" cy="60" rx="%d" ry="14" fill="%s" stroke="%s" stroke-width="1.2"/>' % (cx, r, body[0], OUT))
    _raw(g, '<ellipse cx="%d" cy="60" rx="18" ry="5.5" fill="#FFFFFF" stroke="%s" stroke-width="1"/>' % (cx, OUT))
    _raw(g, '<rect x="%d" y="62" width="28" height="56" fill="%s" fill-opacity="0.9" stroke="%s" stroke-width="2.4"/>'
         % (cx + 18, AMBER_FILL, AMBER))
    _raw(g, '<line x1="%d" y1="22" x2="%d" y2="146" stroke="%s" stroke-width="1.4" stroke-dasharray="10,3,2,3"/>'
         % (cx, cx, NAVY), (cx, 22), (cx, 146))
    _raw(g, '<path d="M %d 34 A 64 14 0 1 0 %d 30" fill="none" stroke="%s" stroke-width="1.8"/>' % (cx + 40, cx - 40, NAVY),
         (cx - 66, 18), (cx + 66, 50))
    _raw(g, '<polygon points="%d,30 %d,25 %d,35" fill="%s"/>' % (cx - 40, cx - 49, cx - 48, NAVY))
    g.text(cx + 56, 30, "Ângulo" if mode in ("Angle", "TwoAngles") else "até a face", size=11)
    g.text(cx - 6, 150, "Eixo", size=10, anchor="end", weight="500")
    _legend(g, [(AMBER_FILL, "Esboço (perfil)"), (body[1], "Material removido" if groove else "Sólido criado"),
                (NAVY, "Eixo de revolução")])
    return g


# --------------------------------------------------------------------------- Furo
def hole(cut="None", threaded=False, drill="Angled"):
    g = Iso()
    x0, x1, top, bot = 70, 230, 40, 140
    cx, r, depth = 150, 16, 72
    tip = 12 if drill == "Angled" else 0
    _raw(g, '<rect x="%d" y="%d" width="%d" height="%d" fill="%s" stroke="%s" stroke-width="1.2"/>'
         % (x0, top, x1 - x0, bot - top, BASE[1], OUT), (x0, top), (x1, bot))
    if cut == "Counterbore":
        R, d = 28, 18
        pts = [(cx - R, top), (cx - R, top + d), (cx - r, top + d), (cx - r, top + depth), (cx, top + depth + tip),
               (cx + r, top + depth), (cx + r, top + d), (cx + R, top + d), (cx + R, top)]
    elif cut == "Countersink":
        R = 30
        pts = [(cx - R, top), (cx - r, top + (R - r)), (cx - r, top + depth), (cx, top + depth + tip),
               (cx + r, top + depth), (cx + r, top + (R - r)), (cx + R, top)]
    else:
        pts = [(cx - r, top), (cx - r, top + depth), (cx, top + depth + tip), (cx + r, top + depth), (cx + r, top)]
    _raw(g, '<polygon points="%s" fill="#FFFFFF" stroke="%s" stroke-width="1.4"/>' % (" ".join("%d,%d" % q for q in pts), OUT))
    if threaded:
        for yy in range(top + (26 if cut != "None" else 6), top + depth - 2, 6):
            _raw(g, '<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="%s" stroke-width="0.9"/>' % (cx - r, yy, cx - r + 5, yy + 3, NAVY))
            _raw(g, '<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="%s" stroke-width="0.9"/>' % (cx + r, yy, cx + r - 5, yy + 3, NAVY))
    _raw(g, '<line x1="%d" y1="30" x2="%d" y2="150" stroke="%s" stroke-width="1" stroke-dasharray="8,3,2,3"/>' % (cx, cx, NAVY))
    _raw(g, '<line x1="%d" y1="154" x2="%d" y2="154" stroke="%s" stroke-width="1.2"/>' % (cx - r, cx + r, NAVY))
    g.text(cx, 168, "Diâmetro", size=10, anchor="middle")
    _raw(g, '<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="%s" stroke-width="1.2"/>' % (x1 + 10, top, x1 + 10, top + depth, NAVY))
    g.text(x1 + 15, top + depth / 2 + 4, "Profundidade", size=10)
    label = {"Counterbore": "Rebaixo", "Countersink": "Escareado"}.get(cut, "Furo simples")
    g.text(x0, 30, label + (" · roscado" if threaded else "") + (" · ponta cônica" if tip else " · fundo plano"), size=11)
    _legend(g, [(BASE[1], "Peça (vista em corte)"), ("#FFFFFF", "Furo")])
    return g


# --------------------------------------------------------------------------- Arredondar / Chanfro
def fillet_chamfer(kind="Fillet", ctype="Equal distance"):
    g = Iso(scale=10)
    X0, X1, Y0, Y1, Z1 = 0, 9, 0, 6, 5
    c = 1.8
    top, left, right = SOLID
    if kind == "Fillet":
        n = 8
        arc = [(Y1 - c + c * math.sin(math.pi / 2 * i / n), Z1 - c + c * math.cos(math.pi / 2 * i / n)) for i in range(n + 1)]
        zl = Z1 - c
        edge = arc  # de (Y1-c, Z1) até (Y1, Z1-c)
        label = "Raio"
    else:
        c2 = c * (1.6 if ctype != "Equal distance" else 1.0)
        zl = Z1 - c2
        edge = [(Y1 - c, Z1), (Y1, Z1 - c2)]
        label = {"Two distances": "Distância 1 e 2", "Distance and Angle": "Distância e ângulo"}.get(ctype, "Distância")
    g.poly([(X0, Y1, 0), (X1, Y1, 0), (X1, Y1, zl), (X0, Y1, zl)], left)
    g.poly([(X1, Y0, 0), (X1, Y1, 0)] + [(X1, y, z) for y, z in reversed(edge)] + [(X1, Y0, Z1)], right)
    g.poly([(X0, Y0, Z1), (X1, Y0, Z1), (X1, Y1 - c, Z1), (X0, Y1 - c, Z1)], top)
    g.poly([(X0, y, z) for y, z in edge] + [(X1, y, z) for y, z in reversed(edge)], TARGET)
    g.arrow((X1 + 2.5, Y1 + 0.5, Z1 + 2), (X1 + 0.3, Y1 - 0.6, Z1 - 0.4))
    g.text3((X1 + 2.5, Y1 + 0.5, Z1 + 2), label, dx=4, dy=-2)
    _legend(g, [(TARGET, "Aresta selecionada"), (SOLID[0], "Peça")])
    return g


# --------------------------------------------------------------------------- Padrões / Espelhar
def pattern(kind="Linear"):
    if kind == "Polar":
        g = Iso()
        cx, cy, R = 150, 86, 56
        _raw(g, '<ellipse cx="%d" cy="%d" rx="%d" ry="%d" fill="none" stroke="%s" stroke-width="1" stroke-dasharray="4,3"/>'
             % (cx, cy, R, R * 0.5, PLANE_LINE), (cx - R - 12, cy - 40), (cx + R + 12, cy + 40))
        for i in range(6):
            a = 2 * math.pi * i / 6 + 0.3
            x, y = cx + R * math.cos(a), cy + R * 0.5 * math.sin(a)
            fill = SOLID[1] if i == 0 else COPY_PAL[1]
            _raw(g, '<rect x="%.1f" y="%.1f" width="18" height="18" rx="3" fill="%s" stroke="%s" stroke-width="1.1"/>'
                 % (x - 9, y - 13, fill, OUT))
        _raw(g, '<line x1="%d" y1="34" x2="%d" y2="140" stroke="%s" stroke-width="1.4" stroke-dasharray="10,3,2,3"/>' % (cx, cx, NAVY))
        _raw(g, '<path d="M %d %d A %d %d 0 0 1 %d %d" fill="none" stroke="%s" stroke-width="1.8"/>'
             % (cx + R + 8, cy, R + 8, (R + 8) * 0.5, cx, cy + (R + 8) * 0.5, NAVY))
        g.text(cx + 8, 30, "Eixo · Ângulo · Ocorrências", size=10)
    elif kind == "Linear":
        g = Iso(scale=7.5)
        g.plane(-2, 22, -2, 8, 0)
        for i in range(4):
            g.box(i * 5, i * 5 + 3, 1, 5, 0, 3, pal=SOLID if i == 0 else COPY_PAL)
        g.arrow((0, 7, 0), (18, 7, 0))
        g.text3((9, 7, 0), "Direção · Comprimento · Ocorrências", dx=-60, dy=22, size=10)
    else:
        g = Iso(scale=8.5)
        g.box(-9, -3, 0, 5, 0, 4)
        g.poly([(0, -2, -1), (0, 7, -1), (0, 7, 7), (0, -2, 7)], TARGET, stroke="#C99A00", sw=1.0, opacity=0.35, dash="5,3")
        g.box(3, 9, 0, 5, 0, 4, pal=COPY_PAL)
        g.text3((0, -2, 7), "Plano de espelho", dx=6, dy=-2)
    _legend(g, [(SOLID[1], "Original"), (COPY_PAL[1], "Cópias")])
    return g


# --------------------------------------------------------------------------- Casca / Ângulo de saída
def thickness():
    g = Iso(scale=9)
    X0, X1, Y0, Y1, Z1, t = 0, 9, 0, 6, 6, 1
    # paredes internas do fundo (visíveis pela abertura) e piso
    g.poly([(X0 + t, Y0 + t, t), (X1 - t, Y0 + t, t), (X1 - t, Y1 - t, t), (X0 + t, Y1 - t, t)], SOLID[0], sw=1.0)
    g.poly([(X0 + t, Y0 + t, Z1), (X1 - t, Y0 + t, Z1), (X1 - t, Y0 + t, t), (X0 + t, Y0 + t, t)], SOLID[2], sw=1.0)
    g.poly([(X0 + t, Y0 + t, Z1), (X0 + t, Y1 - t, Z1), (X0 + t, Y1 - t, t), (X0 + t, Y0 + t, t)], SOLID[1], sw=1.0)
    # faces externas da frente cobrem o que fica atrás
    g.poly([(X0, Y1, 0), (X1, Y1, 0), (X1, Y1, Z1), (X0, Y1, Z1)], SOLID[1])
    g.poly([(X1, Y0, 0), (X1, Y1, 0), (X1, Y1, Z1), (X1, Y0, Z1)], SOLID[2])
    # aro do topo (anel)
    o = [g.p(X0, Y0, Z1), g.p(X1, Y0, Z1), g.p(X1, Y1, Z1), g.p(X0, Y1, Z1)]
    i = [g.p(X0 + t, Y0 + t, Z1), g.p(X0 + t, Y1 - t, Z1), g.p(X1 - t, Y1 - t, Z1), g.p(X1 - t, Y0 + t, Z1)]
    d = "M " + " L ".join("%.1f %.1f" % q for q in o) + " Z M " + " L ".join("%.1f %.1f" % q for q in i) + " Z"
    g.parts.append('<path d="%s" fill="%s" fill-rule="evenodd" stroke="%s" stroke-width="1.2"/>' % (d, TARGET, OUT))
    g.text3((X1, Y0, Z1), "Face removida (aberta)", dx=8, dy=-4)
    g.text3((X1, Y1, 2.5), "Espessura da parede", dx=8)
    _legend(g, [(TARGET, "Face removida"), (SOLID[0], "Parede com espessura")])
    return g


def draft():
    g = Iso(scale=9)
    k = 1.4
    g.poly([(0, 6, 0), (9, 6, 0), (9, 6 - k, 6), (0, 6 - k, 6)], TARGET)
    g.poly([(9, 0, 0), (9, 6, 0), (9, 6 - k, 6), (9, k, 6)], SOLID[2])
    g.poly([(0, k, 6), (9, k, 6), (9, 6 - k, 6), (0, 6 - k, 6)], SOLID[0])
    g.line((0, 6, 0), (0, 6, 6.5), color=NAVY, sw=1.0, dash="4,3")
    g.text3((0, 6, 6.5), "Ângulo de saída", dx=-100, dy=-4)
    g.line((-1, 6, 0), (10.5, 6, 0), color=NAVY, sw=1.0, dash="8,3,2,3")
    g.text3((10.5, 6, 0), "Plano neutro", dx=6, dy=6)
    _legend(g, [(TARGET, "Face inclinada"), (SOLID[0], "Peça")])
    return g
