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
"""Provide the object code for Draft PolarArray."""
## @package polararray
# \ingroup DRAFT
# \brief This module provides the object code for Draft PolarArray.

import FreeCAD as App
import Draft
import draftutils.utils as utils
from draftutils.messages import _msg, _err
from draftutils.translate import _tr


def make_polar_array(obj,
                     number=4, angle=360, center=App.Vector(0, 0, 0),
                     use_link=True):
    """Create a polar array from the given object.

    Parameters
    ----------
    obj: Part::Feature
        Any type of object that has a `Part::TopoShape`
        that can be duplicated.
        This means most 2D and 3D objects produced
        with any workbench.

    number: int, optional
        It defaults to 4.
        The number of copies produced in the circular pattern.

    angle: float, optional
        It defaults to 360.
        The magnitude in degrees swept by the polar pattern.

    center: Base::Vector3, optional
        It defaults to the origin `App.Vector(0, 0, 0)`.
        The vector indicating the center of rotation of the array.

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
    _name = "make_polar_array"
    utils.print_header(_name, _tr("Polar array"))

    _msg("Number: {}".format(number))
    _msg("Angle: {}".format(angle))
    _msg("Center: {}".format(center))

    try:
        utils.type_check([(number, int)], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be an integer number."))
        return None

    try:
        utils.type_check([(angle, (int, float))], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a number."))
        return None

    try:
        utils.type_check([(center, App.Vector)], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a vector."))
        return None

    _msg("use_link: {}".format(bool(use_link)))

    new_obj = Draft.makeArray(obj,
                              arg1=center, arg2=angle, arg3=number,
                              use_link=use_link)
    return new_obj
