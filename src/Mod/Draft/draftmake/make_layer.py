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
from draftobjects.layer import Layer, LayerContainer
from draftutils import utils
from draftutils.messages import _err
from draftutils.translate import translate

if App.GuiUp:
    from draftviewproviders.view_layer import (ViewProviderLayer,
                                               ViewProviderLayerContainer)


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

    obj = doc.addObject("App::DocumentObjectGroupPython",
                            "LayerContainer")
    obj.Label = translate("draft", "Layers")

    LayerContainer(obj)

    if App.GuiUp:
        ViewProviderLayerContainer(obj.ViewObject)

    return obj


def getLayerContainer():
    """Get the Layer container. DEPRECATED. Use 'get_layer_container'."""
    utils.use_instead("get_layer_container")

    return get_layer_container()


def make_layer(name=None,
               line_color=(0.0, 0.0, 0.0),   # does not match default DefaultShapeLineColor
               shape_color=(0.8, 0.8, 0.8),  # does not match default DefaultShapeColor
               line_width=2.0,
               draw_style="Solid",
               transparency=0):
    """Create a Layer object in the active document.

    If a layer container named `'LayerContainer'` does not exist, it is created.

    A layer controls the view properties of the objects inside the layer.
    All parameters except for `name` only apply if the graphical interface
    is up.

    All parameters that control view properties can be set to `None`. Their
    value, as set by the view provider (matching the current preferences), is
    then not changed.

    Parameters
    ----------
    name: str or `None`, optional
        It defaults to `None`.
        It is used to set the layer's `Label`. If it is `None` the `Label` is
        set to `'Layer'` or to its translation in the current language.

    line_color: tuple or `None`, optional
        It defaults to `(0.0, 0.0, 0.0)`.
        If it is given, it should be a tuple of three floating point values
        from 0.0 to 1.0.

    shape_color: tuple or `None`, optional
        It defaults to `(0.8, 0.8, 0.8)`.
        If it is given, it should be a tuple of three floating point values
        from 0.0 to 1.0.

    line_width: float or `None`, optional
        It defaults to 2.0.
        It determines the width of the edges of the objects contained in the layer.

    draw_style: str or `None`, optional
        It defaults to `'Solid'`.
        It determines the style of the edges of the objects contained in the layer.
        If it is given, it should be 'Solid', 'Dashed', 'Dotted' or 'Dashdot'.

    transparency: int or `None`, optional
        It defaults to 0.
        It should be an integer from 0 to 100.

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

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft","No active document. Aborting."))
        return None

    if name is not None:
        try:
            utils.type_check([(name, str)], name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: it must be a string."))
            return None
    else:
        name = translate("draft", "Layer")

    if line_color is not None:
        try:
            utils.type_check([(line_color, tuple)], name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: must be a tuple of three floats 0.0 to 1.0."))
            return None

        if not all(isinstance(color, (int, float)) for color in line_color):
            _err(translate("draft","Wrong input: must be a tuple of three floats 0.0 to 1.0."))
            return None

    if shape_color is not None:
        try:
            utils.type_check([(shape_color, tuple)], name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: must be a tuple of three floats 0.0 to 1.0."))
            return None

        if not all(isinstance(color, (int, float)) for color in shape_color):
            _err(translate("draft","Wrong input: must be a tuple of three floats 0.0 to 1.0."))
            return None

    if line_width is not None:
        try:
            utils.type_check([(line_width, (int, float))], name=_name)
            line_width = float(abs(line_width))
        except TypeError:
            _err(translate("draft","Wrong input: must be a number."))
            return None

    if draw_style is not None:
        try:
            utils.type_check([(draw_style, str)], name=_name)
        except TypeError:
            _err(translate("draft","Wrong input: must be 'Solid', 'Dashed', 'Dotted', or 'Dashdot'."))
            return None

        if draw_style not in ('Solid', 'Dashed', 'Dotted', 'Dashdot'):
            _err(translate("draft","Wrong input: must be 'Solid', 'Dashed', 'Dotted', or 'Dashdot'."))
            return None

    if transparency is not None:
        try:
            utils.type_check([(transparency, (int, float))], name=_name)
            transparency = int(abs(transparency))
        except TypeError:
            _err(translate("draft","Wrong input: must be a number between 0 and 100."))
            return None

    obj = doc.addObject("App::FeaturePython", "Layer")
    Layer(obj)

    obj.Label = name

    if App.GuiUp:
        vobj = obj.ViewObject
        ViewProviderLayer(vobj)
        if line_color is not None:
            vobj.LineColor = line_color
        if shape_color is not None:
            vobj.ShapeColor = shape_color
        if line_width is not None:
            vobj.LineWidth = line_width
        if draw_style is not None:
            vobj.DrawStyle = draw_style
        if transparency is not None:
            vobj.Transparency = transparency

    container = get_layer_container()
    container.addObject(obj)

    return obj

## @}
