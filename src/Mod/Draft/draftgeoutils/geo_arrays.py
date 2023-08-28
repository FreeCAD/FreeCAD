# ***************************************************************************
# *   Copyright (c) 2020 three_d                                            *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides various functions to work with arrays.

One of the functions is used to create a `twisted array` object.
See `draftobjects/twistedarray.py`.

This array was developed in order to build a `twisted bridge` object.

See https://forum.freecad.org/viewtopic.php?f=23&t=49617
"""
## @package geo_arrays
# \ingroup draftgeoutils
# \brief Provides various functions to work with arrays.

import lazy_loader.lazy_loader as lz

import FreeCAD as App
from draftutils.messages import _msg

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


def print_places(places, title="Places"):
    """Print a vector with a title."""
    _msg(12*"-")
    _msg(title)
    for i in places:
        _msg("{}".format(i))


def get_init_values(path, count=6):
    """Set values needed to create the array."""
    norm = App.Vector(0, 0, 1)

    # Currently this works with a sketch that has a single edge.
    # Here we need a more general function to extract all edges from a shape,
    # so that the array uses all of them.
    edge = path.Shape.Edges[0]
    edge_length = edge.Length

    step = edge_length / (count - 1)
    inc = 360 / (count - 1)

    return norm, edge, step, inc


def get_n_params(edge, number, step, norm):
    """Get the parameters needed in each iteration."""
    parameter = edge.getParameterByLength(number * step)

    v0 = edge.valueAt(parameter)
    tan = edge.tangentAt(parameter).normalize()
    binorm = tan.cross(norm).normalize()
    rot = App.Rotation(binorm, tan, norm)

    return v0, tan, rot


def get_twisted_placements(path, count=15, rot_factor=0.25):
    """Get the placements of the twisted array elements."""
    (norm, edge,
     step, inc) = get_init_values(path, count)

    increment = 0
    places = []
    params = []

    for number in range(count):
        v0, tan, rot = get_n_params(edge, number, step, norm)

        angle = increment * rot_factor
        place = App.Placement(v0, tan, angle)
        place.Rotation = place.Rotation * rot
        places.append(place)

        params.append((v0, tan, angle, rot))

        increment += inc

    return places, params


def get_twisted_array_shape(base, path, count=15, rot_factor=0.25):
    """Get the twisted array shape as a compound."""
    places, _ = get_twisted_placements(path,
                                       count=count,
                                       rot_factor=rot_factor)
    shapes, _ = create_frames(base, places)

    shape = Part.makeCompound(shapes)
    return shape


def get_twisted_bridge_shape(base, path, count=15, rot_factor=0.25,
                             width=100,
                             thickness=10):
    """Get the twisted bridge array shape as a compound."""
    compound = list()

    places, _ = get_twisted_placements(path,
                                       count=count,
                                       rot_factor=rot_factor)
    # print_places(places)

    shapes, profiles = create_frames(base, places)
    compound.extend(shapes)

    tunnel = make_tunnel(path, profiles)
    compound.append(tunnel)

    # size = base.Shape.Wires[-1].BoundBox.XLength * 0.9
    # size = int(size)
    # thickness = size/12.0

    walkway = make_walkway(path, width, thickness)
    compound.append(walkway)
    shape = Part.makeCompound(compound)

    return shape


def create_frames(obj, places):
    """Create the frames from the placements."""
    len_wires = len(obj.Shape.Wires)
    frames = list()
    profiles = list()
    # _msg("{}: {} wires".format(obj.Label, len_wires))
    # _msg("places: {}".format(len(places)))

    for i in places:
        frame = obj.Shape.copy()
        frame.Placement = i
        frames.append(frame)
        profiles.append(frame.Wires[len_wires - 1])

    return frames, profiles


def make_tunnel(path, profiles):
    """Create the tunnel shape."""
    edge = path.Shape.Edges[0]
    wire = Part.Wire(edge)
    sweep = wire.makePipeShell(profiles)
    return sweep


def make_walkway(path, width=100, thickness=10):
    """Construct the walkway of the twisted bridge array."""
    spine = path.Shape.Edges[0]

    half_size = width/2
    offset_height = thickness

    norm1 = App.Vector(0, 0, 1)
    v1 = spine.valueAt(1)
    tan1 = spine.tangentAt(1).normalize()
    binorm1 = tan1.cross(norm1)

    place = App.Placement()
    place.Rotation = App.Rotation(binorm1, norm1, tan1)
    place.move(v1 - App.Vector(0, 0, 3 * offset_height))

    plane = Part.makePlane(width, thickness,
                           App.Vector(-half_size, -2 * offset_height, 0))
    face = Part.Face(plane)
    face.Placement = place.multiply(face.Placement)

    wire = Part.Wire(spine)
    sweep = wire.makePipe(face)

    return sweep

## @}
