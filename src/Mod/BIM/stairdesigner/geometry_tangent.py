# SPDX-License-Identifier: LGPL-2.1-or-later

"""Tangential circular-flight path construction."""

import math

from .geometry_core import (
    BalancedSection,
    _cross,
    distribute_treads,
    tread_stations,
)

from .geometry_winders import (
    _append_endpoint_transition,
    _apply_endpoint_boundary_sections,
    _dense_path_length,
    _endpoint_balance_trim,
    _endpoint_side,
    _herse_curve_point,
    _sample_dense_path,
    _winding_controls,
)


def tangent_flight_sections(
    flight_specs,
    tread_count,
    winding_coefficient=1.0,
    turn_types=None,
    start_angle=0.0,
    end_angle=0.0,
    entry_direction="Straight",
    exit_direction="Straight",
    landing_replaces_tread=True,
    winding_parameters=None,
    extra_widths=None,
):
    """Return sections for paths containing tangential circular flights."""

    if not flight_specs or tread_count < 1:
        return [], 0.0
    specs, primitives = _tangent_path_primitives(flight_specs)
    corner_types = list(turn_types or [])
    corner_types.extend(["Herse balancing"] * (len(specs) - 1 - len(corner_types)))
    corner_types = corner_types[: len(specs) - 1]
    modes = _tangent_junction_modes(primitives, corner_types)
    corner_controls = _winding_controls(len(modes), winding_coefficient, winding_parameters)
    start_trims = [0.0] * len(primitives)
    end_trims = [0.0] * len(primitives)
    for index, mode in enumerate(modes):
        incoming = primitives[index]
        outgoing = primitives[index + 1]
        if mode == "Herse balancing":
            factor = corner_controls[index][1]
            end_trims[index] = min(
                incoming["width"] * factor,
                incoming["length"] * 0.48,
            )
            start_trims[index + 1] = min(
                outgoing["width"] * factor,
                outgoing["length"] * 0.48,
            )
        elif mode == "Landing":
            turn_sine = abs(_cross(incoming["end_tangent"], outgoing["tangent"]))
            end_trims[index] = min(
                outgoing["width"] / (2.0 * turn_sine),
                incoming["length"],
            )
            start_trims[index + 1] = min(
                incoming["width"] / (2.0 * turn_sine),
                outgoing["length"],
            )

    if primitives[0]["type"] == "Straight":
        start_trims[0] = _endpoint_balance_trim(
            primitives[0]["length"],
            primitives[0]["width"],
            winding_coefficient,
            entry_direction,
        )
    if primitives[-1]["type"] == "Straight":
        end_trims[-1] = _endpoint_balance_trim(
            primitives[-1]["length"],
            primitives[-1]["width"],
            winding_coefficient,
            exit_direction,
        )

    for index, primitive in enumerate(primitives):
        total_trim = start_trims[index] + end_trims[index]
        available = primitive["length"] * 0.98
        if total_trim > available:
            scale = available / total_trim
            start_trims[index] *= scale
            end_trims[index] *= scale

    chunks = [[]]
    separators = []
    landing_count = 0
    if start_trims[0] and primitives[0]["type"] == "Straight":
        _append_endpoint_transition(
            lambda point, width, owner: _append_dense_point(chunks[-1], point, width, owner),
            primitives[0]["start"],
            primitives[0]["tangent"],
            primitives[0]["width"],
            start_trims[0],
            entry_direction,
            0,
            True,
            start_angle,
        )
    for index, primitive in enumerate(primitives):
        _append_primitive_range(
            chunks[-1],
            primitive,
            start_trims[index],
            primitive["length"] - end_trims[index],
        )
        if index >= len(modes):
            continue
        mode = modes[index]
        if mode == "Landing":
            chunks.append([])
            separators.append("Landing")
            landing_count += 1
        elif mode == "Tangent":
            chunks.append([])
            separators.append("Tangent")
        elif mode == "Herse balancing":
            incoming = primitive
            outgoing = primitives[index + 1]
            curve_start = _primitive_point(incoming, incoming["length"] - end_trims[index])
            corner = incoming["end"]
            curve_end = _primitive_point(outgoing, start_trims[index + 1])
            for sample in range(1, 65):
                ratio = sample / 64.0
                point = _herse_curve_point(
                    curve_start,
                    corner,
                    curve_end,
                    ratio,
                    corner_controls[index][0],
                )
                width = incoming["width"] + (outgoing["width"] - incoming["width"]) * ratio
                owner = index if ratio < 0.5 else index + 1
                _append_dense_point(chunks[-1], point, width, owner)

    if end_trims[-1] and primitives[-1]["type"] == "Straight":
        _append_endpoint_transition(
            lambda point, width, owner: _append_dense_point(chunks[-1], point, width, owner),
            primitives[-1]["end"],
            primitives[-1]["end_tangent"],
            primitives[-1]["width"],
            end_trims[-1],
            exit_direction,
            len(primitives) - 1,
            False,
            end_angle,
        )

    chunk_lengths = [_dense_path_length(chunk) for chunk in chunks]
    chunk_is_landing = [bool(chunk) and primitives[chunk[0][3]]["is_landing"] for chunk in chunks]
    explicit_landing_count = sum(chunk_is_landing)
    free_tread_count = (
        tread_count - landing_count - (explicit_landing_count if landing_replaces_tread else 0)
    )
    stair_chunk_indices = [
        index for index, is_landing in enumerate(chunk_is_landing) if not is_landing
    ]
    required_free_treads = sum(chunk_lengths[index] > 1e-7 for index in stair_chunk_indices)
    distributed_tread_count = free_tread_count
    if not landing_replaces_tread:
        distributed_tread_count -= explicit_landing_count
    if distributed_tread_count < required_free_treads:
        without_landings = [
            "Herse balancing" if mode == "Landing" else mode for mode in corner_types
        ]
        if without_landings == corner_types:
            return [], 0.0
        return tangent_flight_sections(
            flight_specs,
            tread_count,
            winding_coefficient,
            without_landings,
            start_angle,
            end_angle,
            entry_direction,
            exit_direction,
            landing_replaces_tread,
            winding_parameters,
            extra_widths,
        )

    stair_chunk_counts = distribute_treads(
        [chunk_lengths[index] for index in stair_chunk_indices],
        distributed_tread_count,
    )
    if not landing_replaces_tread:
        stair_count_positions = {
            chunk_index: position for position, chunk_index in enumerate(stair_chunk_indices)
        }
        for landing_index, is_landing in enumerate(chunk_is_landing):
            if not is_landing:
                continue
            receiving_index = next(
                (
                    chunk_index
                    for chunk_index in reversed(stair_chunk_indices)
                    if chunk_index < landing_index
                ),
                None,
            )
            if receiving_index is None:
                receiving_index = next(
                    (
                        chunk_index
                        for chunk_index in stair_chunk_indices
                        if chunk_index > landing_index
                    ),
                    None,
                )
            if receiving_index is not None:
                stair_chunk_counts[stair_count_positions[receiving_index]] += 1
    chunk_tread_counts = [1 if is_landing else 0 for is_landing in chunk_is_landing]
    for index, count in zip(stair_chunk_indices, stair_chunk_counts):
        chunk_tread_counts[index] = count
    extras = list(extra_widths or [])[:tread_count]
    extras.extend([0.0] * (tread_count - len(extras)))
    sampled = []
    extra_cursor = 0
    for index, (chunk, count) in enumerate(zip(chunks, chunk_tread_counts)):
        if index and separators[index - 1] == "Landing" and not chunk_is_landing[index - 1]:
            # A junction landing is the unsampled interval between chunks.
            extra_cursor += 1
        chunk_extras = extras[extra_cursor : extra_cursor + count]
        chunk_sections = _sample_dense_path(chunk, count, chunk_extras)
        extra_cursor += count
        if chunk_is_landing[index]:
            if sampled:
                sampled[-1]["flight_index"] = chunk_sections[0]["flight_index"]
                sampled[-1]["landing_to_next"] = True
                sampled[-1]["level_to_next"] = True
                sampled.extend(chunk_sections[1:])
            else:
                chunk_sections[0]["landing_to_next"] = True
                chunk_sections[0]["level_to_next"] = True
                sampled.extend(chunk_sections)
        elif index and separators[index - 1] == "Landing":
            sampled[-1]["landing_to_next"] = True
            sampled.extend(chunk_sections)
        elif index and chunk_is_landing[index - 1]:
            sampled[-1] = chunk_sections[0]
            sampled.extend(chunk_sections[1:])
        elif index:
            sampled.extend(chunk_sections[1:])
        else:
            sampled.extend(chunk_sections)

    free_length = sum(chunk_lengths[index] for index in stair_chunk_indices)
    nominal_going = free_length / max(free_tread_count, 1)
    sampled_tread_count = max(len(sampled) - 1, 0)
    going, sampled_stations = tread_stations(
        nominal_going * sampled_tread_count,
        sampled_tread_count,
        extras[:sampled_tread_count],
    )
    riser_index = 0
    for index, sample in enumerate(sampled):
        if index < len(sampled) - 1 and not (
            not landing_replaces_tread and sample.get("level_to_next", False)
        ):
            riser_index += 1
        sample["riser_index"] = riser_index
    sections = []
    for index, sample in enumerate(sampled):
        center = sample["center"]
        tangent = sample["tangent"]
        primitive = primitives[sample["flight_index"]]
        if primitive["type"] == "Circular":
            radial = (
                center[0] - primitive["circle_center"][0],
                center[1] - primitive["circle_center"][1],
            )
            radial_length = math.hypot(radial[0], radial[1])
            unit_radial = (
                radial[0] / radial_length,
                radial[1] / radial_length,
            )
            center = (
                primitive["circle_center"][0] + unit_radial[0] * primitive["radius"],
                primitive["circle_center"][1] + unit_radial[1] * primitive["radius"],
            )
            tangent = (
                -primitive["sign"] * unit_radial[1],
                primitive["sign"] * unit_radial[0],
            )
        normal = (-tangent[1], tangent[0])
        half_width = sample["width"] / 2.0
        sections.append(
            BalancedSection(
                center=center,
                tangent=tangent,
                left=(
                    center[0] + normal[0] * half_width,
                    center[1] + normal[1] * half_width,
                ),
                right=(
                    center[0] - normal[0] * half_width,
                    center[1] - normal[1] * half_width,
                ),
                station=sampled_stations[index],
                width=sample["width"],
                flight_index=sample["flight_index"],
                landing_to_next=sample["landing_to_next"],
                locked_to_flight=True,
                level_to_next=sample.get("level_to_next", False),
                riser_index=sample["riser_index"],
            )
        )
    endpoint_specs = [(primitive["length"], primitive["width"], 0.0) for primitive in primitives]
    endpoint_vertices = [primitive["start"] for primitive in primitives] + [primitives[-1]["end"]]
    endpoint_directions = [primitive["tangent"] for primitive in primitives]
    sections = _apply_endpoint_boundary_sections(
        sections,
        endpoint_vertices,
        endpoint_directions,
        endpoint_specs,
        start_angle if primitives[0]["type"] == "Straight" else 0.0,
        end_angle if primitives[-1]["type"] == "Straight" else 0.0,
        entry_direction if primitives[0]["type"] == "Straight" else "Straight",
        exit_direction if primitives[-1]["type"] == "Straight" else "Straight",
        winding_coefficient,
    )
    return sections, going


