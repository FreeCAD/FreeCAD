# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
"""Provides functions to create Layer objects."""
## @package make_layer
# \ingroup draftmake
# \brief Provides functions to create Layer objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.utils as utils

from draftutils.messages import _msg, _err
from draftutils.translate import translate
from draftobjects.layer import (Layer,
                                LayerContainer)

if App.GuiUp:
    from draftviewproviders.view_layer import (ViewProviderLayer,
                                               ViewProviderLayerContainer)

view_group = App.ParamGet("User parameter:BaseApp/Preferences/View")


def get_layer_container():
    """Return a group object to put layers in.

    Returns
    -------
    App::DocumentObjectGroupPython
        The existing group object named `'LayerContainer'`
        of type `LayerContainer`.
        If it doesn't exist it will create it with this default Name.
    """
    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft","No active document. Aborting."))
        return None

    for obj in doc.Objects:
        if obj.Name == "LayerContainer":
            return obj

    new_obj = doc.addObject("App::DocumentObjectGroupPython",
                            "LayerContainer")
    new_obj.Label = translate("draft", "Layers")

    LayerContainer(new_obj)

    if App.GuiUp:
        ViewProviderLayerContainer(new_obj.ViewObject)

    return new_obj


def getLayerContainer():
    """Get the Layer container. DEPRECATED. Use 'get_layer_container'."""
    utils.use_instead("get_layer_container")

    return get_layer_container()


def make_layer(name=None,
               line_color=None, shape_color=None,
               line_width=2.0,
               draw_style="Solid", transparency=0):
    """Create a Layer object in the active document.

    If a layer container named `'LayerContainer'` does not exist,
    it is created with this name.

    A layer controls the view properties of the objects inside the layer,
    so all parameters except for `name` only apply if the graphical interface
    is up.

    Parameters
    ----------
    name: str, optional
        It is used to set the layer's `Label` (user editable).
        It defaults to `None`, in which case the `Label`
        is set to `'Layer'` or to its translation in the current language.

    line_color: tuple, optional
        It defaults to `None`, in which case it uses the value of the parameter
        `User parameter:BaseApp/Preferences/View/DefaultShapeLineColor`.
        If it is given, it should be a tuple of three
        floating point values from 0.0 to 1.0.

    shape_color: tuple, optional
        It defaults to `None`, in which case it uses the value of the parameter
        `User parameter:BaseApp/Preferences/View/DefaultShapeColor`.
        If it is given, it should be a tuple of three
        floating point values from 0.0 to 1.0.

    line_width: float, optional
        It defaults to 2.0.
        It determines the width of the edges of the objects contained
        in the layer.

    draw_style: str, optional
        It defaults to `'Solid'`.
        It determines the style of the edges of the objects contained
        in the layer.
        If it is given, it should be 'Solid', 'Dashed', 'Dotted',
        or 'Dashdot'.

    transparency: int, optional
        It defaults to 0.
        It should be an integer value from 0 (completely opaque)
        to 100 (completely transparent).

    Return
    ------
    App::FeaturePython
        A scripted object of type `'Layer'`.
        This object does not have a `Shape` attribute.
        Modifying the view properties of this object will affect the objects
        inside of it.

    None
        If there is a problem it will return `None`.
    """
    _name = "make_layer"
    utils.print_header(_name, translate("draft","Layer"))

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft","No active document. Aborting."))
        return None

    if name:
        _msg("name: {}".format(name))
        try:
            utils.type_check([(name, str)], name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: it must be a string."))
            return None
    else:
        name = translate("draft", "Layer")

    if line_color:
        _msg("line_color: {}".format(line_color))
        try:
            utils.type_check([(line_color, tuple)], name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: must be a tuple of three floats 0.0 to 1.0."))
            return None

        if not all(isinstance(color, (int, float)) for color in line_color):
            _err(translate("draft","Wrong input: must be a tuple of three floats 0.0 to 1.0."))
            return None
    else:
        c = view_group.GetUnsigned("DefaultShapeLineColor", 255)
        line_color = (((c >> 24) & 0xFF) / 255,
                      ((c >> 16) & 0xFF) / 255,
                      ((c >> 8) & 0xFF) / 255)

    if shape_color:
        _msg("shape_color: {}".format(shape_color))
        try:
            utils.type_check([(shape_color, tuple)], name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: must be a tuple of three floats 0.0 to 1.0."))
            return None

        if not all(isinstance(color, (int, float)) for color in shape_color):
            _err(translate("draft","Wrong input: must be a tuple of three floats 0.0 to 1.0."))
            return None
    else:
        c = view_group.GetUnsigned("DefaultShapeColor", 4294967295)
        shape_color = (((c >> 24) & 0xFF) / 255,
                       ((c >> 16) & 0xFF) / 255,
                       ((c >> 8) & 0xFF) / 255)

    _msg("line_width: {}".format(line_width))
    try:
        utils.type_check([(line_width, (int, float))], name=_name)
        line_width = float(abs(line_width))
    except TypeError:
        _err(translate("draft","Wrong input: must be a number."))
        return None

    _msg("draw_style: {}".format(draw_style))
    try:
        utils.type_check([(draw_style, str)], name=_name)
    except TypeError:
        _err(translate("draft","Wrong input: must be 'Solid', 'Dashed', 'Dotted', or 'Dashdot'."))
        return None

    if draw_style not in ('Solid', 'Dashed', 'Dotted', 'Dashdot'):
        _err(translate("draft","Wrong input: must be 'Solid', 'Dashed', 'Dotted', or 'Dashdot'."))
        return None

    _msg("transparency: {}".format(transparency))
    try:
        utils.type_check([(transparency, (int, float))], name=_name)
        transparency = int(abs(transparency))
    except TypeError:
        _err(translate("draft","Wrong input: must be a number between 0 and 100."))
        return None

    new_obj = doc.addObject("App::FeaturePython", "Layer")
    Layer(new_obj)

    new_obj.Label = name

    if App.GuiUp:
        ViewProviderLayer(new_obj.ViewObject)

        new_obj.ViewObject.LineColor = line_color
        new_obj.ViewObject.ShapeColor = shape_color
        new_obj.ViewObject.LineWidth = line_width
        new_obj.ViewObject.DrawStyle = draw_style
        new_obj.ViewObject.Transparency = transparency

    container = get_layer_container()
    container.addObject(new_obj)

    return new_obj


def makeLayer(name=None, linecolor=None, drawstyle=None,
              shapecolor=None, transparency=None):
    """Create a Layer. DEPRECATED. Use 'make_layer'."""
    utils.use_instead("make_layer")

    if not drawstyle:
        drawstyle = "Solid"

    if not transparency:
        transparency = 0

    return make_layer(name,
                      line_color=linecolor, shape_color=shapecolor,
                      draw_style=drawstyle, transparency=transparency)

## @}
