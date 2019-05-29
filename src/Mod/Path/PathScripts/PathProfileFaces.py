# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

import ArchPanel
import FreeCAD
import Part
import Path
import PathScripts.PathAreaOp as PathAreaOp
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathProfileBase as PathProfileBase
import PathScripts.PathUtils as PathUtils
import numpy

from PathScripts.PathUtils import depth_params
from PySide import QtCore

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

__title__ = "Path Profile Faces Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Path Profile operation based on faces."


class ObjectProfile(PathProfileBase.ObjectProfile):
    '''Proxy object for Profile operations based on faces.'''

    def baseObject(self):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        return super(self.__class__, self)

    def areaOpFeatures(self, obj):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        return PathOp.FeatureBaseFaces | PathOp.FeatureBasePanels

    def initAreaOp(self, obj):
        '''initAreaOp(obj) ... adds properties for hole, circle and perimeter processing.'''
        # Face specific Properties
        obj.addProperty("App::PropertyBool", "processHoles", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile holes as well as the outline"))
        obj.addProperty("App::PropertyBool", "processPerimeter", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile the outline"))
        obj.addProperty("App::PropertyBool", "processCircles", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile round holes"))

        self.baseObject().initAreaOp(obj)

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... returns envelope for all base shapes or wires for Arch.Panels.'''
        if obj.UseComp:
            self.commandlist.append(Path.Command("(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"))
        else:
            self.commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        shapes = []
        self.profileshape = []

        if obj.Base:  # The user has selected subobjects from the base.  Process each.
            for base in obj.Base:
                holes = []
                faces = []
                for sub in base[1]:
                    shape = getattr(base[0].Shape, sub)
                    if isinstance(shape, Part.Face):
                        faces.append(shape)
                        if numpy.isclose(abs(shape.normalAt(0, 0).z), 1):  # horizontal face
                            for wire in shape.Wires[1:]:
                                holes.append((base[0].Shape, wire))
                    else:
                        FreeCAD.Console.PrintWarning("found a base object which is not a face.  Can't continue.")
                        return

                for shape, wire in holes:
                    f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                    drillable = PathUtils.isDrillable(shape, wire)
                    if (drillable and obj.processCircles) or (not drillable and obj.processHoles):
                        env = PathUtils.getEnvelope(shape, subshape=f, depthparams=self.depthparams)
                        PathLog.track()
                        shapes.append((env, True))

                if len(faces) > 0:
                    profileshape = Part.makeCompound(faces)
                    self.profileshape.append(profileshape)

                if obj.processPerimeter:
                    env = PathUtils.getEnvelope(base[0].Shape, subshape=profileshape, depthparams=self.depthparams)
                    PathLog.track()
                    shapes.append((env, False))

        else:  # Try to build targets from the job base
            if 1 == len(self.model) and hasattr(self.model[0], "Proxy"):
                if isinstance(self.model[0].Proxy, ArchPanel.PanelSheet):  # process the sheet
                    if obj.processCircles or obj.processHoles:
                        for shape in self.model[0].Proxy.getHoles(self.model[0], transform=True):
                            for wire in shape.Wires:
                                drillable = PathUtils.isDrillable(self.model[0].Proxy, wire)
                                if (drillable and obj.processCircles) or (not drillable and obj.processHoles):
                                    f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                                    env = PathUtils.getEnvelope(self.model[0].Shape, subshape=f, depthparams=self.depthparams)
                                    shapes.append((env, True))

                    if obj.processPerimeter:
                        for shape in self.model[0].Proxy.getOutlines(self.model[0], transform=True):
                            for wire in shape.Wires:
                                f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                                env = PathUtils.getEnvelope(self.model[0].Shape, subshape=f, depthparams=self.depthparams)
                                shapes.append((env, False))

        self.removalshapes = shapes
        PathLog.debug("%d shapes" % len(shapes))
        return shapes

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ... sets default values for hole, circle and perimeter processing.'''
        self.baseObject().areaOpSetDefaultValues(obj, job)

        obj.processHoles = False
        obj.processCircles = False
        obj.processPerimeter = True

def SetupProperties():
    setup = []
    setup.append("processHoles")
    setup.append("processPerimeter")
    setup.append("processCircles")
    return PathProfileBase.SetupProperties() + setup

def Create(name, obj = None):
    '''Create(name) ... Creates and returns a Profile based on faces operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectProfile(obj, name)
    return obj
