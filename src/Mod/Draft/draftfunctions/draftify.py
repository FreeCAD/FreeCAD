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
"""This module provides the code for Draft draftify function.
"""
## @package draftify
# \ingroup DRAFT
# \brief This module provides the code for Draft draftify function.

import FreeCAD as App

import draftutils.gui_utils as gui_utils
import draftutils.utils as utils

from draftmake.make_block import makeBlock
from draftmake.make_wire import makeWire
from draftmake.make_circle import makeCircle


def draftify(objectslist, makeblock=False, delete=True):
    """draftify(objectslist,[makeblock],[delete])
    
    Turn each object of the given list (objectslist can also be a single 
    object) into a Draft parametric wire. 
    
    TODO: support more objects

    Parameters
    ----------
    objectslist :

    makeblock : bool
        If makeblock is True, multiple objects will be grouped in a block.
    
    delete : bool
        If delete = False, old objects are not deleted
    """
    import Part
    import DraftGeomUtils

    if not isinstance(objectslist,list):
        objectslist = [objectslist]
    newobjlist = []
    for obj in objectslist:
        if hasattr(obj,'Shape'):
            for cluster in Part.getSortedClusters(obj.Shape.Edges):
                w = Part.Wire(cluster)
                if DraftGeomUtils.hasCurves(w):
                    if (len(w.Edges) == 1) and (DraftGeomUtils.geomType(w.Edges[0]) == "Circle"):
                        nobj = makeCircle(w.Edges[0])
                    else:
                        nobj = App.ActiveDocument.addObject("Part::Feature", obj.Name)
                        nobj.Shape = w
                else:
                    nobj = makeWire(w)
                newobjlist.append(nobj)
                gui_utils.format_object(nobj, obj)
                # sketches are always in wireframe mode. In Draft we don't like that!
                if App.GuiUp:
                    nobj.ViewObject.DisplayMode = "Flat Lines"
            if delete:
                App.ActiveDocument.removeObject(obj.Name)

    if makeblock:
        return makeBlock(newobjlist)
    else:
        if len(newobjlist) == 1:
            return newobjlist[0]
        return newobjlist
