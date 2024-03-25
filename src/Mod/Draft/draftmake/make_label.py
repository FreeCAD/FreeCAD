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
"""Provides functions to create Label objects."""
## @package make_label
# \ingroup draftmake
# \brief Provides functions to create Label objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
from draftobjects import label
from draftutils import gui_utils
from draftutils import params
from draftutils import utils
from draftutils.messages import _wrn, _err
from draftutils.translate import translate

if App.GuiUp:
    from draftviewproviders.view_label import ViewProviderLabel


def make_label(target_point=App.Vector(0, 0, 0),
               placement=App.Vector(30, 30, 0),
               target_object=None, subelements=None,
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

    target_object: Part::Feature or str, optional
        It defaults to `None`.
        If it exists it should be an object which will be used to provide
        information to the label, as long as `label_type` is different
        from `'Custom'`.

        If it is a string, it must be the `Label` of that object.
        Since a `Label` is not guaranteed to be unique in a document,
        it will use the first object found with this `Label`.

    subelements: str, optional
        It defaults to `None`.
        If `subelements` is provided, `target_object` should be provided
        as well, otherwise it is ignored.

        It should be a string indicating a subelement name, either
        `'VertexN'`, `'EdgeN'`, or `'FaceN'` which should exist
        within `target_object`.
        In this case `'N'` is an integer that indicates the specific number
        of vertex, edge, or face in `target_object`.

        Both `target_object` and `subelements` are used to link the label
        to a particular object, or to the particular vertex, edge, or face,
        and get information from them.
        ::
            make_label(..., target_object=App.ActiveDocument.Box)
            make_label(..., target_object="My box", subelements="Face3")

        These two parameters can be can be obtained from the `Gui::Selection`
        module.
        ::
            sel_object = Gui.Selection.getSelectionEx()[0]
            target_object = sel_object.Object
            subelements = sel_object.SubElementNames[0]

    label_type: str, optional
        It defaults to `'Custom'`.
        It indicates the type of information that will be shown in the label.
        See the get_label_types function in label.py for supported types.

        Only `'Custom'` allows you to manually set the text
        by defining `custom_text`. The other types take their information
        from the object included in `target`.

        - `'Position'` will show the base position of the target object,
          or of the indicated `'VertexN'` in `target`.
        - `'Length'` will show the `Length` of the target object's `Shape`,
          or of the indicated `'EdgeN'` in `target`.
        - `'Area'` will show the `Area` of the target object's `Shape`,
          or of the indicated `'FaceN'` in `target`.

    custom_text: str, or list of str, optional
        It defaults to `'Label'`.
        If it is a list, each element in the list represents a new text line.

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

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft","No active document. Aborting."))
        return None

    if not target_point:
        target_point = App.Vector(0, 0, 0)
    try:
        utils.type_check([(target_point, App.Vector)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a vector."))
        return None

    if not placement:
        placement = App.Placement()
    try:
        utils.type_check([(placement, (App.Placement,
                                       App.Vector,
                                       App.Rotation))], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a placement, a vector, or a rotation."))
        return None

    # Convert the vector or rotation to a full placement
    if isinstance(placement, App.Vector):
        placement = App.Placement(placement, App.Rotation())
    elif isinstance(placement, App.Rotation):
        placement = App.Placement(App.Vector(), placement)
        
    if target_object:
        if isinstance(target_object, (list, tuple)):
            _err(translate("draft","Wrong input: target_object must not be a list."))
            return None

        found, target_object = utils.find_object(target_object, doc)
        if not found:
            _err(translate("draft","Wrong input: target_object not in document."))
            return None

    if target_object and subelements:
        try:
            # Make a list
            if isinstance(subelements, str):
                subelements = [subelements]

            utils.type_check([(subelements, (list, tuple, str))],
                             name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: subelements must be a list or tuple of strings, or a single string."))
            return None

        # The subelements list is used to build a special list
        # called a LinkSub, which includes the target_object
        # and the subelements.
        # Single: (target_object, "Edge1")
        # Multiple: (target_object, ("Edge1", "Edge2"))
        for sub in subelements:
            _sub = target_object.getSubObject(sub)
            if not _sub:
                _err(translate("draft","Wrong input: subelement {} not in object.").format(sub))
                return None

    if not label_type:
        label_type = "Custom"
    try:
        utils.type_check([(label_type, str)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: label_type must be a string."))
        return None

    types = label.get_label_types()
    if label_type not in types:
        _err(translate("draft", "Wrong input: label_type must be one of the following:") + " " + str(types).strip("[]"))
        return None

    if not custom_text:
        custom_text = "Label"
    try:
        utils.type_check([(custom_text, (str, list))], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a list of strings or a single string."))
        return None

    if (type(custom_text) is list
            and not all(isinstance(element, str) for element in custom_text)):
        _err(translate("draft","Wrong input: must be a list of strings or a single string."))
        return None

    if not direction:
        direction = "Horizontal"
    try:
        utils.type_check([(direction, str)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a string, 'Horizontal', 'Vertical', or 'Custom'."))
        return None

    if direction not in ("Horizontal", "Vertical", "Custom"):
        _err(translate("draft","Wrong input: must be a string, 'Horizontal', 'Vertical', or 'Custom'."))
        return None

    if not distance:
        distance = 1
    try:
        utils.type_check([(distance, (int, float))], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a number."))
        return None

    if points:
        _err_msg = translate("draft","Wrong input: points {} must be a list of at least two vectors.").format(points)
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
    label.Label(new_obj)

    new_obj.TargetPoint = target_point
    new_obj.Placement = placement
    if target_object:
        if subelements:
            new_obj.Target = [target_object, subelements]
        else:
            new_obj.Target = [target_object, []]

    new_obj.LabelType = label_type
    new_obj.CustomText = custom_text

    new_obj.StraightDirection = direction
    new_obj.StraightDistance = distance
    if points:
        if direction != "Custom":
            _wrn(translate("draft","Direction is not 'Custom'; points won't be used."))
        new_obj.Points = points

    if App.GuiUp:
        ViewProviderLabel(new_obj.ViewObject)
        h = params.get_param("textheight")
        new_obj.ViewObject.FontSize = h

        gui_utils.format_object(new_obj)
        gui_utils.select(new_obj)

    return new_obj


def makeLabel(targetpoint=None, target=None, direction=None,
              distance=None, labeltype=None, placement=None):
    """Create a Label. DEPRECATED. Use 'make_label'."""
    utils.use_instead("make_label")

    _name = "makeLabel"
    subelements = None

    if target:
        try:
            utils.type_check([(target, (tuple, list))],
                             name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: must be a list of two elements. For example, [object, 'Edge1']."))
            return None

    # In the old function `target` is the original parameter,
    # a list of two elements, the target object itself, and the subelement.
    # If the list is a single element, it is expanded to two elements
    # with the second being empty
    # target = [object]
    # target = [object, ]
    # target = [object, []]
    # target = (object, )
    # target = (object, ())

    # Parentheses can be used as well except a single pair
    # target = (object)
    target = list(target)
    if len(target) == 1:
        target.append([])

    target_object = target[0]
    subelements = target[1]

    return make_label(target_point=targetpoint,
                      placement=placement,
                      target_object=target_object,
                      subelements=subelements,
                      label_type=labeltype,
                      direction=direction,
                      distance=distance)

## @}
