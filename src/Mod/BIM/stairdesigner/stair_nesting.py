# SPDX-License-Identifier: LGPL-2.1-or-later

"""Lay flat parts out on stock: planks or panels of one material.

Every part is a 2D outline with its grain along X. Parts keep the grain
along the stock's length, so they may only be turned half a turn. Tapered
parts, the winder treads and the strips they are glued up from, are first
paired head to tail where the pair takes notably less stock than the two
apart. Parts and pairs are then packed by their rectangles, plank after
plank, keeping a cut spacing between them. Pure geometry: no document.
"""

import math
from dataclasses import dataclass, field

# A part filling less of its rectangle than this is worth pairing.
_TAPERED_FILL = 0.8
# A pair is kept when it saves at least this share of the two rectangles.
_PAIR_SAVING = 0.1


def polygon_area(points):
    area = 0.0
    for (x0, y0), (x1, y1) in zip(points, points[1:] + points[:1]):
        area += x0 * y1 - x1 * y0
    return abs(area) / 2.0


def _bounds(points):
    xs = [x for x, _y in points]
    ys = [y for _x, y in points]
    return min(xs), min(ys), max(xs), max(ys)


def turned(points):
    """Return ``points`` turned half a turn, back in the positive quadrant."""

    x0, y0, x1, y1 = _bounds(points)
    return [(x1 + x0 - x, y1 + y0 - y) for x, y in points]


def _x_at(points, y, rightmost):
    """Return the extreme X where the outline crosses the level ``y``."""

    found = None
    for (x0, y0), (x1, y1) in zip(points, points[1:] + points[:1]):
        low, high = min(y0, y1), max(y0, y1)
        if y < low - 1e-9 or y > high + 1e-9:
            continue
        if high - low < 1e-12:
            candidates = (x0, x1)
        else:
            t = (y - y0) / (y1 - y0)
            candidates = (x0 + t * (x1 - x0),)
        for x in candidates:
            if found is None or (x > found if rightmost else x < found):
                found = x
    return found


def _slide(left, right, shift_y, spacing):
    """Return how far ``right``, raised by ``shift_y``, must sit from
    ``left`` along X to keep ``spacing`` from it.

    Both outlines are piecewise straight, so the gap between them changes
    slope only at their vertices: checking at every vertex level suffices.
    """

    raised = [(x, y + shift_y) for x, y in right]
    levels = sorted({y for _x, y in left} | {y for _x, y in raised})
    low = max(min(y for _x, y in left), min(y for _x, y in raised))
    high = min(max(y for _x, y in left), max(y for _x, y in raised))
    needed = None
    for y in levels:
        if y < low - 1e-9 or y > high + 1e-9:
            continue
        a = _x_at(left, y, True)
        b = _x_at(raised, y, False)
        if a is None or b is None:
            continue
        if needed is None or a - b > needed:
            needed = a - b
    if needed is None:
        return None
    x = needed + spacing
    # Across a slanted joint the gap along X is wider than the gap square
    # to the cut: move on until the cut itself has its spacing.
    for _attempt in range(8):
        gap = _distance(left, [(px + x, py) for px, py in raised])
        if gap >= spacing - 1e-6:
            break
        x += (spacing - gap) * 1.5 + 1e-3
    return x


def _point_segment(point, start, end):
    dx, dy = end[0] - start[0], end[1] - start[1]
    length = dx * dx + dy * dy
    t = 0.0
    if length > 1e-18:
        t = max(0.0, min(1.0, ((point[0] - start[0]) * dx + (point[1] - start[1]) * dy) / length))
    return math.hypot(point[0] - start[0] - t * dx, point[1] - start[1] - t * dy)


def _distance(first, second):
    """Return the distance between two outlines that do not cross."""

    edges_a = list(zip(first, first[1:] + first[:1]))
    edges_b = list(zip(second, second[1:] + second[:1]))
    return min(
        min(
            _point_segment(a0, b0, b1),
            _point_segment(a1, b0, b1),
            _point_segment(b0, a0, a1),
            _point_segment(b1, a0, a1),
        )
        for a0, a1 in edges_a
        for b0, b1 in edges_b
    )


