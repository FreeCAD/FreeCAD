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
__created__ = "2014"
__scriptVersion__ = "1c testing"
__lastModified__ = "2019-06-03 03:53 CST"


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
        if not hasattr(obj, 'ReverseDirection'):
            obj.addProperty('App::PropertyBool', 'ReverseDirection', 'Rotation', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'Reverse direction of pocket operation.'))
        if not hasattr(obj, 'InverseAngle'):
            obj.addProperty('App::PropertyBool', 'InverseAngle', 'Rotation', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'Inverse the angle. Example: -22.5 -> 22.5 degrees.'))
        if not hasattr(obj, 'B_AxisErrorOverride'):
            obj.addProperty('App::PropertyBool', 'B_AxisErrorOverride', 'Rotation', QtCore.QT_TRANSLATE_NOOP('PathPocketShape', 'Match B rotations to model (error in FreeCAD rendering).'))

        self.baseObject().initAreaOp(obj)

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... returns envelope for all base shapes or wires for Arch.Panels.'''
        import math
        import Draft

        if obj.UseComp:
            self.commandlist.append(Path.Command("(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"))
        else:
            self.commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        shapes = []
        self.profileshape = []

        baseSubsTuples = []
        self.cloneNames = []
        subCount = 0
        allTuples = []

        def judgeFinalDepth(obj, fD):
            if obj.FinalDepth.Value >= fD:
                return obj.FinalDepth.Value
            else:
                return fD

        def judgeStartDepth(obj, sD):
            if obj.StartDepth.Value >= sD:
                return obj.StartDepth.Value
            else:
                return sD

        def sortTuplesByIndex(TupleList, tagIdx): # return (TagList, GroupList)
            # Separate elements, regroup by orientation (axis_angle combination)
            TagList = ['X34.2']
            GroupList = [[(2.3, 3.4, 'X')]]
            for tup in TupleList:
                #(shape, sub, angle, axis, tag, strDep, finDep) = tup
                if tup[tagIdx] in TagList:
                    # Determine index of found string
                    i = 0
                    for orn in TagList:
                        if orn == tup[4]:
                            break
                        i += 1
                    GroupList[i].append(tup)
                else:
                    TagList.append(tup[4])  # add orientation entry
                    GroupList.append([tup])  # add orientation entry
            # Remove temp elements
            TagList.pop(0)
            GroupList.pop(0)
            return (TagList, GroupList)

        if obj.Base:  # The user has selected subobjects from the base.  Process each.
            if obj.EnableRotation != 'Off':
                for p in range(0, len(obj.Base)):
                    (base, subsList) = obj.Base[p]
                    for sub in subsList:
                        shape = getattr(base.Shape, sub)
                        if isinstance(shape, Part.Face):
                            rtn = False
                            (rtn, angle, axis, praInfo) = self.pocketRotationAnalysis(obj, shape, prnt=True)
                            PathLog.info("praInfo: \n" + str(praInfo))
                            if rtn is True:
                                PathLog.debug(str(sub) + ": rotating model to make face normal at (0,0,1) ...")

                                if obj.InverseAngle is True:
                                    angle = -1 * angle

                                # Create a temporary clone of model for rotational use.
                                rndAng = round(angle, 8)
                                if rndAng < 0.0:  # neg sign is converted to underscore in clone name creation.
                                    tag = base.Name + '__' + axis + '_' + str(math.fabs(rndAng)).replace('.', '_')
                                else:
                                    tag = base.Name + '__' + axis + str(rndAng).replace('.', '_')
                                clnNm = base.Name + '_' + tag
                                if clnNm not in self.cloneNames:
                                    self.cloneNames.append(clnNm)
                                    PathLog.info("tmp clone created: " + str(clnNm))
                                    FreeCAD.ActiveDocument.addObject('Part::Feature', clnNm).Shape = base.Shape
                                newBase = FreeCAD.ActiveDocument.getObject(clnNm)

                                if axis == 'X':
                                    vect = FreeCAD.Vector(1, 0, 0)
                                elif axis == 'Y':
                                    vect = FreeCAD.Vector(0, 1, 0)
                                # Rotate base to such that Surface.Axis of pocket bottom is Z=1
                                base = Draft.rotate(newBase, angle, center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect, copy=False)
                                # shape = getattr(base.Shape, sub)
                            else:
                                PathLog.debug(str(sub) + ": no rotation used")
                                axis = 'X'
                                angle = 0.0
                                tag = base.Name + '__' + axis + str(angle).replace('.', '_')
                            # Eif
                            tup = base, sub, tag, angle, axis
                            allTuples.append(tup)
                        # Eif
                        subCount += 1
                    # Efor
                # Efor
                (hTags, hGrps) = sortTuplesByIndex(allTuples, 2) # return (TagList, GroupList)
                subList = []
                for o in range(0, len(hTags)):
                    PathLog.debug('hTag: {}'.format(hTags[o]))
                    subList = []
                    for (base, sub, tag, angle, axis) in hGrps[o]:
                        subList.append(sub)
                    pair = base, subList, angle, axis
                    baseSubsTuples.append(pair)
                # Efor                       
            else:
                PathLog.info("Use Rotation feature(property) is 'Off'.")
                for (base, subList) in obj.Base:
                    baseSubsTuples.append((base, subList, 'pathProfileFaces', 0.0, 'X'))

            # for base in obj.Base:
            for base in baseSubsTuples:
                holes = []
                faces = []
                angle = base[2]
                axis = base[3]

                for sub in base[1]:
                    shape = getattr(base[0].Shape, sub)
                    if isinstance(shape, Part.Face):
                        faces.append(shape)
                        if numpy.isclose(abs(shape.normalAt(0, 0).z), 1):  # horizontal face
                            for wire in shape.Wires[1:]:
                                holes.append((base[0].Shape, wire))

                        finish_step = obj.FinishDepth.Value if hasattr(obj, "FinishDepth") else 0.0
                        finDep = judgeFinalDepth(obj, shape.BoundBox.ZMin)
                        self.depthparams = PathUtils.depth_params(
                            clearance_height=obj.ClearanceHeight.Value, safe_height=obj.SafeHeight.Value,
                            start_depth=obj.StartDepth.Value, step_down=obj.StepDown.Value,
                            z_finish_step=finish_step, final_depth=finDep,
                            user_depths=None)
                    else:
                        FreeCAD.Console.PrintWarning("found a base object which is not a face.  Can't continue.")
                        return

                for shape, wire in holes:
                    f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                    drillable = PathUtils.isDrillable(shape, wire)
                    if (drillable and obj.processCircles) or (not drillable and obj.processHoles):
                        env = PathUtils.getEnvelope(shape, subshape=f, depthparams=self.depthparams)
                        PathLog.track()
                        # shapes.append((env, True))
                        shapes.append((env, True, 'pathProfileFaces', base[2], base[3]))

                if len(faces) > 0:
                    profileshape = Part.makeCompound(faces)
                    self.profileshape.append(profileshape)

                if obj.processPerimeter:
                    env = PathUtils.getEnvelope(base[0].Shape, subshape=profileshape, depthparams=self.depthparams)
                    PathLog.track()
                    # shapes.append((env, False))
                    shapes.append((env, False, 'pathProfileFaces', base[2], base[3]))

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

        # self.cloneNames = [] # Comment out to leave temp clones visible
        return shapes

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ... sets default values for hole, circle and perimeter processing.'''
        self.baseObject().areaOpSetDefaultValues(obj, job)

        obj.processHoles = False
        obj.processCircles = False
        obj.processPerimeter = True
        obj.ReverseDirection = False
        obj.InverseAngle = True
        obj.B_AxisErrorOverride = False

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
