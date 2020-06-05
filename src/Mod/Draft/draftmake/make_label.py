# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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
"""Provides the make function to create Draft Label objects."""
## @package make_label
# \ingroup DRAFT
# \brief Provides the make function to create Draft Label objects.

import FreeCAD as App
import draftutils.gui_utils as gui_utils
import draftutils.utils as utils

from draftutils.messages import _msg, _wrn, _err
from draftutils.translate import _tr
from draftobjects.label import Label

if App.GuiUp:
    from draftviewproviders.view_label import ViewProviderLabel


def make_label(target_point=App.Vector(0, 0, 0),
               placement=App.Vector(30, 30, 0),
               target=None,
               label_type="Custom", custom_text="Label",
               direction="Horizontal", distance=-10,
               points=None):
    """Create a Label object containing different types of information.

    The current color and text height and font specified in preferences
    are used.

    Parameters
    ----------
    target_point: Base::Vector3, optional
        It defaults to the origin `App.Vector(0, 0, 0)`.
        This is the point which is pointed to by the label's leader line.
        This point can be adorned with a marker like an arrow or circle.

    placement: Base::Placement, Base::Vector3, or Base::Rotation, optional
        It defaults to `App.Vector(30, 30, 0)`.
        If it is provided, it defines the base point of the textual
        label.
        The input could be a full placement, just a vector indicating
        the translation, or just a rotation.

    target: list, optional
        It defaults to `None`.
        The list should be a `LinkSubList`, that is, it should contain
        two elements; the first element should be an object which will be used
        to provide information to the label; the second element should be
        a string indicating a subelement name, either `'VertexN'`, `'EdgeN'`,
        or `'FaceN'` which exists within the first element.
        In this case `'N'` is a number that starts with `1`
        and goes up to the maximum number of vertices, edges, or faces.
        ::
            target = [Part::Feature, 'Edge1']

        The target may not need a subelement, in which case the second
        element of the list may be empty.
        ::
            target = [Part::Feature, ]

        This `LinkSubList` can be obtained from the `Gui::Selection`
        module.
        ::
            sel_object = Gui.Selection.getSelectionEx()[0]
            object = sel_object.Object
            subelement = sel_object.SubElementNames[0]
            target = [object, subelement]

    label_type: str, optional
        It defaults to `'Custom'`.
        It can be `'Custom'`, `'Name'`, `'Label'`, `'Position'`,
        `'Length'`, `'Area'`, `'Volume'`, `'Tag'`, or `'Material'`.
        It indicates the type of information that will be shown in the label.

        Only `'Custom'` allows you to manually set the text
        by defining `custom_text`. The other types take their information
        from the object included in `target`.

        - `'Position'` will show the base position of the target object,
          or of the indicated `'VertexN'` in `target`.
        - `'Length'` will show the `Length` of the target object's `Shape`,
          or of the indicated `'EdgeN'` in `target`.
        - `'Area'` will show the `Area` of the target object's `Shape`,
          or of the indicated `'FaceN'` in `target`.

    custom_text: str, optional
        It defaults to `'Label'`.
        It is the text that will be displayed by the label when
        `label_type` is `'Custom'`.

    direction: str, optional
        It defaults to `'Horizontal'`.
        It can be `'Horizontal'`, `'Vertical'`, or `'Custom'`.
        It indicates the direction of the straight segment of the leader line
        that ends up next to the textual label.

        If `'Custom'` is selected, the leader line can be manually drawn
        by specifying the value of `points`.
        Normally, the leader line has only three points, but with `'Custom'`
        you can specify as many points as needed.

    distance: int, float, Base::Quantity, optional
        It defaults to -10.
        It indicates the length of the horizontal or vertical segment
        of the leader line.

        The leader line is composed of two segments, the first segment is
        inclined, while the second segment is either horizontal or vertical
        depending on the value of `direction`.
        ::
            T
            |
            |
            o------- L text

        The `oL` segment's length is defined by `distance`
        while the `oT` segment is automatically calculated depending
        on the values of `placement` (L) and `distance` (o).

        This `distance` is oriented, meaning that if it is positive
        the segment will be to the right and above of the textual
        label, depending on if `direction` is `'Horizontal'` or `'Vertical'`,
        respectively.
        If it is negative, the segment will be to the left
        and below of the text.

    points: list of Base::Vector3, optional
        It defaults to `None`.
        It is a list of vectors defining the shape of the leader line;
        the list must have at least two points.
        This argument must be used together with `direction='Custom'`
        to display this custom leader.

        However, notice that if the Label's `StraightDirection` property
        is later changed to `'Horizontal'` or `'Vertical'`,
        the custom point list will be overwritten with a new,
        automatically calculated three-point list.

        For the object to use custom points, `StraightDirection`
        must remain `'Custom'`, and then the `Points` property
        can be overwritten by a suitable list of points.

    Returns
    -------
    App::FeaturePython
        A scripted object of type `'Label'`.
        This object does not have a `Shape` attribute, as the text and lines
        are created on screen by Coin (pivy).

    None
        If there is a problem it will return `None`.
    """
    _name = "make_label"
    utils.print_header(_name, "Label")

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(_tr("No active document. Aborting."))
        return None

    _msg("target_point: {}".format(target_point))
    if not target_point:
        target_point = App.Vector(0, 0, 0)
    try:
        utils.type_check([(target_point, App.Vector)], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a vector."))
        return None

    _msg("placement: {}".format(placement))
    if not placement:
        placement = App.Placement()
    try:
        utils.type_check([(placement, (App.Placement,
                                       App.Vector,
                                       App.Rotation))], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a placement, a vector, "
                 "or a rotation."))
        return None

    # Convert the vector or rotation to a full placement
    if isinstance(placement, App.Vector):
        placement = App.Placement(placement, App.Rotation())
    elif isinstance(placement, App.Rotation):
        placement = App.Placement(App.Vector(), placement)

    _msg("target: {}".format(target))
    if target:
        try:
            utils.type_check([(target, (tuple, list))],
                             name=_name)
        except TypeError:
            _err(_tr("Wrong input: must be a LinkSubList of two elements. "
                     "For example, [object, 'Edge1']"))
            return None

        target = list(target)
        if len(target) == 1:
            target.append([])

    _msg("label_type: {}".format(label_type))
    if not label_type:
        label_type = "Custom"
    try:
        utils.type_check([(label_type, str)], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a string, "
                 "'Custom', 'Name', 'Label', 'Position', "
                 "'Length', 'Area', 'Volume', 'Tag', or 'Material'."))
        return None

    if label_type not in ("Custom", "Name", "Label", "Position",
                          "Length", "Area", "Volume", "Tag", "Material"):
        _err(_tr("Wrong input: must be a string, "
                 "'Custom', 'Name', 'Label', 'Position', "
                 "'Length', 'Area', 'Volume', 'Tag', or 'Material'."))
        return None

    _msg("custom_text: {}".format(custom_text))
    if not custom_text:
        custom_text = "Label"
    try:
        utils.type_check([(custom_text, str)], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a string."))
        return None

    _msg("direction: {}".format(direction))
    if not direction:
        direction = "Horizontal"
    try:
        utils.type_check([(direction, str)], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a string, "
                 "'Horizontal', 'Vertical', or 'Custom'."))
        return None

    if direction not in ("Horizontal", "Vertical", "Custom"):
        _err(_tr("Wrong input: must be a string, "
                 "'Horizontal', 'Vertical', or 'Custom'."))
        return None

    _msg("distance: {}".format(distance))
    if not distance:
        distance = 1
    try:
        utils.type_check([(distance, (int, float))], name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a number."))
        return None

    if points:
        _msg("points: {}".format(points))

        _err_msg = _tr("Wrong input: must be a list of at least two vectors.")
        try:
            utils.type_check([(points, (tuple, list))], name=_name)
        except TypeError:
            _err(_err_msg)
            return None

        if len(points) < 2:
            _err(_err_msg)
            return None

        if not all(isinstance(p, App.Vector) for p in points):
            _err(_err_msg)
            return None

    new_obj = doc.addObject("App::FeaturePython",
                            "dLabel")
    Label(new_obj)

    new_obj.TargetPoint = target_point
    new_obj.Placement = placement
    if target:
        new_obj.Target = target

    new_obj.LabelType = label_type
    new_obj.CustomText = custom_text

    new_obj.StraightDirection = direction
    new_obj.StraightDistance = distance
    if points:
        if direction != "Custom":
            _wrn(_tr("Direction is not 'Custom'; "
                     "points won't be used."))
        new_obj.Points = points

    if App.GuiUp:
        ViewProviderLabel(new_obj.ViewObject)
        h = utils.get_param("textheight", 0.20)
        new_obj.ViewObject.TextSize = h

        gui_utils.format_object(new_obj)
        gui_utils.select(new_obj)

    return new_obj


def makeLabel(targetpoint=None, target=None, direction=None,
              distance=None, labeltype=None, placement=None):
    """Create a Label. DEPRECATED. Use 'make_label'."""
    utils.use_instead("make_label")

    return make_label(target_point=targetpoint,
                      placement=placement,
                      target=target,
                      label_type=labeltype,
                      direction=direction,
                      distance=distance)
