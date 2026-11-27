# SPDX-License-Identifier: LGPL-2.1-or-later

"""Fair curves for board edges: a batten sprung over pins.

A joiner sets out the edge of a string over winders by pinning the margin
above every nosing and springing a thin batten against the pins: the batten
takes the smoothest line that clears them all, and where the pins are uneven
it stands off some of them rather than wobble. ``fair_curve`` does the same
with a cubic spline, the curve of least bending.
"""

from typing import NamedTuple


class Pin(NamedTuple):
    """A point a fair curve is sprung over.

    A ``hard`` pin holds the curve up: it never passes under ``value`` at
    ``u``. Each pin also draws the curve to itself, as strongly as its
    relative ``weight``; a weightless soft pin only places a knot.
    """

    u: float
    value: float
    hard: bool = True
    weight: float = 1.0


def _solve(matrix, rhs):
    """Solve a small dense linear system by Gaussian elimination."""

    size = len(rhs)
    rows = [list(row) + [value] for row, value in zip(matrix, rhs)]
    for column in range(size):
        pivot = max(range(column, size), key=lambda row: abs(rows[row][column]))
        if abs(rows[pivot][column]) < 1e-300:
            raise ValueError("singular fairing system")
        rows[column], rows[pivot] = rows[pivot], rows[column]
        lead = rows[column]
        for row in rows[column + 1 :]:
            factor = row[column] / lead[column]
            if factor:
                for index in range(column, size + 1):
                    row[index] -= factor * lead[index]
    solution = [0.0] * size
    for row in range(size - 1, -1, -1):
        solution[row] = (
            rows[row][size]
            - sum(rows[row][index] * solution[index] for index in range(row + 1, size))
        ) / rows[row][row]
    return solution


def _moments(knots, values, start_slope=None, end_slope=None):
    """Return the second derivatives of the cubic spline through the knots.

    An end without a slope is natural: the curve leaves it straight.
    """

    count = len(knots)
    spans = [second - first for first, second in zip(knots, knots[1:])]
    pitches = [(second - first) / span for first, second, span in zip(values, values[1:], spans)]
    lower = [0.0] * count
    diagonal = [1.0] * count
    upper = [0.0] * count
    rhs = [0.0] * count
    for index in range(1, count - 1):
        lower[index] = spans[index - 1]
        diagonal[index] = 2.0 * (spans[index - 1] + spans[index])
        upper[index] = spans[index]
        rhs[index] = 6.0 * (pitches[index] - pitches[index - 1])
    if start_slope is not None:
        diagonal[0] = 2.0 * spans[0]
        upper[0] = spans[0]
        rhs[0] = 6.0 * (pitches[0] - start_slope)
    if end_slope is not None:
        lower[-1] = spans[-1]
        diagonal[-1] = 2.0 * spans[-1]
        rhs[-1] = 6.0 * (end_slope - pitches[-1])
    # Thomas algorithm.
    for index in range(1, count):
        factor = lower[index] / diagonal[index - 1]
        diagonal[index] -= factor * upper[index - 1]
        rhs[index] -= factor * rhs[index - 1]
    moments = [0.0] * count
    moments[-1] = rhs[-1] / diagonal[-1]
    for index in range(count - 2, -1, -1):
        moments[index] = (rhs[index] - upper[index] * moments[index + 1]) / diagonal[index]
    return moments


class FairCurve:
    """A height ``z(u)``: a cubic spline, running on straight past its ends."""

    def __init__(self, knots, values, moments):
        self.knots = [float(knot) for knot in knots]
        self.values = [float(value) for value in values]
        self.moments = [float(moment) for moment in moments]

    @classmethod
    def line(cls, start, end, slope, intercept):
        """Return the straight pitch ``intercept + slope * u``."""

        return cls(
            [start, end],
            [intercept + slope * start, intercept + slope * end],
            [0.0, 0.0],
        )

    @classmethod
    def through(cls, points, start_slope=None, end_slope=None):
        """Return the spline through ``points``, sorted ``(u, z)`` pairs."""

        knots = [u for u, _z in points]
        values = [z for _u, z in points]
        return cls(knots, values, _moments(knots, values, start_slope, end_slope))

    def negated(self):
        return FairCurve(
            self.knots,
            [-value for value in self.values],
            [-moment for moment in self.moments],
        )

    def _span(self, u):
        low, high = 0, len(self.knots) - 2
        while low < high:
            middle = (low + high + 1) // 2
            if self.knots[middle] <= u:
                low = middle
            else:
                high = middle - 1
        return low

    def _end_slope(self, at_end):
        knots, values, moments = self.knots, self.values, self.moments
        if at_end:
            span = knots[-1] - knots[-2]
            return (values[-1] - values[-2]) / span + span * (2.0 * moments[-1] + moments[-2]) / 6.0
        span = knots[1] - knots[0]
        return (values[1] - values[0]) / span - span * (2.0 * moments[0] + moments[1]) / 6.0

    def value(self, u):
        knots, values, moments = self.knots, self.values, self.moments
        if u <= knots[0]:
            return values[0] + self._end_slope(False) * (u - knots[0])
        if u >= knots[-1]:
            return values[-1] + self._end_slope(True) * (u - knots[-1])
        index = self._span(u)
        span = knots[index + 1] - knots[index]
        before = (knots[index + 1] - u) / span
        after = (u - knots[index]) / span
        return (
            before * values[index]
            + after * values[index + 1]
            + ((before**3 - before) * moments[index] + (after**3 - after) * moments[index + 1])
            * span
            * span
            / 6.0
        )

    def slope(self, u):
        knots, values, moments = self.knots, self.values, self.moments
        if u <= knots[0]:
            return self._end_slope(False)
        if u >= knots[-1]:
            return self._end_slope(True)
        index = self._span(u)
        span = knots[index + 1] - knots[index]
        before = (knots[index + 1] - u) / span
        after = (u - knots[index]) / span
        return (values[index + 1] - values[index]) / span + (
            (3.0 * after * after - 1.0) * moments[index + 1]
            - (3.0 * before * before - 1.0) * moments[index]
        ) * span / 6.0

    def is_straight(self, tolerance=1e-7):
        """Tell whether the curve bows less than ``tolerance`` off a line."""

        reach = self.knots[-1] - self.knots[0]
        return max(abs(moment) for moment in self.moments) * reach * reach / 8.0 < tolerance