def _tangent_path_primitives(flight_specs):
    specs = []
    for flight_spec in flight_specs:
        flight_type, dimension, width, angle, rotation = flight_spec[:5]
        entry_direction = str(flight_spec[5]) if len(flight_spec) > 5 else "Straight"
        exit_direction = str(flight_spec[6]) if len(flight_spec) > 6 else "Straight"
        requested_type = str(flight_type)
        is_landing = requested_type in {
            "Straight landing",
            "Circular landing",
        }
        flight_type = "Circular" if requested_type.startswith("Circular") else "Straight"
        width = max(float(width), 0.01)
        sweep = min(max(abs(math.radians(float(angle))), 1e-6), 2.0 * math.pi - 1e-6)
        if flight_type == "Circular":
            inner_radius = max(float(dimension), 0.01)
            radius = inner_radius + width / 2.0
            length = radius * sweep
        else:
            inner_radius = None
            radius = None
            length = max(float(dimension), 0.01)
        specs.append(
            {
                "type": flight_type,
                "length": length,
                "width": width,
                "angle": float(angle),
                "rotation": "Right" if str(rotation) == "Right" else "Left",
                "inner_radius": inner_radius,
                "radius": radius,
                "sweep": sweep,
                "is_landing": is_landing,
                "entry_direction": entry_direction,
                "exit_direction": exit_direction,
            }
        )

    center = (0.0, specs[0]["width"] / 2.0)
    heading = 0.0
    primitives = []
    for index, spec in enumerate(specs):
        if index:
            previous = specs[index - 1]
            landing_entry = _endpoint_side(spec["entry_direction"])
            if spec["is_landing"] and spec["type"] == "Straight" and landing_entry:
                heading += landing_entry * math.radians(abs(spec["angle"]))
            elif (
                not spec["is_landing"]
                and not previous["is_landing"]
                and previous["type"] == "Straight"
                and spec["type"] == "Straight"
            ):
                turn = math.radians(spec["angle"])
                if spec["rotation"] == "Right":
                    turn = -turn
                heading += turn
        tangent = (math.cos(heading), math.sin(heading))
        primitive = {
            **spec,
            "index": index,
            "start": center,
            "heading": heading,
            "tangent": tangent,
        }
        if spec["type"] == "Circular":
            sign = -1.0 if spec["rotation"] == "Right" else 1.0
            normal = (-tangent[1], tangent[0])
            primitive["sign"] = sign
            primitive["circle_center"] = (
                center[0] + sign * normal[0] * spec["radius"],
                center[1] + sign * normal[1] * spec["radius"],
            )
            center = _primitive_point(primitive, spec["length"])
            heading += sign * spec["sweep"]
        elif spec["is_landing"]:
            normal = (-tangent[1], tangent[0])
            half_width = spec["width"] / 2.0
            entry_side = _endpoint_side(spec["entry_direction"])
            exit_side = _endpoint_side(spec["exit_direction"])
            side_port_offset = min(half_width, max((spec["length"] - 0.01) / 2.0, 0.0))
            entry_port = center
            face_start = (
                entry_port[0]
                - tangent[0] * side_port_offset * abs(entry_side)
                - normal[0] * half_width * entry_side,
                entry_port[1]
                - tangent[1] * side_port_offset * abs(entry_side)
                - normal[1] * half_width * entry_side,
            )
            face_end = (
                face_start[0] + tangent[0] * spec["length"],
                face_start[1] + tangent[1] * spec["length"],
            )
            exit_port = (
                face_end[0]
                - tangent[0] * side_port_offset * abs(exit_side)
                + normal[0] * half_width * exit_side,
                face_end[1]
                - tangent[1] * side_port_offset * abs(exit_side)
                + normal[1] * half_width * exit_side,
            )
            primitive["face_start"] = face_start
            primitive["face_end"] = face_end
            primitive["path_start"] = entry_port
            primitive["path_end"] = exit_port
            center = exit_port
            if exit_side:
                heading += exit_side * math.radians(abs(spec["angle"]))
        else:
            center = (
                center[0] + tangent[0] * spec["length"],
                center[1] + tangent[1] * spec["length"],
            )
        primitive["end"] = center
        primitive["end_tangent"] = (math.cos(heading), math.sin(heading))
        primitives.append(primitive)
    return specs, primitives


