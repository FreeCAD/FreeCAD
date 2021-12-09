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
"""Provides functions to create DrawingView objects.

These functions must be considered obsolete as the Drawing Workbench
is obsolete since v0.17.
"""
## @package make_drawingview
# \ingroup draftmake
# \brief Provides functions to create DrawingView objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.utils as utils

from draftobjects.drawingview import DrawingView


def make_drawing_view(obj, page, lwmod=None, tmod=None, otherProjection=None):
    """
    make_drawing_view(object,page,[lwmod,tmod])
    
    This function is OBSOLETE, since TechDraw substituted the Drawing Workbench.
    Add a View of the given object to the given page. 

    Parameters
    ----------
    lwmod : 
        modifies lineweights (in percent), 
    
    tmod :
        modifies text heights (in percent). 

    The Hint scale, X and Y of the page are used.
        TODO: Document it properly
    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
    if utils.get_type(obj) == "SectionPlane":
        import ArchSectionPlane
        viewobj = App.ActiveDocument.addObject("Drawing::FeatureViewPython","View")
        page.addObject(viewobj)
        ArchSectionPlane._ArchDrawingView(viewobj)
        viewobj.Source = obj
        viewobj.Label = "View of "+obj.Name
    elif utils.get_type(obj) == "Panel":
        import ArchPanel
        viewobj = ArchPanel.makePanelView(obj, page)
    else:
        viewobj = App.ActiveDocument.addObject("Drawing::FeatureViewPython",
                                               "View"+ obj.Name)
        DrawingView(viewobj)
        page.addObject(viewobj)
        if (otherProjection):
            if hasattr(otherProjection,"Scale"):
                viewobj.Scale = otherProjection.Scale
            if hasattr(otherProjection,"X"):
                viewobj.X = otherProjection.X
            if hasattr(otherProjection,"Y"):
                viewobj.Y = otherProjection.Y
            if hasattr(otherProjection,"Rotation"):
                viewobj.Rotation = otherProjection.Rotation
            if hasattr(otherProjection,"Direction"):
                viewobj.Direction = otherProjection.Direction
        else:
            if hasattr(page.ViewObject,"HintScale"):
                viewobj.Scale = page.ViewObject.HintScale
            if hasattr(page.ViewObject,"HintOffsetX"):
                viewobj.X = page.ViewObject.HintOffsetX
            if hasattr(page.ViewObject,"HintOffsetY"):
                viewobj.Y = page.ViewObject.HintOffsetY
        viewobj.Source = obj
        if lwmod: viewobj.LineweightModifier = lwmod
        if tmod: viewobj.TextModifier = tmod
        if hasattr(obj.ViewObject,"Pattern"):
            if str(obj.ViewObject.Pattern) in list(utils.svgpatterns().keys()):
                viewobj.FillStyle = str(obj.ViewObject.Pattern)
        if hasattr(obj.ViewObject,"DrawStyle"):
            viewobj.LineStyle = obj.ViewObject.DrawStyle
        if hasattr(obj.ViewObject,"LineColor"):
            viewobj.LineColor = obj.ViewObject.LineColor
        elif hasattr(obj.ViewObject,"TextColor"):
            viewobj.LineColor = obj.ViewObject.TextColor
    return viewobj


makeDrawingView = make_drawing_view

## @}
