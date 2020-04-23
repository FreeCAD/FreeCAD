# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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
"""This module provides the code for Draft move function.
"""
## @package move
# \ingroup DRAFT
# \brief This module provides the code for Draft move function.

import FreeCAD as App

import draftutils.utils as utils

from draftutils.gui_utils import format_object
from draftutils.gui_utils import select

import draftutils.utils as utils
from draftmake.make_copy import make_copy

from draftobjects.dimension import LinearDimension
from draftobjects.text import Text
if App.GuiUp:
    from draftviewproviders.view_text import ViewProviderText
    from draftviewproviders.view_dimension import ViewProviderLinearDimension


def move(objectslist, vector, copy=False):
    """move(objectslist, vector, [copy])
    
    Moves the objects contained in objects (that can be an object or a 
    list of objects) in the direction and distance indicated by the given
    vector. 

    Parameters
    ----------
    objectlist : list
        The list of objects to be moved.

    vector : Base.Vector
        The vector to be applied for the transformation.
    
    copy : bool (default = False)
        If copy is True, the given objects are not moved, but copies
        are created instead.

    Returns
    -------
    list
        The objects (or their copies) are returned.
    """
    utils.type_check([(vector, App.Vector), (copy, bool)], "move")
    
    if not isinstance(objectslist,list): objectslist = [objectslist]
    objectslist.extend(utils.get_movable_children(objectslist))
    newobjlist = []
    newgroups = {}
    objectslist = utils.filter_objects_for_modifiers(objectslist, copy)
    for obj in objectslist:
        newobj = None
        # real_vector have been introduced to take into account
        # the possibility that object is inside an App::Part
        if hasattr(obj, "getGlobalPlacement"):
            v_minus_global = obj.getGlobalPlacement().inverse().Rotation.multVec(vector)
            real_vector = obj.Placement.Rotation.multVec(v_minus_global)
        else:
            real_vector = vector
        if utils.get_type(obj) == "Point":
            v = App.Vector(obj.X,obj.Y,obj.Z)
            v = v.add(real_vector)
            if copy:
                newobj = make_copy(obj)
            else:
                newobj = obj
            newobj.X = v.x
            newobj.Y = v.y
            newobj.Z = v.z
        elif obj.isDerivedFrom("App::DocumentObjectGroup"):
            pass
        elif hasattr(obj,'Shape'):
            if copy:
                newobj = make_copy(obj)
            else:
                newobj = obj
            pla = newobj.Placement
            pla.move(real_vector)
        elif utils.get_type(obj) == "Annotation":
            if copy:
                newobj = App.ActiveDocument.addObject("App::Annotation",
                                                      utils.get_real_name(obj.Name))
                newobj.LabelText = obj.LabelText
                if App.GuiUp:
                    format_object(newobj,obj)
            else:
                newobj = obj
            newobj.Position = obj.Position.add(real_vector)
        elif utils.get_type(obj) == "DraftText":
            if copy:
                newobj = App.ActiveDocument.addObject("App::FeaturePython",
                                                      utils.get_real_name(obj.Name))
                Text(newobj)
                if App.GuiUp:
                    ViewProviderText(newobj.ViewObject)
                    format_object(newobj,obj)
                newobj.Text = obj.Text
                newobj.Placement = obj.Placement
                if App.GuiUp:
                    format_object(newobj,obj)
            else:
                newobj = obj
            newobj.Placement.Base = obj.Placement.Base.add(real_vector)
        elif utils.get_type(obj) in ["Dimension","LinearDimension"]:
            if copy:
                newobj = App.ActiveDocument.addObject("App::FeaturePython",
                                                      utils.get_real_name(obj.Name))
                LinearDimension(newobj)
                if App.GuiUp:
                    ViewProviderLinearDimension(newobj.ViewObject)
                    format_object(newobj,obj)
            else:
                newobj = obj
            newobj.Start = obj.Start.add(real_vector)
            newobj.End = obj.End.add(real_vector)
            newobj.Dimline = obj.Dimline.add(real_vector)
        else:
            if copy and obj.isDerivedFrom("Mesh::Feature"):
                print("Mesh copy not supported at the moment") # TODO
            newobj = obj
            if "Placement" in obj.PropertiesList:
                pla = obj.Placement
                pla.move(real_vector)
        if newobj is not None:
            newobjlist.append(newobj)
        if copy:
            for p in obj.InList:
                if p.isDerivedFrom("App::DocumentObjectGroup") and (p in objectslist):
                    g = newgroups.setdefault(p.Name, 
                            App.ActiveDocument.addObject(p.TypeId, p.Name))
                    g.addObject(newobj)
                    break
                if utils.get_type(p) == "Layer":
                    p.Proxy.addObject(p,newobj)
    if copy and utils.get_param("selectBaseObjects",False):
        select(objectslist)
    else:
        select(newobjlist)
    if len(newobjlist) == 1: return newobjlist[0]
    return newobjlist