def _primitive_point(primitive, distance):
    distance = min(max(float(distance), 0.0), primitive["length"])
    if primitive["type"] == "Straight":
        if primitive.get("is_landing"):
            ratio = distance / primitive["length"]
            start = primitive.get("path_start", primitive["start"])
            end = primitive.get("path_end", primitive["end"])
            return (
                start[0] + (end[0] - start[0]) * ratio,
                start[1] + (end[1] - start[1]) * ratio,
            )
        return (
            primitive["start"][0] + primitive["tangent"][0] * distance,
            primitive["start"][1] + primitive["tangent"][1] * distance,
        )
    angle = primitive["sign"] * distance / primitive["radius"]
    relative = (
        primitive["start"][0] - primitive["circle_center"][0],
        primitive["start"][1] - primitive["circle_center"][1],
    )
    cosine = math.cos(angle)
    sine = math.sin(angle)
    return (
        primitive["circle_center"][0] + relative[0] * cosine - relative[1] * sine,
        primitive["circle_center"][1] + relative[0] * sine + relative[1] * cosine,
    )


def _primitive_tangent(primitive, distance):
    if primitive["type"] == "Straight":
        return primitive["tangent"]
    heading = primitive["heading"] + (primitive["sign"] * distance / primitive["radius"])
    return math.cos(heading), math.sin(heading)


