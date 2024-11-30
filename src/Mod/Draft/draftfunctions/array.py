# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides functions to create non-parametric arrayed copies."""
## @package array
# \ingroup draftfunctions
# \brief Provides functions to create non-parametric arrayed copies.

## \addtogroup draftfunctions
# @{
import FreeCAD as App
import draftutils.utils as utils
import draftfunctions.move as move
import draftfunctions.rotate as rotate


def array(objectslist, arg1, arg2, arg3, arg4=None, arg5=None, arg6=None):
    """
    This function creates an array of independent objects.
    Use make_array() to create a parametric array object.

    Creates an array of the given objects (that can be an object or a list
    of objects).

    In case of rectangular array, xnum of iterations in the x direction
    at xvector distance between iterations, and same for y and z directions
    with yvector and ynum and zvector and znum.

    In case of polar array, center is a vector, totalangle is the angle
    to cover (in degrees) and totalnum is the number of objects, including
    the original.

    Use
    ---
    array(objectslist, xvector, yvector, xnum, ynum) for rectangular array

    array(objectslist, xvector, yvector, zvector, xnum, ynum, znum) for rectangular array

    array(objectslist, center, totalangle, totalnum) for polar array
    """

    if arg6:
        rectArray2(objectslist, arg1, arg2, arg3, arg4, arg5, arg6)
    elif arg4:
        rectArray(objectslist, arg1,arg2, arg3, arg4)
    else:
        polarArray(objectslist, arg1, arg2, arg3)


def rectArray(objectslist,xvector,yvector,xnum,ynum):
    utils.type_check([(xvector, App.Vector),
                        (yvector, App.Vector),
                        (xnum,int), (ynum,int)],
                        "rectArray")
    if not isinstance(objectslist,list): objectslist = [objectslist]
    for xcount in range(xnum):
        currentxvector=App.Vector(xvector).multiply(xcount)
        if not xcount==0:
            move.move(objectslist,currentxvector,True)
        for ycount in range(ynum):
            currentxvector=App.Vector(currentxvector)
            currentyvector=currentxvector.add(App.Vector(yvector).multiply(ycount))
            if not ycount==0:
                move.move(objectslist,currentyvector,True)


def rectArray2(objectslist,xvector,yvector,zvector,xnum,ynum,znum):
    utils.type_check([(xvector,App.Vector), (yvector,App.Vector), (zvector,App.Vector),(xnum,int), (ynum,int),(znum,int)], "rectArray2")
    if not isinstance(objectslist,list): objectslist = [objectslist]
    for xcount in range(xnum):
        currentxvector=App.Vector(xvector).multiply(xcount)
        if not xcount==0:
            move.move(objectslist,currentxvector,True)
        for ycount in range(ynum):
            currentxvector=App.Vector(currentxvector)
            currentyvector=currentxvector.add(App.Vector(yvector).multiply(ycount))
            if not ycount==0:
                move.move(objectslist,currentyvector,True)
            for zcount in range(znum):
                currentzvector=currentyvector.add(App.Vector(zvector).multiply(zcount))
                if not zcount==0:
                    move.move(objectslist,currentzvector,True)


def polarArray(objectslist,center,angle,num):
    utils.type_check([(center,App.Vector), (num,int)], "polarArray")
    if not isinstance(objectslist,list): objectslist = [objectslist]
    fraction = float(angle)/num
    for i in range(num):
        currangle = fraction + (i*fraction)
        rotate.rotate(objectslist,currangle,center,copy=True)

## @}
