# SPDX-License-Identifier: LGPL-2.1-or-later
"""Gera os modelos de folha do Desenho técnico (TechDraw) com a legenda Sework.

Uso:  python gerar_folhas.py <pasta_saida>

Campos com freecad:editable podem ser editados no FreeCAD (duplo clique na legenda).
Campos com freecad:autofill são preenchidos sozinhos: título, autor, data, escala e folha.
"""
import os
import sys

NAVY, YEL, INK, LABEL, SUB = "#003A70", "#F7BE00", "#1E2A38", "#46627F", "#C9DAEA"
FONT = "Poppins"

# (nome, largura, altura, margem esquerda, demais margens) em mm
SHEETS = [
    ("Sework_A4_Retrato", 210, 297, 25, 7),
    ("Sework_A4_Paisagem", 297, 210, 25, 7),
    ("Sework_A3_Paisagem", 420, 297, 25, 7),
    ("Sework_A2_Paisagem", 594, 420, 25, 7),
    ("Sework_A1_Paisagem", 841, 594, 25, 10),
]

LW = 178.0      # largura da legenda (mm)

# Ícone Sework (mesma peça isométrica do ícone do aplicativo), desenhado em 0..256
_ICON = None


def icon_svg(x, y, size):
    import math
    C, S = math.cos(math.radians(30)), math.sin(math.radians(30))
    k = 10.2
    faces = [
        ([(4, 0, 4), (10, 0, 4), (10, 6, 4), (4, 6, 4)], "#C7DAEE"),
        ([(0, 6, 0), (10, 6, 0), (10, 6, 4), (0, 6, 4)], "#8FB0D3"),
        ([(10, 0, 0), (10, 6, 0), (10, 6, 4), (10, 0, 4)], "#5F86B2"),
        ([(0, 6, 4), (4, 6, 4), (4, 6, 9), (0, 6, 9)], "#8FB0D3"),
        ([(4, 0, 4), (4, 6, 4), (4, 6, 9), (4, 0, 9)], "#5F86B2"),
        ([(0, 0, 9), (4, 0, 9), (4, 6, 9), (0, 6, 9)], YEL),
    ]
    P = lambda a, b, c: ((a - b) * C * k, (a + b) * S * k - c * k)
    pts = [P(*v) for f, _ in faces for v in f]
    minx, maxx = min(p[0] for p in pts), max(p[0] for p in pts)
    miny, maxy = min(p[1] for p in pts), max(p[1] for p in pts)
    sc = size / max(maxx - minx, maxy - miny)
    ox = x + (size - (maxx - minx) * sc) / 2 - minx * sc
    oy = y + (size - (maxy - miny) * sc) / 2 - miny * sc
    out = []
    for f, col in faces:
        q = " ".join("%.2f,%.2f" % (ox + P(*v)[0] * sc, oy + P(*v)[1] * sc) for v in f)
        out.append('<polygon points="%s" fill="%s" stroke="%s" stroke-width="0.25" stroke-linejoin="round"/>'
                   % (q, col, INK))
    return "".join(out)


def text(x, y, s, size, color=INK, weight="400", anchor="start", editable=None, autofill=None, ident=None):
    attrs = ""
    if ident:
        attrs += ' id="%s"' % ident
    if editable:
        attrs += ' freecad:editable="%s"' % editable
    if autofill:
        attrs += ' freecad:autofill="%s"' % autofill
    return ('<text x="%.2f" y="%.2f"%s style="font-family:%s;font-size:%.2fpx;font-weight:%s;fill:%s;'
            'text-anchor:%s"><tspan>%s</tspan></text>' % (x, y, attrs, FONT, size, weight, color, anchor, s))


def cell(x, y, w, h, label, field=None, default="", autofill=None, size=3.0, anchor="start"):
    """Célula com rótulo pequeno em cima e valor editável embaixo."""
    out = ['<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="none" stroke="%s" stroke-width="0.25"/>'
           % (x, y, w, h, INK)]
    out.append(text(x + 1.2, y + 2.6, label, 1.8, LABEL, "500"))
    if field:
        tx = x + 1.6 if anchor == "start" else x + w / 2
        out.append(text(tx, y + h - 2.0, default, size, INK, "400", anchor, editable=field, autofill=autofill,
                        ident=field + "_campo"))
    return "".join(out)