@dataclass
class Placed:
    """A part laid on a stock: its outline there, whether it was turned
    half a turn, and the move from its own outline's place to the stock."""

    part: object
    outline: list
    turned: bool
    source_bounds: tuple
    offset: tuple

    def map(self, points):
        """Carry points given like the part's own outline onto the stock."""

        x0, y0, x1, y1 = self.source_bounds
        dx, dy = self.offset
        if self.turned:
            return [(x1 + x0 - x + dx, y1 + y0 - y + dy) for x, y in points]
        return [(x + dx, y + dy) for x, y in points]


@dataclass
class _Item:
    width: float
    height: float
    members: list  # (part, outline in the item, turned)


@dataclass
class Stock:
    length: float
    width: float
    placed: list = field(default_factory=list)

    @property
    def used(self):
        """Share of the stock's area the parts cover."""

        area = sum(polygon_area(item.outline) for item in self.placed)
        return area / (self.length * self.width) if self.length * self.width else 0.0


def _single(part, outline):
    x0, y0, x1, y1 = _bounds(outline)
    shifted = [(x - x0, y - y0) for x, y in outline]
    return _Item(x1 - x0, y1 - y0, [(part, shifted, False)])


def _pair(first, second, spacing):
    """Return the tightest item of ``first`` with ``second`` beside it."""

    def swap(points):
        return [(y, x) for x, y in points]

    best = None
    a = first.members[0][1]
    for turn in (True, False):
        b = turned(second.members[0][1]) if turn else second.members[0][1]
        # End to end along the grain, or side by side across it: a winder
        # tapers across its nosing, so its turned twin fits along its back.
        candidates = []
        for shift_y in (0.0, first.height - second.height):
            x = _slide(a, b, shift_y, spacing)
            if x is not None:
                candidates.append((x, shift_y))
        for shift_x in (0.0, first.width - second.width):
            y = _slide(swap(a), swap(b), shift_x, spacing)
            if y is not None:
                candidates.append((shift_x, y))
        for x, shift_y in candidates:
            placed_b = [(px + x, py + shift_y) for px, py in b]
            x0, y0, x1, y1 = _bounds(a + placed_b)
            width, height = x1 - x0, y1 - y0
            if best is None or width * height < best.width * best.height - 1e-6:
                best = _Item(
                    width,
                    height,
                    [
                        (first.members[0][0], [(px - x0, py - y0) for px, py in a], False),
                        (
                            second.members[0][0],
                            [(px - x0, py - y0) for px, py in placed_b],
                            turn,
                        ),
                    ],
                )
    return best


def _pair_tapered(items, spacing, length, width):
    """Pair tapered parts head to tail where it saves stock."""

    tapered = [
        item
        for item in items
        if polygon_area(item.members[0][1]) < _TAPERED_FILL * item.width * item.height
    ]
    options = []
    for i, first in enumerate(tapered):
        for second in tapered[i + 1 :]:
            for one, two in ((first, second), (second, first)):
                pair = _pair(one, two, spacing)
                if pair is None or pair.width > length + 1e-6 or pair.height > width + 1e-6:
                    continue
                apart = one.width * one.height + two.width * two.height
                saving = apart - pair.width * pair.height
                if saving > _PAIR_SAVING * apart:
                    options.append((saving, id(one), id(two), one, two, pair))
    options.sort(key=lambda option: option[0], reverse=True)
    used = set()
    pairs = []
    for _saving, one_id, two_id, _one, _two, pair in options:
        if one_id in used or two_id in used:
            continue
        used.update((one_id, two_id))
        pairs.append(pair)
    return [item for item in items if id(item) not in used] + pairs


