# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provides functions to create Array objects.

This includes orthogonal arrays, polar arrays, and circular arrays.
"""
## @package make_array
# \ingroup draftmake
# \brief Provides functions to create Array objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftutils.messages import _wrn, _err
from draftutils.translate import translate
from draftobjects.array import Array

if App.GuiUp:
    from draftutils.todo import ToDo
    from draftviewproviders.view_array import ViewProviderDraftArray
    from draftviewproviders.view_draftlink import ViewProviderDraftLink


def make_array(base_object,
               arg1, arg2, arg3,
               arg4=None, arg5=None, arg6=None,
               use_link=True):
    """Create a Draft Array of the given object.

    Rectangular array
    -----------------
    make_array(object, xvector, yvector, xnum, ynum)
    make_array(object, xvector, yvector, zvector, xnum, ynum, znum)

    xnum of iterations in the x direction
    at xvector distance between iterations, same for y direction
    with yvector and ynum, same for z direction with zvector and znum.

    Polar array
    -----------
    make_array(object, center, totalangle, totalnum) for polar array, or

    center is a vector, totalangle is the angle to cover (in degrees)
    and totalnum is the number of objects, including the original.

    Circular array
    --------------
    make_array(object, rdistance, tdistance, axis, center, ncircles, symmetry)

    In case of a circular array, rdistance is the distance of the
    circles, tdistance is the distance within circles, axis the rotation-axis,
    center the center of rotation, ncircles the number of circles
    and symmetry the number of symmetry-axis of the distribution.

    To Do
    -----
    The `Array` class currently handles three types of arrays,
    orthogonal, polar, and circular. In the future, probably they should be
    split in separate classes so that they are easier to manage.
    """
    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(translate("draft","No active document. Aborting."))
        return None

    if use_link:
        # The Array class must be called in this special way
        # to make it a LinkArray
        new_obj = doc.addObject("Part::FeaturePython", "Array",
                                Array(None), None, True)
    else:
        new_obj = doc.addObject("Part::FeaturePython", "Array")
        Array(new_obj)

    new_obj.Base = base_object
    if arg6:
        if isinstance(arg1, (int, float, App.Units.Quantity)):
            new_obj.ArrayType = "circular"
            new_obj.RadialDistance = arg1
            new_obj.TangentialDistance = arg2
            new_obj.Axis = arg3
            new_obj.Center = arg4
            new_obj.NumberCircles = arg5
            new_obj.Symmetry = arg6
        else:
            new_obj.ArrayType = "ortho"
            new_obj.IntervalX = arg1
            new_obj.IntervalY = arg2
            new_obj.IntervalZ = arg3
            new_obj.NumberX = arg4
            new_obj.NumberY = arg5
            new_obj.NumberZ = arg6
    elif arg4:
        new_obj.ArrayType = "ortho"
        new_obj.IntervalX = arg1
        new_obj.IntervalY = arg2
        new_obj.NumberX = arg3
        new_obj.NumberY = arg4
    else:
        new_obj.ArrayType = "polar"
        new_obj.Center = arg1
        new_obj.Angle = arg2
        new_obj.NumberPolar = arg3

    if App.GuiUp:
        if use_link:
            ViewProviderDraftLink(new_obj.ViewObject)
        else:
            if new_obj.ArrayType == "circular":
                new_obj.Proxy.execute(new_obj) # Updates Count which is required for correct DiffuseColor.
            ViewProviderDraftArray(new_obj.ViewObject)
            gui_utils.format_object(new_obj, new_obj.Base)
            new_obj.ViewObject.Proxy.resetColors(new_obj.ViewObject)
            # Workaround to trigger update of DiffuseColor:
            ToDo.delay(reapply_diffuse_color, new_obj.ViewObject)
        new_obj.Base.ViewObject.hide()
        gui_utils.select(new_obj)

    return new_obj


def makeArray(baseobject,
              arg1, arg2, arg3,
              arg4=None, arg5=None, arg6=None,
              name="Array", use_link=False):
    """Create an Array. DEPRECATED. Use 'make_array'."""
    _wrn("Do not use this function directly; instead, use "
         "'make_ortho_array', 'make_polar_array', "
         "or 'make_circular_array'.")

    return make_array(baseobject,
                      arg1, arg2, arg3,
                      arg4, arg5, arg6, use_link)


def reapply_diffuse_color(vobj):
    try:
        vobj.DiffuseColor = vobj.DiffuseColor
    except:
        pass

## @}
