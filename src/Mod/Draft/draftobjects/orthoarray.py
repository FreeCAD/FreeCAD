# ***************************************************************************
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
"""Provide the object code for Draft Array."""
## @package orthoarray
# \ingroup DRAFT
# \brief Provide the object code for Draft Array.

import FreeCAD as App
import Draft
import draftutils.utils as utils
from draftutils.messages import _msg, _wrn, _err
from draftutils.translate import _tr


def make_ortho_array(obj,
                     v_x=App.Vector(10, 0, 0),
                     v_y=App.Vector(0, 10, 0),
                     v_z=App.Vector(0, 0, 10),
                     n_x=2,
                     n_y=2,
                     n_z=1,
                     use_link=True):
    """Create an orthogonal array from the given object.

    Parameters
    ----------
    obj: Part::Feature
        Any type of object that has a `Part::TopoShape`
        that can be duplicated.
        This means most 2D and 3D objects produced
        with any workbench.

    v_x, v_y, v_z: Base::Vector3, optional
        The vector indicating the vector displacement between two elements
        in the specified orthogonal direction X, Y, Z.

        By default:
        ::
            v_x = App.Vector(10, 0, 0)
            v_y = App.Vector(0, 10, 0)
            v_z = App.Vector(0, 0, 10)

        Given that this is a vectorial displacement
        the next object can appear displaced in one, two or three axes
        at the same time.

        For example
        ::
            v_x = App.Vector(10, 5, 0)

        means that the next element in the X direction will be displaced
        10 mm in X, 5 mm in Y, and 0 mm in Z.

        A traditional "rectangular" array is obtained when
        the displacement vector only has its corresponding component,
        like in the default case.

        If these values are entered as single numbers instead
        of vectors, the single value is expanded into a vector
        of the corresponding direction, and the other components are assumed
        to be zero.

        For example
        ::
            v_x = 15
            v_y = 10
            v_z = 1
        becomes
        ::
            v_x = App.Vector(15, 0, 0)
            v_y = App.Vector(0, 10, 0)
            v_z = App.Vector(0, 0, 1)

    n_x, n_y, n_z: int, optional
        The number of copies in the specified orthogonal direction X, Y, Z.
        This number includes the original object, therefore, it must be
        at least 1.

        The values of `n_x` and `n_y` default to 2,
        while `n_z` defaults to 1.
        This means the array by default is a planar array.

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

    See Also
    --------
    make_ortho_array2d, make_rect_array, make_rect_array2d
    """
    _name = "make_ortho_array"
    utils.print_header(_name, _tr("Orthogonal array"))

    _msg("v_x: {}".format(v_x))
    _msg("v_y: {}".format(v_y))
    _msg("v_z: {}".format(v_z))

    try:
        utils.type_check([(v_x, (int, float, App.Vector)),
                          (v_y, (int, float, App.Vector)),
                          (v_z, (int, float, App.Vector))],
                         name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a number or vector."))
        return None

    _text = "Input: single value expanded to vector."
    if not isinstance(v_x, App.Vector):
        v_x = App.Vector(v_x, 0, 0)
        _wrn(_tr(_text))
    if not isinstance(v_y, App.Vector):
        v_y = App.Vector(0, v_y, 0)
        _wrn(_tr(_text))
    if not isinstance(v_z, App.Vector):
        v_z = App.Vector(0, 0, v_z)
        _wrn(_tr(_text))

    _msg("n_x: {}".format(n_x))
    _msg("n_y: {}".format(n_y))
    _msg("n_z: {}".format(n_z))

    try:
        utils.type_check([(n_x, int),
                          (n_y, int),
                          (n_z, int)], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be an integer number."))
        return None

    _text = ("Input: number of elements must be at least 1. "
             "It is set to 1.")
    if n_x < 1:
        _wrn(_tr(_text))
        n_x = 1
    if n_y < 1:
        _wrn(_tr(_text))
        n_y = 1
    if n_z < 1:
        _wrn(_tr(_text))
        n_z = 1

    _msg("use_link: {}".format(bool(use_link)))

    new_obj = Draft.makeArray(obj,
                              arg1=v_x, arg2=v_y, arg3=v_z,
                              arg4=n_x, arg5=n_y, arg6=n_z,
                              use_link=use_link)
    return new_obj


def make_ortho_array2d(obj,
                       v_x=App.Vector(10, 0, 0),
                       v_y=App.Vector(0, 10, 0),
                       n_x=2,
                       n_y=2,
                       use_link=True):
    """Create a 2D orthogonal array from the given object.

    This works the same as `make_ortho_array`.
    The Z component is ignored so it only considers vector displacements
    in X and Y directions.

    Parameters
    ----------
    obj: Part::Feature
        Any type of object that has a `Part::TopoShape`
        that can be duplicated.
        This means most 2D and 3D objects produced
        with any workbench.

    v_x, v_y: Base::Vector3, optional
        Vectorial displacement of elements
        in the corresponding X and Y directions.
        See `make_ortho_array`.

    n_x, n_y: int, optional
        Number of elements
        in the corresponding X and Y directions.
        See `make_ortho_array`.

    use_link: bool, optional
        If it is `True`, create `App::Link` array.
        See `make_ortho_array`.

    Returns
    -------
    Part::FeaturePython
        A scripted object with `Proxy.Type='Array'`.
        Its `Shape` is a compound of the copies of the original object.

    See Also
    --------
    make_ortho_array, make_rect_array, make_rect_array2d
    """
    _name = "make_ortho_array2d"
    utils.print_header(_name, _tr("Orthogonal array 2D"))

    _msg("v_x: {}".format(v_x))
    _msg("v_y: {}".format(v_y))

    try:
        utils.type_check([(v_x, (int, float, App.Vector)),
                          (v_y, (int, float, App.Vector))],
                         name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a number or vector."))
        return None

    _text = "Input: single value expanded to vector."
    if not isinstance(v_x, App.Vector):
        v_x = App.Vector(v_x, 0, 0)
        _wrn(_tr(_text))
    if not isinstance(v_y, App.Vector):
        v_y = App.Vector(0, v_y, 0)
        _wrn(_tr(_text))

    _msg("n_x: {}".format(n_x))
    _msg("n_y: {}".format(n_y))

    try:
        utils.type_check([(n_x, int),
                          (n_y, int)], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be an integer number."))
        return None

    _text = ("Input: number of elements must be at least 1. "
             "It is set to 1.")
    if n_x < 1:
        _wrn(_tr(_text))
        n_x = 1
    if n_y < 1:
        _wrn(_tr(_text))
        n_y = 1

    _msg("use_link: {}".format(bool(use_link)))

    new_obj = Draft.makeArray(obj,
                              arg1=v_x, arg2=v_y,
                              arg3=n_x, arg4=n_y,
                              use_link=use_link)
    return new_obj


def make_rect_array(obj,
                    d_x=10,
                    d_y=10,
                    d_z=10,
                    n_x=2,
                    n_y=2,
                    n_z=1,
                    use_link=True):
    """Create a rectangular array from the given object.

    This function wraps around `make_ortho_array`
    to produce strictly rectangular arrays, in which
    the displacement vectors `v_x`, `v_y`, and `v_z`
    only have their respective components in X, Y, and Z.

    Parameters
    ----------
    obj: Part::Feature
        Any type of object that has a `Part::TopoShape`
        that can be duplicated.
        This means most 2D and 3D objects produced
        with any workbench.

    d_x, d_y, d_z: Base::Vector3, optional
        Displacement of elements in the corresponding X, Y, and Z directions.

    n_x, n_y, n_z: int, optional
        Number of elements in the corresponding X, Y, and Z directions.

    use_link: bool, optional
        If it is `True`, create `App::Link` array.
        See `make_ortho_array`.

    Returns
    -------
    Part::FeaturePython
        A scripted object with `Proxy.Type='Array'`.
        Its `Shape` is a compound of the copies of the original object.

    See Also
    --------
    make_ortho_array, make_ortho_array2d, make_rect_array2d
    """
    _name = "make_rect_array"
    utils.print_header(_name, _tr("Rectangular array"))

    _msg("d_x: {}".format(d_x))
    _msg("d_y: {}".format(d_y))
    _msg("d_z: {}".format(d_z))

    try:
        utils.type_check([(d_x, (int, float)),
                          (d_y, (int, float)),
                          (d_z, (int, float))],
                         name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a number."))
        return None

    new_obj = make_ortho_array(obj,
                               v_x=App.Vector(d_x, 0, 0),
                               v_y=App.Vector(0, d_y, 0),
                               v_z=App.Vector(0, 0, d_z),
                               n_x=n_x,
                               n_y=n_y,
                               n_z=n_z,
                               use_link=use_link)
    return new_obj


def make_rect_array2d(obj,
                      d_x=10,
                      d_y=10,
                      n_x=2,
                      n_y=2,
                      use_link=True):
    """Create a 2D rectangular array from the given object.

    This function wraps around `make_ortho_array2d`
    to produce strictly rectangular arrays, in which
    the displacement vectors `v_x` and `v_y`
    only have their respective components in X and Y.

    Parameters
    ----------
    obj: Part::Feature
        Any type of object that has a `Part::TopoShape`
        that can be duplicated.
        This means most 2D and 3D objects produced
        with any workbench.

    d_x, d_y: Base::Vector3, optional
        Displacement of elements in the corresponding X and Y directions.

    n_x, n_y: int, optional
        Number of elements in the corresponding X and Y directions.

    use_link: bool, optional
        If it is `True`, create `App::Link` array.
        See `make_ortho_array`.

    Returns
    -------
    Part::FeaturePython
        A scripted object with `Proxy.Type='Array'`.
        Its `Shape` is a compound of the copies of the original object.

    See Also
    --------
    make_ortho_array, make_ortho_array2d, make_rect_array
    """
    _name = "make_rect_array2d"
    utils.print_header(_name, _tr("Rectangular array 2D"))

    _msg("d_x: {}".format(d_x))
    _msg("d_y: {}".format(d_y))

    try:
        utils.type_check([(d_x, (int, float)),
                          (d_y, (int, float))],
                         name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a number."))
        return None

    new_obj = make_ortho_array2d(obj,
                                 v_x=App.Vector(d_x, 0, 0),
                                 v_y=App.Vector(0, d_y, 0),
                                 n_x=n_x,
                                 n_y=n_y,
                                 use_link=use_link)
    return new_obj
