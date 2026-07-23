# SPDX-License-Identifier: LGPL-2.1-or-later

"""Core dimensions and shared stair geometry data."""

from dataclasses import dataclass, replace
import math

BLONDEL_MINIMUM = 620.0
BLONDEL_MAXIMUM = 640.0

@dataclass(frozen=True)
class StraightStairMetrics:
    """Computed dimensions for one straight flight."""

    floor_height: float
    flight_length: float
    riser_count: int
    tread_count: int
    riser_height: float
    tread_width: float
    blondel_value: float
    blondel_compliant: bool


@dataclass(frozen=True)
class BalancedSection:
    """One tread-nosing section across a balanced stair plan."""

    center: tuple
    tangent: tuple
    left: tuple
    right: tuple
    station: float
    width: float
    flight_index: int
    landing_to_next: bool = False
    locked_to_flight: bool = False
    level_to_next: bool = False
    riser_index: int = 0
    profile_nosing_aligned: bool = False
    top_elevation: float = None


@dataclass(frozen=True)
class _CircularProfile:
    """Exact annular-sector data shared by circular concrete helpers."""

    center: tuple
    inner_radius: float
    outer_radius: float
    start_angle: float
    sweep: float


def straight_stair_metrics(floor_height, flight_length, riser_count):
    """Return the derived dimensions for a straight flight.

    ``riser_count`` is exposed to users as the number of steps. The upper
    floor supplies the final tread, so a flight has one fewer manufactured
    tread than risers.
    """

    floor_height = max(float(floor_height), 0.0)
    flight_length = max(float(flight_length), 0.0)
    riser_count = max(int(riser_count), 2)
    tread_count = riser_count - 1
    riser_height = floor_height / riser_count
    tread_width = flight_length / tread_count
    blondel_value = 2.0 * riser_height + tread_width
    return StraightStairMetrics(
        floor_height=floor_height,
        flight_length=flight_length,
        riser_count=riser_count,
        tread_count=tread_count,
        riser_height=riser_height,
        tread_width=tread_width,
        blondel_value=blondel_value,
        blondel_compliant=BLONDEL_MINIMUM <= blondel_value <= BLONDEL_MAXIMUM,
    )


def flight_stair_metrics(
    flight_length,
    tread_count,
    riser_height,
    extra_widths=None,
):
    """Return geometry metrics for one flight of a multi-flight stair."""

    flight_length = max(float(flight_length), 0.0)
    tread_count = max(int(tread_count), 0)
    riser_height = max(float(riser_height), 0.0)
    tread_width, _goings = tread_goings(
        flight_length, tread_count, extra_widths
    )
    blondel_value = 2.0 * riser_height + tread_width
    return StraightStairMetrics(
        floor_height=tread_count * riser_height,
        flight_length=flight_length,
        riser_count=tread_count,
        tread_count=tread_count,
        riser_height=riser_height,
        tread_width=tread_width,
        blondel_value=blondel_value,
        blondel_compliant=BLONDEL_MINIMUM <= blondel_value <= BLONDEL_MAXIMUM,
    )


def _distributed_dimensions(total, count, adjustments=None):
    """Return a general dimension and signed-adjusted individual dimensions."""

    total = max(float(total), 0.0)
    count = max(int(count), 0)
    if not count:
        return 0.0, []
    values = list(adjustments or [])[:count]
    values.extend([0.0] * (count - len(values)))
    extras = [float(value) for value in values]

    # Keep every dimension geometrically usable even if an excessive set of
    # positive or negative adjustments is entered.  The effective departure
    # from the mean is scaled uniformly, preserving both the fixed total
    # and the relative adjustments.  Requested values remain stored on the
    # component objects.
    uniform = total / count
    minimum = min(0.01, uniform)
    mean_extra = sum(extras) / count
    deviations = [extra - mean_extra for extra in extras]
    scale = 1.0
    for deviation in deviations:
        if deviation < 0.0:
            scale = min(
                scale,
                max(
                    (uniform - minimum) / -deviation,
                    0.0,
                ),
            )
    if scale < 1.0:
        extras = [extra * scale for extra in extras]

    extra_total = sum(extras)
    general = (total - extra_total) / count
    dimensions = [general + value for value in extras]
    # Avoid accumulated floating-point drift at the terminal boundary.
    dimensions[-1] += total - sum(dimensions)
    return general, dimensions


