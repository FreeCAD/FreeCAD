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
"""Provides the make function to create Draft Text objects."""
## @package make_text
# \ingroup DRAFT
# \brief Provides the make function to create Draft Text objects.

import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftutils.messages import _msg, _err
from draftutils.translate import _tr
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

        If it is a list, each element in the list should be a string.
        In this case each element will be printed in its own line, that is,
        a newline will be added at the end of each string.

        If an empty string is passed `''` this won't cause an error
        but the text `'Label'` will be displayed in the 3D view.

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
        _err(_tr("No active document. Aborting."))
        return None

    _msg("string: {}".format(string))
    try:
        utils.type_check([(string, (str, list))])
    except TypeError:
        _err(_tr("Wrong input: must be a list of strings "
                 "or a single string."))
        return None

    if not all(isinstance(element, str) for element in string):
        _err(_tr("Wrong input: must be a list of strings "
                 "or a single string."))
        return None

    if isinstance(string, str):
        string = [string]

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

    new_obj = doc.addObject("App::FeaturePython",
                            "Text")
    Text(new_obj)
    new_obj.Text = string
    new_obj.Placement = placement

    if App.GuiUp:
        ViewProviderText(new_obj.ViewObject)

        h = utils.get_param("textheight", 0.20)

        if screen:
            _msg("screen: {}".format(screen))
            new_obj.ViewObject.DisplayMode = "3D text"
            h = h*10

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
