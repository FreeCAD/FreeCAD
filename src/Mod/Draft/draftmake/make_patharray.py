# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2013 WandererFan <wandererfan@gmail.com>                *
# *   Copyright (c) 2019 Zheng, Lei (realthunder)<realthunder.dev@gmail.com>*
# *   Copyright (c) 2020 Carlo Pavan <carlopav@gmail.com>                   *
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
"""Provides functions to create PathArray objects.

The copies will be placed along a path like a polyline, spline, or bezier
curve.
"""
## @package make_patharray
# \ingroup draftmake
# \brief Provides functions to create PathArray objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftutils.messages import _msg, _err
from draftutils.translate import translate
from draftobjects.patharray import PathArray
from draftobjects.pathtwistedarray import PathTwistedArray

if App.GuiUp:
    from draftutils.todo import ToDo
    from draftviewproviders.view_array import ViewProviderDraftArray
    from draftviewproviders.view_draftlink import ViewProviderDraftLink


def make_path_array(base_object, path_object, count=4,
                    extra=App.Vector(0, 0, 0), subelements=None,
                    align=False, align_mode="Original",
                    tan_vector=App.Vector(1, 0, 0),
                    force_vertical=False,
                    vertical_vector=App.Vector(0, 0, 1),
                    start_offset=0.0, end_offset=0.0,
                    use_link=True):
    """Make a Draft PathArray object.

    Distribute copies of a `base_object` along `path_object`
    or `subelements` from `path_object`.

    Parameters
    ----------
    base_object: Part::Feature or str
        Any of object that has a `Part::TopoShape` that can be duplicated.
        This means most 2D and 3D objects produced with any workbench.
        If it is a string, it must be the `Label` of that object.
        Since a label is not guaranteed to be unique in a document,
        it will use the first object found with this label.

    path_object: Part::Feature or str
        Path object like a polyline, B-Spline, or bezier curve that should
        contain edges.
        Just like `base_object` it can also be `Label`.

    count: int, float, optional
        It defaults to 4.
        Number of copies to create along the `path_object`.
        It must be at least 2.
        If a `float` is provided, it will be truncated by `int(count)`.

    extra: Base.Vector3, optional
        It defaults to `App.Vector(0, 0, 0)`.
        It translates each copy by the value of `extra`.
        This is useful to adjust for the difference between shape centre
        and shape reference point.

    subelements: list or tuple of str, optional
        It defaults to `None`.
        It should be a list of names of edges that must exist in `path_object`.
        Then the path array will be created along these edges only,
        and not the entire `path_object`.
        ::
            subelements = ['Edge1', 'Edge2']

        The edges must be contiguous, meaning that it is not allowed to
        input `'Edge1'` and `'Edge3'` if they do not touch each other.

        A single string value is also allowed.
        ::
            subelements = 'Edge1'

    align: bool, optional
        It defaults to `False`.
        If it is `True` it will align `base_object` to tangent, normal,
        or binormal to the `path_object`, depending on the value
        of `tan_vector`.

    align_mode: str, optional
        It defaults to `'Original'` which is the traditional alignment.
        It can also be `'Frenet'` or `'Tangent'`.

        - Original. It does not calculate curve normal.
          `X` is curve tangent, `Y` is normal parameter, Z is the cross
          product `X` x `Y`.
        - Frenet. It defines a local coordinate system along the path.
          `X` is tangent to curve, `Y` is curve normal, `Z` is curve binormal.
          If normal cannot be computed, for example, in a straight path,
          a default is used.
        - Tangent. It is similar to `'Original'` but includes a pre-rotation
          to align the base object's `X` to the value of `tan_vector`,
          then `X` follows curve tangent.

    tan_vector: Base::Vector3, optional
        It defaults to `App.Vector(1, 0, 0)` or the +X axis.
        It aligns the tangent of the path to this local unit vector
        of the object.

    force_vertical: Base::Vector3, optional
        It defaults to `False`.
        If it is `True`, the value of `vertical_vector`
        will be used when `align_mode` is `'Original'` or `'Tangent'`.

    vertical_vector: Base::Vector3, optional
        It defaults to `App.Vector(0, 0, 1)` or the +Z axis.
        It will force this vector to be the vertical direction
        when `force_vertical` is `True`.

    start_offset: float, optional
        It defaults to 0.0.
        It is the length from the start of the path to the first copy.

    end_offset: float, optional
        It defaults to 0.0.
        It is the length from the end of the path to the last copy.

    use_link: bool, optional
        It defaults to `True`, in which case the copies are `App::Link`
        elements. Otherwise, the copies are shape copies which makes
        the resulting array heavier.

    Returns
    -------
    Part::FeaturePython
        The scripted object of type `'PathArray'`.
        Its `Shape` is a compound of the copies of the original object.

    None
        If there is a problem it will return `None`.
    """
    _name = "make_path_array"
    utils.print_header(_name, "Path array")

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft","No active document. Aborting."))
        return None

    if isinstance(base_object, str):
        base_object_str = base_object

    found, base_object = utils.find_object(base_object, doc)
    if not found:
        _msg("base_object: {}".format(base_object_str))
        _err(translate("draft","Wrong input: object not in document."))
        return None

    _msg("base_object: {}".format(base_object.Label))

    if isinstance(path_object, str):
        path_object_str = path_object

    found, path_object = utils.find_object(path_object, doc)
    if not found:
        _msg("path_object: {}".format(path_object_str))
        _err(translate("draft","Wrong input: object not in document."))
        return None

    _msg("path_object: {}".format(path_object.Label))

    _msg("count: {}".format(count))
    try:
        utils.type_check([(count, (int, float))],
                         name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a number."))
        return None
    count = int(count)

    _msg("extra: {}".format(extra))
    try:
        utils.type_check([(extra, App.Vector)],
                         name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a vector."))
        return None

    _msg("subelements: {}".format(subelements))
    if subelements:
        try:
            # Make a list
            if isinstance(subelements, str):
                subelements = [subelements]

            utils.type_check([(subelements, (list, tuple, str))],
                             name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: must be a list or tuple of strings, or a single string."))
            return None

        # The subelements list is used to build a special list
        # called a LinkSubList, which includes the path_object.
        # Old style: [(path_object, "Edge1"), (path_object, "Edge2")]
        # New style: [(path_object, ("Edge1", "Edge2"))]
        #
        # If a simple list is given ["a", "b"], this will create an old-style
        # SubList.
        # If a nested list is given [["a", "b"]], this will create a new-style
        # SubList.
        # In any case, the property of the object accepts both styles.
        #
        # If the old style is deprecated then this code should be updated
        # to create new style lists exclusively.
        sub_list = list()
        for sub in subelements:
            sub_list.append((path_object, sub))
    else:
        sub_list = None

    align = bool(align)
    _msg("align: {}".format(align))

    _msg("align_mode: {}".format(align_mode))
    try:
        utils.type_check([(align_mode, str)],
                         name=_name)

        if align_mode not in ("Original", "Frenet", "Tangent"):
            raise TypeError
    except TypeError:
        _err(translate("draft","Wrong input: must be 'Original', 'Frenet', or 'Tangent'."))
        return None

    _msg("tan_vector: {}".format(tan_vector))
    try:
        utils.type_check([(tan_vector, App.Vector)],
                         name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a vector."))
        return None

    force_vertical = bool(force_vertical)
    _msg("force_vertical: {}".format(force_vertical))

    _msg("vertical_vector: {}".format(vertical_vector))
    try:
        utils.type_check([(vertical_vector, App.Vector)],
                         name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a vector."))
        return None

    _msg("start_offset: {}".format(start_offset))
    try:
        utils.type_check([(start_offset, (int, float))],
                         name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a number."))
        return None
    start_offset = float(start_offset)

    _msg("end_offset: {}".format(end_offset))
    try:
        utils.type_check([(end_offset, (int, float))],
                         name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a number."))
        return None
    end_offset = float(end_offset)

    use_link = bool(use_link)
    _msg("use_link: {}".format(use_link))

    if use_link:
        # The PathArray class must be called in this special way
        # to make it a PathLinkArray
        new_obj = doc.addObject("Part::FeaturePython", "PathArray",
                                PathArray(None), None, True)
    else:
        new_obj = doc.addObject("Part::FeaturePython", "PathArray")
        PathArray(new_obj)

    new_obj.Base = base_object
    new_obj.PathObject = path_object
    new_obj.Count = count
    new_obj.ExtraTranslation = extra
    new_obj.PathSubelements = sub_list
    new_obj.Align = align
    new_obj.AlignMode = align_mode
    new_obj.TangentVector = tan_vector
    new_obj.ForceVertical = force_vertical
    new_obj.VerticalVector = vertical_vector
    new_obj.StartOffset = start_offset
    new_obj.EndOffset = end_offset

    if App.GuiUp:
        if use_link:
            ViewProviderDraftLink(new_obj.ViewObject)
        else:
            ViewProviderDraftArray(new_obj.ViewObject)
            gui_utils.formatObject(new_obj, new_obj.Base)
            new_obj.ViewObject.Proxy.resetColors(new_obj.ViewObject)
            # Workaround to trigger update of DiffuseColor:
            ToDo.delay(reapply_diffuse_color, new_obj.ViewObject)
        new_obj.Base.ViewObject.hide()
        gui_utils.select(new_obj)

    return new_obj


def makePathArray(baseobject, pathobject, count,
                  xlate=None, align=False,
                  pathobjsubs=[],
                  use_link=False):
    """Create PathArray. DEPRECATED. Use 'make_path_array'."""
    utils.use_instead('make_path_array')

    return make_path_array(baseobject, pathobject, count,
                           xlate, pathobjsubs,
                           align,
                           use_link)


def make_path_twisted_array(base_object, path_object,
                            count=15, rot_factor=0.25,
                            use_link=True):
    """Create a Path twisted array."""
    _name = "make_path_twisted_array"
    utils.print_header(_name, "Path twisted array")

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft","No active document. Aborting."))
        return None

    if isinstance(base_object, str):
        base_object_str = base_object

    found, base_object = utils.find_object(base_object, doc)
    if not found:
        _msg("base_object: {}".format(base_object_str))
        _err(translate("draft","Wrong input: object not in document."))
        return None

    _msg("base_object: {}".format(base_object.Label))

    if isinstance(path_object, str):
        path_object_str = path_object

    found, path_object = utils.find_object(path_object, doc)
    if not found:
        _msg("path_object: {}".format(path_object_str))
        _err(translate("draft","Wrong input: object not in document."))
        return None

    _msg("path_object: {}".format(path_object.Label))

    _msg("count: {}".format(count))
    try:
        utils.type_check([(count, (int, float))],
                         name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a number."))
        return None
    count = int(count)

    use_link = bool(use_link)
    _msg("use_link: {}".format(use_link))

    if use_link:
        # The PathTwistedArray class must be called in this special way
        # to make it a PathTwistLinkArray
        new_obj = doc.addObject("Part::FeaturePython", "PathTwistedArray",
                                PathTwistedArray(None), None, True)
    else:
        new_obj = doc.addObject("Part::FeaturePython", "PathTwistedArray")
        PathTwistedArray(new_obj)

    new_obj.Base = base_object
    new_obj.PathObject = path_object
    new_obj.Count = count
    new_obj.RotationFactor = rot_factor

    if App.GuiUp:
        if use_link:
            ViewProviderDraftLink(new_obj.ViewObject)
        else:
            ViewProviderDraftArray(new_obj.ViewObject)
            gui_utils.formatObject(new_obj, new_obj.Base)
            new_obj.ViewObject.Proxy.resetColors(new_obj.ViewObject)
            # Workaround to trigger update of DiffuseColor:
            ToDo.delay(reapply_diffuse_color, new_obj.ViewObject)
        new_obj.Base.ViewObject.hide()
        gui_utils.select(new_obj)

    return new_obj


def reapply_diffuse_color(vobj):
    try:
        vobj.DiffuseColor = vobj.DiffuseColor
    except:
        pass

## @}
