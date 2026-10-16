#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Gera o tema de ícones "Sework Sóbrio" a partir dos SVGs originais do FreeCAD.

Uso: python3 gerar_tema_icones.py <pasta com o código do FreeCAD> <pasta de saída>

- recolore cada família de cor para a paleta Sework (cores sóbrias)
- contornos escuros viram ardósia (#1E2A38)
- todo preenchimento chapado ganha degradê vertical (efeito 3D)
O resultado é uma pasta plana de SVGs com o mesmo nome dos originais, que o
FreeCAD encontra antes dos ícones embutidos quando fica em <dados do usuário>/icons.
"""
import colorsys
import glob
import os
import re
import sys

from lxml import etree

SVG_NS = "http://www.w3.org/2000/svg"
XLINK = "http://www.w3.org/1999/xlink"
STROKE_DARK = "#1E2A38"

NAMED = {"black": "#000000", "white": "#ffffff", "red": "#ff0000", "green": "#008000", "blue": "#0000ff",
         "yellow": "#ffff00", "gray": "#808080", "grey": "#808080", "orange": "#ffa500", "cyan": "#00ffff",
         "magenta": "#ff00ff", "lime": "#00ff00", "navy": "#000080", "silver": "#c0c0c0", "maroon": "#800000",
         "purple": "#800080", "teal": "#008080", "olive": "#808000", "darkgray": "#a9a9a9", "lightgray": "#d3d3d3"}

HEX_RE = re.compile(r"#([0-9a-fA-F]{6}|[0-9a-fA-F]{3})\b")


def norm(c):
    c = c.strip().lower()
    if c in NAMED:
        return NAMED[c]
    m = HEX_RE.fullmatch(c)
    if not m:
        m2 = re.fullmatch(r"rgb\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)", c)
        if m2:
            return "#%02x%02x%02x" % tuple(int(x) for x in m2.groups())
        return None
    h = m.group(1)
    if len(h) == 3:
        h = "".join(ch * 2 for ch in h)
    return "#" + h


def to_hls(hx):
    r, g, b = (int(hx[i:i + 2], 16) / 255 for i in (1, 3, 5))
    return colorsys.rgb_to_hls(r, g, b)


def from_hls(h, l, s):
    r, g, b = colorsys.hls_to_rgb(h % 1.0, max(0, min(1, l)), max(0, min(1, s)))
    return "#%02x%02x%02x" % (round(r * 255), round(g * 255), round(b * 255))


def recolor(hx):
    """Mapeia uma cor para a paleta sóbria Sework, preservando a luminosidade."""
    h, l, s = to_hls(hx)
    deg = h * 360
    if s < 0.12 or l > 0.96 or l < 0.04:          # cinzas, branco, preto
        return from_hls(212 / 360, l, min(s, 0.08))
    if 40 <= deg < 72:                            # amarelo -> âmbar Sework
        return from_hls(44 / 360, min(max(l, 0.50), 0.68), 0.86)
    if 20 <= deg < 40:                            # laranja -> âmbar escuro
        return from_hls(36 / 360, l, 0.62)
    if deg >= 335 or deg < 20:                    # vermelho -> tijolo
        return from_hls(7 / 360, l, 0.46)
    if 72 <= deg < 175:                           # verde -> sálvia
        return from_hls(168 / 360, l, 0.24)
    if 175 <= deg < 265:                          # azul/ciano -> azul-aço
        return from_hls(209 / 360, l, 0.42)
    return from_hls(216 / 360, l, 0.22)           # roxo/magenta -> ardósia


def stroke_color(hx):
    h, l, s = to_hls(hx)
    if l < 0.28:
        return STROKE_DARK
    return recolor(hx)


def shade(hx, dl):
    h, l, s = to_hls(hx)
    return from_hls(h, l + dl, s)


SHAPES = {"path", "rect", "circle", "ellipse", "polygon"}


def parse_style(st):
    out = {}
    for part in st.split(";"):
        if ":" in part:
            k, v = part.split(":", 1)
            out[k.strip()] = v.strip()
    return out


def dump_style(d):
    return ";".join("%s:%s" % (k, v) for k, v in d.items())


def process(src, dst):
    parser = etree.XMLParser(remove_blank_text=False, recover=True, huge_tree=True)
    tree = etree.parse(src, parser)
    root = tree.getroot()
    if root is None or not root.tag.endswith("svg"):
        return False
    defs = root.find("{%s}defs" % SVG_NS)
    if defs is None:
        defs = etree.Element("{%s}defs" % SVG_NS)
        root.insert(0, defs)
    counter = [0]
    grad_cache = {}

    def gradient_for(color):
        if color in grad_cache:
            return grad_cache[color]
        counter[0] += 1
        gid = "swg%d" % counter[0]
        g = etree.SubElement(defs, "{%s}linearGradient" % SVG_NS, id=gid, x1="0", y1="0", x2="0", y2="1")
        etree.SubElement(g, "{%s}stop" % SVG_NS, offset="0", style="stop-color:%s;stop-opacity:1" % shade(color, 0.11))
        etree.SubElement(g, "{%s}stop" % SVG_NS, offset="1", style="stop-color:%s;stop-opacity:1" % shade(color, -0.12))
        grad_cache[color] = gid
        return gid

    for el in root.iter():
        if not isinstance(el.tag, str):
            continue
        local = el.tag.split("}")[-1]
        # atributos diretos
        for attr in ("fill", "stroke", "stop-color", "flood-color", "color"):
            v = el.get(attr)
            if v:
                c = norm(v)
                if c:
                    el.set(attr, stroke_color(c) if attr == "stroke" else recolor(c))
        st = el.get("style")
        if st:
            d = parse_style(st)
            for k in ("fill", "stroke", "stop-color", "flood-color", "color"):
                if k in d:
                    c = norm(d[k])
                    if c:
                        d[k] = stroke_color(c) if k == "stroke" else recolor(c)
            el.set("style", dump_style(d))
        # efeito 3D: preenchimento chapado -> degradê vertical
        if local in SHAPES and not _inside_defs(el, defs):
            d = parse_style(el.get("style", ""))
            fill = d.get("fill", el.get("fill"))
            c = norm(fill) if fill else None
            if c:
                h, l, s = to_hls(c)
                if 0.12 < l < 0.93:
                    gid = gradient_for(c)
                    if "fill" in d:
                        d["fill"] = "url(#%s)" % gid
                        el.set("style", dump_style(d))
                    else:
                        el.set("fill", "url(#%s)" % gid)
    tree.write(dst, xml_declaration=True, encoding="UTF-8")
    return True


def _inside_defs(el, defs):
    p = el.getparent()
    while p is not None:
        if p is defs or (isinstance(p.tag, str) and p.tag.endswith("defs")):
            return True
        p = p.getparent()
    return False


SKIP_DIRS = ("Stylesheets", "/Tux/", "/Test/")


def main():
    src_root, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    files = [p for p in glob.glob(os.path.join(src_root, "src", "**", "*.svg"), recursive=True)
             if re.search(r"[/\\][Ii]cons[/\\]", p) and not any(s in p for s in SKIP_DIRS)]
    seen, ok, fail = set(), 0, 0
    for p in sorted(files):
        name = os.path.basename(p)
        if name in seen:
            continue
        seen.add(name)
        try:
            if process(p, os.path.join(out, name)):
                ok += 1
        except Exception as exc:  # noqa
            fail += 1
            print("falhou:", p, exc)
    print("ícones gerados:", ok, "falhas:", fail)


if __name__ == "__main__":
    main()
