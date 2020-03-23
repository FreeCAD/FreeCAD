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
# *                                                                         *
# *   Additional modifications and contributions beginning 2019             *
# *   by Russell Johnson  <russ4262@gmail.com>  2020-03-15 10:55 CST        *
# *                                                                         *
# ***************************************************************************

from __future__ import print_function

import FreeCAD
import MeshPart
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
import PathScripts.PathOp as PathOp

from PySide import QtCore
import time
import math
import Part
import Draft

if FreeCAD.GuiUp:
    import FreeCADGui

__title__ = "Path Surface Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of Mill Facing operation."

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


# OCL must be installed
try:
    import ocl
except ImportError:
    FreeCAD.Console.PrintError(
        translate("Path_Surface", "This operation requires OpenCamLib to be installed.") + "\n")
    import sys
    sys.exit(translate("Path_Surface", "This operation requires OpenCamLib to be installed."))


class ObjectSurface(PathOp.ObjectOp):
    '''Proxy object for Surfacing operation.'''

    def baseObject(self):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        return super(self.__class__, self)

    def opFeatures(self, obj):
        '''opFeatures(obj) ... return all standard features and edges based geomtries'''
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureStepDown | PathOp.FeatureCoolant | PathOp.FeatureBaseFaces

    def initOperation(self, obj):
        '''initPocketOp(obj) ... create facing specific properties'''
        obj.addProperty("App::PropertyEnumeration", "BoundBox", "Waterline", QtCore.QT_TRANSLATE_NOOP("App::Property", "Should the operation be limited by the stock object or by the bounding box of the base object"))
        obj.addProperty("App::PropertyEnumeration", "LayerMode", "Waterline", QtCore.QT_TRANSLATE_NOOP("App::Property", "The completion mode for the operation: single or multi-pass"))
        obj.addProperty("App::PropertyEnumeration", "ScanType", "Waterline", QtCore.QT_TRANSLATE_NOOP("App::Property", "Planar: Flat, 3D surface scan.  Rotational: 4th-axis rotational scan."))

        obj.addProperty("App::PropertyFloat", "CutterTilt", "Rotational", QtCore.QT_TRANSLATE_NOOP("App::Property", "Stop index(angle) for rotational scan"))
        obj.addProperty("App::PropertyEnumeration", "RotationAxis", "Rotational", QtCore.QT_TRANSLATE_NOOP("App::Property", "The model will be rotated around this axis."))
        obj.addProperty("App::PropertyFloat", "StartIndex", "Rotational", QtCore.QT_TRANSLATE_NOOP("App::Property", "Start index(angle) for rotational scan"))
        obj.addProperty("App::PropertyFloat", "StopIndex", "Rotational", QtCore.QT_TRANSLATE_NOOP("App::Property", "Stop index(angle) for rotational scan"))

        obj.addProperty("App::PropertyInteger", "AvoidLastX_Faces", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Avoid cutting the last 'N' faces in the Base Geometry list of selected faces."))
        obj.addProperty("App::PropertyBool", "AvoidLastX_InternalFeatures", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Do not cut internal features on avoided faces."))
        obj.addProperty("App::PropertyDistance", "BoundaryAdjustment", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Positive values push the cutter toward, or beyond, the boundary. Negative values retract the cutter away from the boundary."))
        obj.addProperty("App::PropertyBool", "BoundaryEnforcement", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "If true, the cutter will remain inside the boundaries of the model or selected face(s)."))
        obj.addProperty("App::PropertyDistance", "DepthOffset", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Z-axis offset from the surface of the object"))
        obj.addProperty("App::PropertyEnumeration", "HandleMultipleFeatures", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Choose how to process multiple Base Geometry features."))
        obj.addProperty("App::PropertyDistance", "InternalFeaturesAdjustment", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Positive values push the cutter toward, or into, the feature. Negative values retract the cutter away from the feature."))
        obj.addProperty("App::PropertyBool", "InternalFeaturesCut", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Ignore internal feature areas within a larger selected face."))
        obj.addProperty("App::PropertyEnumeration", "ProfileEdges", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile the edges of the selection."))
        obj.addProperty("App::PropertyDistance", "SampleInterval", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "The Sample Interval. Small values cause long wait times"))
        obj.addProperty("App::PropertyPercent", "StepOver", "Surface", QtCore.QT_TRANSLATE_NOOP("App::Property", "Step over percentage of the drop cutter path"))

        obj.addProperty("App::PropertyVectorDistance", "CircularCenterCustom", "Surface Cut Options", QtCore.QT_TRANSLATE_NOOP("PathOp", "The start point of this path"))
        obj.addProperty("App::PropertyEnumeration", "CircularCenterAt", "Surface Cut Options", QtCore.QT_TRANSLATE_NOOP("PathOp", "Choose what point to start the ciruclar pattern: Center Of Mass, Center Of Boundbox, Xmin Ymin of boundbox, Custom."))
        obj.addProperty("App::PropertyEnumeration", "CutMode", "Surface Cut Options", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction that the toolpath should go around the part: Climb(ClockWise) or Conventional(CounterClockWise)"))
        obj.addProperty("App::PropertyEnumeration", "CutPattern", "Surface Cut Options", QtCore.QT_TRANSLATE_NOOP("App::Property", "Clearing pattern to use"))
        obj.addProperty("App::PropertyFloat", "CutPatternAngle", "Surface Cut Options", QtCore.QT_TRANSLATE_NOOP("App::Property", "Yaw angle for certain clearing patterns"))
        obj.addProperty("App::PropertyBool", "CutPatternReversed", "Surface Cut Options", QtCore.QT_TRANSLATE_NOOP("App::Property", "If true, the order of the step-overs will be reversed; the operation will begin cutting the outer most line/arc, and work toward the inner most line/arc."))

        obj.addProperty("App::PropertyBool", "OptimizeLinearPaths", "Surface Optimization", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable optimization of linear paths (co-linear points). Removes unnecessary co-linear points from G-Code output."))
        obj.addProperty("App::PropertyBool", "OptimizeStepOverTransitions", "Surface Optimization", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable separate optimization of transitions between, and breaks within, each step over path."))
        obj.addProperty("App::PropertyBool", "CircularUseG2G3", "Surface Optimization", QtCore.QT_TRANSLATE_NOOP("App::Property", "Convert co-planar arcs to G2/G3 gcode commands for `Circular` and `CircularZigZag` cut patterns."))
        obj.addProperty("App::PropertyDistance", "GapThreshold", "Surface Optimization", QtCore.QT_TRANSLATE_NOOP("App::Property", "Collinear and co-radial artifact gaps that are smaller than this threshold are closed in the path."))
        obj.addProperty("App::PropertyString", "GapSizes", "Surface Optimization", QtCore.QT_TRANSLATE_NOOP("App::Property", "Feedback: three smallest gaps identified in the path geometry."))

        obj.addProperty("App::PropertyBool", "IgnoreWaste", "Waste", QtCore.QT_TRANSLATE_NOOP("App::Property", "Ignore areas that proceed below specified depth."))
        obj.addProperty("App::PropertyFloat", "IgnoreWasteDepth", "Waste", QtCore.QT_TRANSLATE_NOOP("App::Property", "Depth used to identify waste areas to ignore."))
        obj.addProperty("App::PropertyBool", "ReleaseFromWaste", "Waste", QtCore.QT_TRANSLATE_NOOP("App::Property", "Cut through waste to depth at model edge, releasing the model."))

        obj.addProperty("App::PropertyVectorDistance", "StartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("PathOp", "The start point of this path"))
        obj.addProperty("App::PropertyBool", "UseStartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("PathOp", "Make True, if specifying a Start Point"))

        # For debugging
        obj.addProperty('App::PropertyString', 'AreaParams', 'Debugging')
        obj.setEditorMode('AreaParams', 2)  # hide
        obj.addProperty("App::PropertyBool", "ShowTempObjects", "Debug", QtCore.QT_TRANSLATE_NOOP("App::Property", "If true, the temporary path construction objects will be shown."))
        if PathLog.getLevel(PathLog.thisModule()) != 4:
            obj.setEditorMode('ShowTempObjects', 2)  # hide

        obj.BoundBox = ['BaseBoundBox', 'Stock']
        obj.CircularCenterAt = ['CenterOfMass', 'CenterOfBoundBox', 'XminYmin', 'Custom']
        obj.CutMode = ['Conventional', 'Climb']
        obj.CutPattern = ['Line', 'ZigZag', 'Circular', 'CircularZigZag']  # Additional goals ['Offset', 'Spiral', 'ZigZagOffset', 'Grid', 'Triangle']
        obj.HandleMultipleFeatures = ['Collectively', 'Individually']
        obj.LayerMode = ['Single-pass', 'Multi-pass']
        obj.ProfileEdges = ['None', 'Only', 'First', 'Last']
        obj.RotationAxis = ['X', 'Y']
        obj.ScanType = ['Planar', 'Rotational']

        if not hasattr(obj, 'DoNotSetDefaultValues'):
            self.setEditorProperties(obj)

        self.addedAllProperties = True

    def setEditorProperties(self, obj):
        # Used to hide inputs in properties list

        '''
        obj.setEditorMode('CutPattern', 0)
        obj.setEditorMode('HandleMultipleFeatures', 0)
        obj.setEditorMode('CircularCenterAt', 0)
        obj.setEditorMode('CircularCenterCustom', 0)
        obj.setEditorMode('CutPatternAngle', 0)
        # obj.setEditorMode('BoundaryEnforcement', 0)

        if obj.ScanType == 'Planar':
            obj.setEditorMode('RotationAxis', 2)  # 2=hidden
            obj.setEditorMode('StartIndex', 2)
            obj.setEditorMode('StopIndex', 2)
            obj.setEditorMode('CutterTilt', 2)
            if obj.CutPattern == 'Circular' or obj.CutPattern == 'CircularZigZag':
                obj.setEditorMode('CutPatternAngle', 2)
            else:  # if obj.CutPattern == 'Line' or obj.CutPattern == 'ZigZag':
                obj.setEditorMode('CircularCenterAt', 2)
                obj.setEditorMode('CircularCenterCustom', 2)
        elif obj.ScanType == 'Rotational':
            obj.setEditorMode('RotationAxis', 0)  # 0=show & editable
            obj.setEditorMode('StartIndex', 0)
            obj.setEditorMode('StopIndex', 0)
            obj.setEditorMode('CutterTilt', 0)
        '''

        obj.setEditorMode('HandleMultipleFeatures', 2)
        obj.setEditorMode('CutPattern', 2)
        obj.setEditorMode('CutPatternAngle', 2)
        # obj.setEditorMode('BoundaryEnforcement', 2)

        # Disable IgnoreWaste feature
        obj.setEditorMode('IgnoreWaste', 2)
        obj.setEditorMode('IgnoreWasteDepth', 2)
        obj.setEditorMode('ReleaseFromWaste', 2)

    def onChanged(self, obj, prop):
        if hasattr(self, 'addedAllProperties'):
            if self.addedAllProperties is True:
                if prop == 'ScanType':
                    self.setEditorProperties(obj)
                if prop == 'CutPattern':
                    self.setEditorProperties(obj)

    def opOnDocumentRestored(self, obj):
        if PathLog.getLevel(PathLog.thisModule()) != 4:
            obj.setEditorMode('ShowTempObjects', 2)  # hide
        else:
            obj.setEditorMode('ShowTempObjects', 0)  # show
        self.addedAllProperties = True
        self.setEditorProperties(obj)

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj, job) ... initialize defaults'''
        job = PathUtils.findParentJob(obj)

        obj.OptimizeLinearPaths = True
        obj.IgnoreWaste = False
        obj.ReleaseFromWaste = False
        obj.InternalFeaturesCut = True
        obj.OptimizeStepOverTransitions = False
        obj.CircularUseG2G3 = False
        obj.BoundaryEnforcement = True
        obj.UseStartPoint = False
        obj.AvoidLastX_InternalFeatures = True
        obj.CutPatternReversed = False
        obj.StartPoint.x = 0.0
        obj.StartPoint.y = 0.0
        obj.StartPoint.z = obj.ClearanceHeight.Value
        obj.ProfileEdges = 'None'
        obj.LayerMode = 'Single-pass'
        obj.ScanType = 'Planar'
        obj.RotationAxis = 'X'
        obj.CutMode = 'Conventional'
        obj.CutPattern = 'Line'
        obj.HandleMultipleFeatures = 'Collectively'  # 'Individually'
        obj.CircularCenterAt = 'CenterOfMass'  # 'CenterOfBoundBox', 'XminYmin', 'Custom'
        obj.AreaParams = ''
        obj.GapSizes = 'No gaps identified.'
        obj.StepOver = 100
        obj.CutPatternAngle = 0.0
        obj.CutterTilt = 0.0
        obj.StartIndex = 0.0
        obj.StopIndex = 360.0
        obj.SampleInterval.Value = 1.0
        obj.BoundaryAdjustment.Value = 0.0
        obj.InternalFeaturesAdjustment.Value = 0.0
        obj.AvoidLastX_Faces = 0
        obj.CircularCenterCustom.x = 0.0
        obj.CircularCenterCustom.y = 0.0
        obj.CircularCenterCustom.z = 0.0
        obj.GapThreshold.Value = 0.005
        # For debugging
        obj.ShowTempObjects = False

        # need to overwrite the default depth calculations for facing
        d = None
        if job:
            if job.Stock:
                d = PathUtils.guessDepths(job.Stock.Shape, None)
                PathLog.debug("job.Stock exists")
            else:
                PathLog.debug("job.Stock NOT exist")
        else:
            PathLog.debug("job NOT exist")

        if d is not None:
            obj.OpFinalDepth.Value = d.final_depth
            obj.OpStartDepth.Value = d.start_depth
        else:
            obj.OpFinalDepth.Value = -10
            obj.OpStartDepth.Value = 10

        PathLog.debug('Default OpFinalDepth: {}'.format(obj.OpFinalDepth.Value))
        PathLog.debug('Defualt OpStartDepth: {}'.format(obj.OpStartDepth.Value))

    def opApplyPropertyLimits(self, obj):
        '''opApplyPropertyLimits(obj) ... Apply necessary limits to user input property values before performing main operation.'''
        # Limit start index
        if obj.StartIndex < 0.0:
            obj.StartIndex = 0.0
        if obj.StartIndex > 360.0:
            obj.StartIndex = 360.0

        # Limit stop index
        if obj.StopIndex > 360.0:
            obj.StopIndex = 360.0
        if obj.StopIndex < 0.0:
            obj.StopIndex = 0.0

        # Limit cutter tilt
        if obj.CutterTilt < -90.0:
            obj.CutterTilt = -90.0
        if obj.CutterTilt > 90.0:
            obj.CutterTilt = 90.0

        # Limit sample interval
        if obj.SampleInterval.Value < 0.001:
            obj.SampleInterval.Value = 0.001
            PathLog.error(translate('PathSurface', 'Sample interval limits are 0.001 to 25.4 millimeters.'))
        if obj.SampleInterval.Value > 25.4:
            obj.SampleInterval.Value = 25.4
            PathLog.error(translate('PathSurface', 'Sample interval limits are 0.001 to 25.4 millimeters.'))

        # Limit cut pattern angle
        if obj.CutPatternAngle < -360.0:
            obj.CutPatternAngle = 0.0
            PathLog.error(translate('PathSurface', 'Cut pattern angle limits are +-360 degrees.'))
        if obj.CutPatternAngle >= 360.0:
            obj.CutPatternAngle = 0.0
            PathLog.error(translate('PathSurface', 'Cut pattern angle limits are +- 360 degrees.'))

        # Limit StepOver to natural number percentage
        if obj.StepOver > 100:
            obj.StepOver = 100
        if obj.StepOver < 1:
            obj.StepOver = 1

        # Limit AvoidLastX_Faces to zero and positive values
        if obj.AvoidLastX_Faces < 0:
            obj.AvoidLastX_Faces = 0
            PathLog.error(translate('PathSurface', 'AvoidLastX_Faces: Only zero or positive values permitted.'))
        if obj.AvoidLastX_Faces > 100:
            obj.AvoidLastX_Faces = 100
            PathLog.error(translate('PathSurface', 'AvoidLastX_Faces: Avoid last X faces count limited to 100.'))

    def opExecute(self, obj):
        '''opExecute(obj) ... process surface operation'''
        PathLog.track()

        self.modelSTLs = list()
        self.safeSTLs = list()
        self.modelTypes = list()
        self.boundBoxes = list()
        self.profileShapes = list()
        self.collectiveShapes = list()
        self.individualShapes = list()
        self.avoidShapes = list()
        self.deflection = None
        self.tempGroup = None
        self.CutClimb = False
        self.closedGap = False
        self.gaps = [0.1, 0.2, 0.3]
        CMDS = list()
        modelVisibility = list()
        FCAD = FreeCAD.ActiveDocument

        # Set debugging behavior
        self.showDebugObjects = False  # Set to true if you want a visual DocObjects created for some path construction objects
        self.showDebugObjects = obj.ShowTempObjects
        deleteTempsFlag = True  # Set to False for debugging
        if PathLog.getLevel(PathLog.thisModule()) == 4:
            deleteTempsFlag = False
        else:
            self.showDebugObjects = False

        # mark beginning of operation and identify parent Job
        PathLog.info('\nBegin 3D Surface operation...')
        startTime = time.time()

        # Disable(ignore) ReleaseFromWaste option(input)
        obj.ReleaseFromWaste = False

        # Identify parent Job
        JOB = PathUtils.findParentJob(obj)
        if JOB is None:
            PathLog.error(translate('PathSurface', "No JOB"))
            return
        self.stockZMin = JOB.Stock.Shape.BoundBox.ZMin

        # set cut mode; reverse as needed
        if obj.CutMode == 'Climb':
            self.CutClimb = True
        if obj.CutPatternReversed is True:
            if self.CutClimb is True:
                self.CutClimb = False
            else:
                self.CutClimb = True

        # Begin GCode for operation with basic information
        # ... and move cutter to clearance height and startpoint
        output = ''
        if obj.Comment != '':
            output += '(' + str(obj.Comment) + ')\n'
        output += '(' + obj.Label + ')\n'
        output += '(Tool type: ' + str(obj.ToolController.Tool.ToolType) + ')\n'
        output += '(Compensated Tool Path. Diameter: ' + str(obj.ToolController.Tool.Diameter) + ')\n'
        output += '(Sample interval: ' + str(obj.SampleInterval.Value) + ')\n'
        output += '(Step over %: ' + str(obj.StepOver) + ')\n'
        self.commandlist.append(Path.Command('N ({})'.format(output), {}))
        self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        if obj.UseStartPoint is True:
            self.commandlist.append(Path.Command('G0', {'X': obj.StartPoint.x, 'Y': obj.StartPoint.y, 'F': self.horizRapid}))

        # Instantiate additional class operation variables
        self.resetOpVariables()

        # Impose property limits
        self.opApplyPropertyLimits(obj)

        # Create temporary group for temporary objects, removing existing
        # if self.showDebugObjects is True:
        tempGroupName = 'tempPathSurfaceGroup'
        if FCAD.getObject(tempGroupName):
            for to in FCAD.getObject(tempGroupName).Group:
                FCAD.removeObject(to.Name)
            FCAD.removeObject(tempGroupName)  # remove temp directory if already exists
        if FCAD.getObject(tempGroupName + '001'):
            for to in FCAD.getObject(tempGroupName + '001').Group:
                FCAD.removeObject(to.Name)
            FCAD.removeObject(tempGroupName + '001')  # remove temp directory if already exists
        tempGroup = FCAD.addObject('App::DocumentObjectGroup', tempGroupName)
        tempGroupName = tempGroup.Name
        self.tempGroup = tempGroup
        tempGroup.purgeTouched()
        # Add temp object to temp group folder with following code:
        # ... self.tempGroup.addObject(OBJ)

        # Setup cutter for OCL and cutout value for operation - based on tool controller properties
        self.cutter = self.setOclCutter(obj)
        self.safeCutter = self.setOclCutter(obj, safe=True)
        if self.cutter is False or self.safeCutter is False:
            PathLog.error(translate('PathSurface', "Canceling 3D Surface operation. Error creating OCL cutter."))
            return
        toolDiam = self.cutter.getDiameter()
        self.cutOut = (toolDiam * (float(obj.StepOver) / 100.0))
        self.radius = toolDiam / 2.0
        self.gaps = [toolDiam, toolDiam, toolDiam]

        # Get height offset values for later use
        self.SafeHeightOffset = JOB.SetupSheet.SafeHeightOffset.Value
        self.ClearHeightOffset = JOB.SetupSheet.ClearanceHeightOffset.Value

        # Calculate default depthparams for operation
        self.depthParams = PathUtils.depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown.Value, 0.0, obj.FinalDepth.Value)
        self.midDep = (obj.StartDepth.Value + obj.FinalDepth.Value) / 2.0

        # make circle for workplane
        self.wpc = Part.makeCircle(2.0)

        # Set deflection values for mesh generation
        self.angularDeflection = 0.05
        try:  # try/except is for Path Jobs created before GeometryTolerance
            self.deflection = JOB.GeometryTolerance.Value
        except AttributeError as ee:
            PathLog.warning('Error setting Mesh deflection: {}.  Using PathPreferences.defaultGeometryTolerance().'.format(ee))
            import PathScripts.PathPreferences as PathPreferences
            self.deflection = PathPreferences.defaultGeometryTolerance()

        # Save model visibilities for restoration
        if FreeCAD.GuiUp:
            for m in range(0, len(JOB.Model.Group)):
                mNm = JOB.Model.Group[m].Name
                modelVisibility.append(FreeCADGui.ActiveDocument.getObject(mNm).Visibility)

        # Setup STL, model type, and bound box containers for each model in Job
        for m in range(0, len(JOB.Model.Group)):
            M = JOB.Model.Group[m]
            self.modelSTLs.append(False)
            self.safeSTLs.append(False)
            self.profileShapes.append(False)
            # Set bound box
            if obj.BoundBox == 'BaseBoundBox':
                if M.TypeId.startswith('Mesh'):
                    self.modelTypes.append('M')  # Mesh
                    self.boundBoxes.append(M.Mesh.BoundBox)
                else:
                    self.modelTypes.append('S')  # Solid
                    self.boundBoxes.append(M.Shape.BoundBox)
            elif obj.BoundBox == 'Stock':
                self.modelTypes.append('S')  # Solid
                self.boundBoxes.append(JOB.Stock.Shape.BoundBox)

        # ######  MAIN COMMANDS FOR OPERATION ######

        # If algorithm is `Waterline`, force certain property values
        '''
        # Save initial value for restoration later.
        if obj.Algorithm == 'OCL Waterline':
            preCP = obj.CutPattern
            preCPA = obj.CutPatternAngle
            preRB = obj.BoundaryEnforcement
            obj.CutPattern = 'Line'
            obj.CutPatternAngle = 0.0
            obj.BoundaryEnforcement = False
        '''

        # Begin processing obj.Base data and creating GCode
        # Process selected faces, if available
        pPM = self._preProcessModel(JOB, obj)
        if pPM is False:
            PathLog.error('Unable to pre-process obj.Base.')
        else:
            (FACES, VOIDS) = pPM

            # Create OCL.stl model objects
            self._prepareModelSTLs(JOB, obj)

            for m in range(0, len(JOB.Model.Group)):
                Mdl = JOB.Model.Group[m]
                if FACES[m] is False:
                    PathLog.error('No data for model base: {}'.format(JOB.Model.Group[m].Label))
                else:
                    if m > 0:
                        # Raise to clearance between moddels
                        CMDS.append(Path.Command('N (Transition to base: {}.)'.format(Mdl.Label)))
                        CMDS.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
                        PathLog.info('Working on Model.Group[{}]: {}'.format(m, Mdl.Label))
                    # make stock-model-voidShapes STL model for avoidance detection on transitions
                    self._makeSafeSTL(JOB, obj, m, FACES[m], VOIDS[m])
                    time.sleep(0.2)
                    # Process model/faces - OCL objects must be ready
                    CMDS.extend(self._processCutAreas(JOB, obj, m, FACES[m], VOIDS[m]))

            # Save gcode produced
            self.commandlist.extend(CMDS)

            # If algorithm is `Waterline`, restore initial property values
            '''
            if obj.Algorithm == 'OCL Waterline':
                obj.CutPattern = preCP
                obj.CutPatternAngle = preCPA
                obj.BoundaryEnforcement = preRB
            '''

        # ######  CLOSING COMMANDS FOR OPERATION ######

        # Delete temporary objects
        # Restore model visibilities for restoration
        if FreeCAD.GuiUp:
            FreeCADGui.ActiveDocument.getObject(tempGroupName).Visibility = False
            for m in range(0, len(JOB.Model.Group)):
                M = JOB.Model.Group[m]
                M.Visibility = modelVisibility[m]

        if deleteTempsFlag is True:
            for to in tempGroup.Group:
                if hasattr(to, 'Group'):
                    for go in to.Group:
                        FCAD.removeObject(go.Name)
                FCAD.removeObject(to.Name)
            FCAD.removeObject(tempGroupName)
        else:
            if len(tempGroup.Group) == 0:
                FCAD.removeObject(tempGroupName)
            else:
                tempGroup.purgeTouched()

        # Provide user feedback for gap sizes
        gaps = list()
        for g in self.gaps:
            if g != toolDiam:
                gaps.append(g)
        if len(gaps) > 0:
            obj.GapSizes = '{} mm'.format(gaps)
        else:
            if self.closedGap is True:
                obj.GapSizes = 'Closed gaps < Gap Threshold.'
            else:
                obj.GapSizes = 'No gaps identified.'

        # clean up class variables
        self.resetOpVariables()
        self.deleteOpVariables()

        self.modelSTLs = None
        self.safeSTLs = None
        self.modelTypes = None
        self.boundBoxes = None
        self.gaps = None
        self.closedGap = None
        self.SafeHeightOffset = None
        self.ClearHeightOffset = None
        self.depthParams = None
        self.midDep = None
        self.wpc = None
        self.angularDeflection = None
        self.deflection = None
        del self.modelSTLs
        del self.safeSTLs
        del self.modelTypes
        del self.boundBoxes
        del self.gaps
        del self.closedGap
        del self.SafeHeightOffset
        del self.ClearHeightOffset
        del self.depthParams
        del self.midDep
        del self.wpc
        del self.angularDeflection
        del self.deflection

        execTime = time.time() - startTime
        PathLog.info('Operation time: {} sec.'.format(execTime))

        return True

    # Methods for constructing the cut area
    def _preProcessModel(self, JOB, obj):
        PathLog.debug('_preProcessModel()')

        FACES = list()
        VOIDS = list()
        fShapes = list()
        vShapes = list()
        preProcEr = translate('PathSurface', 'Error pre-processing Face')
        warnFinDep = translate('PathSurface', 'Final Depth might need to be lower. Internal features detected in Face')
        GRP = JOB.Model.Group
        lenGRP = len(GRP)

        # Crete place holders for each base model in Job
        for m in range(0, lenGRP):
            FACES.append(False)
            VOIDS.append(False)
            fShapes.append(False)
            vShapes.append(False)

        # The user has selected subobjects from the base.  Pre-Process each.
        if obj.Base and len(obj.Base) > 0:
            PathLog.debug(' -obj.Base exists. Pre-processing for selected faces.')

            (FACES, VOIDS) = self._identifyFacesAndVoids(JOB, obj, FACES, VOIDS)

            # Cycle through each base model, processing faces for each
            for m in range(0, lenGRP):
                base = GRP[m]
                (mFS, mVS, mPS) = self._preProcessFacesAndVoids(obj, base, m, FACES, VOIDS)
                fShapes[m] = mFS
                vShapes[m] = mVS
                self.profileShapes[m] = mPS
        else:
            PathLog.debug(' -No obj.Base data.')
            for m in range(0, lenGRP):
                self.modelSTLs[m] = True

        # Process each model base, as a whole, as needed
        # PathLog.debug(' -Pre-processing all models in Job.')
        for m in range(0, lenGRP):
            if fShapes[m] is False:
                PathLog.debug(' -Pre-processing {} as a whole.'.format(GRP[m].Label))
                if obj.BoundBox == 'BaseBoundBox':
                    base = GRP[m]
                elif obj.BoundBox == 'Stock':
                    base = JOB.Stock

                pPEB = self._preProcessEntireBase(obj, base, m)
                if pPEB is False:
                    PathLog.error(' -Failed to pre-process base as a whole.')
                else:
                    (fcShp, prflShp) = pPEB
                    if fcShp is not False:
                        if fcShp is True:
                            PathLog.debug(' -fcShp is True.')
                            fShapes[m] = True
                        else:
                            fShapes[m] = [fcShp]
                    if prflShp is not False:
                        if fcShp is not False:
                            PathLog.debug('vShapes[{}]: {}'.format(m, vShapes[m]))
                            if vShapes[m] is not False:
                                PathLog.debug(' -Cutting void from base profile shape.')
                                adjPS = prflShp.cut(vShapes[m][0])
                                self.profileShapes[m] = [adjPS]
                            else:
                                PathLog.debug(' -vShapes[m] is False.')
                                self.profileShapes[m] = [prflShp]
                        else:
                            PathLog.debug(' -Saving base profile shape.')
                            self.profileShapes[m] = [prflShp]
                        PathLog.debug('self.profileShapes[{}]: {}'.format(m, self.profileShapes[m]))
        # Efor

        return (fShapes, vShapes)

    def _identifyFacesAndVoids(self, JOB, obj, F, V):
        TUPS = list()
        GRP = JOB.Model.Group
        lenGRP = len(GRP)

        # Separate selected faces into (base, face) tuples and flag model(s) for STL creation
        for (bs, SBS) in obj.Base:
            for sb in SBS:
                # Flag model for STL creation
                mdlIdx = None
                for m in range(0, lenGRP):
                    if bs is GRP[m]:
                        self.modelSTLs[m] = True
                        mdlIdx = m
                        break
                TUPS.append((mdlIdx, bs, sb))  # (model idx, base, sub)

        # Apply `AvoidXFaces` value
        faceCnt = len(TUPS)
        add = faceCnt - obj.AvoidLastX_Faces
        for bst in range(0, faceCnt):
            (m, base, sub) = TUPS[bst]
            shape = getattr(base.Shape, sub)
            if isinstance(shape, Part.Face):
                faceIdx = int(sub[4:]) - 1
                if bst < add:
                    if F[m] is False:
                        F[m] = list()
                    F[m].append((shape, faceIdx))
                else:
                    if V[m] is False:
                        V[m] = list()
                    V[m].append((shape, faceIdx))
        return (F, V)

    def _preProcessFacesAndVoids(self, obj, base, m, FACES, VOIDS):
        mFS = False
        mVS = False
        mPS = False
        mIFS = list()
        BB = base.Shape.BoundBox

        if FACES[m] is not False:
            isHole = False
            if obj.HandleMultipleFeatures == 'Collectively':
                cont = True
                fsL = list()  # face shape list
                ifL = list()  # avoid shape list
                outFCS = list()

                # Get collective envelope slice of selected faces
                for (fcshp, fcIdx) in FACES[m]:
                    fNum = fcIdx + 1
                    fsL.append(fcshp)
                    gFW = self._getFaceWires(base, fcshp, fcIdx)
                    if gFW is False:
                        PathLog.debug('Failed to get wires from Face{}'.format(fNum))
                    elif gFW[0] is False:
                        PathLog.debug('Cannot process Face{}. Check that it has horizontal surface exposure.'.format(fNum))
                    else:
                        ((otrFace, raised), intWires) = gFW
                        outFCS.append(otrFace)
                        if obj.InternalFeaturesCut is False:
                            if intWires is not False:
                                for (iFace, rsd) in intWires:
                                    ifL.append(iFace)

                PathLog.debug('Attempting to get cross-section of collective faces.')
                if len(outFCS) == 0:
                    PathLog.error('Cannot process selected faces. Check horizontal surface exposure.'.format(fNum))
                    cont = False
                else:
                    cfsL = Part.makeCompound(outFCS)

                # Handle profile edges request
                if cont is True and obj.ProfileEdges != 'None':
                    ofstVal = self._calculateOffsetValue(obj, isHole)
                    psOfst = self._extractFaceOffset(obj, cfsL, ofstVal)
                    if psOfst is not False:
                        mPS = [psOfst]
                        if obj.ProfileEdges == 'Only':
                            mFS = True
                            cont = False
                    else:
                        PathLog.error(' -Failed to create profile geometry for selected faces.')
                        cont = False

                if cont is True:
                    if self.showDebugObjects is True:
                        T = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpCollectiveShape')
                        T.Shape = cfsL
                        T.purgeTouched()
                        self.tempGroup.addObject(T)

                    ofstVal = self._calculateOffsetValue(obj, isHole)
                    faceOfstShp = self._extractFaceOffset(obj, cfsL, ofstVal)
                    if faceOfstShp is False:
                        PathLog.error(' -Failed to create offset face.')
                        cont = False

                if cont is True:
                    lenIfL = len(ifL)
                    if obj.InternalFeaturesCut is False:
                        if lenIfL == 0:
                            PathLog.debug(' -No internal features saved.')
                        else:
                            if lenIfL == 1:
                                casL = ifL[0]
                            else:
                                casL = Part.makeCompound(ifL)
                            if self.showDebugObjects is True:
                                C = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpCompoundIntFeat')
                                C.Shape = casL
                                C.purgeTouched()
                                self.tempGroup.addObject(C)
                            ofstVal = self._calculateOffsetValue(obj, isHole=True)
                            intOfstShp = self._extractFaceOffset(obj, casL, ofstVal)
                            mIFS.append(intOfstShp)
                            # faceOfstShp = faceOfstShp.cut(intOfstShp)

                    mFS = [faceOfstShp]
                # Eif

            elif obj.HandleMultipleFeatures == 'Individually':
                for (fcshp, fcIdx) in FACES[m]:
                    cont = True
                    fsL = list()  # face shape list
                    ifL = list()  # avoid shape list
                    fNum = fcIdx + 1
                    outerFace = False

                    gFW = self._getFaceWires(base, fcshp, fcIdx)
                    if gFW is False:
                        PathLog.debug('Failed to get wires from Face{}'.format(fNum))
                        cont = False
                    elif gFW[0] is False:
                        PathLog.debug('Cannot process Face{}. Check that it has horizontal surface exposure.'.format(fNum))
                        cont = False
                        outerFace = False
                    else:
                        ((otrFace, raised), intWires) = gFW
                        outerFace = otrFace
                        if obj.InternalFeaturesCut is False:
                            if intWires is not False:
                                for (iFace, rsd) in intWires:
                                    ifL.append(iFace)

                    if outerFace is not False:
                        PathLog.debug('Attempting to create offset face of Face{}'.format(fNum))

                        if obj.ProfileEdges != 'None':
                            ofstVal = self._calculateOffsetValue(obj, isHole)
                            psOfst = self._extractFaceOffset(obj, outerFace, ofstVal)
                            if psOfst is not False:
                                if mPS is False:
                                    mPS = list()
                                mPS.append(psOfst)
                                if obj.ProfileEdges == 'Only':
                                    if mFS is False:
                                        mFS = list()
                                    mFS.append(True)
                                    cont = False
                            else:
                                PathLog.error(' -Failed to create profile geometry for Face{}.'.format(fNum))
                                cont = False

                        if cont is True:
                            ofstVal = self._calculateOffsetValue(obj, isHole)
                            faceOfstShp = self._extractFaceOffset(obj, slc, ofstVal)

                            lenIfl = len(ifL)
                            if obj.InternalFeaturesCut is False and lenIfl > 0:
                                if lenIfl == 1:
                                    casL = ifL[0]
                                else:
                                    casL = Part.makeCompound(ifL)

                                ofstVal = self._calculateOffsetValue(obj, isHole=True)
                                intOfstShp = self._extractFaceOffset(obj, casL, ofstVal)
                                mIFS.append(intOfstShp)
                                # faceOfstShp = faceOfstShp.cut(intOfstShp)

                            if mFS is False:
                                mFS = list()
                            mFS.append(faceOfstShp)
                    # Eif
                # Efor
            # Eif
        # Eif

        if len(mIFS) > 0:
            if mVS is False:
                mVS = list()
            for ifs in mIFS:
                mVS.append(ifs)

        if VOIDS[m] is not False:
            PathLog.debug('Processing avoid faces.')
            cont = True
            isHole = False
            outFCS = list()
            intFEAT = list()

            for (fcshp, fcIdx) in VOIDS[m]:
                fNum = fcIdx + 1
                gFW = self._getFaceWires(base, fcshp, fcIdx)
                if gFW is False:
                    PathLog.debug('Failed to get wires from avoid Face{}'.format(fNum))
                    cont = False
                else:
                    ((otrFace, raised), intWires) = gFW
                    outFCS.append(otrFace)
                    if obj.AvoidLastX_InternalFeatures is False:
                        if intWires is not False:
                            for (iFace, rsd) in intWires:
                                intFEAT.append(iFace)

            lenOtFcs = len(outFCS)
            if lenOtFcs == 0:
                cont = False
            else:
                if lenOtFcs == 1:
                    avoid = outFCS[0]
                else:
                    avoid = Part.makeCompound(outFCS)

                if self.showDebugObjects is True:
                    PathLog.debug('*** tmpAvoidArea')
                    P = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpVoidEnvelope')
                    P.Shape = avoid
                    # P.recompute()
                    P.purgeTouched()
                    self.tempGroup.addObject(P)

            if cont is True:
                if self.showDebugObjects is True:
                    PathLog.debug('*** tmpVoidCompound')
                    P = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpVoidCompound')
                    P.Shape = avoid
                    # P.recompute()
                    P.purgeTouched()
                    self.tempGroup.addObject(P)
                ofstVal = self._calculateOffsetValue(obj, isHole, isVoid=True)
                avdOfstShp = self._extractFaceOffset(obj, avoid, ofstVal)
                if avdOfstShp is False:
                    PathLog.error('Failed to create collective offset avoid face.')
                    cont = False

            if cont is True:
                avdShp = avdOfstShp

                if obj.AvoidLastX_InternalFeatures is False and len(intFEAT) > 0:
                    if len(intFEAT) > 1:
                        ifc = Part.makeCompound(intFEAT)
                    else:
                        ifc = intFEAT[0]
                    ofstVal = self._calculateOffsetValue(obj, isHole=True)
                    ifOfstShp = self._extractFaceOffset(obj, ifc, ofstVal)
                    if ifOfstShp is False:
                        PathLog.error('Failed to create collective offset avoid internal features.')
                    else:
                        avdShp = avdOfstShp.cut(ifOfstShp)

                if mVS is False:
                    mVS = list()
                mVS.append(avdShp)


        return (mFS, mVS, mPS)

    def _getFaceWires(self, base, fcshp, fcIdx):
        outFace = False
        INTFCS = list()
        fNum = fcIdx + 1
        # preProcEr = translate('PathSurface', 'Error pre-processing Face')
        warnFinDep = translate('PathSurface', 'Final Depth might need to be lower. Internal features detected in Face')

        PathLog.debug('_getFaceWires() from Face{}'.format(fNum))
        WIRES = self._extractWiresFromFace(base, fcshp)
        if WIRES is False:
            PathLog.error('Failed to extract wires from Face{}'.format(fNum))
            return False

        # Process remaining internal features, adding to FCS list
        lenW = len(WIRES)
        for w in range(0, lenW):
            (wire, rsd) = WIRES[w]
            PathLog.debug('Processing Wire{} in Face{}.   isRaised: {}'.format(w + 1, fNum, rsd))
            if wire.isClosed() is False:
                PathLog.debug(' -wire is not closed.')
            else:
                slc = self._flattenWireToFace(wire)
                if slc is False:
                    PathLog.error('FAILED to identify horizontal exposure on Face{}.'.format(fNum))
                else:
                    if w == 0:
                        outFace = (slc, rsd)
                    else:
                        # add to VOIDS so cutter avoids area.
                        PathLog.warning(warnFinDep + str(fNum) + '.')
                        INTFCS.append((slc, rsd))
        if len(INTFCS) == 0:
            return (outFace, False)
        else:
            return (outFace, INTFCS)

    def _preProcessEntireBase(self, obj, base, m):
        cont = True
        isHole = False
        prflShp = False
        # Create envelope, extract cross-section and make offset co-planar shape
        # baseEnv = PathUtils.getEnvelope(base.Shape, subshape=None, depthparams=self.depthParams)

        try:
            baseEnv = PathUtils.getEnvelope(partshape=base.Shape, subshape=None, depthparams=self.depthParams)  # Produces .Shape
        except Exception as ee:
            PathLog.error(str(ee))
            shell = base.Shape.Shells[0]
            solid = Part.makeSolid(shell)
            try:
                baseEnv = PathUtils.getEnvelope(partshape=solid, subshape=None, depthparams=self.depthParams)  # Produces .Shape
            except Exception as eee:
                PathLog.error(str(eee))
                cont = False
        time.sleep(0.2)

        if cont is True:
            csFaceShape = self._getShapeSlice(baseEnv)
            if csFaceShape is False:
                PathLog.debug('_getShapeSlice(baseEnv) failed')
                csFaceShape = self._getCrossSection(baseEnv)
                if csFaceShape is False:
                    PathLog.debug('_getCrossSection(baseEnv) failed')
                    csFaceShape = self._getSliceFromEnvelope(baseEnv)
            if csFaceShape is False:
                PathLog.error('Failed to slice baseEnv shape.')
                cont = False

        if cont is True and obj.ProfileEdges != 'None':
            PathLog.debug(' -Attempting profile geometry for model base.')
            ofstVal = self._calculateOffsetValue(obj, isHole)
            psOfst = self._extractFaceOffset(obj, csFaceShape, ofstVal)
            if psOfst is not False:
                if obj.ProfileEdges == 'Only':
                    return (True, psOfst)
                prflShp = psOfst
            else:
                PathLog.error(' -Failed to create profile geometry.')
                cont = False

        if cont is True:
            ofstVal = self._calculateOffsetValue(obj, isHole)
            faceOffsetShape = self._extractFaceOffset(obj, csFaceShape, ofstVal)
            if faceOffsetShape is False:
                PathLog.error('_extractFaceOffset() failed.')
            else:
                faceOffsetShape.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - faceOffsetShape.BoundBox.ZMin))
                return (faceOffsetShape, prflShp)
        return False

    def _extractWiresFromFace(self, base, fc):
        '''_extractWiresFromFace(base, fc) ... 
        Attempts to return all closed wires within a parent face, including the outer most wire of the parent.
        The wires are ordered by area. Each wire is also categorized as a pocket(False) or raised protrusion(True).
        '''
        PathLog.debug('_extractWiresFromFace()')

        WIRES = list()
        lenWrs = len(fc.Wires)
        PathLog.debug(' -Wire count: {}'.format(lenWrs))

        def index0(tup):
            return tup[0]

        # Cycle through wires in face
        for w in range(0, lenWrs):
            PathLog.debug(' -Analyzing wire_{}'.format(w + 1))
            wire = fc.Wires[w]
            checkEdges = False
            cont = True

            # Check for closed edges (circles, ellipses, etc...)
            for E in wire.Edges:
                if E.isClosed() is True:
                    checkEdges = True
                    break

            if checkEdges is True:
                PathLog.debug(' -checkEdges is True')
                for e in range(0, len(wire.Edges)):
                    edge = wire.Edges[e]
                    if edge.isClosed() is True and edge.Mass > 0.01:
                        PathLog.debug(' -Found closed edge')
                        raised = False
                        ip = self._isPocket(base, fc, edge)
                        if ip is False:
                            raised = True
                        ebb = edge.BoundBox
                        eArea = ebb.XLength * ebb.YLength
                        F = Part.Face(Part.Wire([edge]))
                        WIRES.append((eArea, F.Wires[0], raised))
                        cont = False

            if cont is True:
                PathLog.debug(' -cont is True')
                # If only one wire and not checkEdges, return first wire
                if lenWrs == 1:
                    return [(wire, False)]
                    
                raised = False
                wbb = wire.BoundBox
                wArea = wbb.XLength * wbb.YLength
                if w > 0:
                    ip = self._isPocket(base, fc, wire)
                    if ip is False:
                        raised = True
                WIRES.append((wArea, Part.Wire(wire.Edges), raised))

        nf = len(WIRES)
        if nf > 0:
            PathLog.debug(' -number of wires found is {}'.format(nf))
            if nf == 1:
                (area, W, raised) = WIRES[0]
                return [(W, raised)]
            else:
                sortedWIRES = sorted(WIRES, key=index0, reverse=True)
                return [(W, raised) for (area, W, raised) in sortedWIRES]  # outer, then inner by area size
        
        return False

    def _calculateOffsetValue(self, obj, isHole, isVoid=False):
        '''_calculateOffsetValue(obj, isHole, isVoid) ... internal function.
        Calculate the offset for the Path.Area() function.'''
        JOB = PathUtils.findParentJob(obj)
        tolrnc = JOB.GeometryTolerance.Value

        if isVoid is False:
            if isHole is True:
                offset = -1 * obj.InternalFeaturesAdjustment.Value
                offset += self.radius  # (self.radius + (tolrnc / 10.0))
            else:
                offset = -1 * obj.BoundaryAdjustment.Value
                if obj.BoundaryEnforcement is True:
                    offset += self.radius  # (self.radius + (tolrnc / 10.0))
                else:
                    offset -= self.radius  # (self.radius + (tolrnc / 10.0))
                offset = 0.0 - offset
        else:
            offset = -1 * obj.BoundaryAdjustment.Value
            offset += self.radius  # (self.radius + (tolrnc / 10.0))

        return offset

    def _extractFaceOffset(self, obj, fcShape, offset):
        '''_extractFaceOffset(fcShape, offset) ... internal function.
            Original _buildPathArea() version copied from PathAreaOp.py module.  This version is modified.
            Adjustments made based on notes by @sliptonic at this webpage: https://github.com/sliptonic/FreeCAD/wiki/PathArea-notes.'''
        PathLog.debug('_extractFaceOffset()')

        if fcShape.BoundBox.ZMin != 0.0:
            fcShape.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - fcShape.BoundBox.ZMin))

        areaParams = {}
        areaParams['Offset'] = offset
        areaParams['Fill'] = 1
        areaParams['Coplanar'] = 0
        areaParams['SectionCount'] = 1  # -1 = full(all per depthparams??) sections
        areaParams['Reorient'] = True
        areaParams['OpenMode'] = 0
        areaParams['MaxArcPoints'] = 400  # 400
        areaParams['Project'] = True

        area = Path.Area()  # Create instance of Area() class object
        # area.setPlane(PathUtils.makeWorkplane(fcShape))  # Set working plane
        area.setPlane(PathUtils.makeWorkplane(self.wpc))  # Set working plane to normal at Z=1
        area.add(fcShape)
        area.setParams(**areaParams)  # set parameters

        # Save parameters for debugging
        # obj.AreaParams = str(area.getParams())
        # PathLog.debug("Area with params: {}".format(area.getParams()))

        offsetShape = area.getShape()
        wCnt = len(offsetShape.Wires)
        if wCnt == 0:
            return False
        elif wCnt == 1:
            ofstFace = Part.Face(offsetShape.Wires[0])
        else:
            W = list()
            for wr in offsetShape.Wires:
                W.append(Part.Face(wr))
            ofstFace = Part.makeCompound(W)

        return ofstFace  # offsetShape

    def _isPocket(self, b, f, w):
        '''_isPocket(b, f, w)... 
        Attempts to determing if the wire(w) in face(f) of base(b) is a pocket or raised protrusion.
        Returns True if pocket, False if raised protrusion.'''
        e = w.Edges[0]
        for fi in range(0, len(b.Shape.Faces)):
            face = b.Shape.Faces[fi]
            for ei in range(0, len(face.Edges)):
                edge = face.Edges[ei]
                if e.isSame(edge) is True:
                    if f is face:
                        # Alternative: run loop to see if all edges are same
                        pass  # same source face, look for another
                    else:
                        if face.CenterOfMass.z < f.CenterOfMass.z:
                            return True
        return False

    def _flattenWireToFace(self, wire):
        PathLog.debug('_flattenWireToFace()')
        if wire.isClosed() is False:
            PathLog.debug(' -wire.isClosed() is False')
            return False

        # If wire is planar horizontal, convert to a face and return
        if wire.BoundBox.ZLength == 0.0:
            slc = Part.Face(wire)
            return slc

        # Attempt to create a new wire for manipulation, if not, use original
        newWire = Part.Wire(wire.Edges)
        if newWire.isClosed() is True:
            nWire = newWire
        else:
            PathLog.debug(' -newWire.isClosed() is False')
            nWire = wire

        # Attempt extrusion, and then try a manual slice and then cross-section
        ext = self._getExtrudedShape(nWire)
        if ext is False:
            PathLog.debug('_getExtrudedShape() failed')
        else:
            slc = self._getShapeSlice(ext)
            if slc is not False:
                return slc
            cs = self._getCrossSection(ext, True)
            if cs is not False:
                return cs

        # Attempt creating an envelope, and then try a manual slice and then cross-section
        env = self._getShapeEnvelope(nWire)
        if env is False:
            PathLog.debug('_getShapeEnvelope() failed')
        else:
            slc = self._getShapeSlice(env)
            if slc is not False:
                return slc
            cs = self._getCrossSection(env, True)
            if cs is not False:
                return cs

        # Attempt creating a projection
        slc = self._getProjectedFace(nWire)
        if slc is False:
            PathLog.debug('_getProjectedFace() failed')
        else:
            return slc

        return False

    def _getExtrudedShape(self, wire):
        PathLog.debug('_getExtrudedShape()')
        wBB = wire.BoundBox
        extFwd = math.floor(2.0 * wBB.ZLength) + 10.0

        try:
            # slower, but renders collective faces correctly. Method 5 in TESTING
            shell = wire.extrude(FreeCAD.Vector(0.0, 0.0, extFwd))
        except Exception as ee:
            PathLog.error(' -extrude wire failed: \n{}'.format(ee))
            return False

        SHP = Part.makeSolid(shell)
        return SHP

    def _getShapeSlice(self, shape):
        PathLog.debug('_getShapeSlice()')

        bb = shape.BoundBox
        mid = (bb.ZMin + bb.ZMax) / 2.0
        xmin = bb.XMin - 1.0
        xmax = bb.XMax + 1.0
        ymin = bb.YMin - 1.0
        ymax = bb.YMax + 1.0
        p1 = FreeCAD.Vector(xmin, ymin, mid)
        p2 = FreeCAD.Vector(xmax, ymin, mid)
        p3 = FreeCAD.Vector(xmax, ymax, mid)
        p4 = FreeCAD.Vector(xmin, ymax, mid)

        e1 = Part.makeLine(p1, p2)
        e2 = Part.makeLine(p2, p3)
        e3 = Part.makeLine(p3, p4)
        e4 = Part.makeLine(p4, p1)
        face = Part.Face(Part.Wire([e1, e2, e3, e4]))
        fArea = face.BoundBox.XLength * face.BoundBox.YLength  # face.Wires[0].Area
        sArea = shape.BoundBox.XLength * shape.BoundBox.YLength
        midArea = (fArea + sArea) / 2.0

        slcShp = shape.common(face)
        slcArea = slcShp.BoundBox.XLength * slcShp.BoundBox.YLength

        if slcArea < midArea:
            for W in slcShp.Wires:
                if W.isClosed() is False:
                    PathLog.debug(' -wire.isClosed() is False')
                    return False
            if len(slcShp.Wires) == 1:
                wire = slcShp.Wires[0]
                slc = Part.Face(wire)
                slc.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - slc.BoundBox.ZMin))
                return slc
            else:
                fL = list()
                for W in slcShp.Wires:
                    slc = Part.Face(W)
                    slc.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - slc.BoundBox.ZMin))
                    fL.append(slc)
                comp = Part.makeCompound(fL)
                if self.showDebugObjects is True:
                    PathLog.debug('*** tmpSliceCompound')
                    P = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpSliceCompound')
                    P.Shape = comp
                    # P.recompute()
                    P.purgeTouched()
                    self.tempGroup.addObject(P)
                return comp

        PathLog.debug(' -slcArea !< midArea')
        PathLog.debug(' -slcShp.Edges count: {}.  Might be a vertically oriented face.'.format(len(slcShp.Edges)))
        return False

    def _getProjectedFace(self, wire):
        PathLog.debug('_getProjectedFace()')
        F = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpProjectionWire')
        F.Shape = wire
        F.purgeTouched()
        self.tempGroup.addObject(F)
        try:
            prj = Draft.makeShape2DView(F, FreeCAD.Vector(0, 0, 1))
            prj.recompute()
            prj.purgeTouched()
            self.tempGroup.addObject(prj)
        except Exception as ee:
            PathLog.error(str(ee))
            return False
        else:
            pWire = Part.Wire(prj.Shape.Edges)
            if pWire.isClosed() is False:
                # PathLog.debug(' -pWire.isClosed() is False')
                return False
            slc = Part.Face(pWire)
            slc.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - slc.BoundBox.ZMin))
            return slc
        return False

    def _getCrossSection(self, shape, withExtrude=False):
        PathLog.debug('_getCrossSection()')
        wires = list()
        bb = shape.BoundBox
        mid = (bb.ZMin + bb.ZMax) / 2.0

        for i in shape.slice(FreeCAD.Vector(0, 0, 1), mid):
            wires.append(i)

        if len(wires) > 0:
            comp = Part.Compound(wires)  # produces correct cross-section wire !
            comp.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - comp.BoundBox.ZMin))
            csWire = comp.Wires[0]
            if csWire.isClosed() is False:
                PathLog.debug(' -comp.Wires[0] is not closed')
                return False
            if withExtrude is True:
                ext = self._getExtrudedShape(csWire)
                CS = self._getShapeSlice(ext)
            else:
                CS = Part.Face(csWire)
            CS.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - CS.BoundBox.ZMin))
            return CS
        else:
            PathLog.debug(' -No wires from .slice() method')

        return False

    def _getShapeEnvelope(self, shape):
        PathLog.debug('_getShapeEnvelope()')

        wBB = shape.BoundBox
        extFwd = wBB.ZLength + 10.0
        minz = wBB.ZMin
        maxz = wBB.ZMin + extFwd
        stpDwn = (maxz - minz) / 4.0
        dep_par = PathUtils.depth_params(maxz + 5.0, maxz + 3.0, maxz, stpDwn, 0.0, minz)

        try:
            env = PathUtils.getEnvelope(partshape=shape, depthparams=dep_par)  # Produces .Shape
        except Exception as ee:
            PathLog.error('try: PathUtils.getEnvelope() failed.\n' + str(ee))
            return False
        else:
            return env

        return False

    def _getSliceFromEnvelope(self, env):
        PathLog.debug('_getSliceFromEnvelope()')
        eBB = env.BoundBox
        extFwd = eBB.ZLength + 10.0
        maxz = eBB.ZMin + extFwd

        maxMax = env.Edges[0].BoundBox.ZMin
        emax = math.floor(maxz - 1.0)
        E = list()
        for e in range(0, len(env.Edges)):
            emin = env.Edges[e].BoundBox.ZMin
            if emin > emax:
                E.append(env.Edges[e])
        tf = Part.Face(Part.Wire(Part.__sortEdges__(E)))
        tf.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - tf.BoundBox.ZMin))

        return tf

    def _prepareModelSTLs(self, JOB, obj):
        PathLog.debug('_prepareModelSTLs()')
        for m in range(0, len(JOB.Model.Group)):
            M = JOB.Model.Group[m]

            # PathLog.debug(f" -self.modelTypes[{m}] == 'M'")
            if self.modelTypes[m] == 'M':
                mesh = M.Mesh
            else:
                # base.Shape.tessellate(0.05) # 0.5 original value
                # mesh = MeshPart.meshFromShape(base.Shape, Deflection=self.deflection)
                mesh = MeshPart.meshFromShape(Shape=M.Shape, LinearDeflection=self.deflection, AngularDeflection=self.angularDeflection, Relative=False)

            if self.modelSTLs[m] is True:
                stl = ocl.STLSurf()

                for f in mesh.Facets:
                    p = f.Points[0]
                    q = f.Points[1]
                    r = f.Points[2]
                    t = ocl.Triangle(ocl.Point(p[0], p[1], p[2] + obj.DepthOffset.Value),
                                        ocl.Point(q[0], q[1], q[2] + obj.DepthOffset.Value),
                                        ocl.Point(r[0], r[1], r[2] + obj.DepthOffset.Value))
                    stl.addTriangle(t)
                self.modelSTLs[m] = stl
        return

    def _makeSafeSTL(self, JOB, obj, mdlIdx, faceShapes, voidShapes):
        '''_makeSafeSTL(JOB, obj, mdlIdx, faceShapes, voidShapes)...
        Creates and OCL.stl object with combined data with waste stock,
        model, and avoided faces.  Travel lines can be checked against this 
        STL object to determine minimum travel height to clear stock and model.'''
        PathLog.debug('_makeSafeSTL()')

        fuseShapes = list()
        Mdl = JOB.Model.Group[mdlIdx]
        FCAD = FreeCAD.ActiveDocument
        mBB = Mdl.Shape.BoundBox
        sBB = JOB.Stock.Shape.BoundBox

        # add Model shape to safeSTL shape
        fuseShapes.append(Mdl.Shape)

        if obj.BoundBox == 'BaseBoundBox':
            cont = False
            extFwd = (sBB.ZLength)
            zmin = mBB.ZMin
            zmax = mBB.ZMin + extFwd
            stpDwn = (zmax - zmin) / 4.0
            dep_par = PathUtils.depth_params(zmax + 5.0, zmax + 3.0, zmax, stpDwn, 0.0, zmin)
            
            try:
                envBB = PathUtils.getEnvelope(partshape=Mdl.Shape, depthparams=dep_par)  # Produces .Shape
                cont = True
            except Exception as ee:
                PathLog.error(str(ee))
                shell = Mdl.Shape.Shells[0]
                solid = Part.makeSolid(shell)
                try:
                    envBB = PathUtils.getEnvelope(partshape=solid, depthparams=dep_par)  # Produces .Shape
                    cont = True
                except Exception as eee:
                    PathLog.error(str(eee))

            if cont is True:
                stckWst = JOB.Stock.Shape.cut(envBB)
                if obj.BoundaryAdjustment > 0.0:
                    cmpndFS = Part.makeCompound(faceShapes)
                    baBB = PathUtils.getEnvelope(partshape=cmpndFS, depthparams=self.depthParams)  # Produces .Shape
                    adjStckWst = stckWst.cut(baBB)
                else:
                    adjStckWst = stckWst
                fuseShapes.append(adjStckWst)
            else:
                PathLog.warning('Path transitions might not avoid the model. Verify paths.')
            time.sleep(0.3)

        else:
            # If boundbox is Job.Stock, add hidden pad under stock as base plate
            toolDiam = self.cutter.getDiameter()
            zMin = JOB.Stock.Shape.BoundBox.ZMin
            xMin = JOB.Stock.Shape.BoundBox.XMin - toolDiam
            yMin = JOB.Stock.Shape.BoundBox.YMin - toolDiam
            bL = JOB.Stock.Shape.BoundBox.XLength + (2 * toolDiam)
            bW = JOB.Stock.Shape.BoundBox.YLength + (2 * toolDiam)
            bH = 1.0
            crnr = FreeCAD.Vector(xMin, yMin, zMin - 1.0)
            B = Part.makeBox(bL, bW, bH, crnr, FreeCAD.Vector(0, 0, 1))
            fuseShapes.append(B)

        if voidShapes is not False:
            voidComp = Part.makeCompound(voidShapes)
            voidEnv = PathUtils.getEnvelope(partshape=voidComp, depthparams=self.depthParams)  # Produces .Shape
            fuseShapes.append(voidEnv)

        f0 = fuseShapes.pop(0)
        if len(fuseShapes) > 0:
            fused = f0.fuse(fuseShapes)
        else:
            fused = f0

        if self.showDebugObjects is True:
            T = FreeCAD.ActiveDocument.addObject('Part::Feature', 'safeSTLShape')
            T.Shape = fused
            T.purgeTouched()
            self.tempGroup.addObject(T)

        # Extract mesh from fusion
        meshFuse = MeshPart.meshFromShape(Shape=fused, LinearDeflection=(self.deflection / 2.0), AngularDeflection=self.angularDeflection, Relative=False)
        time.sleep(0.2)
        stl = ocl.STLSurf()
        for f in meshFuse.Facets:
            p = f.Points[0]
            q = f.Points[1]
            r = f.Points[2]
            t = ocl.Triangle(ocl.Point(p[0], p[1], p[2]),
                             ocl.Point(q[0], q[1], q[2]),
                             ocl.Point(r[0], r[1], r[2]))
            stl.addTriangle(t)

        self.safeSTLs[mdlIdx] = stl

    def _processCutAreas(self, JOB, obj, mdlIdx, FCS, VDS):
        '''_processCutAreas(JOB, obj, mdlIdx, FCS, VDS)...
        This method applies any avoided faces or regions to the selected faces.
        It then calls the correct scan method depending on the ScanType property.'''
        PathLog.debug('_processCutAreas()')

        final = list()
        base = JOB.Model.Group[mdlIdx]

        # Process faces Collectively or Individually
        if obj.HandleMultipleFeatures == 'Collectively':
            if FCS is True:
                COMP = False
            else:
                ADD = Part.makeCompound(FCS)
                if VDS is not False:
                    DEL = Part.makeCompound(VDS)
                    COMP = ADD.cut(DEL)
                else:
                    COMP = ADD

            final.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
            final.extend(self._waterlineOp(JOB, obj, mdlIdx, COMP))  # independent method set for Waterline

        elif obj.HandleMultipleFeatures == 'Individually':
            for fsi in range(0, len(FCS)):
                fShp = FCS[fsi]
                # self.deleteOpVariables(all=False)
                self.resetOpVariables(all=False)

                if fShp is True:
                    COMP = False
                else:
                    ADD = Part.makeCompound([fShp])
                    if VDS is not False:
                        DEL = Part.makeCompound(VDS)
                        COMP = ADD.cut(DEL)
                    else:
                        COMP = ADD

                final.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
                final.extend(self._waterlineOp(JOB, obj, mdlIdx, COMP))  # independent method set for Waterline
                COMP = None
        # Eif

        return final

    def _planarGetPDC(self, stl, finalDep, SampleInterval, useSafeCutter=False):
        pdc = ocl.PathDropCutter()   # create a pdc [PathDropCutter] object
        pdc.setSTL(stl)  # add stl model
        if useSafeCutter is True:
            pdc.setCutter(self.safeCutter)  # add safeCutter
        else:
            pdc.setCutter(self.cutter)  # add cutter
        pdc.setZ(finalDep)  # set minimumZ (final / target depth value)
        pdc.setSampling(SampleInterval)  # set sampling size
        return pdc

    # Main waterline functions
    def _waterlineOp(self, JOB, obj, mdlIdx, subShp=None):
        '''_waterlineOp(obj, base) ... Main waterline function to perform waterline extraction from model.'''
        commands = []

        t_begin = time.time()
        # JOB = PathUtils.findParentJob(obj)
        base = JOB.Model.Group[mdlIdx]
        bb = self.boundBoxes[mdlIdx]
        stl = self.modelSTLs[mdlIdx]

        # Prepare global holdpoint and layerEndPnt containers
        if self.holdPoint is None:
            self.holdPoint = ocl.Point(float("inf"), float("inf"), float("inf"))
        if self.layerEndPnt is None:
            self.layerEndPnt = ocl.Point(float("inf"), float("inf"), float("inf"))

        # Set extra offset to diameter of cutter to allow cutter to move around perimeter of model
        toolDiam = self.cutter.getDiameter()
        cdeoX = 0.6 * toolDiam
        cdeoY = 0.6 * toolDiam

        if subShp is None:
            # Get correct boundbox
            if obj.BoundBox == 'Stock':
                BS = JOB.Stock
                bb = BS.Shape.BoundBox
            elif obj.BoundBox == 'BaseBoundBox':
                BS = base
                bb = base.Shape.BoundBox

            env = PathUtils.getEnvelope(partshape=BS.Shape, depthparams=self.depthParams)  # Produces .Shape

            xmin = bb.XMin
            xmax = bb.XMax
            ymin = bb.YMin
            ymax = bb.YMax
            zmin = bb.ZMin
            zmax = bb.ZMax
        else:
            xmin = subShp.BoundBox.XMin
            xmax = subShp.BoundBox.XMax
            ymin = subShp.BoundBox.YMin
            ymax = subShp.BoundBox.YMax
            zmin = subShp.BoundBox.ZMin
            zmax = subShp.BoundBox.ZMax

        smplInt = obj.SampleInterval.Value
        minSampInt = 0.001  # value is mm
        if smplInt < minSampInt:
            smplInt = minSampInt

        # Determine bounding box length for the OCL scan
        bbLength = math.fabs(ymax - ymin)
        numScanLines = int(math.ceil(bbLength / smplInt) + 1)  # Number of lines

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == 'Single-pass':
            depthparams = [obj.FinalDepth.Value]
        else:
            depthparams = [dp for dp in self.depthParams]
        lenDP = len(depthparams)

        # Prepare PathDropCutter objects with STL data
        safePDC = self._planarGetPDC(self.safeSTLs[mdlIdx],
                                    depthparams[lenDP - 1], obj.SampleInterval.Value, useSafeCutter=False)

        # Scan the piece to depth at smplInt
        oclScan = []
        oclScan = self._waterlineDropCutScan(stl, smplInt, xmin, xmax, ymin, depthparams[lenDP - 1], numScanLines)
        # oclScan = SCANS
        lenOS = len(oclScan)
        ptPrLn = int(lenOS / numScanLines)

        # Convert oclScan list of points to multi-dimensional list
        scanLines = []
        for L in range(0, numScanLines):
            scanLines.append([])
            for P in range(0, ptPrLn):
                pi = L * ptPrLn + P
                scanLines[L].append(oclScan[pi])
        lenSL = len(scanLines)
        pntsPerLine = len(scanLines[0])
        PathLog.debug("--OCL scan: " + str(lenSL * pntsPerLine) + " points, with " + str(numScanLines) + " lines and " + str(pntsPerLine) + " pts/line")

        # Extract Wl layers per depthparams
        lyr = 0
        cmds = []
        layTime = time.time()
        self.topoMap = []
        for layDep in depthparams:
            cmds = self._getWaterline(obj, scanLines, layDep, lyr, lenSL, pntsPerLine)
            commands.extend(cmds)
            lyr += 1
        PathLog.debug("--All layer scans combined took " + str(time.time() - layTime) + " s")
        return commands

    def _waterlineDropCutScan(self, stl, smplInt, xmin, xmax, ymin, fd, numScanLines):
        '''_waterlineDropCutScan(stl, smplInt, xmin, xmax, ymin, fd, numScanLines) ... 
        Perform OCL scan for waterline purpose.'''
        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(stl)
        pdc.setCutter(self.cutter)
        pdc.setZ(fd)  # set minimumZ (final / target depth value)
        pdc.setSampling(smplInt)

        # Create line object as path
        path = ocl.Path()                   # create an empty path object
        for nSL in range(0, numScanLines):
            yVal = ymin + (nSL * smplInt)
            p1 = ocl.Point(xmin, yVal, fd)   # start-point of line
            p2 = ocl.Point(xmax, yVal, fd)   # end-point of line
            path.append(ocl.Line(p1, p2))
            # path.append(l)        # add the line to the path
        pdc.setPath(path)
        pdc.run()  # run drop-cutter on the path

        # return the list the points
        return pdc.getCLPoints()

    def _getWaterline(self, obj, scanLines, layDep, lyr, lenSL, pntsPerLine):
        '''_getWaterline(obj, scanLines, layDep, lyr, lenSL, pntsPerLine) ... Get waterline.'''
        commands = []
        cmds = []
        loopList = []
        self.topoMap = []
        # Create topo map from scanLines (highs and lows)
        self.topoMap = self._createTopoMap(scanLines, layDep, lenSL, pntsPerLine)
        # Add buffer lines and columns to topo map
        self._bufferTopoMap(lenSL, pntsPerLine)
        # Identify layer waterline from OCL scan
        self._highlightWaterline(4, 9)
        # Extract waterline and convert to gcode
        loopList = self._extractWaterlines(obj, scanLines, lyr, layDep)
        # save commands
        for loop in loopList:
            cmds = self._loopToGcode(obj, layDep, loop)
            commands.extend(cmds)
        return commands

    def _createTopoMap(self, scanLines, layDep, lenSL, pntsPerLine):
        '''_createTopoMap(scanLines, layDep, lenSL, pntsPerLine) ... Create topo map version of OCL scan data.'''
        topoMap = []
        for L in range(0, lenSL):
            topoMap.append([])
            for P in range(0, pntsPerLine):
                if scanLines[L][P].z > layDep:
                    topoMap[L].append(2)
                else:
                    topoMap[L].append(0)
        return topoMap

    def _bufferTopoMap(self, lenSL, pntsPerLine):
        '''_bufferTopoMap(lenSL, pntsPerLine) ... Add buffer boarder of zeros to all sides to topoMap data.'''
        pre = [0, 0]
        post = [0, 0]
        for p in range(0, pntsPerLine):
            pre.append(0)
            post.append(0)
        for l in range(0, lenSL):
            self.topoMap[l].insert(0, 0)
            self.topoMap[l].append(0)
        self.topoMap.insert(0, pre)
        self.topoMap.append(post)
        return True

    def _highlightWaterline(self, extraMaterial, insCorn):
        '''_highlightWaterline(extraMaterial, insCorn) ... Highlight the waterline data, separating from extra material.'''
        TM = self.topoMap
        lastPnt = len(TM[1]) - 1
        lastLn = len(TM) - 1
        highFlag = 0

        # ("--Convert parallel data to ridges")
        for lin in range(1, lastLn):
            for pt in range(1, lastPnt):  # Ignore first and last points
                if TM[lin][pt] == 0:
                    if TM[lin][pt + 1] == 2:  # step up
                        TM[lin][pt] = 1
                    if TM[lin][pt - 1] == 2:  # step down
                        TM[lin][pt] = 1

        # ("--Convert perpendicular data to ridges and highlight ridges")
        for pt in range(1, lastPnt):  # Ignore first and last points
            for lin in range(1, lastLn):
                if TM[lin][pt] == 0:
                    highFlag = 0
                    if TM[lin + 1][pt] == 2:  # step up
                        TM[lin][pt] = 1
                    if TM[lin - 1][pt] == 2:  # step down
                        TM[lin][pt] = 1
                elif TM[lin][pt] == 2:
                    highFlag += 1
                    if highFlag == 3:
                        if TM[lin - 1][pt - 1] < 2 or TM[lin - 1][pt + 1] < 2:
                            highFlag = 2
                        else:
                            TM[lin - 1][pt] = extraMaterial
                            highFlag = 2

        # ("--Square corners")
        for pt in range(1, lastPnt):
            for lin in range(1, lastLn):
                if TM[lin][pt] == 1:                    # point == 1
                    cont = True
                    if TM[lin + 1][pt] == 0:            # forward == 0
                        if TM[lin + 1][pt - 1] == 1:    # forward left == 1
                            if TM[lin][pt - 1] == 2:    # left == 2
                                TM[lin + 1][pt] = 1     # square the corner
                                cont = False

                        if cont is True and TM[lin + 1][pt + 1] == 1:  # forward right == 1
                            if TM[lin][pt + 1] == 2:    # right == 2
                                TM[lin + 1][pt] = 1     # square the corner
                        cont = True

                    if TM[lin - 1][pt] == 0:          # back == 0
                        if TM[lin - 1][pt - 1] == 1:    # back left == 1
                            if TM[lin][pt - 1] == 2:    # left == 2
                                TM[lin - 1][pt] = 1     # square the corner
                                cont = False

                        if cont is True and TM[lin - 1][pt + 1] == 1:  # back right == 1
                            if TM[lin][pt + 1] == 2:    # right == 2
                                TM[lin - 1][pt] = 1     # square the corner

        # remove inside corners
        for pt in range(1, lastPnt):
            for lin in range(1, lastLn):
                if TM[lin][pt] == 1:                    # point == 1
                    if TM[lin][pt + 1] == 1:
                        if TM[lin - 1][pt + 1] == 1 or TM[lin + 1][pt + 1] == 1:
                            TM[lin][pt + 1] = insCorn
                    elif TM[lin][pt - 1] == 1:
                        if TM[lin - 1][pt - 1] == 1 or TM[lin + 1][pt - 1] == 1:
                            TM[lin][pt - 1] = insCorn

        return True

    def _extractWaterlines(self, obj, oclScan, lyr, layDep):
        '''_extractWaterlines(obj, oclScan, lyr, layDep) ... Extract water lines from OCL scan data.'''
        srch = True
        lastPnt = len(self.topoMap[0]) - 1
        lastLn = len(self.topoMap) - 1
        maxSrchs = 5
        srchCnt = 1
        loopList = []
        loop = []
        loopNum = 0

        if self.CutClimb is True:
            lC = [-1, -1, -1, 0, 1, 1, 1, 0, -1, -1, -1, 0, 1, 1, 1, 0, -1, -1, -1, 0, 1, 1, 1, 0]
            pC = [-1, 0, 1, 1, 1, 0, -1, -1, -1, 0, 1, 1, 1, 0, -1, -1, -1, 0, 1, 1, 1, 0, -1, -1]
        else:
            lC = [1, 1, 1, 0, -1, -1, -1, 0, 1, 1, 1, 0, -1, -1, -1, 0, 1, 1, 1, 0, -1, -1, -1, 0]
            pC = [-1, 0, 1, 1, 1, 0, -1, -1, -1, 0, 1, 1, 1, 0, -1, -1, -1, 0, 1, 1, 1, 0, -1, -1]

        while srch is True:
            srch = False
            if srchCnt > maxSrchs:
                PathLog.debug("Max search scans, " + str(maxSrchs) + " reached\nPossible incomplete waterline result!")
                break
            for L in range(1, lastLn):
                for P in range(1, lastPnt):
                    if self.topoMap[L][P] == 1:
                        # start loop follow
                        srch = True
                        loopNum += 1
                        loop = self._trackLoop(oclScan, lC, pC, L, P, loopNum)
                        self.topoMap[L][P] = 0  # Mute the starting point
                        loopList.append(loop)
            srchCnt += 1
        PathLog.debug("Search count for layer " + str(lyr) + " is " + str(srchCnt) + ", with " + str(loopNum) + " loops.")
        return loopList

    def _trackLoop(self, oclScan, lC, pC, L, P, loopNum):
        '''_trackLoop(oclScan, lC, pC, L, P, loopNum) ... Track the loop direction.'''
        loop = [oclScan[L - 1][P - 1]]  # Start loop point list
        cur = [L, P, 1]
        prv = [L, P - 1, 1]
        nxt = [L, P + 1, 1]
        follow = True
        ptc = 0
        ptLmt = 200000
        while follow is True:
            ptc += 1
            if ptc > ptLmt:
                PathLog.debug("Loop number " + str(loopNum) + " at [" + str(nxt[0]) + ", " + str(nxt[1]) + "] pnt count exceeds, " + str(ptLmt) + ".  Stopped following loop.")
                break
            nxt = self._findNextWlPoint(lC, pC, cur[0], cur[1], prv[0], prv[1])  # get next point
            loop.append(oclScan[nxt[0] - 1][nxt[1] - 1])  # add it to loop point list
            self.topoMap[nxt[0]][nxt[1]] = nxt[2]  # Mute the point, if not Y stem
            if nxt[0] == L and nxt[1] == P:  # check if loop complete
                follow = False
            elif nxt[0] == cur[0] and nxt[1] == cur[1]:  # check if line cannot be detected
                follow = False
            prv = cur
            cur = nxt
        return loop

    def _findNextWlPoint(self, lC, pC, cl, cp, pl, pp):
        '''_findNextWlPoint(lC, pC, cl, cp, pl, pp) ...
        Find the next waterline point in the point cloud layer provided.'''
        dl = cl - pl
        dp = cp - pp
        num = 0
        i = 3
        s = 0
        mtch = 0
        found = False
        while mtch < 8:  # check all 8 points around current point
            if lC[i] == dl:
                if pC[i] == dp:
                    s = i - 3
                    found = True
                    # Check for y branch where current point is connection between branches
                    for y in range(1, mtch):
                        if lC[i + y] == dl:
                            if pC[i + y] == dp:
                                num = 1
                                break
                    break
            i += 1
            mtch += 1
        if found is False:
            # ("_findNext: No start point found.")
            return [cl, cp, num]

        for r in range(0, 8):
            l = cl + lC[s + r]
            p = cp + pC[s + r]
            if self.topoMap[l][p] == 1:
                return [l, p, num]

        # ("_findNext: No next pnt found")
        return [cl, cp, num]

    def _loopToGcode(self, obj, layDep, loop):
        '''_loopToGcode(obj, layDep, loop) ... Convert set of loop points to Gcode.'''
        # generate the path commands
        output = []
        optimize = obj.OptimizeLinearPaths

        prev = ocl.Point(float("inf"), float("inf"), float("inf"))
        nxt = ocl.Point(float("inf"), float("inf"), float("inf"))
        pnt = ocl.Point(float("inf"), float("inf"), float("inf"))

        # Create first point
        pnt.x = loop[0].x
        pnt.y = loop[0].y
        pnt.z = layDep

        # Position cutter to begin loop
        output.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        output.append(Path.Command('G0', {'X': pnt.x, 'Y': pnt.y, 'F': self.horizRapid}))
        output.append(Path.Command('G1', {'Z': pnt.z, 'F': self.vertFeed}))

        lenCLP = len(loop)
        lastIdx = lenCLP - 1
        # Cycle through each point on loop
        for i in range(0, lenCLP):
            if i < lastIdx:
                nxt.x = loop[i + 1].x
                nxt.y = loop[i + 1].y
                nxt.z = layDep
            else:
                optimize = False

            if not optimize or not self.isPointOnLine(FreeCAD.Vector(prev.x, prev.y, prev.z), FreeCAD.Vector(nxt.x, nxt.y, nxt.z), FreeCAD.Vector(pnt.x, pnt.y, pnt.z)):
                output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'F': self.horizFeed}))

            # Rotate point data
            prev.x = pnt.x
            prev.y = pnt.y
            prev.z = pnt.z
            pnt.x = nxt.x
            pnt.y = nxt.y
            pnt.z = nxt.z

        # Save layer end point for use in transitioning to next layer
        self.layerEndPnt.x = pnt.x
        self.layerEndPnt.y = pnt.y
        self.layerEndPnt.z = pnt.z

        return output

    # Support functions for both dropcutter and waterline operations
    def isPointOnLine(self, strtPnt, endPnt, pointP):
        '''isPointOnLine(strtPnt, endPnt, pointP) ... Determine if a given point is on the line defined by start and end points.'''
        tolerance = 1e-6
        vectorAB = endPnt - strtPnt
        vectorAC = pointP - strtPnt
        crossproduct = vectorAB.cross(vectorAC)
        dotproduct = vectorAB.dot(vectorAC)

        if crossproduct.Length > tolerance:
            return False

        if dotproduct < 0:
            return False

        if dotproduct > vectorAB.Length * vectorAB.Length:
            return False

        return True

    def holdStopCmds(self, obj, zMax, pd, p2, txt):
        '''holdStopCmds(obj, zMax, pd, p2, txt) ... Gcode commands to be executed at beginning of hold.'''
        cmds = []
        msg = 'N (' + txt + ')'
        cmds.append(Path.Command(msg, {}))  # Raise cutter rapid to zMax in line of travel
        cmds.append(Path.Command('G0', {'Z': zMax, 'F': self.vertRapid}))  # Raise cutter rapid to zMax in line of travel
        cmds.append(Path.Command('G0', {'X': p2.x, 'Y': p2.y, 'F': self.horizRapid}))  # horizontal rapid to current XY coordinate
        if zMax != pd:
            cmds.append(Path.Command('G0', {'Z': pd, 'F': self.vertRapid}))  # drop cutter down rapidly to prevDepth depth
            cmds.append(Path.Command('G0', {'Z': p2.z, 'F': self.vertFeed}))  # drop cutter down to current Z depth, returning to normal cut path and speed
        return cmds

    def holdStopEndCmds(self, obj, p2, txt):
        '''holdStopEndCmds(obj, p2, txt) ... Gcode commands to be executed at end of hold stop.'''
        cmds = []
        msg = 'N (' + txt + ')'
        cmds.append(Path.Command(msg, {}))  # Raise cutter rapid to zMax in line of travel
        cmds.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))  # Raise cutter rapid to zMax in line of travel
        # cmds.append(Path.Command('G0', {'X': p2.x, 'Y': p2.y, 'F': self.horizRapid}))  # horizontal rapid to current XY coordinate
        return cmds

    def subsectionCLP(self, CLP, xmin, ymin, xmax, ymax):
        '''subsectionCLP(CLP, xmin, ymin, xmax, ymax) ...
        This function returns a subsection of the CLP scan, limited to the min/max values supplied.'''
        section = list()
        lenCLP = len(CLP)
        for i in range(0, lenCLP):
            if CLP[i].x < xmax:
                if CLP[i].y < ymax:
                    if CLP[i].x > xmin:
                        if CLP[i].y > ymin:
                            section.append(CLP[i])
        return section

    def getMaxHeightBetweenPoints(self, finalDepth, p1, p2, cutter, CLP):
        ''' getMaxHeightBetweenPoints(finalDepth, p1, p2, cutter, CLP) ...
        This function connects two HOLD points with line.
        Each point within the subsection point list is tested to determinie if it is under cutter.
        Points determined to be under the cutter on line are tested for z height.
        The highest z point is the requirement for clearance between p1 and p2, and returned as zMax with 2 mm extra.
        '''
        dx = (p2.x - p1.x)
        if dx == 0.0:
            dx = 0.00001  # Need to employ a global tolerance here
        m = (p2.y - p1.y) / dx
        b = p1.y - (m * p1.x)

        avoidTool = round(cutter * 0.75, 1)  # 1/2 diam. of cutter is theoretically safe, but 3/4 diam is used for extra clearance
        zMax = finalDepth
        lenCLP = len(CLP)
        for i in range(0, lenCLP):
            mSqrd = m**2
            if mSqrd < 0.0000001:  # Need to employ a global tolerance here
                mSqrd = 0.0000001
            perpDist = math.sqrt((CLP[i].y - (m * CLP[i].x) - b)**2 / (1 + 1 / (mSqrd)))
            if perpDist < avoidTool:  # if point within cutter reach on line of travel, test z height and update as needed
                if CLP[i].z > zMax:
                    zMax = CLP[i].z
        return zMax + 2.0
    
    def resetOpVariables(self, all=True):
        '''resetOpVariables() ... Reset class variables used for instance of operation.'''
        self.holdPoint = None
        self.layerEndPnt = None
        self.onHold = False
        self.SafeHeightOffset = 2.0
        self.ClearHeightOffset = 4.0
        self.layerEndzMax = 0.0
        self.resetTolerance = 0.0
        self.holdPntCnt = 0
        self.bbRadius = 0.0
        self.axialFeed = 0.0
        self.axialRapid = 0.0
        self.FinalDepth = 0.0
        self.clearHeight = 0.0
        self.safeHeight = 0.0
        self.faceZMax = -999999999999.0
        if all is True:
            self.cutter = None
            self.stl = None
            self.fullSTL = None
            self.cutOut = 0.0
            self.radius = 0.0
            self.useTiltCutter = False
        return True

    def deleteOpVariables(self, all=True):
        '''deleteOpVariables() ... Reset class variables used for instance of operation.'''
        del self.holdPoint
        del self.layerEndPnt
        del self.onHold
        del self.SafeHeightOffset
        del self.ClearHeightOffset
        del self.layerEndzMax
        del self.resetTolerance
        del self.holdPntCnt
        del self.bbRadius
        del self.axialFeed
        del self.axialRapid
        del self.FinalDepth
        del self.clearHeight
        del self.safeHeight
        del self.faceZMax
        if all is True:
            del self.cutter
            del self.stl
            del self.fullSTL
            del self.cutOut
            del self.radius
            del self.useTiltCutter
        return True

    def setOclCutter(self, obj, safe=False):
        ''' setOclCutter(obj) ... Translation function to convert FreeCAD tool definition to OCL formatted tool. '''
        # Set cutter details
        #  https://www.freecadweb.org/api/dd/dfe/classPath_1_1Tool.html#details
        diam_1 = float(obj.ToolController.Tool.Diameter)
        lenOfst = obj.ToolController.Tool.LengthOffset if hasattr(obj.ToolController.Tool, 'LengthOffset') else 0
        FR = obj.ToolController.Tool.FlatRadius if hasattr(obj.ToolController.Tool, 'FlatRadius') else 0
        CEH = obj.ToolController.Tool.CuttingEdgeHeight if hasattr(obj.ToolController.Tool, 'CuttingEdgeHeight') else 0
        CEA = obj.ToolController.Tool.CuttingEdgeAngle if hasattr(obj.ToolController.Tool, 'CuttingEdgeAngle') else 0

        # Make safeCutter with 2 mm buffer around physical cutter
        if safe is True:
            diam_1 += 4.0
            if FR != 0.0:
                FR += 2.0
            
        PathLog.debug('ToolType: {}'.format(obj.ToolController.Tool.ToolType))
        if obj.ToolController.Tool.ToolType == 'EndMill':
            # Standard End Mill
            return ocl.CylCutter(diam_1, (CEH + lenOfst))

        elif obj.ToolController.Tool.ToolType == 'BallEndMill' and FR == 0.0:
            # Standard Ball End Mill
            # OCL -> BallCutter::BallCutter(diameter, length)
            self.useTiltCutter = True
            return ocl.BallCutter(diam_1, (diam_1 / 2 + lenOfst))

        elif obj.ToolController.Tool.ToolType == 'BallEndMill' and FR > 0.0:
            # Bull Nose or Corner Radius cutter
            # Reference: https://www.fine-tools.com/halbstabfraeser.html
            # OCL -> BallCutter::BallCutter(diameter, length)
            return ocl.BullCutter(diam_1, FR, (CEH + lenOfst))

        elif obj.ToolController.Tool.ToolType == 'Engraver' and FR > 0.0:
            # Bull Nose or Corner Radius cutter
            # Reference: https://www.fine-tools.com/halbstabfraeser.html
            # OCL -> ConeCutter::ConeCutter(diameter, angle, lengthOffset)
            return ocl.ConeCutter(diam_1, (CEA / 2), lenOfst)

        elif obj.ToolController.Tool.ToolType == 'ChamferMill':
            # Bull Nose or Corner Radius cutter
            # Reference: https://www.fine-tools.com/halbstabfraeser.html
            # OCL -> ConeCutter::ConeCutter(diameter, angle, lengthOffset)
            return ocl.ConeCutter(diam_1, (CEA / 2), lenOfst)
        else:
            # Default to standard end mill
            PathLog.warning("Defaulting cutter to standard end mill.")
            return ocl.CylCutter(diam_1, (CEH + lenOfst))

        # http://www.carbidecutter.net/products/carbide-burr-cone-shape-sm.html
        '''
        # Available FreeCAD cutter types - some still need translation to available OCL cutter classes.
        Drill,  CenterDrill,  CounterSink,  CounterBore,  FlyCutter,   Reamer,  Tap,
        EndMill,  SlotCutter,  BallEndMill,  ChamferMill,  CornerRound,  Engraver
        '''
        # Adittional problem is with new ToolBit user-defined cutter shapes.
        # Some sort of translation/conversion will have to be defined to make compatible with OCL.
        PathLog.error('Unable to set OCL cutter.')
        return False

    def determineVectDirect(self, pnt, nxt, travVect):
        if nxt.x == pnt.x:
            travVect.x = 0
        elif nxt.x < pnt.x:
            travVect.x = -1
        else:
            travVect.x = 1

        if nxt.y == pnt.y:
            travVect.y = 0
        elif nxt.y < pnt.y:
            travVect.y = -1
        else:
            travVect.y = 1
        return travVect

    def determineLineOfTravel(self, travVect):
        if travVect.x == 0 and travVect.y != 0:
            lineOfTravel = "Y"
        elif travVect.y == 0 and travVect.x != 0:
            lineOfTravel = "X"
        else:
            lineOfTravel = "O"  # used for turns
        return lineOfTravel

    def _getMinSafeTravelHeight(self, pdc, p1, p2, minDep=None):
        A = (p1.x, p1.y)
        B = (p2.x, p2.y)
        LINE = self._planarDropCutScan(pdc, A, B)
        zMax = LINE[0].z
        for p in LINE:
            if p.z > zMax:
                zMax = p.z
        if minDep is not None:
            if zMax < minDep:
                zMax = minDep
        return zMax


