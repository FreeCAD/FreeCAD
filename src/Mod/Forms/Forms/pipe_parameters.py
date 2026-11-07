# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

"""Read and update per-segment pipe overrides independently of the GUI."""

import json
import math


def decode_segment_overrides(values):
    result = {}
    for value in values or ():
        try:
            record = json.loads(str(value))
            key = str(record["segment"])
            diameter = float(record["diameter"])
            if diameter > 0.0 and math.isfinite(diameter):
                result[key] = diameter
        except (KeyError, TypeError, ValueError, json.JSONDecodeError):
            continue
    return result


def encode_segment_overrides(values):
    return [
        json.dumps({"segment": key, "diameter": float(diameter)}, sort_keys=True)
        for key, diameter in sorted(values.items())
        if float(diameter) > 0.0 and math.isfinite(float(diameter))
    ]


def set_segment_diameter(obj, key, diameter):
    """Set one path-segment override; zero removes the override."""
    overrides = decode_segment_overrides(obj.SegmentDiameters)
    diameter = float(diameter)
    if diameter > 0.0:
        overrides[str(key)] = diameter
    else:
        overrides.pop(str(key), None)
    obj.SegmentDiameters = encode_segment_overrides(overrides)
    obj.touch()


def decode_segment_sample_overrides(values):
    result = {}
    for value in values or ():
        try:
            record = json.loads(str(value))
            key = str(record["segment"])
            samples = int(record["samples"])
            if samples > 0:
                result[key] = samples
        except (KeyError, TypeError, ValueError, json.JSONDecodeError):
            continue
    return result


def encode_segment_sample_overrides(values):
    return [
        json.dumps({"segment": key, "samples": int(samples)}, sort_keys=True)
        for key, samples in sorted(values.items())
        if int(samples) > 0
    ]


def set_segment_samples(obj, key, samples):
    """Set one segment's longitudinal intervals per source edge; zero uses global."""
    overrides = decode_segment_sample_overrides(obj.SegmentSamples)
    samples = int(samples)
    if samples > 0:
        overrides[str(key)] = samples
    else:
        overrides.pop(str(key), None)
    obj.SegmentSamples = encode_segment_sample_overrides(overrides)
    obj.touch()
