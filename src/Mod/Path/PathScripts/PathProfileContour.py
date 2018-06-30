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

from __future__ import print_function

import ArchPanel
import FreeCAD
import Part
import Path
import PathScripts.PathAreaOp as PathAreaOp
import PathScripts.PathProfileBase as PathProfileBase
import PathScripts.PathLog as PathLog

from PathScripts import PathUtils
from PySide import QtCore

FreeCAD.setLogLevel('Path.Area', 0)

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

__title__ = "Path Contour Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Implementation of the Contour operation."


class ObjectContour(PathProfileBase.ObjectProfile):
    '''Proxy object for Contour operations.'''

    def baseObject(self):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        return super(self.__class__, self)

    def areaOpFeatures(self, obj):
        '''areaOpFeatures(obj) ... returns 0, Contour only requires the base profile features.'''
        return 0

    def initAreaOp(self, obj):
        '''initAreaOp(obj) ... call super's implementation and hide Side property.'''
        self.baseObject().initAreaOp(obj)
        obj.setEditorMode('Side', 2) # it's always outside

    def areaOpSetDefaultValues(self, obj):
        '''areaOpSetDefaultValues(obj) ... call super's implementation and set Side="Outside".'''
        self.baseObject().areaOpSetDefaultValues(obj)
        obj.Side = 'Outside'

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... return envelope over the job's Base.Shape or all Arch.Panel shapes.'''
        if obj.UseComp:
            self.commandlist.append(Path.Command("(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"))
        else:
            self.commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        isPanel = False
        if hasattr(self.baseobject, "Proxy"):
            if isinstance(self.baseobject.Proxy, ArchPanel.PanelSheet):  # process the sheet
                isPanel = True
                self.baseobject.Proxy.execute(self.baseobject)
                shapes = self.baseobject.Proxy.getOutlines(self.baseobject, transform=True)
                for shape in shapes:
                    f = Part.makeFace([shape], 'Part::FaceMakerSimple')
                    thickness = self.baseobject.Group[0].Source.Thickness
                    return [(f.extrude(FreeCAD.Vector(0, 0, thickness)), False)]

        if hasattr(self.baseobject, "Shape") and not isPanel:
            return [(PathUtils.getEnvelope(partshape=self.baseobject.Shape, subshape=None, depthparams=self.depthparams), False)]

    def areaOpAreaParams(self, obj, isHole):
        params = self.baseObject().areaOpAreaParams(obj, isHole)
        params['Coplanar'] = 2
        return params

    def updateDepths(self, obj, ignoreErrors=False):
        stockBB = self.stock.Shape.BoundBox
        obj.OpFinalDepth = stockBB.ZMin
        obj.OpStartDepth = stockBB.ZMax


def Create(name):
    '''Create(name) ... Creates and returns a Contour operation.'''
    obj   = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectContour(obj)
    return obj
