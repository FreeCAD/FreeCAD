# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
import Part
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathProfileBase as PathProfileBase
import PathScripts.PathUtils as PathUtils

from DraftGeomUtils import findWires
from PySide import QtCore

"""Path Profile from Edges Object and Command"""

LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

__title__ = "Path Profile Edges Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Path Profile operation based on edges."


class ObjectProfile(PathProfileBase.ObjectProfile):
    '''Proxy object for Profile operations based on edges.'''

    def baseObject(self):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        return super(self.__class__, self)

    def areaOpFeatures(self, obj):
        '''areaOpFeatures(obj) ... add support for edge base geometry.'''
        return PathOp.FeatureBaseEdges

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... returns envelope for all wires formed by the base edges.'''
        PathLog.track()

        if obj.UseComp:
            self.commandlist.append(Path.Command("(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"))
        else:
            self.commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        shapes = []
        if obj.Base:
            basewires = []

            for b in obj.Base:
                edgelist = []
                for sub in b[1]:
                    edgelist.append(getattr(b[0].Shape, sub))
                basewires.append((b[0], findWires(edgelist)))

            for base,wires in basewires:
                for wire in wires:
                    f = Part.makeFace(wire, 'Part::FaceMakerSimple')

                    # shift the compound to the bottom of the base object for
                    # proper sectioning
                    zShift = b[0].Shape.BoundBox.ZMin - f.BoundBox.ZMin
                    newPlace = FreeCAD.Placement(FreeCAD.Vector(0, 0, zShift), f.Placement.Rotation)
                    f.Placement = newPlace
                    env = PathUtils.getEnvelope(base.Shape, subshape=f, depthparams=self.depthparams)
                    shapes.append((env, False))
        return shapes

def SetupProperties():
    return PathProfileBase.SetupProperties()

def Create(name, obj = None):
    '''Create(name) ... Creates and returns a Profile based on edges operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectProfile(obj, name)
    return obj