def fair_curve(
    pins,
    stiffness,
    start_slope=None,
    end_slope=None,
    start_value=None,
    end_value=None,
):
    """Spring a batten over ``pins`` and return the curve it takes.

    ``pins`` are sorted by ``u`` and at least a millimetre apart; the first
    and the last one are the ends of the curve. The batten bends as little
    as it can while every pin draws it down and the hard ones hold it up.
    ``stiffness`` is a length: a hollow between pins much closer together
    than that is bridged, a longer one is followed. An end may be clamped to
    a slope, a value, or both; a free end is left straight.

    Raises ``ValueError`` when the pins do not hold the batten in place.
    """

    count = len(pins)
    if count < 2:
        raise ValueError("a fair curve needs two pins")
    scale = float(stiffness)
    # In units of the stiffness length the system is well conditioned.
    knots = [pin.u / scale for pin in pins]
    spans = [second - first for first, second in zip(knots, knots[1:])]
    if min(spans) <= 0.0:
        raise ValueError("fair curve pins must be sorted and distinct")
    # Two heights place a batten; one does on an end clamped to a slope.
    placed = sum(1 for pin in pins if pin.weight > 0.0)
    placed += (start_value is not None and pins[0].weight <= 0.0) + (
        end_value is not None and pins[-1].weight <= 0.0
    )
    if placed < (1 if start_slope is not None or end_slope is not None else 2):
        raise ValueError("fair curve pins do not hold the batten in place")

    # The spline's second derivatives M solve R M = 6 (A y + c), and its
    # bending energy is the sum of M (A y + c): quadratic in the heights y.
    bending = [[0.0] * count for _ in range(count)]
    jumps = [[0.0] * count for _ in range(count)]
    constant = [0.0] * count
    for index in range(1, count - 1):
        before, after = spans[index - 1], spans[index]
        bending[index][index - 1] = before
        bending[index][index] = 2.0 * (before + after)
        bending[index][index + 1] = after
        jumps[index][index - 1] = 1.0 / before
        jumps[index][index] = -(1.0 / before + 1.0 / after)
        jumps[index][index + 1] = 1.0 / after
    if start_slope is None:
        bending[0][0] = 1.0
    else:
        bending[0][0] = 2.0 * spans[0]
        bending[0][1] = spans[0]
        jumps[0][0] = -1.0 / spans[0]
        jumps[0][1] = 1.0 / spans[0]
        constant[0] = -start_slope * scale
    if end_slope is None:
        bending[-1][-1] = 1.0
    else:
        bending[-1][-2] = spans[-1]
        bending[-1][-1] = 2.0 * spans[-1]
        jumps[-1][-2] = 1.0 / spans[-1]
        jumps[-1][-1] = -1.0 / spans[-1]
        constant[-1] = end_slope * scale

    indices = range(count)
    # Columns of R^-1 A and R^-1 c: the moments are six times their sum.
    columns = [_solve(bending, [jumps[row][column] for row in indices]) for column in indices]
    offset = _solve(bending, constant)
    hessian = [
        [12.0 * sum(jumps[row][i] * columns[j][row] for row in indices) for j in indices]
        for i in indices
    ]
    gradient = [12.0 * sum(jumps[row][i] * offset[row] for row in indices) for i in indices]
    for index, pin in enumerate(pins):
        pull = 6.0 * max(float(pin.weight), 0.0)
        hessian[index][index] += 2.0 * pull
        gradient[index] -= 2.0 * pull * pin.value

    fixed = {}
    if start_value is not None:
        fixed[0] = float(start_value)
    if end_value is not None:
        fixed[count - 1] = float(end_value)
    floors = [pin.value if pin.hard else None for pin in pins]
    resting = {}
    heights = [0.0] * count
    for _attempt in range(8 * count + 8):
        held = dict(resting)
        held.update(fixed)
        free = [index for index in indices if index not in held]
        heights = [held.get(index, 0.0) for index in indices]
        if free:
            solved = _solve(
                [[hessian[i][j] for j in free] for i in free],
                [-gradient[i] - sum(hessian[i][j] * held[j] for j in held) for i in free],
            )
            for index, height in zip(free, solved):
                heights[index] = height
        # Rest the batten on the pin it dips furthest under.
        under = [
            (floors[index] - heights[index], index)
            for index in free
            if floors[index] is not None and heights[index] < floors[index] - 1e-9
        ]
        if under:
            index = max(under)[1]
            resting[index] = floors[index]
            continue
        # Lift it off the pin that pulls it down hardest, if any does.
        pulling = [
            (gradient[index] + sum(hessian[index][j] * heights[j] for j in indices), index)
            for index in resting
            if index not in fixed
        ]
        if pulling and min(pulling)[0] < -1e-6:
            del resting[min(pulling)[1]]
            continue
        break
    heights = [
        height if floors[index] is None or index in fixed else max(height, floors[index])
        for index, height in enumerate(heights)
    ]
    moments = [
        6.0 * (sum(columns[j][row] * heights[j] for j in indices) + offset[row]) / (scale * scale)
        for row in indices
    ]
    return FairCurve([pin.u for pin in pins], heights, moments)
