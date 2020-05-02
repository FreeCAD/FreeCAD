#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

"""This module provides tools to build Wall objects.  Walls are simple
objects, usually vertical, typically obtained by giving a thickness to a base
line, then extruding it vertically.

Examples
--------
TODO put examples here.

"""
__title__="FreeCAD BlocksLayer"
__author__ = "Yorik van Havre, Carlo Pavan"
__url__ = "http://www.freecadweb.org"

import math

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App

import DraftVecUtils

from archobjects.blockslayer import BlocksLayer

if App.GuiUp:
    from archviewproviders.wiew_blockslayer import ViewProviderBlocksLayer

## @package ArchWall
#  \ingroup ARCH
#  \brief The Wall object and tools
#
#  This module provides tools to build Wall objects.  Walls are simple objects,
#  usually vertical, typically obtained by giving a thickness to a base line,
#  then extruding it vertically.



def make_blocks_layer(baseobj=None, length=None, height=None, width=None, align="Center", face=None, name="Wall"):
    """Create a wall based on a given object, and returns the generated wall.

    TODO: It is unclear what defines which units this function uses.

    Parameters
    ----------
    baseobj: <Part::PartFeature>, optional
        The base object with which to build the wall. This can be a sketch, a
        draft object, a face, or a solid. It can also be left as None.

    Returns
    -------
    <Part::FeaturePython>
        Returns the generated wall.

    Notes
    -----
    Creates a new <Part::FeaturePython> object, and turns it into a parametric wall
    object. This <Part::FeaturePython> object does not yet have any shape.

    The wall then uses the baseobj.Shape as the basis to extrude out a wall shape,
    giving the new <Part::FeaturePython> object a shape.

    It then hides the original baseobj.
    """

    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
    p = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    obj = App.ActiveDocument.addObject("Part::FeaturePython","Wall")
    obj.Label = QT_TRANSLATE_NOOP("Arch", name)
    BlocksLayer(obj)
    if App.GuiUp:
        ViewProviderBlocksLayer(obj.ViewObject)
    if baseobj:
        if hasattr(baseobj,'Shape') or baseobj.isDerivedFrom("Mesh::Feature"):
            obj.Base = baseobj
        else:
            App.Console.PrintWarning(str(QT_TRANSLATE_NOOP("Arch","Walls can only be based on Part or Mesh objects")))

    return obj

