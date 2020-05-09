# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""This module provides the code for Draft make_copy function.
"""
## @package make_copy
# \ingroup DRAFT
# \brief This module provides the code for Draft make_copy function.

import FreeCAD as App

import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftobjects.rectangle import Rectangle
from draftobjects.point import Point
from draftobjects.dimension import LinearDimension
from draftobjects.wire import Wire
from draftobjects.circle import Circle
from draftobjects.polygon import Polygon
from draftobjects.bspline import BSpline
from draftobjects.block import Block


if App.GuiUp:
    from draftviewproviders.view_base import ViewProviderDraft
    from draftviewproviders.view_base import ViewProviderDraftPart
    from draftviewproviders.view_rectangle import ViewProviderRectangle
    from draftviewproviders.view_point import ViewProviderPoint
    from draftviewproviders.view_dimension import ViewProviderLinearDimension
    from draftviewproviders.view_wire import ViewProviderWire


def make_copy(obj, force=None, reparent=False):
    """makeCopy(object, [force], [reparent])
    
    Make an exact copy of an object and return it.
    
    Parameters
    ----------
    obj :
        Object to copy.

    force :
        TODO: Describe.

    reparent :
        TODO: Describe.
    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return

    if (utils.get_type(obj) == "Rectangle") or (force == "Rectangle"):
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject(obj.TypeId, _name)
        Rectangle(newobj)
        if App.GuiUp:
            ViewProviderRectangle(newobj.ViewObject)

    elif (utils.get_type(obj) == "Point") or (force == "Point"):
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject(obj.TypeId, _name)
        Point(newobj)
        if App.GuiUp:
            ViewProviderPoint(newobj.ViewObject)

    elif (utils.get_type(obj) in ["Dimension", 
            "LinearDimension"]) or (force == "Dimension"):
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject(obj.TypeId, _name)
        LinearDimension(newobj)
        if App.GuiUp:
            ViewProviderLinearDimension(newobj.ViewObject)

    elif (utils.get_type(obj) == "Wire") or (force == "Wire"):
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject(obj.TypeId, _name)
        Wire(newobj)
        if App.GuiUp:
            ViewProviderWire(newobj.ViewObject)

    elif (utils.get_type(obj) == "Circle") or (force == "Circle"):
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject(obj.TypeId, _name)
        Circle(newobj)
        if App.GuiUp:
            ViewProviderDraft(newobj.ViewObject)

    elif (utils.get_type(obj) == "Polygon") or (force == "Polygon"):
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject(obj.TypeId, _name)
        Polygon(newobj)
        if App.GuiUp:
            ViewProviderDraft(newobj.ViewObject)

    elif (utils.get_type(obj) == "BSpline") or (force == "BSpline"):
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject(obj.TypeId, _name)
        BSpline(newobj)
        if App.GuiUp:
            ViewProviderWire(newobj.ViewObject)

    elif (utils.get_type(obj) == "Block") or (force == "BSpline"):
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject(obj.TypeId, _name)
        Block(newobj)
        if App.GuiUp:
            ViewProviderDraftPart(newobj.ViewObject)
            
    # drawingview became obsolete with v 0.19
    # TODO: uncomment after splitting DrawingView object from draft py

    #elif (utils.get_type(obj) == "DrawingView") or (force == "DrawingView"):
    #_name = utils.get_real_name(obj.Name)
    #newobj = App.ActiveDocument.addObject(obj.TypeId, _name)
    #DrawingView(newobj)

    elif (utils.get_type(obj) == "Structure") or (force == "Structure"):
        import ArchStructure
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject(obj.TypeId, _name)
        ArchStructure._Structure(newobj)
        if App.GuiUp:
            ArchStructure._ViewProviderStructure(newobj.ViewObject)

    elif (utils.get_type(obj) == "Wall") or (force == "Wall"):
        import ArchWall
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject(obj.TypeId, _name)
        ArchWall._Wall(newobj)
        if App.GuiUp:
            ArchWall._ViewProviderWall(newobj.ViewObject)

    elif (utils.get_type(obj) == "Window") or (force == "Window"):
        import ArchWindow
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject(obj.TypeId, _name)
        ArchWindow._Window(newobj)
        if App.GuiUp:
            ArchWindow._ViewProviderWindow(newobj.ViewObject)

    elif (utils.get_type(obj) == "Panel") or (force == "Panel"):
        import ArchPanel
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject(obj.TypeId, _name)
        ArchPanel._Panel(newobj)
        if App.GuiUp:
            ArchPanel._ViewProviderPanel(newobj.ViewObject)

    elif (utils.get_type(obj) == "Sketch") or (force == "Sketch"):
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject("Sketcher::SketchObject", _name)
        for geo in obj.Geometry:
            newobj.addGeometry(geo)
        for con in obj.Constraints:
            newobj.addConstraint(con)

    elif hasattr(obj, 'Shape'):
        _name = utils.get_real_name(obj.Name)
        newobj = App.ActiveDocument.addObject("Part::Feature", _name)
        newobj.Shape = obj.Shape

    else:
        print("Error: Object type cannot be copied")
        return None

    for p in obj.PropertiesList:
        if not p in ["Proxy", "ExpressionEngine"]:
            if p in newobj.PropertiesList:
                if not "ReadOnly" in newobj.getEditorMode(p):
                    try:
                        setattr(newobj, p, obj.getPropertyByName(p))
                    except AttributeError:
                        try:
                            setattr(newobj, p, obj.getPropertyByName(p).Value)
                        except AttributeError:
                            pass

    if reparent:
        parents = obj.InList
        if parents:
            for par in parents:
                if par.isDerivedFrom("App::DocumentObjectGroup"):
                    par.addObject(newobj)
                else:
                    for prop in par.PropertiesList:
                        if getattr(par, prop) == obj:
                            setattr(par, prop, newobj)

    gui_utils.format_object(newobj, obj)
    return newobj