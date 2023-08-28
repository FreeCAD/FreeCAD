#***************************************************************************
#*   Copyright (c) 2012 Sebastian Hoogen <github@sebastianhoogen.de>       *
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

__title__ = "FreeCAD OpenSCAD Workbench - expand placements and matrices functions"
__author__ = "Sebastian Hoogen"
__url__ = ["https://www.freecad.org"]

'''
This Script includes python functions to shift all placements down the
feature tree to the most basic objects
'''

import FreeCAD
from OpenSCADFeatures import *
from OpenSCADUtils import isspecialorthogonal
import replaceobj


def likeprimitive(obj,extrusion=False):
    '''we can't push the matrix transformation further down'''
    return not obj.OutList or obj.isDerivedFrom('Part::Extrusion')\
        or extrusion and (obj.isDerivedFrom('Part::Revolution') \
        or obj.isDerivedFrom('Part::FeaturePython')) or \
        not obj.isDerivedFrom('Part::Feature')


def expandplacementsmatrix(obj,matrix):
    '''expand afine transformation down the feature tree'''
    ownmatrix = matrix.multiply(obj.Placement.toMatrix())
    if obj.isDerivedFrom('Part::Feature') and \
        isinstance(obj.Proxy, MatrixTransform):
        innermatrix = ownmatrix.multiply(obj.Matrix)
        if likeprimitive(obj.Base,True): #this matrix is needed
            obj.Placement = FreeCAD.Placement()
            obj.Matrix = innermatrix
        else:  #the inner object is not a primitive
            expandplacementsmatrix(obj.Base, innermatrix)
            #remove the matrix object
            for parent in obj.Base.InList:
                replaceobj.replaceobj(parent, obj, obj.Base)
            out.Document.removeObject(obj.Name)
    elif likeprimitive(obj, True):
        #if isspecialorthogonalpython(fcsubmatrix(ownmatrix)):
        if isspecialorthogonal(ownmatrix):
            obj.Placement = FreeCAD.Placement()
            #this should never happen unless matrices cancel out
            obj.Placement = FreeCAD.Placement(ownmatrix)
        else:
            newobj = doc.addObject("Part::FeaturePython", 'exp_trans')
            MatrixTransform(newobj,ownmatrix,obj) #This object is not mutable GUI
            ViewProviderTree(newobj.ViewObject)
            for parent in obj.InList:
                replaceobj.replaceobj(parent, obj, newobj) # register the new object in the feature tree
            obj.Placement=FreeCAD.Placement()
    else: #not a primitive
        for outobj in obj.OutList:
            if outobj.isDerivedFrom('Part::Feature') and \
                    isinstance(obj.Proxy,MatrixTransform):
                newmatrix = ownmatrix.multiply(obj.Matrix).multiply(\
                            outobj.Base.Placement.toMatrix())
                if likeprimitive(outobj.Base,True): #child of is like primtitive
                    outobj.Matrix = newmatrix
                    outobj.Base.Placement=FreeCAD.Placement()
                else: #remove the MatrixTransformation
                    plainobj = outobj.Base
                    for parent in outobj.InList:
                        replaceobj.replaceobj(parent, outobj, plainobj)
                    outobj.Document.removeObject(outobj.Name)
                    expandplacementsmatrix(outobj,newmatrix)
            else:
                expandplacementsmatrix(outobj, ownmatrix)
        obj.Placement = FreeCAD.Placement()


def expandplacements(obj,placement):
    ownplacement = placement.multiply(obj.Placement)
    if obj.isDerivedFrom('Part::FeaturePython') and isinstance(obj.Proxy, MatrixTransform):
        #expandplacementsmatrix(obj,ownplacement.toMatrix())
        expandplacementsmatrix(obj,placement.toMatrix())
    elif likeprimitive(obj, False):
        obj.Placement = ownplacement
    elif obj.isDerivedFrom('Part::Mirroring'):
        import OpenSCADUtils
        mm  = OpenSCADUtils.mirror2mat(obj.Normal,obj.Base)
        #TODO: set the base to 0,0,0
        innerp = FreeCAD.Placement(mm * ownplacement.toMatrix() *mm)
        expandplacements(obj.Source, innerp)
        obj.Placement = FreeCAD.Placement()
    else:
        for outobj in obj.OutList:
            if obj.isDerivedFrom('Part::Extrusion'):
                obj.Dir = ownplacement.Rotation.multVec(obj.Dir)
            elif obj.isDerivedFrom('Part::Revolution'):
                obj.Axis = ownplacement.Rotation.multVec(obj.Axis)
                #obj.Base=ownplacement.Rotation.multVec(obj.Base)
            expandplacements(outobj, ownplacement)
        obj.Placement = FreeCAD.Placement()

#expandplacements(rootobj,FreeCAD.Placement())
