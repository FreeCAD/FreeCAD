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
"""This module provides the code for Draft rotate function.
"""
## @package rotate
# \ingroup DRAFT
# \brief This module provides the code for Draft rotate function.

import math

import FreeCAD as App

import DraftVecUtils

import draftutils.gui_utils as gui_utils
import draftutils.utils as utils

from draftmake.make_copy import make_copy


def rotate(objectslist, angle, center=App.Vector(0,0,0),
           axis=App.Vector(0,0,1), copy=False):
    """rotate(objects,angle,[center,axis,copy])
    
    Rotates the objects contained in objects (that can be a list of objects
    or an object) of the given angle (in degrees) around the center, using 
    axis as a rotation axis. 
    
    Parameters
    ----------
    objectlist : list

    angle : list

    center : Base.Vector

    axis : Base.Vector
        If axis is omitted, the rotation will be around the vertical Z axis.
    
    copy : bool
        If copy is True, the actual objects are not moved, but copies
        are created instead. 

    Return
    ----------
    The objects (or their copies) are returned.
    """
    import Part
    utils.type_check([(copy,bool)], "rotate")
    if not isinstance(objectslist,list): objectslist = [objectslist]
    objectslist.extend(utils.get_movable_children(objectslist))
    newobjlist = []
    newgroups = {}
    objectslist = utils.filter_objects_for_modifiers(objectslist, copy)
    for obj in objectslist:
        newobj = None
        # real_center and real_axis are introduced to take into account
        # the possibility that object is inside an App::Part
        if hasattr(obj, "getGlobalPlacement"):
            ci = obj.getGlobalPlacement().inverse().multVec(center)
            real_center = obj.Placement.multVec(ci)
            ai = obj.getGlobalPlacement().inverse().Rotation.multVec(axis)
            real_axis = obj.Placement.Rotation.multVec(ai)
        else:
            real_center = center
            real_axis = axis

        if copy:
            newobj = make_copy(obj)
        else:
            newobj = obj
        if obj.isDerivedFrom("App::Annotation"):
            # TODO: this is very different from how move handle annotations
            # maybe we can uniform the two methods
            if axis.normalize() == App.Vector(1,0,0):
                newobj.ViewObject.RotationAxis = "X"
                newobj.ViewObject.Rotation = angle
            elif axis.normalize() == App.Vector(0,1,0):
                newobj.ViewObject.RotationAxis = "Y"
                newobj.ViewObject.Rotation = angle
            elif axis.normalize() == App.Vector(0,-1,0):
                newobj.ViewObject.RotationAxis = "Y"
                newobj.ViewObject.Rotation = -angle
            elif axis.normalize() == App.Vector(0,0,1):
                newobj.ViewObject.RotationAxis = "Z"
                newobj.ViewObject.Rotation = angle
            elif axis.normalize() == App.Vector(0,0,-1):
                newobj.ViewObject.RotationAxis = "Z"
                newobj.ViewObject.Rotation = -angle
        elif utils.get_type(obj) == "Point":
            v = App.Vector(obj.X,obj.Y,obj.Z)
            rv = v.sub(real_center)
            rv = DraftVecUtils.rotate(rv, math.radians(angle),real_axis)
            v = real_center.add(rv)
            newobj.X = v.x
            newobj.Y = v.y
            newobj.Z = v.z
        elif obj.isDerivedFrom("App::DocumentObjectGroup"):
            pass
        elif hasattr(obj,"Placement"):
            #FreeCAD.Console.PrintMessage("placement rotation\n")
            shape = Part.Shape()
            shape.Placement = obj.Placement
            shape.rotate(DraftVecUtils.tup(real_center), DraftVecUtils.tup(real_axis), angle)
            newobj.Placement = shape.Placement
        elif hasattr(obj,'Shape') and (utils.get_type(obj) not in ["WorkingPlaneProxy","BuildingPart"]):
            #think it make more sense to try first to rotate placement and later to try with shape. no?
            shape = obj.Shape.copy()
            shape.rotate(DraftVecUtils.tup(real_center), DraftVecUtils.tup(real_axis), angle)
            newobj.Shape = shape
        if copy:
            gui_utils.formatObject(newobj,obj)
        if newobj is not None:
            newobjlist.append(newobj)
        if copy:
            for p in obj.InList:
                if p.isDerivedFrom("App::DocumentObjectGroup") and (p in objectslist):
                    g = newgroups.setdefault(p.Name, App.ActiveDocument.addObject(p.TypeId, p.Name))
                    g.addObject(newobj)
                    break
    if copy and utils.getType("selectBaseObjects"): # was utils.getType("selectBaseObjects", False)
        gui_utils.select(objectslist)
    else:
        gui_utils.select(newobjlist)
    if len(newobjlist) == 1: return newobjlist[0]
    return newobjlist