def legend(x0, y0):
    """Legenda Sework com 178 x 56 mm; (x0, y0) = canto superior esquerdo."""
    o = []
    # cabeçalho
    o.append('<rect x="%.2f" y="%.2f" width="108" height="12" fill="%s"/>' % (x0, y0, NAVY))
    o.append(icon_svg(x0 + 1.5, y0 + 1.2, 9.6))
    o.append(text(x0 + 13, y0 + 6.2, "SEWORK ENGENHARIA", 4.0, "#FFFFFF", "700"))
    o.append(text(x0 + 13, y0 + 9.8, "Engenharia mecânica e segurança do trabalho · Patos de Minas/MG", 1.9, SUB, "400"))
    o.append(cell(x0 + 108, y0, 70, 12, "ART Nº", "art", "—", size=3.4))
    o.append('<rect x="%.2f" y="%.2f" width="178" height="0.9" fill="%s"/>' % (x0, y0 + 12, YEL))
    y = y0 + 12.9
    # linha 1
    o.append(cell(x0, y, 108, 10, "TÍTULO", "titulo", "Título do desenho", autofill="title", size=3.6))
    o.append(cell(x0 + 108, y, 50, 10, "Nº DO DESENHO", "numero", "SW-0000"))
    o.append(cell(x0 + 158, y, 20, 10, "REV.", "revisao", "0", anchor="middle"))
    y += 10
    # linha 2
    o.append(cell(x0, y, 70, 10, "CLIENTE", "cliente", "—"))
    o.append(cell(x0 + 70, y, 108, 10, "PROJETO / OBRA", "projeto", "—"))
    y += 10
    # linha 3
    o.append(cell(x0, y, 70, 10, "MATERIAL", "material", "—"))
    o.append(cell(x0 + 70, y, 38, 10, "TOLERÂNCIA GERAL", "tolerancia", "—"))
    o.append(cell(x0 + 108, y, 35, 10, "ESCALA", "escala", "1 : 1", autofill="scale", anchor="middle"))
    o.append(cell(x0 + 143, y, 35, 10, "FOLHA", "folha", "1 / 1", autofill="sheet", anchor="middle"))
    y += 10
    # linha 4 (responsáveis)
    h = 56 - (y - y0)
    o.append(cell(x0, y, 40, h, "DESENHOU", "desenhou", "—", autofill="author", size=2.8))
    o.append(cell(x0 + 40, y, 40, h, "VERIFICOU", "verificou", "—", size=2.8))
    o.append('<rect x="%.2f" y="%.2f" width="58" height="%.2f" fill="none" stroke="%s" stroke-width="0.25"/>'
             % (x0 + 80, y, h, INK))
    o.append(text(x0 + 81.2, y + 2.6, "RESPONSÁVEL TÉCNICO", 1.8, LABEL, "500"))
    o.append(text(x0 + 81.6, y + h - 6.2, "Eng. Nome Sobrenome", 2.8, INK, editable="responsavel", ident="responsavel_campo"))
    o.append(text(x0 + 81.6, y + h - 2.2, "CREA —", 2.4, LABEL, editable="crea", ident="crea_campo"))
    o.append(cell(x0 + 138, y, 40, h, "DATA", "data", "dd/mm/aaaa", autofill="date", anchor="middle", size=2.8))
    # contorno externo mais forte
    o.append('<rect x="%.2f" y="%.2f" width="178" height="56" fill="none" stroke="%s" stroke-width="0.5"/>'
             % (x0, y0, INK))
    return "".join(o)


