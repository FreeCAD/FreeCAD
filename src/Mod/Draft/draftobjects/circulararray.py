# ***************************************************************************
# *   (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides the object code for Draft CircularArray."""
## @package circulararray
# \ingroup DRAFT
# \brief This module provides the object code for Draft CircularArray.

import FreeCAD as App
import Draft
import draftutils.utils as utils
from draftutils.messages import _msg, _err
from draftutils.translate import _tr


def make_circular_array(obj,
                        r_distance=100, tan_distance=100,
                        number=2, symmetry=1,
                        axis=App.Vector(0, 0, 1), center=App.Vector(0, 0, 0),
                        use_link=True):
    """Create a circular array from the given object.

    Parameters
    ----------
    obj: Part::Feature
        Any type of object that has a `Part::TopoShape`
        that can be duplicated.

    r_distance: float, optional
        It defaults to `100`.
        Radial distance to the next ring of circular arrays.

    tan_distance: float, optional
        It defaults to `100`.
        The tangential distance between two elements located
        in the same circular ring.
        The tangential distance together with the radial distance
        determine how many copies are created.

    number: int, optional
        It defaults to 2.
        The number of layers or rings of repeated objects.
        The original object stays at the center, and is counted
        as a layer itself. So, if you want at least one layer of circular
        copies, this number must be at least 2.

    symmetry: int, optional
        It defaults to 1.
        It indicates how many lines of symmetry the entire circular pattern
        has. That is, with 1, the array is symmetric only after a full
        360 degrees rotation.

        When it is 2, the array is symmetric at 0 and 180 degrees.
        When it is 3, the array is symmetric at 0, 120, and 240 degrees.
        When it is 4, the array is symmetric at 0, 90, 180, and 270 degrees.
        Et cetera.

    axis: Base::Vector3, optional
        It defaults to `App.Vector(0, 0, 1)` or the `+Z` axis.
        The unit vector indicating the axis of rotation.

    center: Base::Vector3, optional
        It defaults to `App.Vector(0, 0, 0)` or the global origin.
        The point through which the `axis` passes to define
        the axis of rotation.

    use_link: bool, optional
        It defaults to `True`.
        If it is `True` the produced copies are not `Part::TopoShape` copies,
        but rather `App::Link` objects.
        The Links repeat the shape of the original `obj` exactly,
        and therefore the resulting array is more memory efficient.

        Also, when `use_link` is `True`, the `Fuse` property
        of the resulting array does not work; the array doesn't
        contain separate shapes, it only has the original shape repeated
        many times, so there is nothing to fuse together.

        If `use_link` is `False` the original shape is copied many times.
        In this case the `Fuse` property is able to fuse
        all copies into a single object, if they touch each other.

    Returns
    -------
    Part::FeaturePython
        A scripted object with `Proxy.Type='Array'`.
        Its `Shape` is a compound of the copies of the original object.
    """
    _name = "make_circular_array"
    utils.print_header(_name, _tr("Circular array"))

    _msg("r_distance: {}".format(r_distance))
    _msg("tan_distance: {}".format(tan_distance))

    try:
        utils.type_check([(r_distance, (int, float, App.Units.Quantity)),
                          (tan_distance, (int, float, App.Units.Quantity))],
                         name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a number or quantity."))
        return None

    _msg("number: {}".format(number))
    _msg("symmetry: {}".format(symmetry))

    try:
        utils.type_check([(number, int),
                          (symmetry, int)], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be an integer number."))
        return None

    _msg("axis: {}".format(axis))
    _msg("center: {}".format(center))

    try:
        utils.type_check([(axis, App.Vector),
                          (center, App.Vector)], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a vector."))
        return None

    _msg("use_link: {}".format(bool(use_link)))

    new_obj = Draft.makeArray(obj,
                              arg1=r_distance, arg2=tan_distance,
                              arg3=axis, arg4=center,
                              arg5=number, arg6=symmetry,
                              use_link=use_link)
    return new_obj
