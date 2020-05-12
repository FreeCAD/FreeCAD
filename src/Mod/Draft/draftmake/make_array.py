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
"""This module provides the code for Draft make_array function.
"""
## @package make_array
# \ingroup DRAFT
# \brief This module provides the code for Draft make_array function.

import FreeCAD as App

import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftobjects.array import Array

from draftviewproviders.view_draftlink import ViewProviderDraftLink
if App.GuiUp:
    from draftviewproviders.view_array import ViewProviderDraftArray


def make_array(baseobject, arg1, arg2, arg3, arg4=None, 
               arg5=None, arg6=None, name="Array", use_link=False):
    """ 
    Creates a Draft Array of the given object.


    Rectangular array
    -----------------
    make_array(object,xvector,yvector,xnum,ynum,[name])
    makeArray(object,xvector,yvector,zvector,xnum,ynum,znum,[name])

    xnum of iterations in the x direction
    at xvector distance between iterations, same for y direction with yvector and ynum,
    same for z direction with zvector and znum. 


    Polar array
    -----------
    makeArray(object,center,totalangle,totalnum,[name]) for polar array, or

    center is a vector, totalangle is the angle to cover (in degrees) and totalnum 
    is the number of objects, including the original. 
    

    Circular array
    --------------
    makeArray(object,rdistance,tdistance,axis,center,ncircles,symmetry,[name])

    In case of a circular array, rdistance is the distance of the
    circles, tdistance is the distance within circles, axis the rotation-axes, center the
    center of rotation, ncircles the number of circles and symmetry the number
    of symmetry-axis of the distribution. The result is a parametric Draft Array.
    """

    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return
    if use_link:
        obj = App.ActiveDocument.addObject("Part::FeaturePython",name, Array(None),None,True)
    else:
        obj = App.ActiveDocument.addObject("Part::FeaturePython",name)
        Array(obj)
    obj.Base = baseobject
    if arg6:
        if isinstance(arg1, (int, float, App.Units.Quantity)):
            obj.ArrayType = "circular"
            obj.RadialDistance = arg1
            obj.TangentialDistance = arg2
            obj.Axis = arg3
            obj.Center = arg4
            obj.NumberCircles = arg5
            obj.Symmetry = arg6
        else:
            obj.ArrayType = "ortho"
            obj.IntervalX = arg1
            obj.IntervalY = arg2
            obj.IntervalZ = arg3
            obj.NumberX = arg4
            obj.NumberY = arg5
            obj.NumberZ = arg6
    elif arg4:
        obj.ArrayType = "ortho"
        obj.IntervalX = arg1
        obj.IntervalY = arg2
        obj.NumberX = arg3
        obj.NumberY = arg4
    else:
        obj.ArrayType = "polar"
        obj.Center = arg1
        obj.Angle = arg2
        obj.NumberPolar = arg3
    if App.GuiUp:
        if use_link:
            ViewProviderDraftLink(obj.ViewObject)
        else:
            ViewProviderDraftArray(obj.ViewObject)
            gui_utils.format_object(obj,obj.Base)
            if len(obj.Base.ViewObject.DiffuseColor) > 1:
                obj.ViewObject.Proxy.resetColors(obj.ViewObject)
        baseobject.ViewObject.hide()
        gui_utils.select(obj)
    return obj


makeArray = make_array