class _MaxRects:
    """Free rectangles of one stock; parts go to the best short-side fit."""

    def __init__(self, length, width):
        self.free = [(0.0, 0.0, length, width)]

    def find(self, w, h):
        best = None
        for x, y, fw, fh in self.free:
            if w <= fw + 1e-6 and h <= fh + 1e-6:
                score = (min(fw - w, fh - h), max(fw - w, fh - h), x, y)
                if best is None or score < best[0]:
                    best = (score, x, y)
        return None if best is None else best[1:]

    def take(self, x, y, w, h):
        result = []
        for fx, fy, fw, fh in self.free:
            if x >= fx + fw or x + w <= fx or y >= fy + fh or y + h <= fy:
                result.append((fx, fy, fw, fh))
                continue
            if x > fx:
                result.append((fx, fy, x - fx, fh))
            if x + w < fx + fw:
                result.append((x + w, fy, fx + fw - x - w, fh))
            if y > fy:
                result.append((fx, fy, fw, y - fy))
            if y + h < fy + fh:
                result.append((fx, y + h, fw, fy + fh - y - h))
        result = [rect for rect in result if rect[2] > 1e-6 and rect[3] > 1e-6]
        self.free = [
            rect
            for i, rect in enumerate(result)
            if not any(
                j != i
                and other[0] <= rect[0] + 1e-9
                and other[1] <= rect[1] + 1e-9
                and other[0] + other[2] >= rect[0] + rect[2] - 1e-9
                and other[1] + other[3] >= rect[1] + rect[3] - 1e-9
                and (other != rect or j < i)
                for j, other in enumerate(result)
            )
        ]


def lay_out(parts, length, width, spacing=0.0, fillers=()):
    """Lay ``parts`` out on as few stocks ``length`` by ``width`` as it can.

    ``parts`` are ``(part, outline)`` with the outline's grain along X.
    ``fillers``, given alike, only fill the room the parts leave on their
    stocks, without a stock of their own, unless there are no parts.
    Returns ``(stocks, unplaced)``: the ``Stock`` list, each with its
    ``Placed`` parts in stock coordinates, and the parts too large for it
    or the fillers finding no room.
    """

    parts = list(parts)
    fillers = list(fillers)
    if not parts:
        parts, fillers = fillers, []
    sources = {id(part): outline for part, outline in parts + fillers}

    def items_of(pieces, unplaced):
        items = [_single(part, outline) for part, outline in pieces if len(outline) >= 3]
        unplaced.extend(
            item.members[0][0]
            for item in items
            if item.width > length + 1e-6 or item.height > width + 1e-6
        )
        items = [
            item for item in items if item.width <= length + 1e-6 and item.height <= width + 1e-6
        ]
        return _pair_tapered(items, spacing, length, width)

    unplaced = []
    items = items_of(parts, unplaced)
    best = None
    # No order suits every stock: widest first fills panels, longest first
    # fills planks. Keep the one needing the fewest stocks, then leaving
    # the most of the last one free.
    for key in (
        lambda item: (round(item.height, 3), round(item.width, 3)),
        lambda item: (round(item.width, 3), round(item.height, 3)),
        lambda item: round(item.width * item.height, 3),
    ):
        stocks, bins, _left = _pack(
            sorted(items, key=key, reverse=True), length, width, spacing, sources
        )
        score = (len(stocks), stocks[-1].used if stocks else 0.0)
        if best is None or score < best[0]:
            best = (score, stocks, bins)
    stocks, bins = (best[1], best[2]) if best else ([], [])
    if fillers:
        filler_items = items_of(fillers, unplaced)
        filler_items.sort(key=lambda item: item.width * item.height, reverse=True)
        stocks, bins, left = _pack(
            filler_items, length, width, spacing, sources, stocks, bins, open_new=False
        )
        unplaced.extend(part for item in left for part, _outline, _turned in item.members)
    return stocks, unplaced


def _pack(items, length, width, spacing, sources, stocks=None, bins=None, open_new=True):
    stocks = list(stocks or [])
    bins = list(bins or [])
    left = []
    for item in items:
        # The spacing goes between parts, not along the stock's edges.
        w, h = item.width + spacing, item.height + spacing
        spot = None
        for stock, free in zip(stocks, bins):
            spot = free.find(w, h)
            if spot is not None:
                break
        if spot is None:
            if not open_new:
                left.append(item)
                continue
            stock = Stock(length, width)
            free = _MaxRects(length + spacing, width + spacing)
            spot = free.find(w, h)
            stocks.append(stock)
            bins.append(free)
        x, y = spot
        free.take(x, y, w, h)
        for part, outline, was_turned in item.members:
            final = [(px + x, py + y) for px, py in outline]
            source = sources[id(part)]
            start = turned(source)[0] if was_turned else source[0]
            stock.placed.append(
                Placed(
                    part,
                    final,
                    was_turned,
                    _bounds(source),
                    (final[0][0] - start[0], final[0][1] - start[1]),
                )
            )
    return stocks, bins, left
