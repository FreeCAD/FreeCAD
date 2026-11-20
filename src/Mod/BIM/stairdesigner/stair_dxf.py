# SPDX-License-Identifier: LGPL-2.1-or-later

"""Write laid-out stock to a DXF drawing for the saw or the router.

Plain DXF R12, read by any CAM program. Each plank or panel is drawn one
above the other, in millimetres, on these layers: ``STOCK`` its outline,
``CUT`` the parts' outlines, ``POCKET_<depth>`` the housings to clear to
that depth from the upper face, ``BACK_POCKET_<depth>`` those to clear
from the underside with the part turned over, drawn as seen from above,
``LABEL`` the parts' names.
"""

# Gap between two stocks drawn one above the other.
STOCK_GAP = 100.0
LABEL_HEIGHT = 20.0


def _pocket_layer(depth, prefix="POCKET_"):
    return prefix + f"{depth:.2f}".rstrip("0").rstrip(".").replace(".", "_")


def _pairs(*items):
    return "".join(f"{code}\n{value}\n" for code, value in items)


def _polyline(layer, points):
    text = _pairs((0, "POLYLINE"), (8, layer), (66, 1), (70, 1), (10, 0.0), (20, 0.0), (30, 0.0))
    for x, y in points:
        text += _pairs((0, "VERTEX"), (8, layer), (10, f"{x:.3f}"), (20, f"{y:.3f}"), (30, 0.0))
    return text + _pairs((0, "SEQEND"), (8, layer))


def _text(x, y, value):
    value = value.replace("\n", " ")
    return _pairs(
        (0, "TEXT"),
        (8, "LABEL"),
        (10, f"{x:.3f}"),
        (20, f"{y:.3f}"),
        (30, 0.0),
        (40, LABEL_HEIGHT),
        (1, value),
        (72, 1),
        (73, 2),
        (11, f"{x:.3f}"),
        (21, f"{y:.3f}"),
        (31, 0.0),
    )


def stock_dxf(stocks):
    """Return the DXF text of ``stocks``, ``nesting.Stock`` laid out with
    ``FlatPiece`` parts, drawn one above the other."""

    entities = ""
    layers = {"STOCK": 8, "CUT": 7, "LABEL": 3}
    base = 0.0
    for stock in stocks:
        entities += _polyline(
            "STOCK",
            [
                (0.0, base),
                (stock.length, base),
                (stock.length, base + stock.width),
                (0.0, base + stock.width),
            ],
        )
        for placed in stock.placed:
            entities += _polyline("CUT", [(x, y + base) for x, y in placed.outline])
            for attribute, prefix in (("pockets", "POCKET_"), ("back_pockets", "BACK_POCKET_")):
                for depth, outline in getattr(placed.part, attribute, ()):
                    layer = _pocket_layer(depth, prefix)
                    layers.setdefault(layer, 1 + len(layers) % 6)
                    entities += _polyline(layer, [(x, y + base) for x, y in placed.map(outline)])
            xs = [x for x, _y in placed.outline]
            ys = [y for _x, y in placed.outline]
            entities += _text(
                (min(xs) + max(xs)) / 2.0,
                (min(ys) + max(ys)) / 2.0 + base,
                getattr(placed.part, "label", ""),
            )
        base += stock.width + STOCK_GAP

    tables = _pairs((0, "SECTION"), (2, "TABLES"), (0, "TABLE"), (2, "LAYER"), (70, len(layers)))
    for name, color in layers.items():
        tables += _pairs((0, "LAYER"), (2, name), (70, 0), (62, color), (6, "CONTINUOUS"))
    tables += _pairs((0, "ENDTAB"), (0, "ENDSEC"))
    header = _pairs(
        (0, "SECTION"),
        (2, "HEADER"),
        (9, "$ACADVER"),
        (1, "AC1009"),
        (9, "$INSUNITS"),
        (70, 4),
        (0, "ENDSEC"),
    )
    return (
        header
        + tables
        + _pairs((0, "SECTION"), (2, "ENTITIES"))
        + entities
        + _pairs((0, "ENDSEC"), (0, "EOF"))
    )