def _append_primitive_range(chunk, primitive, start, end):
    start = min(max(float(start), 0.0), primitive["length"])
    end = min(max(float(end), start), primitive["length"])
    samples = 1
    if primitive["type"] == "Circular":
        samples = max(
            8,
            int(math.ceil(128.0 * (end - start) / primitive["length"])),
        )
    for sample in range(samples + 1):
        ratio = sample / samples
        distance = start + (end - start) * ratio
        _append_dense_point(
            chunk,
            _primitive_point(primitive, distance),
            primitive["width"],
            primitive["index"],
        )


def _append_dense_point(chunk, point, width, flight_index):
    if chunk and math.hypot(point[0] - chunk[-1][0], point[1] - chunk[-1][1]) < 1e-9:
        chunk[-1] = (point[0], point[1], width, flight_index)
    else:
        chunk.append((point[0], point[1], width, flight_index))


def _tangent_junction_modes(primitives, corner_types):
    modes = []
    for index, requested in enumerate(corner_types):
        incoming = primitives[index]
        outgoing = primitives[index + 1]
        tangent = (
            incoming["is_landing"]
            or outgoing["is_landing"]
            or incoming["type"] == "Circular"
            or outgoing["type"] == "Circular"
            or abs(_cross(incoming["end_tangent"], outgoing["tangent"])) < 1e-7
        )
        if tangent:
            modes.append("Tangent")
        else:
            modes.append("Landing" if str(requested) == "Landing" else "Herse balancing")
    return modes
