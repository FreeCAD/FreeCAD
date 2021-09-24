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
"""Provides functions to create polar Array objects."""
## @package make_polararray
# \ingroup draftmake
# \brief Provides functions to create polar Array objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.utils as utils
import draftmake.make_array as make_array

from draftutils.messages import _msg, _err
from draftutils.translate import translate


def make_polar_array(base_object,
                     number=5, angle=360, center=App.Vector(0, 0, 0),
                     use_link=True):
    """Create a polar array from the given object.

    Parameters
    ----------
    base_object: Part::Feature or str
        Any of object that has a `Part::TopoShape` that can be duplicated.
        This means most 2D and 3D objects produced with any workbench.
        If it is a string, it must be the `Label` of that object.
        Since a label is not guaranteed to be unique in a document,
        it will use the first object found with this label.

    number: int, optional
        It defaults to 5.
        The number of copies produced in the polar pattern.

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
        A scripted object of type `'Array'`.
        Its `Shape` is a compound of the copies of the original object.

    None
        If there is a problem it will return `None`.

    See Also
    --------
    make_ortho_array, make_circular_array, make_path_array, make_point_array
    """
    _name = "make_polar_array"
    utils.print_header(_name, translate("draft","Polar array"))

    if isinstance(base_object, str):
        base_object_str = base_object

    found, base_object = utils.find_object(base_object,
                                           doc=App.activeDocument())
    if not found:
        _msg("base_object: {}".format(base_object_str))
        _err(translate("draft","Wrong input: object not in document."))
        return None

    _msg("base_object: {}".format(base_object.Label))

    _msg("number: {}".format(number))
    try:
        utils.type_check([(number, int)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be an integer number."))
        return None

    _msg("angle: {}".format(angle))
    try:
        utils.type_check([(angle, (int, float))], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a number."))
        return None

    _msg("center: {}".format(center))
    try:
        utils.type_check([(center, App.Vector)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a vector."))
        return None

    use_link = bool(use_link)
    _msg("use_link: {}".format(use_link))

    new_obj = make_array.make_array(base_object,
                                    arg1=center, arg2=angle, arg3=number,
                                    use_link=use_link)
    return new_obj

## @}
