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

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

__title__ = "Path Profile Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path Profile object and FreeCAD command"""


class ObjectProfile(PathProfileBase.ObjectProfile):

    def baseObject(self):
        return super(self.__class__, self)

    def initAreaOp(self, obj):
        # Face specific Properties
        obj.addProperty("App::PropertyBool", "processHoles", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile holes as well as the outline"))
        obj.addProperty("App::PropertyBool", "processPerimeter", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile the outline"))
        obj.addProperty("App::PropertyBool", "processCircles", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile round holes"))

        self.baseObject().initAreaOp(obj)

    def areaOpSetDefaultValues(self, obj):
        self.baseObject().areaOpSetDefaultValues(obj)

        obj.processHoles = False
        obj.processCircles = False
        obj.processPerimeter = True

    def areaOpFeatures(self, obj):
        return PathOp.FeatureBaseFaces 

    def areaOpShapes(self, obj):
        if obj.UseComp:
            self.commandlist.append(Path.Command("(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"))
        else:
            self.commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        job = PathUtils.findParentJob(obj)
        if not job or not job.Base:
            return

        baseobject = job.Base
        shapes = []

        if obj.Base:  # The user has selected subobjects from the base.  Process each.
            holes = []
            faces = []
            for b in obj.Base:
                for sub in b[1]:
                    shape = getattr(b[0].Shape, sub)
                    if isinstance(shape, Part.Face):
                        faces.append(shape)
                        if numpy.isclose(abs(shape.normalAt(0, 0).z), 1):  # horizontal face
                            holes += shape.Wires[1:]
                    else:
                        FreeCAD.Console.PrintWarning("found a base object which is not a face.  Can't continue.")
                        return

            for wire in holes:
                f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                drillable = PathUtils.isDrillable(baseobject.Shape, wire)
                if (drillable and obj.processCircles) or (not drillable and obj.processHoles):
                    env = PathUtils.getEnvelope(baseobject.Shape, subshape=f, depthparams=self.depthparams)
                    shapes.append((env, True))

            if len(faces) > 0:
                profileshape = Part.makeCompound(faces)

            if obj.processPerimeter:
                env = PathUtils.getEnvelope(baseobject.Shape, subshape=profileshape, depthparams=self.depthparams)
                shapes.append((env, False))

        else:  # Try to build targets from the job base
            if hasattr(baseobject, "Proxy"):
                if isinstance(baseobject.Proxy, ArchPanel.PanelSheet):  # process the sheet
                    if obj.processCircles or obj.processHoles:
                        for shape in baseobject.Proxy.getHoles(baseobject, transform=True):
                            for wire in shape.Wires:
                                drillable = PathUtils.isDrillable(baseobject.Proxy, wire)
                                if (drillable and obj.processCircles) or (not drillable and obj.processHoles):
                                    f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                                    env = PathUtils.getEnvelope(baseobject.Shape, subshape=f, depthparams=self.depthparams)
                                    shapes.append((env, True))

                    if obj.processPerimeter:
                        for shape in baseobject.Proxy.getOutlines(baseobject, transform=True):
                            for wire in shape.Wires:
                                f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                                env = PathUtils.getEnvelope(baseobject.Shape, subshape=f, depthparams=self.depthparams)
                                shapes.append((env, False))

        return shapes

def Create(name):
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectProfile(obj)
    return obj