def tread_goings(total_length, tread_count, extra_widths=None):
    """Return the general going and each tread's effective going.

    ``extra_widths`` contains signed adjustments to individual tread depths
    without changing the total run.
    """

    return _distributed_dimensions(
        total_length, tread_count, extra_widths
    )


def tread_stations(total_length, tread_count, extra_widths=None):
    """Return the general going and cumulative tread boundary stations."""

    general_going, goings = tread_goings(
        total_length, tread_count, extra_widths
    )
    stations = [0.0]
    for going in goings:
        stations.append(stations[-1] + going)
    if stations:
        stations[-1] = max(float(total_length), 0.0)
    return general_going, stations


def riser_heights(total_height, riser_count, extra_heights=None):
    """Return the general rise and each riser's effective signed-adjusted rise."""

    return _distributed_dimensions(
        total_height, riser_count, extra_heights
    )


def riser_stations(total_height, riser_count, extra_heights=None):
    """Return the general rise and cumulative floor-to-floor elevations."""

    general_height, heights = riser_heights(
        total_height, riser_count, extra_heights
    )
    stations = [0.0]
    for height in heights:
        stations.append(stations[-1] + height)
    if stations:
        stations[-1] = max(float(total_height), 0.0)
    return general_height, stations


def distribute_treads(flight_lengths, tread_count):
    """Distribute manufactured treads proportionally over several flights."""

    lengths = [max(float(length), 0.0) for length in flight_lengths]
    if not lengths:
        return []
    tread_count = max(int(tread_count), 0)
    if tread_count < len(lengths):
        return [1 if index < tread_count else 0 for index in range(len(lengths))]
    result = [1] * len(lengths)
    remaining = tread_count - len(lengths)
    if not remaining:
        return result

    total = sum(lengths)
    weights = lengths if total > 0.0 else [1.0] * len(lengths)
    total = sum(weights)
    shares = [remaining * weight / total for weight in weights]
    whole = [int(math.floor(share)) for share in shares]
    result = [base + extra for base, extra in zip(result, whole)]
    unassigned = remaining - sum(whole)
    order = sorted(
        range(len(lengths)),
        key=lambda index: (shares[index] - whole[index], weights[index], -index),
        reverse=True,
    )
    for index in order[:unassigned]:
        result[index] += 1
    return result


def _cross(first, second):
    return first[0] * second[1] - first[1] * second[0]


def _dot(first, second):
    return first[0] * second[0] + first[1] * second[1]


def _shifted(point, tangent, distance):
    return (
        point[0] + tangent[0] * distance,
        point[1] + tangent[1] * distance,
    )


def _translated_section(section, distance):
    """Return a section translated along its local walking direction."""

    return BalancedSection(
        center=_shifted(section.center, section.tangent, distance),
        tangent=section.tangent,
        left=_shifted(section.left, section.tangent, distance),
        right=_shifted(section.right, section.tangent, distance),
        station=section.station + distance,
        width=section.width,
        flight_index=section.flight_index,
        landing_to_next=section.landing_to_next,
        locked_to_flight=section.locked_to_flight,
        level_to_next=section.level_to_next,
        riser_index=section.riser_index,
        profile_nosing_aligned=section.profile_nosing_aligned,
        top_elevation=section.top_elevation,
    )


def balanced_section_top(section, index, riser_height):
    """Return the tread-top elevation represented by ``section``."""

    top_elevation = getattr(section, "top_elevation", None)
    if top_elevation is not None:
        return float(top_elevation)
    riser_index = int(getattr(section, "riser_index", 0))
    if riser_index <= 0 and not getattr(section, "level_to_next", False):
        riser_index = index + 1
    return riser_index * float(riser_height)


def assign_section_elevations(sections, elevations):
    """Attach cumulative riser elevations to plan sections."""

    elevations = list(elevations or [])
    if not elevations:
        return list(sections)
    result = []
    for index, section in enumerate(sections):
        riser_index = int(getattr(section, "riser_index", 0))
        if riser_index <= 0 and not getattr(
            section, "level_to_next", False
        ):
            riser_index = index + 1
        riser_index = min(max(riser_index, 0), len(elevations) - 1)
        result.append(
            replace(section, top_elevation=elevations[riser_index])
        )
    return result
