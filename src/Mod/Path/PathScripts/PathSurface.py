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
# *   by Russell Johnson  <russ4262@gmail.com>  2020-03-18 12:29 CST        *
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
        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "The library to use to generate the path"))
        obj.addProperty("App::PropertyEnumeration", "BoundBox", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "Should the operation be limited by the stock object or by the bounding box of the base object"))
        obj.addProperty("App::PropertyEnumeration", "DropCutterDir", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction along which dropcutter lines are created"))
        obj.addProperty("App::PropertyVectorDistance", "DropCutterExtraOffset", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "Additional offset to the selected bounding box"))
        obj.addProperty("App::PropertyEnumeration", "LayerMode", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "The completion mode for the operation: single or multi-pass"))
        obj.addProperty("App::PropertyEnumeration", "ScanType", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "Planar: Flat, 3D surface scan.  Rotational: 4th-axis rotational scan."))

        obj.addProperty("App::PropertyDistance", "AngularDeflection", "Mesh Conversion", QtCore.QT_TRANSLATE_NOOP("App::Property", "Smaller values yield a finer, more accurate the mesh. Smaller values increase processing time a lot."))
        obj.addProperty("App::PropertyDistance", "LinearDeflection", "Mesh Conversion", QtCore.QT_TRANSLATE_NOOP("App::Property", "Smaller values yield a finer, more accurate the mesh. Smaller values do not increase processing time much."))

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

        obj.Algorithm = ['OCL Dropcutter', 'OCL Waterline']
        obj.BoundBox = ['BaseBoundBox', 'Stock']
        obj.CircularCenterAt = ['CenterOfMass', 'CenterOfBoundBox', 'XminYmin', 'Custom']
        obj.CutMode = ['Conventional', 'Climb']
        obj.CutPattern = ['Line', 'ZigZag', 'Circular', 'CircularZigZag']  # Additional goals ['Offset', 'Spiral', 'ZigZagOffset', 'Grid', 'Triangle']
        obj.DropCutterDir = ['X', 'Y']
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

        if obj.Algorithm == 'OCL Dropcutter':
            obj.setEditorMode('CutPattern', 0)
            obj.setEditorMode('HandleMultipleFeatures', 0)
            obj.setEditorMode('CircularCenterAt', 0)
            obj.setEditorMode('CircularCenterCustom', 0)
            obj.setEditorMode('CutPatternAngle', 0)
            # obj.setEditorMode('BoundaryEnforcement', 0)

            if obj.ScanType == 'Planar':
                obj.setEditorMode('DropCutterDir', 2)
                obj.setEditorMode('DropCutterExtraOffset', 2)
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
                obj.setEditorMode('DropCutterDir', 0)
                obj.setEditorMode('DropCutterExtraOffset', 0)
                obj.setEditorMode('RotationAxis', 0)  # 0=show & editable
                obj.setEditorMode('StartIndex', 0)
                obj.setEditorMode('StopIndex', 0)
                obj.setEditorMode('CutterTilt', 0)

        elif obj.Algorithm == 'OCL Waterline':
            obj.setEditorMode('DropCutterExtraOffset', 2)
            obj.setEditorMode('DropCutterDir', 2)
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
                if prop == 'Algorithm':
                    self.setEditorProperties(obj)
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
        obj.AngularDeflection.Value = 0.25
        obj.LinearDeflection.Value = job.GeometryTolerance
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
        # Save initial value for restoration later.
        if obj.Algorithm == 'OCL Waterline':
            preCP = obj.CutPattern
            preCPA = obj.CutPatternAngle
            preRB = obj.BoundaryEnforcement
            obj.CutPattern = 'Line'
            obj.CutPatternAngle = 0.0
            obj.BoundaryEnforcement = False

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
            if obj.Algorithm == 'OCL Waterline':
                obj.CutPattern = preCP
                obj.CutPatternAngle = preCPA
                obj.BoundaryEnforcement = preRB

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
                            faceOfstShp = self._extractFaceOffset(obj, outerFace, ofstVal)

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
                if CS is False:
                    return False
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
                mesh = MeshPart.meshFromShape(Shape=M.Shape,
                                              LinearDeflection=obj.LinearDeflection.Value,
                                              AngularDeflection=obj.AngularDeflection.Value,
                                              Relative=False)

            if self.modelSTLs[m] is True:
                stl = ocl.STLSurf()
                if obj.Algorithm == 'OCL Dropcutter':
                    for f in mesh.Facets:
                        p = f.Points[0]
                        q = f.Points[1]
                        r = f.Points[2]
                        t = ocl.Triangle(ocl.Point(p[0], p[1], p[2]),
                                         ocl.Point(q[0], q[1], q[2]),
                                         ocl.Point(r[0], r[1], r[2]))
                        stl.addTriangle(t)
                    self.modelSTLs[m] = stl
                elif obj.Algorithm == 'OCL Waterline':
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
        meshFuse = MeshPart.meshFromShape(Shape=fused,
                                          LinearDeflection=obj.LinearDeflection.Value,
                                          AngularDeflection=obj.AngularDeflection.Value,
                                          Relative=False)
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
        It then calls the correct scan method depending on the Algorithm and ScanType properties.'''
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

            if obj.Algorithm == 'OCL Waterline':
                final.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
                final.extend(self._waterlineOp(JOB, obj, mdlIdx, COMP))  # independent method set for Waterline
            elif obj.ScanType == 'Planar':
                final.extend(self._processPlanarOp(JOB, obj, mdlIdx, COMP, 0))
            elif obj.ScanType == 'Rotational':
                final.extend(self._processRotationalOp(obj, base, COMP))

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

                if obj.Algorithm == 'OCL Waterline':
                    final.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
                    final.extend(self._waterlineOp(JOB, obj, mdlIdx, COMP))  # independent method set for Waterline
                elif obj.ScanType == 'Planar':
                    final.extend(self._processPlanarOp(JOB, obj, mdlIdx, COMP, fsi))
                elif obj.ScanType == 'Rotational':
                    final.extend(self._processRotationalOp(JOB, obj, mdlIdx, COMP))
                COMP = None
        # Eif

        return final

    # Methods for creating path geometry
    def _processPlanarOp(self, JOB, obj, mdlIdx, cmpdShp, fsi):
        '''_processPlanarOp(JOB, obj, mdlIdx, cmpdShp)... 
        This method compiles the main components for the procedural portion of a planar operation (non-rotational).  
        It creates the OCL PathDropCutter objects: model and safeTravel.
        It makes the necessary facial geometries for the actual cut area.
        It calls the correct Single or Multi-pass method as needed.
        It returns the gcode for the operation. '''
        PathLog.debug('_processPlanarOp()')
        final = list()
        SCANDATA = list()
        # base = JOB.Model.Group[mdlIdx]

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == 'Single-pass':
            depthparams = [obj.FinalDepth.Value]
        elif obj.LayerMode == 'Multi-pass':
            depthparams = [i for i in self.depthParams]
        lenDP = len(depthparams)

        # Prepare PathDropCutter objects with STL data
        pdc = self._planarGetPDC(self.modelSTLs[mdlIdx], depthparams[lenDP - 1], obj.SampleInterval.Value)
        safePDC = self._planarGetPDC(self.safeSTLs[mdlIdx],
                                    depthparams[lenDP - 1], obj.SampleInterval.Value, useSafeCutter=False)

        profScan = list()
        if obj.ProfileEdges != 'None':
            prflShp = self.profileShapes[mdlIdx][fsi]
            if prflShp is False:
                PathLog.error('No profile shape is False.')
                return list()
            if self.showDebugObjects is True:
                P = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpNewProfileShape')
                P.Shape = prflShp
                # P.recompute()
                P.purgeTouched()
                self.tempGroup.addObject(P)
            # get offset path geometry and perform OCL scan with that geometry
            pathOffsetGeom = self._planarMakeProfileGeom(obj, prflShp)
            if pathOffsetGeom is False:
                PathLog.error('No profile geometry returned.')
                return list()
            profScan = [self._planarPerformOclScan(obj, pdc, pathOffsetGeom, offsetPoints=True)]

        geoScan = list()
        if obj.ProfileEdges != 'Only':
            if self.showDebugObjects is True:
                F = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpCutArea')
                F.Shape = cmpdShp
                # F.recompute()
                F.purgeTouched()
                self.tempGroup.addObject(F)
            # get internal path geometry and perform OCL scan with that geometry
            pathGeom = self._planarMakePathGeom(obj, cmpdShp)
            if pathGeom is False:
                PathLog.error('No path geometry returned.')
                return list()
            geoScan = self._planarPerformOclScan(obj, pdc, pathGeom, offsetPoints=False)

        if obj.ProfileEdges == 'Only':  # ['None', 'Only', 'First', 'Last']
            SCANDATA.extend(profScan)
        if obj.ProfileEdges == 'None':
            SCANDATA.extend(geoScan)
        if obj.ProfileEdges == 'First':
            SCANDATA.extend(profScan)
            SCANDATA.extend(geoScan)
        if obj.ProfileEdges == 'Last':
            SCANDATA.extend(geoScan)
            SCANDATA.extend(profScan)

        # Apply depth offset
        if obj.DepthOffset.Value != 0.0:
            self._planarApplyDepthOffset(SCANDATA, obj.DepthOffset.Value)

        if len(SCANDATA) == 0:
            PathLog.error('No scan data to convert to Gcode.')
            return list()

        # If cut pattern is `Circular`, there are zero(almost zero) straight lines to optimize
        # Store initial `OptimizeLinearPaths` value for later restoration
        self.preOLP = obj.OptimizeLinearPaths
        if obj.CutPattern in ['Circular', 'CircularZigZag']:
            obj.OptimizeLinearPaths = False

        # Process OCL scan data
        if obj.LayerMode == 'Single-pass':
            final.extend(self._planarDropCutSingle(JOB, obj, pdc, safePDC, depthparams, SCANDATA))
        elif obj.LayerMode == 'Multi-pass':
            final.extend(self._planarDropCutMulti(JOB, obj, pdc, safePDC, depthparams, SCANDATA))

        # If cut pattern is `Circular`, restore initial OLP value
        if obj.CutPattern in ['Circular', 'CircularZigZag']:
            obj.OptimizeLinearPaths = self.preOLP

        # Raise to safe height between individual faces.
        if obj.HandleMultipleFeatures == 'Individually':
            final.insert(0, Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))

        return final

    def _planarMakePathGeom(self, obj, faceShp):
        '''_planarMakePathGeom(obj, faceShp)...
        Creates the line/arc cut pattern geometry and returns the intersection with the received faceShp.
        The resulting intersecting line/arc geometries are then converted to lines or arcs for OCL.'''
        PathLog.debug('_planarMakePathGeom()')
        GeoSet = list()

        # Apply drop cutter extra offset and set the max and min XY area of the operation
        xmin = faceShp.BoundBox.XMin
        xmax = faceShp.BoundBox.XMax
        ymin = faceShp.BoundBox.YMin
        ymax = faceShp.BoundBox.YMax
        zmin = faceShp.BoundBox.ZMin
        zmax = faceShp.BoundBox.ZMax

        # Compute weighted center of mass of all faces combined
        fCnt = 0
        totArea = 0.0
        zeroCOM = FreeCAD.Vector(0.0, 0.0, 0.0)
        for F in faceShp.Faces:
            comF = F.CenterOfMass
            areaF = F.Area
            totArea += areaF
            fCnt += 1
            zeroCOM = zeroCOM.add(FreeCAD.Vector(comF.x, comF.y, 0.0).multiply(areaF))
        if fCnt == 0:
            PathLog.error(translate('PathSurface', 'Cannot calculate the Center Of Mass. Using Center of Boundbox.'))
            zeroCOM = FreeCAD.Vector((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, 0.0)
        else:
            avgArea = totArea / fCnt
            zeroCOM.multiply(1 / fCnt)
            zeroCOM.multiply(1 / avgArea)
        COM = FreeCAD.Vector(zeroCOM.x, zeroCOM.y, 0.0)

        # get X, Y, Z spans; Compute center of rotation
        deltaX = abs(xmax-xmin)
        deltaY = abs(ymax-ymin)
        deltaZ = abs(zmax-zmin)
        deltaC = math.sqrt(deltaX**2 + deltaY**2)
        lineLen = deltaC + (2.0 * self.cutter.getDiameter())  # Line length to span boundbox diag with 2x cutter diameter extra on each end
        halfLL = math.ceil(lineLen / 2.0)
        cutPasses = math.ceil(lineLen / self.cutOut) + 1  # Number of lines(passes) required to cover lineLen
        halfPasses = math.ceil(cutPasses / 2.0)
        bbC = faceShp.BoundBox.Center

        # Generate the Draft line/circle sets to be intersected with the cut-face-area
        if obj.CutPattern in ['ZigZag', 'Line']:
            MaxLC = -1
            centRot = FreeCAD.Vector(0.0, 0.0, 0.0)  # Bottom left corner of face/selection/model
            cAng = math.atan(deltaX / deltaY)  # BoundaryBox angle

            # Determine end points and create top lines
            x1 = centRot.x - halfLL
            x2 = centRot.x + halfLL
            diag = None
            if obj.CutPatternAngle == 0 or obj.CutPatternAngle == 180:
                MaxLC = math.floor(deltaY / self.cutOut)
                diag = deltaY
            elif obj.CutPatternAngle == 90 or obj.CutPatternAngle == 270:
                MaxLC = math.floor(deltaX / self.cutOut)
                diag = deltaX
            else:
                perpDist = math.cos(cAng - math.radians(obj.CutPatternAngle)) * deltaC
                MaxLC = math.floor(perpDist / self.cutOut)
                diag = perpDist
            y1 = centRot.y + diag
            # y2 = y1

            p1 = FreeCAD.Vector(x1, y1, 0.0)
            p2 = FreeCAD.Vector(x2, y1, 0.0)
            topLineTuple = (p1, p2)
            ny1 = centRot.y - diag
            n1 = FreeCAD.Vector(x1, ny1, 0.0)
            n2 = FreeCAD.Vector(x2, ny1, 0.0)
            negTopLineTuple = (n1, n2)

            # Create end points for set of lines to intersect with cross-section face
            pntTuples = list()
            for lc in range((-1 * (halfPasses - 1)), halfPasses + 1):
                # if lc == (cutPasses - MaxLC - 1):
                #    pntTuples.append(negTopLineTuple)
                # if lc == (MaxLC + 1):
                #    pntTuples.append(topLineTuple)
                x1 = centRot.x - halfLL
                x2 = centRot.x + halfLL
                y1 = centRot.y + (lc * self.cutOut)
                # y2 = y1
                p1 = FreeCAD.Vector(x1, y1, 0.0)
                p2 = FreeCAD.Vector(x2, y1, 0.0)
                pntTuples.append( (p1, p2) )

            # Convert end points to lines
            for (p1, p2) in pntTuples:
                line = Part.makeLine(p1, p2)
                GeoSet.append(line)
        elif obj.CutPattern in ['Circular', 'CircularZigZag']:
            zTgt = faceShp.BoundBox.ZMin
            axisRot = FreeCAD.Vector(0.0, 0.0, 1.0)
            cntr = FreeCAD.Placement()
            cntr.Rotation = FreeCAD.Rotation(axisRot, 0.0)

            if obj.CircularCenterAt == 'CenterOfMass':
                cntr.Base = FreeCAD.Vector(COM.x, COM.y, zTgt)  # COM  # Use center of Mass
            elif obj.CircularCenterAt == 'CenterOfBoundBox':
                cent = faceShp.BoundBox.Center
                cntr.Base = FreeCAD.Vector(cent.x, cent.y, zTgt)  
            elif obj.CircularCenterAt == 'XminYmin':
                cntr.Base = FreeCAD.Vector(faceShp.BoundBox.XMin, faceShp.BoundBox.YMin, zTgt)
            elif obj.CircularCenterAt == 'Custom':
                newCent = FreeCAD.Vector(obj.CircularCenterCustom.x, obj.CircularCenterCustom.y, zTgt)
                cntr.Base = newCent

            # recalculate cutPasses value, if need be
            radialPasses = halfPasses
            if obj.CircularCenterAt != 'CenterOfBoundBox':
                # make 4 corners of boundbox in XY plane, find which is greatest distance to new circular center
                EBB = faceShp.BoundBox
                CORNERS = [
                    FreeCAD.Vector(EBB.XMin, EBB.YMin, 0.0),
                    FreeCAD.Vector(EBB.XMin, EBB.YMax, 0.0),
                    FreeCAD.Vector(EBB.XMax, EBB.YMax, 0.0),
                    FreeCAD.Vector(EBB.XMax, EBB.YMin, 0.0),
                ]
                dMax = 0.0
                for c in range(0, 4):
                    dist = CORNERS[c].sub(cntr.Base).Length
                    if dist > dMax:
                        dMax = dist
                lineLen = dMax + (2.0 * self.cutter.getDiameter())  # Line length to span boundbox diag with 2x cutter diameter extra on each end
                radialPasses = math.ceil(lineLen / self.cutOut) + 1  # Number of lines(passes) required to cover lineLen
            
            # Update COM point and current CircularCenter
            if obj.CircularCenterAt != 'Custom':
                obj.CircularCenterCustom = cntr.Base

            minRad = self.cutter.getDiameter() * 0.45
            siX3 = 3 * obj.SampleInterval.Value
            minRadSI = (siX3 / 2.0) / math.pi
            if minRad < minRadSI:
                minRad = minRadSI

            # Make small center circle to start pattern
            if obj.StepOver > 50:
                circle = Part.makeCircle(minRad, cntr.Base)
                GeoSet.append(circle)

            for lc in range(1, radialPasses + 1):
                rad = (lc * self.cutOut)
                if rad >= minRad:
                    circle = Part.makeCircle(rad, cntr.Base)
                    GeoSet.append(circle)
            # Efor
            COM = cntr.Base
        # Eif

        if obj.CutPatternReversed is True:
            GeoSet.reverse()

        if faceShp.BoundBox.ZMin != 0.0:
            faceShp.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - faceShp.BoundBox.ZMin))

        # Create compound object to bind all lines in Lineset
        geomShape = Part.makeCompound(GeoSet)

        # Position and rotate the Line and ZigZag geometry
        if obj.CutPattern in ['Line', 'ZigZag']:
            if obj.CutPatternAngle != 0.0:
                geomShape.Placement.Rotation = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), obj.CutPatternAngle)
            geomShape.Placement.Base = FreeCAD.Vector(bbC.x, bbC.y, 0.0 - geomShape.BoundBox.ZMin)

        if self.showDebugObjects is True:
            F = FreeCAD.ActiveDocument.addObject('Part::Feature','tmpGeometrySet')
            F.Shape = geomShape
            F.purgeTouched()
            self.tempGroup.addObject(F)

        # Identify intersection of cross-section face and lineset
        cmnShape = faceShp.common(geomShape)

        if self.showDebugObjects is True:
            F = FreeCAD.ActiveDocument.addObject('Part::Feature','tmpPathGeometry')
            F.Shape = cmnShape
            F.purgeTouched()
            self.tempGroup.addObject(F)

        self.tmpCOM = FreeCAD.Vector(COM.x, COM.y, faceShp.BoundBox.ZMin)
        return cmnShape

    def _planarMakeProfileGeom(self, obj, subShp):
        PathLog.debug('_planarMakeProfileGeom()')

        offsetLists = list()
        dist = obj.SampleInterval.Value / 5.0
        defl = obj.SampleInterval.Value / 5.0

        # Reference https://forum.freecadweb.org/viewtopic.php?t=28861#p234939
        for fc in subShp.Faces:
            # Reverse order of wires in each face - inside to outside
            for w in range(len(fc.Wires) - 1, -1, -1):
                W = fc.Wires[w]
                PNTS = W.discretize(Distance=dist)
                # PNTS = W.discretize(Deflection=defl)
                if self.CutClimb is True:
                    PNTS.reverse()
                offsetLists.append(PNTS)

        return offsetLists

    def _planarPerformOclScan(self, obj, pdc, pathGeom, offsetPoints=False):
        '''_planarPerformOclScan(obj, pdc, pathGeom, offsetPoints=False)...
        Switching fuction for calling the appropriate path-geometry to OCL points conversion fucntion
        for the various cut patterns.'''
        PathLog.debug('_planarPerformOclScan()')
        SCANS = list()

        if offsetPoints is True:
            PNTSET = self._pathGeomToOffsetPointSet(obj, pathGeom)
            for D in PNTSET:
                stpOvr = list()
                ofst = list()
                for I in D:
                    if I == 'BRK':
                        stpOvr.append(ofst)
                        stpOvr.append(I)
                        ofst = list()
                    else:
                        # D format is ((p1, p2), (p3, p4))
                        (A, B) = I
                        ofst.extend(self._planarDropCutScan(pdc, A, B))
                if len(ofst) > 0:
                    stpOvr.append(ofst)
                SCANS.extend(stpOvr)
        elif obj.CutPattern == 'Line':
            stpOvr = list()
            PNTSET = self._pathGeomToLinesPointSet(obj, pathGeom)
            for D in PNTSET:
                for I in D:
                    if I == 'BRK':
                        stpOvr.append(I)
                    else:
                        # D format is ((p1, p2), (p3, p4))
                        (A, B) = I
                        stpOvr.append(self._planarDropCutScan(pdc, A, B))
                SCANS.append(stpOvr)
                stpOvr = list()
        elif obj.CutPattern == 'ZigZag':
            stpOvr = list()
            PNTSET = self._pathGeomToZigzagPointSet(obj, pathGeom)
            for (dirFlg, LNS) in PNTSET:
                for SEG in LNS:
                    if SEG == 'BRK':
                        stpOvr.append(SEG)
                    else:
                        # D format is ((p1, p2), (p3, p4))
                        (A, B) = SEG
                        stpOvr.append(self._planarDropCutScan(pdc, A, B))
                SCANS.append(stpOvr)
                stpOvr = list()
        elif obj.CutPattern in ['Circular', 'CircularZigZag']:
            # PNTSET is list, by stepover.
            # Each stepover is a list containing arc/loop descriptions, (sp, ep, cp)
            PNTSET = self._pathGeomToArcPointSet(obj, pathGeom)

            for so in range(0, len(PNTSET)):
                stpOvr = list()
                erFlg = False
                (aTyp, dirFlg, ARCS) = PNTSET[so]

                if dirFlg == 1:  # 1
                    cMode = True
                else:
                    cMode = False

                for a in range(0, len(ARCS)):
                    Arc = ARCS[a]
                    if Arc == 'BRK':
                        stpOvr.append('BRK')
                    else:
                        scan = self._planarCircularDropCutScan(pdc, Arc, cMode)
                        if scan is False:
                            erFlg = True
                        else:
                            if aTyp == 'L':
                                scan.append(FreeCAD.Vector(scan[0].x, scan[0].y, scan[0].z))
                            stpOvr.append(scan)
                if erFlg is False:
                    SCANS.append(stpOvr)

        return SCANS

    def _pathGeomToOffsetPointSet(self, obj, compGeoShp):
        '''_pathGeomToOffsetPointSet(obj, compGeoShp)...
        Convert a compound set of 3D profile segmented wires to 2D segments, applying linear optimization.'''
        PathLog.debug('_pathGeomToOffsetPointSet()')

        LINES = list()
        optimize = obj.OptimizeLinearPaths
        ofstCnt = len(compGeoShp)

        # Cycle through offeset loops
        for ei in range(0, ofstCnt):
            OS = compGeoShp[ei]
            lenOS = len(OS)

            if ei > 0:
                LINES.append('BRK')

            fp = FreeCAD.Vector(OS[0].x, OS[0].y, OS[0].z)
            OS.append(fp)

            # Cycle through points in each loop
            prev = OS[0]
            pnt = OS[1]
            for v in range(1, lenOS):
                nxt = OS[v + 1]
                if optimize is True:
                    iPOL = self.isPointOnLine(prev, nxt, pnt)
                    if iPOL is True:
                        pnt = nxt
                    else:
                        tup = ((prev.x, prev.y), (pnt.x, pnt.y))
                        LINES.append(tup)
                        prev = pnt
                        pnt = nxt
                else:
                    tup = ((prev.x, prev.y), (pnt.x, pnt.y))
                    LINES.append(tup)
                    prev = pnt
                    pnt = nxt
            if iPOL is True:
                tup = ((prev.x, prev.y), (pnt.x, pnt.y))
                LINES.append(tup)
       # Efor

        return [LINES]

    def _pathGeomToLinesPointSet(self, obj, compGeoShp):
        '''_pathGeomToLinesPointSet(obj, compGeoShp)...
        Convert a compound set of sequential line segments to directionally-oriented collinear groupings.'''
        PathLog.debug('_pathGeomToLinesPointSet()')
        # Extract intersection line segments for return value as list()
        LINES = list()
        inLine = list()
        chkGap = False
        lnCnt = 0
        ec = len(compGeoShp.Edges)
        cutClimb = self.CutClimb
        toolDiam = 2.0 * self.radius
        cpa = obj.CutPatternAngle

        edg0 = compGeoShp.Edges[0]
        p1 = (edg0.Vertexes[0].X, edg0.Vertexes[0].Y)
        p2 = (edg0.Vertexes[1].X, edg0.Vertexes[1].Y)
        if cutClimb is True:
            tup = (p2, p1)
            lst = FreeCAD.Vector(p1[0], p1[1], 0.0)
        else:
            tup = (p1, p2)
            lst = FreeCAD.Vector(p2[0], p2[1], 0.0)
        inLine.append(tup)
        sp = FreeCAD.Vector(p1[0], p1[1], 0.0)  # start point

        for ei in range(1, ec):
            chkGap = False
            edg = compGeoShp.Edges[ei]  # Get edge for vertexes
            v1 = (edg.Vertexes[0].X, edg.Vertexes[0].Y)  # vertex 0
            v2 = (edg.Vertexes[1].X, edg.Vertexes[1].Y)  # vertex 1

            ep = FreeCAD.Vector(v2[0], v2[1], 0.0)  # end point
            cp = FreeCAD.Vector(v1[0], v1[1], 0.0)  # check point (first / middle point)
            iC = self.isPointOnLine(sp, ep, cp)
            if iC is True:
                inLine.append('BRK')
                chkGap = True
            else:
                if cutClimb is True:
                    inLine.reverse()
                LINES.append(inLine)  # Save inLine segments
                lnCnt += 1
                inLine = list()  # reset collinear container
                if cutClimb is True:
                    sp = cp  # FreeCAD.Vector(v1[0], v1[1], 0.0)
                else:
                    sp = ep

            if cutClimb is True:
                tup = (v2, v1)
                if chkGap is True:
                    gap = abs(toolDiam - lst.sub(ep).Length)
                lst = cp
            else:
                tup = (v1, v2)
                if chkGap is True:
                    gap = abs(toolDiam - lst.sub(cp).Length)
                lst = ep

            if chkGap is True:
                if gap < obj.GapThreshold.Value:
                    b = inLine.pop()  # pop off 'BRK' marker
                    (vA, vB) = inLine.pop()  # pop off previous line segment for combining with current
                    tup = (vA, tup[1])
                    self.closedGap = True
                else:
                    # PathLog.debug('---- Gap: {} mm'.format(gap))
                    gap = round(gap, 6)
                    if gap < self.gaps[0]:
                        self.gaps.insert(0, gap)
                        self.gaps.pop()
            inLine.append(tup)
        # Efor
        lnCnt += 1
        if cutClimb is True:
            inLine.reverse()
        LINES.append(inLine)  # Save inLine segments

        # Handle last inLine set, reversing it.
        if obj.CutPatternReversed is True:
            if cpa != 0.0 and cpa % 90.0 == 0.0:
                F = LINES.pop(0)
                rev = list()
                for iL in F:
                    if iL == 'BRK':
                        rev.append(iL)
                    else:
                        (p1, p2) = iL
                        rev.append((p2, p1))
                rev.reverse()
                LINES.insert(0, rev)

        isEven = lnCnt % 2
        if isEven == 0:
            PathLog.debug('Line count is ODD.')
        else:
            PathLog.debug('Line count is even.')

        return LINES

    def _pathGeomToZigzagPointSet(self, obj, compGeoShp):
        '''_pathGeomToZigzagPointSet(obj, compGeoShp)...
        Convert a compound set of sequential line segments to directionally-oriented collinear groupings
        with a ZigZag directional indicator included for each collinear group.'''
        PathLog.debug('_pathGeomToZigzagPointSet()')
        # Extract intersection line segments for return value as list()
        LINES = list()
        inLine = list()
        lnCnt = 0
        chkGap = False
        ec = len(compGeoShp.Edges)
        toolDiam = 2.0 * self.radius

        if self.CutClimb is True:
            dirFlg = -1
        else:
            dirFlg = 1

        edg0 = compGeoShp.Edges[0]
        p1 = (edg0.Vertexes[0].X, edg0.Vertexes[0].Y)
        p2 = (edg0.Vertexes[1].X, edg0.Vertexes[1].Y)
        if dirFlg == 1:
            tup = (p1, p2)
            lst = FreeCAD.Vector(p2[0], p2[1], 0.0)
            sp = FreeCAD.Vector(p1[0], p1[1], 0.0)  # start point
        else:
            tup = (p2, p1)
            lst = FreeCAD.Vector(p1[0], p1[1], 0.0)
            sp = FreeCAD.Vector(p2[0], p2[1], 0.0)  # start point
        inLine.append(tup)
        otr = lst

        for ei in range(1, ec):
            edg = compGeoShp.Edges[ei]
            v1 = (edg.Vertexes[0].X, edg.Vertexes[0].Y)
            v2 = (edg.Vertexes[1].X, edg.Vertexes[1].Y)

            cp = FreeCAD.Vector(v1[0], v1[1], 0.0)  # check point (start point of segment)
            ep = FreeCAD.Vector(v2[0], v2[1], 0.0)  # end point
            iC = self.isPointOnLine(sp, ep, cp)
            if iC is True:
                inLine.append('BRK')
                chkGap = True
                gap = abs(toolDiam - lst.sub(cp).Length)
            else:
                chkGap = False
                if dirFlg == -1:
                    inLine.reverse()
                LINES.append((dirFlg, inLine))
                lnCnt += 1
                dirFlg = -1 * dirFlg  # Change zig to zag
                inLine = list()  # reset collinear container
                sp = cp  # FreeCAD.Vector(v1[0], v1[1], 0.0)
                otr = ep

            lst = ep
            if dirFlg == 1:
                tup = (v1, v2)
            else:
                tup = (v2, v1)

            if chkGap is True:
                if gap < obj.GapThreshold.Value:
                    b = inLine.pop()  # pop off 'BRK' marker
                    (vA, vB) = inLine.pop()  # pop off previous line segment for combining with current
                    if dirFlg == 1:
                        tup = (vA, tup[1])
                    else:
                        #tup = (vA, tup[1])
                        #tup = (tup[1], vA)
                        tup = (tup[0], vB)
                    self.closedGap = True
                else:
                    gap = round(gap, 6)
                    if gap < self.gaps[0]:
                        self.gaps.insert(0, gap)
                        self.gaps.pop()
            inLine.append(tup)
        # Efor
        lnCnt += 1

        # Fix directional issue with LAST line when line count is even
        isEven = lnCnt % 2
        if isEven == 0:  #  Changed to != with 90 degree CutPatternAngle
            PathLog.debug('Line count is even.')
        else:
            PathLog.debug('Line count is ODD.')
            dirFlg = -1 * dirFlg
            if obj.CutPatternReversed is False:
                if self.CutClimb is True:
                    dirFlg = -1 * dirFlg

        if obj.CutPatternReversed is True:
            dirFlg = -1 * dirFlg

        # Handle last inLine list
        if dirFlg == 1:
            rev = list()
            for iL in inLine:
                if iL == 'BRK':
                    rev.append(iL)
                else:
                    (p1, p2) = iL
                    rev.append((p2, p1))

            if obj.CutPatternReversed is False:
                rev.reverse()
            else:
                rev2 = list()
                for iL in rev:
                    if iL == 'BRK':
                        rev2.append(iL)
                    else:
                        (p1, p2) = iL
                        rev2.append((p2, p1))
                rev2.reverse()
                rev = rev2

            LINES.append((dirFlg, rev))
        else:
            LINES.append((dirFlg, inLine))

        return LINES

    def _pathGeomToArcPointSet(self, obj, compGeoShp):
        '''_pathGeomToArcPointSet(obj, compGeoShp)...
        Convert a compound set of arcs/circles to a set of directionally-oriented arc end points
        and the corresponding center point.'''
        # Extract intersection line segments for return value as list()
        PathLog.debug('_pathGeomToArcPointSet()')
        ARCS = list()
        stpOvrEI = list()
        segEI = list()
        isSame = False
        sameRad = None
        COM = self.tmpCOM
        toolDiam = 2.0 * self.radius
        ec = len(compGeoShp.Edges)

        def gapDist(sp, ep):
            X = (ep[0] - sp[0])**2
            Y = (ep[1] - sp[1])**2
            Z = (ep[2] - sp[2])**2
            # return math.sqrt(X + Y + Z)
            return math.sqrt(X + Y)  # the 'z' value is zero in both points

        # Separate arc data into Loops and Arcs
        for ei in range(0, ec):
            edg = compGeoShp.Edges[ei]
            if edg.Closed is True:
                stpOvrEI.append(('L', ei, False))
            else:
                if isSame is False:
                    segEI.append(ei)
                    isSame = True
                    pnt = FreeCAD.Vector(edg.Vertexes[0].X, edg.Vertexes[0].Y, 0.0)
                    sameRad = pnt.sub(COM).Length
                else:
                    # Check if arc is co-radial to current SEGS
                    pnt = FreeCAD.Vector(edg.Vertexes[0].X, edg.Vertexes[0].Y, 0.0)
                    if abs(sameRad - pnt.sub(COM).Length) > 0.00001:
                        isSame = False

                    if isSame is True:
                        segEI.append(ei)
                    else:
                        # Move co-radial arc segments
                        stpOvrEI.append(['A', segEI, False])
                        # Start new list of arc segments
                        segEI = [ei]
                        isSame = True
                        pnt = FreeCAD.Vector(edg.Vertexes[0].X, edg.Vertexes[0].Y, 0.0)
                        sameRad = pnt.sub(COM).Length
        # Process trailing `segEI` data, if available
        if isSame is True:
            stpOvrEI.append(['A', segEI, False])

        # Identify adjacent arcs with y=0 start/end points that connect
        for so in range(0, len(stpOvrEI)):
            SO = stpOvrEI[so]
            if SO[0] == 'A':
                startOnAxis = list()
                endOnAxis = list()
                EI = SO[1]  # list of corresponding compGeoShp.Edges indexes

                # Identify startOnAxis and endOnAxis arcs
                for i in range(0, len(EI)):
                    ei = EI[i]  # edge index
                    E = compGeoShp.Edges[ei]  # edge object
                    if abs(COM.y - E.Vertexes[0].Y) < 0.00001:
                        startOnAxis.append((i, ei, E.Vertexes[0]))
                    elif abs(COM.y - E.Vertexes[1].Y) < 0.00001:
                        endOnAxis.append((i, ei, E.Vertexes[1]))

                # Look for connections between startOnAxis and endOnAxis arcs. Consolidate data when connected
                lenSOA = len(startOnAxis)
                lenEOA = len(endOnAxis)
                if lenSOA > 0 and lenEOA > 0:
                    delIdxs = list()
                    lstFindIdx = 0
                    for soa in range(0, lenSOA):
                        (iS, eiS, vS) = startOnAxis[soa]
                        for eoa in range(0, len(endOnAxis)):
                            (iE, eiE, vE) = endOnAxis[eoa]
                            dist = vE.X - vS.X
                            if abs(dist) < 0.00001:  # They connect on axis at same radius
                                SO[2] = (eiE, eiS)
                                break
                            elif dist > 0:
                                break  # stop searching
                # Eif
            # Eif
        # Efor

        # Construct arc data tuples for OCL
        dirFlg = 1
        # cutPat = obj.CutPattern
        if self.CutClimb is False:  # True yields Climb when set to Conventional
            dirFlg = -1

        # Cycle through stepOver data
        for so in range(0, len(stpOvrEI)):
            SO = stpOvrEI[so]
            if SO[0] == 'L':  # L = Loop/Ring/Circle
                lei = SO[1]  # loop Edges index
                v1 = compGeoShp.Edges[lei].Vertexes[0]

                space = obj.SampleInterval.Value / 2.0

                p1 = FreeCAD.Vector(v1.X, v1.Y, v1.Z)
                sp = (v1.X, v1.Y, 0.0)
                rad = p1.sub(COM).Length
                tolrncAng = math.asin(space/rad)
                X = COM.x + (rad * math.cos(tolrncAng))
                Y = v1.Y - space  # rad * math.sin(tolrncAng)

                sp = (v1.X, v1.Y, 0.0)
                ep = (X, Y, 0.0)
                cp = (COM.x, COM.y, 0.0)
                if dirFlg == 1:
                    arc = (sp, ep, cp)
                else:
                    arc = (ep, sp, cp)  # OCL.Arc(firstPnt, lastPnt, centerPnt, dir=True(CCW direction))
                ARCS.append(('L', dirFlg, [arc]))
            else:  # SO[0] == 'A'    A = Arc
                PRTS = list()
                EI = SO[1]  # list of corresponding Edges indexes
                CONN = SO[2]  # list of corresponding connected edges tuples (iE, iS)
                chkGap = False
                lst = None

                if CONN is not False:
                    (iE, iS) = CONN
                    v1 = compGeoShp.Edges[iE].Vertexes[0]
                    v2 = compGeoShp.Edges[iS].Vertexes[1]
                    sp = (v1.X, v1.Y, 0.0)
                    ep = (v2.X, v2.Y, 0.0)
                    cp = (COM.x, COM.y, 0.0)
                    if dirFlg == 1:
                        arc = (sp, ep, cp)
                        lst = ep
                    else:
                        arc = (ep, sp, cp)  # OCL.Arc(firstPnt, lastPnt, centerPnt, dir=True(CCW direction))
                        lst = sp
                    PRTS.append(arc)
                    # Pop connected edge index values from arc segments index list
                    iEi = EI.index(iE)
                    iSi = EI.index(iS)
                    EI.pop(iEi)
                    EI.pop(iSi)
                    if len(EI) > 0:
                        PRTS.append('BRK')
                        chkGap = True
                cnt = 0
                for ei in EI:
                    if cnt > 0:
                        PRTS.append('BRK')
                        chkGap = True
                    v1 = compGeoShp.Edges[ei].Vertexes[0]
                    v2 = compGeoShp.Edges[ei].Vertexes[1]
                    sp = (v1.X, v1.Y, 0.0)
                    ep = (v2.X, v2.Y, 0.0)
                    cp = (COM.x, COM.y, 0.0)
                    if dirFlg == 1:
                        arc = (sp, ep, cp)
                        if chkGap is True:
                            gap = abs(toolDiam - gapDist(lst, sp))  # abs(toolDiam - lst.sub(sp).Length)
                        lst = ep
                    else:
                        arc = (ep, sp, cp)  # OCL.Arc(firstPnt, lastPnt, centerPnt, dir=True(CCW direction))
                        if chkGap is True:
                            gap = abs(toolDiam - gapDist(lst, ep))  # abs(toolDiam - lst.sub(ep).Length)
                        lst = sp
                    if chkGap is True:
                        if gap < obj.GapThreshold.Value:
                            b = PRTS.pop()  # pop off 'BRK' marker
                            (vA, vB, vC) = PRTS.pop()  # pop off previous arc segment for combining with current
                            arc = (vA, arc[1], vC)
                            self.closedGap = True
                        else:
                            # PathLog.debug('---- Gap: {} mm'.format(gap))
                            gap = round(gap, 6)
                            if gap < self.gaps[0]:
                                self.gaps.insert(0, gap)
                                self.gaps.pop()
                    PRTS.append(arc)
                    cnt += 1

                if dirFlg == -1:
                    PRTS.reverse()

                ARCS.append(('A', dirFlg, PRTS))
            # Eif
            if obj.CutPattern == 'CircularZigZag':
                dirFlg = -1 * dirFlg
        # Efor

        return ARCS

    def _planarDropCutScan(self, pdc, A, B):
        PNTS = list()
        (x1, y1) = A
        (x2, y2) = B
        path = ocl.Path()                   # create an empty path object
        p1 = ocl.Point(x1, y1, 0)   # start-point of line
        p2 = ocl.Point(x2, y2, 0)   # end-point of line
        lo = ocl.Line(p1, p2)     # line-object
        path.append(lo)        # add the line to the path
        pdc.setPath(path)
        pdc.run()  # run dropcutter algorithm on path
        CLP = pdc.getCLPoints()
        for p in CLP:
            PNTS.append(FreeCAD.Vector(p.x, p.y, p.z))
        return PNTS  # pdc.getCLPoints()

    def _planarCircularDropCutScan(self, pdc, Arc, cMode):
        PNTS = list()
        path = ocl.Path()  # create an empty path object
        (sp, ep, cp) = Arc

        # process list of segment tuples (vect, vect)
        path = ocl.Path()  # create an empty path object
        p1 = ocl.Point(sp[0], sp[1], 0)   # start point of arc
        p2 = ocl.Point(ep[0], ep[1], 0)   # end point of arc
        C = ocl.Point(cp[0], cp[1], 0)   # center point of arc
        ao = ocl.Arc(p1, p2, C, cMode)     # arc object
        path.append(ao)        # add the arc to the path
        pdc.setPath(path)
        pdc.run()  # run dropcutter algorithm on path
        CLP = pdc.getCLPoints()

        # Convert OCL object data to FreeCAD vectors
        for p in CLP:
            PNTS.append(FreeCAD.Vector(p.x, p.y, p.z))

        return PNTS

    # Main planar scan functions
    def _planarDropCutSingle(self, JOB, obj, pdc, safePDC, depthparams, SCANDATA):
        PathLog.debug('_planarDropCutSingle()')

        GCODE = [Path.Command('N (Beginning of Single-pass layer.)', {})]
        tolrnc = JOB.GeometryTolerance.Value
        prevDepth = obj.SafeHeight.Value
        lenDP = len(depthparams)
        lenSCANDATA = len(SCANDATA)
        gDIR = ['G3', 'G2']

        if self.CutClimb is True:
            gDIR = ['G2', 'G3']

        # Set `ProfileEdges` specific trigger indexes
        peIdx = lenSCANDATA  # off by default
        if obj.ProfileEdges == 'Only':
            peIdx = -1
        elif obj.ProfileEdges == 'First':
            peIdx = 0
        elif obj.ProfileEdges == 'Last':
            peIdx = lenSCANDATA - 1

        # Send cutter to x,y position of first point on first line
        first = SCANDATA[0][0][0]  # [step][item][point]
        GCODE.append(Path.Command('G0', {'X': first.x, 'Y': first.y, 'F': self.horizRapid}))

        # Cycle through step-over sections (line segments or arcs)
        odd = True
        lstStpEnd = None
        prevDepth = obj.SafeHeight.Value  # Not used for Single-pass
        for so in range(0, lenSCANDATA):
            cmds = list()
            PRTS = SCANDATA[so]
            lenPRTS = len(PRTS)
            first = PRTS[0][0]  # first point of arc/line stepover group
            start = PRTS[0][0]  # will change with each line/arc segment
            last = None
            cmds.append(Path.Command('N (Begin step {}.)'.format(so), {}))

            if so > 0:
                if obj.CutPattern == 'CircularZigZag':
                    if odd is True:
                        odd = False
                    else:
                        odd = True
                minTrnsHght = self._getMinSafeTravelHeight(safePDC, lstStpEnd, first)  # Check safe travel height against fullSTL
                # cmds.append(Path.Command('N (Transition: last, first: {}, {}:  minSTH: {})'.format(lstStpEnd, first, minTrnsHght), {}))
                cmds.extend(self._stepTransitionCmds(obj, lstStpEnd, first, minTrnsHght, tolrnc))

            # Override default `OptimizeLinearPaths` behavior to allow `ProfileEdges` optimization
            if so == peIdx or peIdx == -1:
                obj.OptimizeLinearPaths = self.preOLP

            # Cycle through current step-over parts
            for i in range(0, lenPRTS):
                prt = PRTS[i]
                lenPrt = len(prt)
                if prt == 'BRK':
                    nxtStart = PRTS[i + 1][0]
                    minSTH = self._getMinSafeTravelHeight(safePDC, last, nxtStart)  # Check safe travel height against fullSTL
                    cmds.append(Path.Command('N (Break)', {}))
                    cmds.extend(self._breakCmds(obj, last, nxtStart, minSTH, tolrnc))
                else:
                    cmds.append(Path.Command('N (part {}.)'.format(i + 1), {}))
                    start = prt[0]
                    last = prt[lenPrt - 1]
                    if so == peIdx or peIdx == -1:
                        cmds.extend(self._planarSinglepassProcess(obj, prt))
                    elif obj.CutPattern in ['Circular', 'CircularZigZag'] and obj.CircularUseG2G3 is True and lenPrt > 2:
                        (rtnVal, gcode) = self._arcsToG2G3(prt, lenPrt, odd, gDIR, tolrnc)
                        if rtnVal is True:
                            cmds.extend(gcode)
                        else:
                            cmds.extend(self._planarSinglepassProcess(obj, prt))
                    else:
                        cmds.extend(self._planarSinglepassProcess(obj, prt))
            cmds.append(Path.Command('N (End of step {}.)'.format(so), {}))
            GCODE.extend(cmds)  # save line commands
            lstStpEnd = last

            # Return `OptimizeLinearPaths` to disabled
            if so == peIdx or peIdx == -1:
                if obj.CutPattern in ['Circular', 'CircularZigZag']:
                    obj.OptimizeLinearPaths = False
        # Efor

        return GCODE

    def _planarSinglepassProcess(self, obj, PNTS):
        output = []
        optimize = obj.OptimizeLinearPaths
        lenPNTS = len(PNTS)
        lstIdx = lenPNTS - 1
        lop = None
        onLine = False

        # Initialize first three points
        nxt = None
        pnt = PNTS[0]
        prev = FreeCAD.Vector(-442064564.6, 258539656553.27, 3538553425.847)

        #  Add temp end point
        PNTS.append(FreeCAD.Vector(-4895747464.6, -25855763553.2, 35865763425))

        # Begin processing ocl points list into gcode
        for i in range(0, lenPNTS):
            # Calculate next point for consideration with current point
            nxt = PNTS[i + 1]

            # Process point
            if optimize is True:
                iPOL = self.isPointOnLine(prev, nxt, pnt)
                if iPOL is True:
                    onLine = True
                else:
                    onLine = False
                    output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))
            else:
                output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))

            # Rotate point data
            if onLine is False:
                prev = pnt
            pnt = nxt
        # Efor
        
        temp = PNTS.pop()  # Remove temp end point

        return output

    def _planarDropCutMulti(self, JOB, obj, pdc, safePDC, depthparams, SCANDATA):
        GCODE = [Path.Command('N (Beginning of Multi-pass layers.)', {})]
        tolrnc = JOB.GeometryTolerance.Value
        lenDP = len(depthparams)
        prevDepth = depthparams[0]
        lenSCANDATA = len(SCANDATA)
        gDIR = ['G3', 'G2']

        if self.CutClimb is True:
            gDIR = ['G2', 'G3']

        # Set `ProfileEdges` specific trigger indexes
        peIdx = lenSCANDATA  # off by default
        if obj.ProfileEdges == 'Only':
            peIdx = -1
        elif obj.ProfileEdges == 'First':
            peIdx = 0
        elif obj.ProfileEdges == 'Last':
            peIdx = lenSCANDATA - 1

        # Process each layer in depthparams
        prvLyrFirst = None
        prvLyrLast = None
        actvLyrs = 0
        for lyr in range(0, lenDP):
            odd = True  # ZigZag directional switch
            lyrHasCmds = False
            lstStpEnd = None
            actvSteps = 0
            LYR = list()
            prvStpFirst = None
            prvStpLast = None
            lyrDep = depthparams[lyr]

            # Cycle through step-over sections (line segments or arcs)
            for so in range(0, len(SCANDATA)):
                SO = SCANDATA[so]
                lenSO = len(SO)

                # Pre-process step-over parts for layer depth and holds
                ADJPRTS = list()
                LMAX = list()
                soHasPnts = False
                brkFlg = False
                for i in range(0, lenSO):
                    prt = SO[i]
                    lenPrt = len(prt)
                    if prt == 'BRK':
                        if brkFlg is True:
                            ADJPRTS.append(prt)
                            LMAX.append(prt)
                            brkFlg = False
                    else:
                        (PTS, lMax) = self._planarMultipassPreProcess(obj, prt, prevDepth, lyrDep)
                        if len(PTS) > 0:
                            ADJPRTS.append(PTS)
                            soHasPnts = True
                            brkFlg = True
                            LMAX.append(lMax)
                # Efor
                lenAdjPrts = len(ADJPRTS)

                # Process existing parts within current step over
                prtsHasCmds = False
                stepHasCmds = False
                prtsCmds = list()
                stpOvrCmds = list()
                transCmds = list()
                if soHasPnts is True:
                    first = ADJPRTS[0][0]  # first point of arc/line stepover group

                    # Manage step over transition and CircularZigZag direction
                    if so > 0:
                        # Control ZigZag direction
                        if obj.CutPattern == 'CircularZigZag':
                            if odd is True:
                                odd = False
                            else:
                                odd = True
                        # Control step over transition
                        minTrnsHght = self._getMinSafeTravelHeight(safePDC, prvStpLast, first, minDep=None)  # Check safe travel height against fullSTL
                        transCmds.append(Path.Command('N (--Step {} transition)'.format(so), {}))
                        transCmds.extend(self._stepTransitionCmds(obj, prvStpLast, first, minTrnsHght, tolrnc))

                    # Override default `OptimizeLinearPaths` behavior to allow `ProfileEdges` optimization
                    if so == peIdx or peIdx == -1:
                        obj.OptimizeLinearPaths = self.preOLP

                    # Cycle through current step-over parts
                    for i in range(0, lenAdjPrts):
                        prt = ADJPRTS[i]
                        lenPrt = len(prt)
                        if prt == 'BRK' and prtsHasCmds is True:
                            nxtStart = ADJPRTS[i + 1][0]
                            minSTH = self._getMinSafeTravelHeight(safePDC, last, nxtStart, minDep=None)  # Check safe travel height against fullSTL
                            prtsCmds.append(Path.Command('N (--Break)', {}))
                            prtsCmds.extend(self._breakCmds(obj, last, nxtStart, minSTH, tolrnc))
                        else:
                            segCmds = False
                            prtsCmds.append(Path.Command('N (part {})'.format(i + 1), {}))
                            last = prt[lenPrt - 1]
                            if so == peIdx or peIdx == -1:
                                segCmds = self._planarSinglepassProcess(obj, prt)
                            elif obj.CutPattern in ['Circular', 'CircularZigZag'] and obj.CircularUseG2G3 is True and lenPrt > 2:
                                (rtnVal, gcode) = self._arcsToG2G3(prt, lenPrt, odd, gDIR, tolrnc)
                                if rtnVal is True:
                                    segCmds = gcode
                                else:
                                    segCmds = self._planarSinglepassProcess(obj, prt)
                            else:
                                segCmds = self._planarSinglepassProcess(obj, prt)

                            if segCmds is not False:
                                prtsCmds.extend(segCmds)
                                prtsHasCmds = True
                                prvStpLast = last
                        # Eif
                    # Efor
                # Eif

                # Return `OptimizeLinearPaths` to disabled
                if so == peIdx or peIdx == -1:
                    if obj.CutPattern in ['Circular', 'CircularZigZag']:
                        obj.OptimizeLinearPaths = False

                # Compile step over(prts) commands
                if prtsHasCmds is True:
                    stepHasCmds = True
                    actvSteps += 1
                    prvStpFirst = first
                    stpOvrCmds.extend(transCmds)
                    stpOvrCmds.append(Path.Command('N (Begin step {}.)'.format(so), {}))
                    stpOvrCmds.append(Path.Command('G0', {'X': first.x, 'Y': first.y, 'F': self.horizRapid}))
                    stpOvrCmds.extend(prtsCmds)
                    stpOvrCmds.append(Path.Command('N (End of step {}.)'.format(so), {}))

                # Layer transition at first active step over in current layer
                if actvSteps == 1:
                    prvLyrFirst = first
                    LYR.append(Path.Command('N (Layer {} begins)'.format(lyr), {}))
                    if lyr > 0:
                        LYR.append(Path.Command('N (Layer transition)', {}))
                        LYR.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
                        LYR.append(Path.Command('G0', {'X': first.x, 'Y': first.y, 'F': self.horizRapid}))

                if stepHasCmds is True:
                    lyrHasCmds = True
                    LYR.extend(stpOvrCmds)
            # Eif

            # Close layer, saving commands, if any
            if lyrHasCmds is True:
                prvLyrLast = last
                GCODE.extend(LYR)  # save line commands
                GCODE.append(Path.Command('N (End of layer {})'.format(lyr), {}))

            # Set previous depth
            prevDepth = lyrDep
        # Efor

        PathLog.debug('Multi-pass op has {} layers (step downs).'.format(lyr + 1))

        return GCODE

    def _planarMultipassPreProcess(self, obj, LN, prvDep, layDep):
        ALL = list()
        PTS = list()
        brkFlg = False
        optLinTrans = obj.OptimizeStepOverTransitions
        safe = math.ceil(obj.SafeHeight.Value)

        if optLinTrans is True:
            for P in LN:
                ALL.append(P)
                # Handle layer depth AND hold points
                if P.z <= layDep:
                    PTS.append(FreeCAD.Vector(P.x, P.y, layDep))
                elif P.z > prvDep:
                    PTS.append(FreeCAD.Vector(P.x, P.y, safe))
                else:
                    PTS.append(FreeCAD.Vector(P.x, P.y, P.z))
            # Efor
        else:
            for P in LN:
                ALL.append(P)
                # Handle layer depth only
                if P.z <= layDep:
                    PTS.append(FreeCAD.Vector(P.x, P.y, layDep))
                else:
                    PTS.append(FreeCAD.Vector(P.x, P.y, P.z))
            # Efor
        
        if optLinTrans is True:
            # Remove leading and trailing Hold Points
            popList = list()
            for i in range(0, len(PTS)):  # identify leading string
                if PTS[i].z == safe:
                    popList.append(i)
                else:
                    break
            popList.sort(reverse=True)
            for p in popList:  # Remove hold points
                PTS.pop(p)
                ALL.pop(p)
            popList = list()
            for i in range(len(PTS) - 1, -1, -1):  # identify trailing string
                if PTS[i].z == safe:
                    popList.append(i)
                else:
                    break
            popList.sort(reverse=True)
            for p in popList:  # Remove hold points
                PTS.pop(p)
                ALL.pop(p)

        # Determine max Z height for remaining points on line
        lMax = obj.FinalDepth.Value
        if len(ALL) > 0:
            lMax = ALL[0].z
            for P in ALL:
                if P.z > lMax:
                    lMax = P.z

        return (PTS, lMax)

    def _planarMultipassProcess(self, obj, PNTS, lMax):
        output = list()
        optimize = obj.OptimizeLinearPaths
        safe = math.ceil(obj.SafeHeight.Value)
        lenPNTS = len(PNTS)
        lastPNTS = lenPNTS - 1
        prcs = True
        onHold = False
        onLine = False
        clrScnLn = lMax + 2.0

        # Initialize first three points
        nxt = None
        pnt = PNTS[0]
        prev = FreeCAD.Vector(-442064564.6, 258539656553.27, 3538553425.847)

        #  Add temp end point
        PNTS.append(FreeCAD.Vector(-4895747464.6, -25855763553.2, 35865763425))

        # Begin processing ocl points list into gcode
        for i in range(0, lenPNTS):
            prcs = True
            nxt = PNTS[i + 1]

            if pnt.z == safe:
                prcs = False
                if onHold is False:
                    onHold = True
                    output.append( Path.Command('N (Start hold)', {}) )
                    output.append( Path.Command('G0', {'Z': clrScnLn, 'F': self.vertRapid}) )
            else:
                if onHold is True:
                    onHold = False
                    output.append( Path.Command('N (End hold)', {}) )
                    output.append( Path.Command('G0', {'X': pnt.x, 'Y': pnt.y, 'F': self.horizRapid}) )

            # Process point
            if prcs is True:
                if optimize is True:
                    iPOL = self.isPointOnLine(prev, nxt, pnt)
                    if iPOL is True:
                        onLine = True
                    else:
                        onLine = False
                        output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))
                else:
                    output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))

            # Rotate point data
            if onLine is False:
                prev = pnt
            pnt = nxt
        # Efor

        temp = PNTS.pop()  # Remove temp end point

        return output

    def _stepTransitionCmds(self, obj, lstPnt, first, minSTH, tolrnc):
        cmds = list()
        rtpd = False
        horizGC = 'G0'
        hSpeed = self.horizRapid
        height = obj.SafeHeight.Value

        if obj.CutPattern in ['Line', 'Circular']:
            if obj.OptimizeStepOverTransitions is True:
                height = minSTH + 2.0
            # if obj.LayerMode == 'Multi-pass':
            #    rtpd = minSTH
        elif obj.CutPattern in ['ZigZag', 'CircularZigZag']:
            if obj.OptimizeStepOverTransitions is True:
                zChng = first.z - lstPnt.z
                # PathLog.debug('first.z: {}'.format(first.z))
                # PathLog.debug('lstPnt.z: {}'.format(lstPnt.z))
                # PathLog.debug('zChng: {}'.format(zChng))
                # PathLog.debug('minSTH: {}'.format(minSTH))
                if abs(zChng) < tolrnc:  # transitions to same Z height
                    PathLog.debug('abs(zChng) < tolrnc')
                    if (minSTH - first.z) > tolrnc:
                        PathLog.debug('(minSTH - first.z) > tolrnc')
                        height = minSTH + 2.0
                    else:
                        PathLog.debug('ELSE (minSTH - first.z) > tolrnc')
                        horizGC = 'G1'
                        height = first.z
                elif (minSTH + (2.0 * tolrnc)) >= max(first.z, lstPnt.z):
                        height = False  # allow end of Zig to cut to beginning of Zag
                    

        # Create raise, shift, and optional lower commands
        if height is not False:
            cmds.append(Path.Command('G0', {'Z': height, 'F': self.vertRapid}))
            cmds.append(Path.Command(horizGC, {'X': first.x, 'Y': first.y, 'F': hSpeed}))
        if rtpd is not False:  # ReturnToPreviousDepth
            cmds.append(Path.Command('G0', {'Z': rtpd, 'F': self.vertRapid}))

        return cmds

    def _breakCmds(self, obj, lstPnt, first, minSTH, tolrnc):
        cmds = list()
        rtpd = False
        horizGC = 'G0'
        hSpeed = self.horizRapid
        height = obj.SafeHeight.Value

        if obj.CutPattern in ['Line', 'Circular']:
            if obj.OptimizeStepOverTransitions is True:
                height = minSTH + 2.0
        elif obj.CutPattern in ['ZigZag', 'CircularZigZag']:
            if obj.OptimizeStepOverTransitions is True:
                zChng = first.z - lstPnt.z
                if abs(zChng) < tolrnc:  # transitions to same Z height
                    if (minSTH - first.z) > tolrnc:
                        height = minSTH + 2.0
                    else:
                        height = first.z + 2.0  # first.z

        cmds.append(Path.Command('G0', {'Z': height, 'F': self.vertRapid}))
        cmds.append(Path.Command(horizGC, {'X': first.x, 'Y': first.y, 'F': hSpeed}))
        if rtpd is not False:  # ReturnToPreviousDepth
            cmds.append(Path.Command('G0', {'Z': rtpd, 'F': self.vertRapid}))

        return cmds

    def _arcsToG2G3(self, LN, numPts, odd, gDIR, tolrnc):
        cmds = list()
        strtPnt = LN[0]
        endPnt = LN[numPts - 1]
        strtHght = strtPnt.z
        coPlanar = True
        isCircle = False
        inrPnt = None
        gdi = 0
        if odd is True:
            gdi = 1

        # Test if pnt set is circle
        if abs(strtPnt.x - endPnt.x) < tolrnc:
            if abs(strtPnt.y - endPnt.y) < tolrnc:
                if abs(strtPnt.z - endPnt.z) < tolrnc:
                    isCircle = True
        isCircle = False

        if isCircle is True:
            # convert LN to G2/G3 arc, consolidating GCode
            # https://wiki.shapeoko.com/index.php/G-Code#G2_-_clockwise_arc
            # https://www.cnccookbook.com/cnc-g-code-arc-circle-g02-g03/
            # Dividing circle into two arcs allows for G2/G3 on inclined surfaces

            # ijk = self.tmpCOM - strtPnt  # vector from start to center
            ijk = self.tmpCOM - strtPnt  # vector from start to center
            xyz = self.tmpCOM.add(ijk)  # end point
            cmds.append(Path.Command('G1', {'X': strtPnt.x, 'Y': strtPnt.y, 'Z': strtPnt.z, 'F': self.horizFeed}))
            cmds.append(Path.Command(gDIR[gdi], {'X': xyz.x, 'Y': xyz.y, 'Z': xyz.z,
                                                'I': ijk.x, 'J': ijk.y, 'K': ijk.z,  # leave same xyz.z height
                                                'F': self.horizFeed}))
            cmds.append(Path.Command('G1', {'X': xyz.x, 'Y': xyz.y, 'Z': xyz.z, 'F': self.horizFeed}))
            ijk = self.tmpCOM - xyz  # vector from start to center
            rst = strtPnt  # end point
            cmds.append(Path.Command(gDIR[gdi], {'X': rst.x, 'Y': rst.y, 'Z': rst.z,
                                                'I': ijk.x, 'J': ijk.y, 'K': ijk.z,  # leave same xyz.z height
                                                'F': self.horizFeed}))
            cmds.append(Path.Command('G1', {'X': strtPnt.x, 'Y': strtPnt.y, 'Z': strtPnt.z, 'F': self.horizFeed}))
        else:
            for pt in LN:
                if abs(pt.z - strtHght) > tolrnc:  # test for horizontal coplanar
                    coPlanar = False
                    break
            if coPlanar is True:
                # ijk = self.tmpCOM - strtPnt
                ijk = self.tmpCOM.sub(strtPnt)  # vector from start to center
                xyz = endPnt
                cmds.append(Path.Command('G1', {'X': strtPnt.x, 'Y': strtPnt.y, 'Z': strtPnt.z, 'F': self.horizFeed}))
                cmds.append(Path.Command(gDIR[gdi], {'X': xyz.x, 'Y': xyz.y, 'Z': xyz.z,
                                                    'I': ijk.x, 'J': ijk.y, 'K': ijk.z,  # leave same xyz.z height
                                                    'F': self.horizFeed}))
                cmds.append(Path.Command('G1', {'X': endPnt.x, 'Y': endPnt.y, 'Z': endPnt.z, 'F': self.horizFeed}))

        return (coPlanar, cmds)

    def _planarApplyDepthOffset(self, SCANDATA, DepthOffset):
        PathLog.debug('Applying DepthOffset value: {}'.format(DepthOffset))
        lenScans = len(SCANDATA)
        for s in range(0, lenScans):
            SO = SCANDATA[s]  # StepOver
            numParts = len(SO)
            for prt in range(0, numParts):
                PRT = SO[prt]
                if PRT != 'BRK':
                    numPts = len(PRT)
                    for pt in range(0, numPts):
                        SCANDATA[s][prt][pt].z += DepthOffset

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

    # Main rotational scan functions
    def _processRotationalOp(self, JOB, obj, mdlIdx, compoundFaces=None):
        PathLog.debug('_processRotationalOp(self, obj, mdlIdx, compoundFaces=None)')
        initIdx = 0.0
        final = list()

        JOB = PathUtils.findParentJob(obj)
        base = JOB.Model.Group[mdlIdx]
        bb = self.boundBoxes[mdlIdx]
        stl = self.modelSTLs[mdlIdx]

        # Rotate model to initial index
        initIdx = obj.CutterTilt + obj.StartIndex
        if initIdx != 0.0:
            self.basePlacement = FreeCAD.ActiveDocument.getObject(base.Name).Placement
            if obj.RotationAxis == 'X':
                base.Placement = FreeCAD.Placement(FreeCAD.Vector(0.0, 0.0, 0.0), FreeCAD.Rotation(FreeCAD.Vector(1.0, 0.0, 0.0), initIdx))
            else:
                base.Placement = FreeCAD.Placement(FreeCAD.Vector(0.0, 0.0, 0.0), FreeCAD.Rotation(FreeCAD.Vector(0.0, 1.0, 0.0), initIdx))

        # Prepare global holdpoint container
        if self.holdPoint is None:
            self.holdPoint = ocl.Point(float("inf"), float("inf"), float("inf"))
        if self.layerEndPnt is None:
            self.layerEndPnt = ocl.Point(float("inf"), float("inf"), float("inf"))

        # Avoid division by zero in rotational scan calculations
        if obj.FinalDepth.Value <= 0.0:
            zero = obj.SampleInterval.Value  # 0.00001
            self.FinalDepth = zero
            obj.FinalDepth.Value = 0.0
        else:
            self.FinalDepth = obj.FinalDepth.Value

        # Determine boundbox radius based upon xzy limits data
        if math.fabs(bb.ZMin) > math.fabs(bb.ZMax):
            vlim = bb.ZMin
        else:
            vlim = bb.ZMax
        if obj.RotationAxis == 'X':
            # Rotation is around X-axis, cutter moves along same axis
            if math.fabs(bb.YMin) > math.fabs(bb.YMax):
                hlim = bb.YMin
            else:
                hlim = bb.YMax
        else:
            # Rotation is around Y-axis, cutter moves along same axis
            if math.fabs(bb.XMin) > math.fabs(bb.XMax):
                hlim = bb.XMin
            else:
                hlim = bb.XMax

        # Compute max radius of stock, as it rotates, and rotational clearance & safe heights
        self.bbRadius = math.sqrt(hlim**2 + vlim**2)
        self.clearHeight = self.bbRadius + JOB.SetupSheet.ClearanceHeightOffset.Value
        self.safeHeight = self.bbRadius + JOB.SetupSheet.ClearanceHeightOffset.Value

        final = self._rotationalDropCutterOp(obj, stl, bb)

        return final

    def _rotationalDropCutterOp(self, obj, stl, bb):
        self.resetTolerance = 0.0000001  # degrees
        self.layerEndzMax = 0.0
        commands = []
        scanLines = []
        advances = []
        iSTG = []
        rSTG = []
        rings = []
        lCnt = 0
        rNum = 0
        # stepDeg = 1.1
        # layCircum = 1.1
        # begIdx = 0.0
        # endIdx = 0.0
        # arc = 0.0
        # sumAdv = 0.0
        bbRad = self.bbRadius

        def invertAdvances(advances):
            idxs = [1.1]
            for adv in advances:
                idxs.append(-1 * adv)
            idxs.pop(0)
            return idxs

        def linesToPointRings(scanLines):
            rngs = []
            numPnts = len(scanLines[0])  # Number of points per line along axis, at obj.SampleInterval.Value spacing
            for line in scanLines:  # extract circular set(ring) of points from scan lines
                if len(line) != numPnts:
                    PathLog.debug('Error: line lengths not equal')
                    return rngs

            for num in range(0, numPnts):
                rngs.append([1.1])  # Initiate new ring
                for line in scanLines:  # extract circular set(ring) of points from scan lines
                    rngs[num].append(line[num])
                rngs[num].pop(0)
            return rngs

        def indexAdvances(arc, stepDeg):
            indexes = [0.0]
            numSteps = int(math.floor(arc / stepDeg))
            for ns in range(0, numSteps):
                indexes.append(stepDeg)

            travel = sum(indexes)
            if arc == 360.0:
                indexes.insert(0, 0.0)
            else:
                indexes.append(arc - travel)

            return indexes

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == 'Single-pass':
            depthparams = [self.FinalDepth]
        else:
            dep_par = PathUtils.depth_params(self.clearHeight, self.safeHeight, self.bbRadius, obj.StepDown.Value, 0.0, self.FinalDepth)
            depthparams = [i for i in dep_par]
        prevDepth = depthparams[0]
        lenDP = len(depthparams)

        # Set drop cutter extra offset
        cdeoX = obj.DropCutterExtraOffset.x
        cdeoY = obj.DropCutterExtraOffset.y

        # Set updated bound box values and redefine the new min/mas XY area of the operation based on greatest point radius of model
        bb.ZMin = -1 * bbRad
        bb.ZMax = bbRad
        if obj.RotationAxis == 'X':
            bb.YMin = -1 * bbRad
            bb.YMax = bbRad
            ymin = 0.0
            ymax = 0.0
            xmin = bb.XMin - cdeoX
            xmax = bb.XMax + cdeoX
        else:
            bb.XMin = -1 * bbRad
            bb.XMax = bbRad
            ymin = bb.YMin - cdeoY
            ymax = bb.YMax + cdeoY
            xmin = 0.0
            xmax = 0.0

        # Calculate arc
        begIdx = obj.StartIndex
        endIdx = obj.StopIndex
        if endIdx < begIdx:
            begIdx -= 360.0
        arc = endIdx - begIdx

        # Begin gcode operation with raising cutter to safe height
        commands.append(Path.Command('G0', {'Z': self.safeHeight, 'F': self.vertRapid}))

        # Complete rotational scans at layer and translate into gcode
        for layDep in depthparams:
            t_before = time.time()

            # Compute circumference and step angles for current layer
            layCircum = 2 * math.pi * layDep
            if lenDP == 1:
                layCircum = 2 * math.pi * bbRad

            # Set axial feed rates
            self.axialFeed = 360 / layCircum * self.horizFeed
            self.axialRapid = 360 / layCircum * self.horizRapid

            # Determine step angle.
            if obj.RotationAxis == obj.DropCutterDir:  # Same == indexed
                stepDeg = (self.cutOut / layCircum) * 360.0
            else:
                stepDeg = (obj.SampleInterval.Value / layCircum) * 360.0

            # Limit step angle and determine rotational index angles [indexes].
            if stepDeg > 120.0:
                stepDeg = 120.0
            advances = indexAdvances(arc, stepDeg)  # Reset for each step down layer

            # Perform rotational indexed scans to layer depth
            if obj.RotationAxis == obj.DropCutterDir:  # Same == indexed OR parallel
                sample = obj.SampleInterval.Value
            else:
                sample = self.cutOut
            scanLines = self._indexedDropCutScan(obj, stl, advances, xmin, ymin, xmax, ymax, layDep, sample)

            # Complete rotation if necessary
            if arc == 360.0:
                advances.append(360.0 - sum(advances))
                advances.pop(0)
                zero = scanLines.pop(0)
                scanLines.append(zero)

            # Translate OCL scans into gcode
            if obj.RotationAxis == obj.DropCutterDir:  # Same == indexed (cutter runs parallel to axis)
                # Invert advances if RotationAxis == Y
                if obj.RotationAxis == 'Y':
                    advances = invertAdvances(advances)

                # Translate scan to gcode
                # sumAdv = 0.0
                sumAdv = begIdx
                for sl in range(0, len(scanLines)):
                    sumAdv += advances[sl]
                    # Translate scan to gcode
                    iSTG = self._indexedScanToGcode(obj, sl, scanLines[sl], sumAdv, prevDepth, layDep, lenDP)
                    commands.extend(iSTG)

                    # Add rise to clear height before beginning next index in CutPattern: Line
                    # if obj.CutPattern == 'Line':
                    #    commands.append(Path.Command('G0', {'Z': self.clearHeight, 'F': self.vertRapid}))

                    # Raise cutter to safe height after each index cut
                    commands.append(Path.Command('G0', {'Z': self.clearHeight, 'F': self.vertRapid}))
                # Eol
            else:
                if self.CutClimb is False:
                    advances = invertAdvances(advances)
                    advances.reverse()
                    scanLines.reverse()

                # Invert advances if RotationAxis == Y
                if obj.RotationAxis == 'Y':
                    advances = invertAdvances(advances)

                # Begin gcode operation with raising cutter to safe height
                commands.append(Path.Command('G0', {'Z': self.clearHeight, 'F': self.vertRapid}))

                # Convert rotational scans into gcode
                rings = linesToPointRings(scanLines)
                rNum = 0
                for rng in rings:
                    rSTG = self._rotationalScanToGcode(obj, rng, rNum, prevDepth, layDep, advances)
                    commands.extend(rSTG)
                    if arc != 360.0:
                        clrZ = self.layerEndzMax + self.SafeHeightOffset
                        commands.append(Path.Command('G0', {'Z': clrZ, 'F': self.vertRapid}))
                    rNum += 1
                # Eol

                # Add rise to clear height before beginning next index in CutPattern: Line
                # if obj.CutPattern == 'Line':
                #    commands.append(Path.Command('G0', {'Z': self.clearHeight, 'F': self.vertRapid}))

            prevDepth = layDep
            lCnt += 1  # increment layer count
            PathLog.debug("--Layer " + str(lCnt) + ": " + str(len(advances)) + " OCL scans and gcode in " + str(time.time() - t_before) + " s")
            time.sleep(0.2)
        # Eol
        return commands

    def _indexedDropCutScan(self, obj, stl, advances, xmin, ymin, xmax, ymax, layDep, sample):
        cutterOfst = 0.0
        # radsRot = 0.0
        # reset = 0.0
        iCnt = 0
        Lines = []
        result = None

        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setCutter(self.cutter)
        pdc.setZ(layDep)  # set minimumZ (final / ta9rget depth value)
        pdc.setSampling(sample)

        # if self.useTiltCutter == True:
        if obj.CutterTilt != 0.0:
            cutterOfst = layDep * math.sin(math.radians(obj.CutterTilt))
            PathLog.debug("CutterTilt: cutterOfst is " + str(cutterOfst))

        sumAdv = 0.0
        for adv in advances:
            sumAdv += adv
            if adv > 0.0:
                # Rotate STL object using OCL method
                radsRot = math.radians(adv)
                if obj.RotationAxis == 'X':
                    stl.rotate(radsRot, 0.0, 0.0)
                else:
                    stl.rotate(0.0, radsRot, 0.0)

            # Set STL after rotation is made
            pdc.setSTL(stl)

            # add Line objects to the path in this loop
            if obj.RotationAxis == 'X':
                p1 = ocl.Point(xmin, cutterOfst, 0.0)   # start-point of line
                p2 = ocl.Point(xmax, cutterOfst, 0.0)   # end-point of line
            else:
                p1 = ocl.Point(cutterOfst, ymin, 0.0)   # start-point of line
                p2 = ocl.Point(cutterOfst, ymax, 0.0)   # end-point of line

            # Create line object
            if obj.RotationAxis == obj.DropCutterDir:  # parallel cut
                if obj.CutPattern == 'ZigZag':
                    if (iCnt % 2 == 0.0):  # even
                        lo = ocl.Line(p1, p2)
                    else:  # odd
                        lo = ocl.Line(p2, p1)
                elif obj.CutPattern == 'Line':
                    if self.CutClimb is True:
                        lo = ocl.Line(p2, p1)
                    else:
                        lo = ocl.Line(p1, p2)
            else:
                lo = ocl.Line(p1, p2)   # line-object

            path = ocl.Path()                   # create an empty path object
            path.append(lo)         # add the line to the path
            pdc.setPath(path)       # set path
            pdc.run()               # run drop-cutter on the path
            result = pdc.getCLPoints()
            Lines.append(result)  # request the list of points

            iCnt += 1
        # End loop
        # Rotate STL object back to original position using OCL method
        reset = -1 * math.radians(sumAdv - self.resetTolerance)
        if obj.RotationAxis == 'X':
            stl.rotate(reset, 0.0, 0.0)
        else:
            stl.rotate(0.0, reset, 0.0)
        self.resetTolerance = 0.0

        return Lines

    def _indexedScanToGcode(self, obj, li, CLP, idxAng, prvDep, layerDepth, numDeps):
        # generate the path commands
        output = []
        optimize = obj.OptimizeLinearPaths
        holdCount = 0
        holdStart = False
        holdStop = False
        zMax = prvDep
        lenCLP = len(CLP)
        lastCLP = lenCLP - 1
        prev = ocl.Point(float("inf"), float("inf"), float("inf"))
        nxt = ocl.Point(float("inf"), float("inf"), float("inf"))
        pnt = ocl.Point(float("inf"), float("inf"), float("inf"))

        # Create first point
        pnt.x = CLP[0].x
        pnt.y = CLP[0].y
        pnt.z = CLP[0].z + float(obj.DepthOffset.Value)

        # Rotate to correct index location
        if obj.RotationAxis == 'X':
            output.append(Path.Command('G0', {'A': idxAng, 'F': self.axialFeed}))
        else:
            output.append(Path.Command('G0', {'B': idxAng, 'F': self.axialFeed}))

        if li > 0:
            if pnt.z > self.layerEndPnt.z:
                clrZ = pnt.z + 2.0
                output.append(Path.Command('G1', {'Z': clrZ, 'F': self.vertRapid}))
        else:
            output.append(Path.Command('G0', {'Z': self.clearHeight, 'F': self.vertRapid}))

        output.append(Path.Command('G0', {'X': pnt.x, 'Y': pnt.y, 'F': self.horizRapid}))
        output.append(Path.Command('G1', {'Z': pnt.z, 'F': self.vertFeed}))

        for i in range(0, lenCLP):
            if i < lastCLP:
                nxt.x = CLP[i + 1].x
                nxt.y = CLP[i + 1].y
                nxt.z = CLP[i + 1].z + float(obj.DepthOffset.Value)
            else:
                optimize = False

            # Update zMax values
            if pnt.z > zMax:
                zMax = pnt.z

            if obj.LayerMode == 'Multi-pass':
                # if z travels above previous layer, start/continue hold high cycle
                if pnt.z > prvDep and optimize is True:
                    if self.onHold is False:
                        holdStart = True
                    self.onHold = True

                if self.onHold is True:
                    if holdStart is True:
                        # go to current coordinate
                        output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))
                        # Save holdStart coordinate and prvDep values
                        self.holdPoint.x = pnt.x
                        self.holdPoint.y = pnt.y
                        self.holdPoint.z = pnt.z
                        holdCount += 1  # Increment hold count
                        holdStart = False  # cancel holdStart

                    # hold cutter high until Z value drops below prvDep
                    if pnt.z <= prvDep:
                        holdStop = True

                if holdStop is True:
                    # Send hold and current points to
                    zMax += 2.0
                    for cmd in self.holdStopCmds(obj, zMax, prvDep, pnt, "Hold Stop: in-line"):
                        output.append(cmd)
                    # reset necessary hold related settings
                    zMax = prvDep
                    holdStop = False
                    self.onHold = False
                    self.holdPoint = ocl.Point(float("inf"), float("inf"), float("inf"))

            if self.onHold is False:
                if not optimize or not self.isPointOnLine(FreeCAD.Vector(prev.x, prev.y, prev.z), FreeCAD.Vector(nxt.x, nxt.y, nxt.z), FreeCAD.Vector(pnt.x, pnt.y, pnt.z)):
                    output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))
                # elif i == lastCLP:
                #     output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))

            # Rotate point data
            prev.x = pnt.x
            prev.y = pnt.y
            prev.z = pnt.z
            pnt.x = nxt.x
            pnt.y = nxt.y
            pnt.z = nxt.z
        output.append(Path.Command('N (End index angle ' + str(round(idxAng, 4)) + ')', {}))

        # Save layer end point for use in transitioning to next layer
        self.layerEndPnt.x = pnt.x
        self.layerEndPnt.y = pnt.y
        self.layerEndPnt.z = pnt.z

        return output

    def _rotationalScanToGcode(self, obj, RNG, rN, prvDep, layDep, advances):
        '''_rotationalScanToGcode(obj, RNG, rN, prvDep, layDep, advances) ...
        Convert rotational scan data to gcode path commands.'''
        output = []
        nxtAng = 0
        zMax = 0.0
        # prev = ocl.Point(float("inf"), float("inf"), float("inf"))
        nxt = ocl.Point(float("inf"), float("inf"), float("inf"))
        pnt = ocl.Point(float("inf"), float("inf"), float("inf"))

        begIdx = obj.StartIndex
        endIdx = obj.StopIndex
        if endIdx < begIdx:
            begIdx -= 360.0

        # Rotate to correct index location
        axisOfRot = 'A'
        if obj.RotationAxis == 'Y':
            axisOfRot = 'B'

        # Create first point
        ang = 0.0 + obj.CutterTilt
        pnt.x = RNG[0].x
        pnt.y = RNG[0].y
        pnt.z = RNG[0].z + float(obj.DepthOffset.Value)

        # Adjust feed rate based on radius/circumference of cutter.
        # Original feed rate based on travel at circumference.
        if rN > 0:
            # if pnt.z > self.layerEndPnt.z:
            if pnt.z >= self.layerEndzMax:
                clrZ = pnt.z + 5.0
                output.append(Path.Command('G1', {'Z': clrZ, 'F': self.vertRapid}))
        else:
            output.append(Path.Command('G1', {'Z': self.clearHeight, 'F': self.vertRapid}))

        output.append(Path.Command('G0', {axisOfRot: ang, 'F': self.axialFeed}))
        output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'F': self.axialFeed}))
        output.append(Path.Command('G1', {'Z': pnt.z, 'F': self.axialFeed}))

        lenRNG = len(RNG)
        lastIdx = lenRNG - 1
        for i in range(0, lenRNG):
            if i < lastIdx:
                nxtAng = ang + advances[i + 1]
                nxt.x = RNG[i + 1].x
                nxt.y = RNG[i + 1].y
                nxt.z = RNG[i + 1].z + float(obj.DepthOffset.Value)

            if pnt.z > zMax:
                zMax = pnt.z

            output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, axisOfRot: ang, 'F': self.axialFeed}))
            pnt.x = nxt.x
            pnt.y = nxt.y
            pnt.z = nxt.z
            ang = nxtAng

        # Save layer end point for use in transitioning to next layer
        self.layerEndPnt.x = RNG[0].x
        self.layerEndPnt.y = RNG[0].y
        self.layerEndPnt.z = RNG[0].z
        self.layerEndzMax = zMax

        # Move cutter to final point
        # output.append(Path.Command('G1', {'X': self.layerEndPnt.x, 'Y': self.layerEndPnt.y, 'Z': self.layerEndPnt.z, axisOfRot: endang, 'F': self.axialFeed}))

        return output

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
        # Need to make DropCutterExtraOffset available for waterline algorithm
        # cdeoX = obj.DropCutterExtraOffset.x
        # cdeoY = obj.DropCutterExtraOffset.y
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
    setup.append('Algorithm')
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
    setup.append('DropCutterDir')
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
    setup.append('AngularDeflection')
    setup.append('LinearDeflection')
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