def SetupProperties():
    ''' SetupProperties() ... Return list of properties required for operation.'''
    setup = []
    setup.append('AvoidLastX_Faces')
    setup.append('AvoidLastX_InternalFeatures')
    setup.append('BoundBox')
    setup.append('BoundaryAdjustment')
    setup.append('CircularCenterAt')
    setup.append('CircularCenterCustom')
    setup.append('CircularUseG2G3')
    setup.append('InternalFeaturesCut')
    setup.append('InternalFeaturesAdjustment')
    setup.append('CutMode')
    setup.append('CutPattern')
    setup.append('CutPatternAngle')
    setup.append('CutPatternReversed')
    setup.append('CutterTilt')
    setup.append('DepthOffset')
    setup.append('GapSizes')
    setup.append('GapThreshold')
    setup.append('HandleMultipleFeatures')
    setup.append('LayerMode')
    setup.append('OptimizeStepOverTransitions')
    setup.append('ProfileEdges')
    setup.append('BoundaryEnforcement')
    setup.append('RotationAxis')
    setup.append('SampleInterval')
    setup.append('ScanType')
    setup.append('StartIndex')
    setup.append('StartPoint')
    setup.append('StepOver')
    setup.append('StopIndex')
    setup.append('UseStartPoint')
    # For debugging
    setup.append('AreaParams')
    setup.append('ShowTempObjects')
    # Targeted for possible removal
    setup.append('IgnoreWaste')
    setup.append('IgnoreWasteDepth')
    setup.append('ReleaseFromWaste')
    return setup


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Surface operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectSurface(obj, name)
    return obj
