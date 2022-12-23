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
"""Provides functions to create Text objects."""
## @package make_text
# \ingroup draftmake
# \brief Provides functions to create Text objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftutils.messages import _msg, _err
from draftutils.translate import translate
from draftobjects.text import Text

if App.GuiUp:
    from draftviewproviders.view_text import ViewProviderText


def make_text(string, placement=None, screen=False):
    """Create a Text object containing the given list of strings.

    The current color and text height and font specified in preferences
    are used.

    Parameters
    ----------
    string: str, or list of str
        String to display on screen.
        If it is a list, each element in the list represents a new text line.

    placement: Base::Placement, Base::Vector3, or Base::Rotation, optional
        It defaults to `None`.
        If it is provided, it is the placement of the new text.
        The input could be a full placement, just a vector indicating
        the translation, or just a rotation.

    screen: bool, optional
        It defaults to `False`, in which case the text is placed in 3D space
        oriented like any other object, on top of a given plane,
        by the default the XY plane.
        If it is `True`, the text will always face perpendicularly
        to the camera direction, that is, it will be flat on the screen.

    Returns
    -------
    App::FeaturePython
        A scripted object of type `'Text'`.
        This object does not have a `Shape` attribute, as the text is created
        on screen by Coin (pivy).

    None
        If there is a problem it will return `None`.
    """
    _name = "make_text"
    utils.print_header(_name, "Text")

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft","No active document. Aborting."))
        return None

    _msg("string: {}".format(string))
    try:
        utils.type_check([(string, (str, list))], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be a list of strings or a single string."))
        return None

    if (type(string) is list
            and not all(isinstance(element, str) for element in string)):
        _err(translate("draft","Wrong input: must be a list of strings or a single string."))
        return None

    _msg("placement: {}".format(placement))
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

    new_obj = doc.addObject("App::FeaturePython",
                            "Text")
    Text(new_obj)
    new_obj.Text = string
    new_obj.Placement = placement

    if App.GuiUp:
        ViewProviderText(new_obj.ViewObject)

        h = utils.get_param("textheight", 2)

        new_obj.ViewObject.DisplayMode = "World"
        if screen:
            _msg("screen: {}".format(screen))
            new_obj.ViewObject.DisplayMode = "Screen"
            h = h * 10

        new_obj.ViewObject.FontSize = h
        new_obj.ViewObject.FontName = utils.get_param("textfont", "")
        new_obj.ViewObject.LineSpacing = 1

        gui_utils.format_object(new_obj)
        gui_utils.select(new_obj)

    return new_obj


def makeText(stringlist, point=App.Vector(0, 0, 0), screen=False):
    """Create Text. DEPRECATED. Use 'make_text'."""
    utils.use_instead("make_text")

    return make_text(stringlist, point, screen)


def convert_draft_texts(textslist=None):
    """Convert the given Annotation to a Draft text.

    In the past, the `Draft Text` object didn't exist; text objects
    were of type `App::Annotation`. This function was introduced
    to convert those older objects to a `Draft Text` scripted object.

    This function was already present at splitting time during v0.19.

    Parameters
    ----------
    textslist: list of objects, optional
        It defaults to `None`.
        A list containing `App::Annotation` objects or a single of these
        objects.
        If it is `None` it will convert all objects in the current document.
    """
    _name = "convert_draft_texts"
    utils.print_header(_name, "Convert Draft texts")

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft","No active document. Aborting."))
        return None

    if not textslist:
        textslist = list()
        for obj in doc.Objects:
            if obj.TypeId == "App::Annotation":
                textslist.append(obj)

    if not isinstance(textslist, list):
        textslist = [textslist]

    to_delete = []

    for obj in textslist:
        label = obj.Label
        obj.Label = label + ".old"

        # Create a new Draft Text object
        new_obj = make_text(obj.LabelText, placement=obj.Position)
        new_obj.Label = label
        to_delete.append(obj)

        # Move the new object to the group which contained the old object
        for in_obj in obj.InList:
            if in_obj.isDerivedFrom("App::DocumentObjectGroup"):
                if obj in in_obj.Group:
                    group = in_obj.Group
                    group.append(new_obj)
                    in_obj.Group = group

    for obj in to_delete:
        doc.removeObject(obj.Name)


def convertDraftTexts(textslist=[]):
    """Convert Text. DEPRECATED. Use 'convert_draft_texts'."""
    utils.use_instead("convert_draft_texts")
    return convert_draft_texts(textslist)

## @}