def revisions(x0, y0):
    """Quadro de revisões (3 linhas) logo acima da legenda; (x0, y0) = canto superior esquerdo."""
    cols = [("REV.", 12), ("DESCRIÇÃO", 110), ("DATA", 28), ("POR", 28)]
    o = []
    rh = 5.0
    # cabeçalho
    x = x0
    for name, w in cols:
        o.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="#EEF2F8" stroke="%s" stroke-width="0.25"/>'
                 % (x, y0 + 3 * rh, w, rh, INK))
        o.append(text(x + w / 2, y0 + 3 * rh + 3.4, name, 1.9, NAVY, "600", "middle"))
        x += w
    for r in range(3):
        x = x0
        yy = y0 + (2 - r) * rh
        for i, (name, w) in enumerate(cols):
            o.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="none" stroke="%s" stroke-width="0.25"/>'
                     % (x, yy, w, rh, INK))
            key = "rev%d_%s" % (r + 1, ["num", "desc", "data", "por"][i])
            anchor = "start" if i == 1 else "middle"
            tx = x + 1.5 if i == 1 else x + w / 2
            o.append(text(tx, yy + 3.5, "", 2.4, INK, "400", anchor, editable=key, ident=key + "_campo"))
            x += w
    o.append('<rect x="%.2f" y="%.2f" width="178" height="%.2f" fill="none" stroke="%s" stroke-width="0.5"/>'
             % (x0, y0, 4 * rh, INK))
    return "".join(o)


def first_angle(x, y):
    """Símbolo de projeção no 1º diedro, em caixa 22 x 13 mm."""
    o = ['<rect x="%.2f" y="%.2f" width="22" height="13" fill="#FFFFFF" stroke="%s" stroke-width="0.25"/>' % (x, y, INK)]
    o.append(text(x + 11, y + 2.6, "1º DIEDRO", 1.8, LABEL, "600", "middle"))
    cy = y + 7.6
    o.append('<path d="M %.2f %.2f L %.2f %.2f L %.2f %.2f L %.2f %.2f Z" fill="none" stroke="%s" stroke-width="0.35"/>'
             % (x + 3, cy - 1.6, x + 9, cy - 3.2, x + 9, cy + 3.2, x + 3, cy + 1.6, INK))
    o.append('<circle cx="%.2f" cy="%.2f" r="3.2" fill="none" stroke="%s" stroke-width="0.35"/>' % (x + 15.5, cy, INK))
    o.append('<circle cx="%.2f" cy="%.2f" r="1.6" fill="none" stroke="%s" stroke-width="0.35"/>' % (x + 15.5, cy, INK))
    return "".join(o)


def sheet(name, W, H, ml, m):
    L, T, R, B = ml, m, W - m, H - m
    o = ['<?xml version="1.0" encoding="UTF-8" standalone="no"?>',
         '<svg xmlns="http://www.w3.org/2000/svg" xmlns:freecad="http://www.freecad.org/wiki/index.php?title=Svg_Namespace" '
         'width="%dmm" height="%dmm" viewBox="0 0 %d %d" version="1.1">' % (W, H, W, H),
         '<!-- Sework Engenharia - modelo de folha %s (%d x %d mm) -->' % (name, W, H),
         '<!-- Working space %d %d %d %d -->' % (L, T, R, B - 56 - 20 - 2)]
    # moldura
    o.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="none" stroke="%s" stroke-width="0.7"/>'
             % (L, T, R - L, B - T, INK))
    # marcas de centro
    for (x1, y1, x2, y2) in ((W / 2, 0, W / 2, T + 4), (W / 2, B - 4, W / 2, H), (0, H / 2, L + 4, H / 2), (R - 4, H / 2, W, H / 2)):
        o.append('<path d="M %.2f %.2f L %.2f %.2f" stroke="%s" stroke-width="0.5"/>' % (x1, y1, x2, y2, INK))
    # legenda, revisões e diedro
    lx, ly = R - LW, B - 56
    o.append(revisions(lx, ly - 20 - 1.5))
    o.append(legend(lx, ly))
    o.append(first_angle(lx - 24, B - 13) if lx - 24 > L + 2 else "")
    o.append('</svg>')
    return "\n".join(o)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    os.makedirs(out, exist_ok=True)
    for name, W, H, ml, m in SHEETS:
        with open(os.path.join(out, name + ".svg"), "w", encoding="utf-8") as f:
            f.write(sheet(name, W, H, ml, m))
        print("ok", name)


if __name__ == "__main__":
    main()
