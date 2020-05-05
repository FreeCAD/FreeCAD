# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Schildkroet                                        *
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
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathProfileBase as PathProfileBase
import PathScripts.PathUtils as PathUtils
import numpy

from PySide import QtCore

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
ArchPanel = LazyLoader('ArchPanel', globals(), 'ArchPanel')
Part = LazyLoader('Part', globals(), 'Part')

__title__ = "Path Profile Faces Operation"
__author__ = "sliptonic (Brad Collette), Schildkroet"
__url__ = "http://www.freecadweb.org"
__doc__ = "Path Profile operation based on faces."

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectProfile(PathProfileBase.ObjectProfile):
    '''Proxy object for Profile operations based on faces.'''

    def baseObject(self):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        return super(self.__class__, self)

    def areaOpFeatures(self, obj):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        # return PathOp.FeatureBaseFaces | PathOp.FeatureBasePanels | PathOp.FeatureRotation
        return PathOp.FeatureBaseFaces | PathOp.FeatureBasePanels

    def initAreaOp(self, obj):
        '''initAreaOp(obj) ... adds properties for hole, circle and perimeter processing.'''
        # Face specific Properties
        obj.addProperty("App::PropertyBool", "processHoles", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile holes as well as the outline"))
        obj.addProperty("App::PropertyBool", "processPerimeter", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile the outline"))
        obj.addProperty("App::PropertyBool", "processCircles", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile round holes"))

        if not hasattr(obj, 'HandleMultipleFeatures'):
            obj.addProperty('App::PropertyEnumeration', 'HandleMultipleFeatures', 'Profile', QtCore.QT_TRANSLATE_NOOP('PathPocket', 'Choose how to process multiple Base Geometry features.'))

        obj.HandleMultipleFeatures = ['Collectively', 'Individually']

        self.initRotationOp(obj)
        self.baseObject().initAreaOp(obj)

    def initRotationOp(self, obj):
        '''initRotationOp(obj) ... setup receiver for rotation'''
        if not hasattr(obj, 'ReverseDirection'):
            obj.addProperty('App::PropertyBool', 'ReverseDirection', 'Rotation', QtCore.QT_TRANSLATE_NOOP('App::Property', 'Reverse direction of pocket operation.'))
        if not hasattr(obj, 'InverseAngle'):
            obj.addProperty('App::PropertyBool', 'InverseAngle', 'Rotation', QtCore.QT_TRANSLATE_NOOP('App::Property', 'Inverse the angle. Example: -22.5 -> 22.5 degrees.'))
        if not hasattr(obj, 'AttemptInverseAngle'):
            obj.addProperty('App::PropertyBool', 'AttemptInverseAngle', 'Rotation', QtCore.QT_TRANSLATE_NOOP('App::Property', 'Attempt the inverse angle for face access if original rotation fails.'))
        if not hasattr(obj, 'LimitDepthToFace'):
            obj.addProperty('App::PropertyBool', 'LimitDepthToFace', 'Rotation', QtCore.QT_TRANSLATE_NOOP('App::Property', 'Enforce the Z-depth of the selected face as the lowest value for final depth. Higher user values will be observed.'))

    def extraOpOnChanged(self, obj, prop):
        '''extraOpOnChanged(obj, porp) ... process operation specific changes to properties.'''
        if prop == 'EnableRotation':
            self.setOpEditorProperties(obj)

    def setOpEditorProperties(self, obj):
        if obj.EnableRotation == 'Off':
            obj.setEditorMode('ReverseDirection', 2)
            obj.setEditorMode('InverseAngle', 2)
            obj.setEditorMode('AttemptInverseAngle', 2)
            obj.setEditorMode('LimitDepthToFace', 2)
        else:
            obj.setEditorMode('ReverseDirection', 0)
            obj.setEditorMode('InverseAngle', 0)
            obj.setEditorMode('AttemptInverseAngle', 0)
            obj.setEditorMode('LimitDepthToFace', 0)

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... returns envelope for all base shapes or wires for Arch.Panels.'''
        PathLog.track()

        if obj.UseComp:
            self.commandlist.append(Path.Command("(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"))
        else:
            self.commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        shapes = []
        self.profileshape = [] # pylint: disable=attribute-defined-outside-init

        baseSubsTuples = []
        subCount = 0
        allTuples = []

        if obj.Base:  # The user has selected subobjects from the base.  Process each.
            if obj.EnableRotation != 'Off':
                for p in range(0, len(obj.Base)):
                    (base, subsList) = obj.Base[p]
                    for sub in subsList:
                        subCount += 1
                        shape = getattr(base.Shape, sub)
                        if isinstance(shape, Part.Face):
                            rtn = False
                            (norm, surf) = self.getFaceNormAndSurf(shape)
                            (rtn, angle, axis, praInfo) = self.faceRotationAnalysis(obj, norm, surf) # pylint: disable=unused-variable
                            PathLog.debug("initial faceRotationAnalysis: {}".format(praInfo))
                            if rtn is True:
                                (clnBase, angle, clnStock, tag) = self.applyRotationalAnalysis(obj, base, angle, axis, subCount)
                                # Verify faces are correctly oriented - InverseAngle might be necessary
                                faceIA = getattr(clnBase.Shape, sub)
                                (norm, surf) = self.getFaceNormAndSurf(faceIA)
                                (rtn, praAngle, praAxis, praInfo2) = self.faceRotationAnalysis(obj, norm, surf) # pylint: disable=unused-variable
                                PathLog.debug("follow-up faceRotationAnalysis: {}".format(praInfo2))

                                if abs(praAngle) == 180.0:
                                    rtn = False
                                    if self.isFaceUp(clnBase, faceIA) is False:
                                        PathLog.debug('isFaceUp 1 is False')
                                        angle -= 180.0

                                if rtn is True:
                                    PathLog.debug(translate("Path", "Face appears misaligned after initial rotation."))
                                    if obj.InverseAngle is False:
                                        if obj.AttemptInverseAngle is True:
                                            (clnBase, clnStock, angle) = self.applyInverseAngle(obj, clnBase, clnStock, axis, angle)
                                        else:
                                            msg = translate("Path", "Consider toggling the 'InverseAngle' property and recomputing.")
                                            PathLog.warning(msg)

                                    if self.isFaceUp(clnBase, faceIA) is False:
                                        PathLog.debug('isFaceUp 2 is False')
                                        angle += 180.0
                                    else:
                                        PathLog.debug('  isFaceUp')

                                else:
                                    PathLog.debug("Face appears to be oriented correctly.")

                                if angle < 0.0:
                                    angle += 360.0

                                tup = clnBase, sub, tag, angle, axis, clnStock
                            else:
                                if self.warnDisabledAxis(obj, axis) is False:
                                    PathLog.debug(str(sub) + ": No rotation used")
                                axis = 'X'
                                angle = 0.0
                                tag = base.Name + '_' + axis + str(angle).replace('.', '_')
                                stock = PathUtils.findParentJob(obj).Stock
                                tup = base, sub, tag, angle, axis, stock

                            allTuples.append(tup)

                if subCount > 1:
                    msg = translate('Path', "Multiple faces in Base Geometry.") + "  "
                    msg += translate('Path', "Depth settings will be applied to all faces.")
                    PathLog.warning(msg)

                (Tags, Grps) = self.sortTuplesByIndex(allTuples, 2)  # return (TagList, GroupList)
                subList = []
                for o in range(0, len(Tags)):
                    subList = []
                    for (base, sub, tag, angle, axis, stock) in Grps[o]:
                        subList.append(sub)

                    pair = base, subList, angle, axis, stock
                    baseSubsTuples.append(pair)
                # Efor
            else:
                PathLog.debug(translate("Path", "EnableRotation property is 'Off'."))
                stock = PathUtils.findParentJob(obj).Stock
                for (base, subList) in obj.Base:
                    baseSubsTuples.append((base, subList, 0.0, 'X', stock))

            # for base in obj.Base:
            finish_step = obj.FinishDepth.Value if hasattr(obj, "FinishDepth") else 0.0
            for (base, subsList, angle, axis, stock) in baseSubsTuples:
                holes = []
                faces = []
                faceDepths = []
                startDepths = []

                for sub in subsList:
                    shape = getattr(base.Shape, sub)
                    if isinstance(shape, Part.Face):
                        faces.append(shape)
                        if numpy.isclose(abs(shape.normalAt(0, 0).z), 1):  # horizontal face
                            for wire in shape.Wires[1:]:
                                holes.append((base.Shape, wire))

                        # Add face depth to list
                        faceDepths.append(shape.BoundBox.ZMin)
                    else:
                        ignoreSub = base.Name + '.' + sub
                        msg = translate('Path', "Found a selected object which is not a face. Ignoring: {}".format(ignoreSub))
                        PathLog.error(msg)
                        FreeCAD.Console.PrintWarning(msg)

                # Set initial Start and Final Depths and recalculate depthparams
                finDep = obj.FinalDepth.Value
                strDep = obj.StartDepth.Value
                if strDep > stock.Shape.BoundBox.ZMax:
                    strDep = stock.Shape.BoundBox.ZMax

                startDepths.append(strDep)
                self.depthparams = self._customDepthParams(obj, strDep, finDep)

                for shape, wire in holes:
                    f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                    drillable = PathUtils.isDrillable(shape, wire)
                    if (drillable and obj.processCircles) or (not drillable and obj.processHoles):
                        env = PathUtils.getEnvelope(shape, subshape=f, depthparams=self.depthparams)
                        tup = env, True, 'pathProfileFaces', angle, axis, strDep, finDep
                        shapes.append(tup)

                if len(faces) > 0:
                    profileshape = Part.makeCompound(faces)
                    self.profileshape.append(profileshape)

                if obj.processPerimeter:
                    if obj.HandleMultipleFeatures == 'Collectively':
                        custDepthparams = self.depthparams

                        if obj.LimitDepthToFace is True and obj.EnableRotation != 'Off':
                            if profileshape.BoundBox.ZMin > obj.FinalDepth.Value:
                                finDep = profileshape.BoundBox.ZMin
                                envDepthparams = self._customDepthParams(obj, strDep + 0.5, finDep)  # only an envelope
                        try:
                            # env = PathUtils.getEnvelope(base.Shape, subshape=profileshape, depthparams=envDepthparams)
                            env = PathUtils.getEnvelope(profileshape, depthparams=envDepthparams)
                        except Exception: # pylint: disable=broad-except
                            # PathUtils.getEnvelope() failed to return an object.
                            PathLog.error(translate('Path', 'Unable to create path for face(s).'))
                        else:
                            tup = env, False, 'pathProfileFaces', angle, axis, strDep, finDep
                            shapes.append(tup)

                    elif obj.HandleMultipleFeatures == 'Individually':
                        for shape in faces:
                            # profShape = Part.makeCompound([shape])
                            finalDep = obj.FinalDepth.Value
                            custDepthparams = self.depthparams
                            if obj.Side == 'Inside':
                                if finalDep < shape.BoundBox.ZMin:
                                    # Recalculate depthparams
                                    finalDep = shape.BoundBox.ZMin
                                    custDepthparams = self._customDepthParams(obj, strDep + 0.5, finalDep)

                            # env = PathUtils.getEnvelope(base.Shape, subshape=profShape, depthparams=custDepthparams)
                            env = PathUtils.getEnvelope(shape, depthparams=custDepthparams)
                            tup = env, False, 'pathProfileFaces', angle, axis, strDep, finalDep
                            shapes.append(tup)

            # Lower high Start Depth to top of Stock
            startDepth = max(startDepths)
            if obj.StartDepth.Value > startDepth:
                obj.StartDepth.Value = startDepth

        else:  # Try to build targets from the job base
            if 1 == len(self.model):
                if hasattr(self.model[0], "Proxy"):
                    PathLog.debug("hasattr() Proxy")
                    if isinstance(self.model[0].Proxy, ArchPanel.PanelSheet):  # process the sheet
                        if obj.processCircles or obj.processHoles:
                            for shape in self.model[0].Proxy.getHoles(self.model[0], transform=True):
                                for wire in shape.Wires:
                                    drillable = PathUtils.isDrillable(self.model[0].Proxy, wire)
                                    if (drillable and obj.processCircles) or (not drillable and obj.processHoles):
                                        f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                                        env = PathUtils.getEnvelope(self.model[0].Shape, subshape=f, depthparams=self.depthparams)
                                        tup = env, True, 'pathProfileFaces', 0.0, 'X', obj.StartDepth.Value, obj.FinalDepth.Value
                                        shapes.append(tup)

                        if obj.processPerimeter:
                            for shape in self.model[0].Proxy.getOutlines(self.model[0], transform=True):
                                for wire in shape.Wires:
                                    f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                                    env = PathUtils.getEnvelope(self.model[0].Shape, subshape=f, depthparams=self.depthparams)
                                    tup = env, False, 'pathProfileFaces', 0.0, 'X', obj.StartDepth.Value, obj.FinalDepth.Value
                                    shapes.append(tup)

        self.removalshapes = shapes # pylint: disable=attribute-defined-outside-init
        PathLog.debug("%d shapes" % len(shapes))

        return shapes

    def areaOpSetDefaultValues(self, obj, job):
        '''areaOpSetDefaultValues(obj, job) ... sets default values for hole, circle and perimeter processing.'''
        self.baseObject().areaOpSetDefaultValues(obj, job)

        obj.processHoles = False
        obj.processCircles = False
        obj.processPerimeter = True
        obj.ReverseDirection = False
        obj.InverseAngle = False
        obj.AttemptInverseAngle = True
        obj.LimitDepthToFace = True
        obj.HandleMultipleFeatures = 'Individually'


def SetupProperties():
    setup = PathProfileBase.SetupProperties()
    setup.append("processHoles")
    setup.append("processPerimeter")
    setup.append("processCircles")
    setup.append("ReverseDirection")
    setup.append("InverseAngle")
    setup.append("AttemptInverseAngle")
    setup.append("HandleMultipleFeatures")
    return setup


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Profile based on faces operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectProfile(obj, name)
    return obj
