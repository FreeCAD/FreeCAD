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

__title__ = "Path Surface Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and implementation of 3D Surface operation."
__contributors__ = "russ4262 (Russell Johnson)"

import FreeCAD
from PySide import QtCore

# OCL must be installed
try:
    import ocl
except ImportError:
    msg = QtCore.QCoreApplication.translate("PathSurface", "This operation requires OpenCamLib to be installed.")
    FreeCAD.Console.PrintError(msg + "\n")
    raise ImportError
    # import sys
    # sys.exit(msg)

import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
import PathScripts.PathOp as PathOp
import PathScripts.PathSurfaceSupport as PathSurfaceSupport
import time
import math

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
MeshPart = LazyLoader('MeshPart', globals(), 'MeshPart')
Part = LazyLoader('Part', globals(), 'Part')

if FreeCAD.GuiUp:
    import FreeCADGui

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectSurface(PathOp.ObjectOp):
    '''Proxy object for Surfacing operation.'''

    def baseObject(self):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        return super(self.__class__, self)

    def opFeatures(self, obj):
        '''opFeatures(obj) ... return all standard features and edges based geometries'''
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureStepDown | PathOp.FeatureCoolant | PathOp.FeatureBaseFaces

    def initOperation(self, obj):
        '''initPocketOp(obj) ... create operation specific properties'''
        self.initOpProperties(obj)

        # For debugging
        if PathLog.getLevel(PathLog.thisModule()) != 4:
            obj.setEditorMode('ShowTempObjects', 2)  # hide

        if not hasattr(obj, 'DoNotSetDefaultValues'):
            self.setEditorProperties(obj)

    def initOpProperties(self, obj, warn=False):
        '''initOpProperties(obj) ... create operation specific properties'''
        missing = list()

        for (prtyp, nm, grp, tt) in self.opProperties():
            if not hasattr(obj, nm):
                obj.addProperty(prtyp, nm, grp, tt)
                missing.append(nm)
                if warn:
                    newPropMsg = translate('PathSurface', 'New property added to') + ' "{}": '.format(obj.Label) + nm + '. '
                    newPropMsg += translate('PathSurface', 'Check its default value.')
                    PathLog.warning(newPropMsg)

        # Set enumeration lists for enumeration properties
        if len(missing) > 0:
            ENUMS = self.propertyEnumerations()
            for n in ENUMS:
                if n in missing:
                    setattr(obj, n, ENUMS[n])

        self.addedAllProperties = True

    def opProperties(self):
        '''opProperties(obj) ... Store operation specific properties'''

        return [
            ("App::PropertyBool", "ShowTempObjects", "Debug",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Show the temporary path construction objects when module is in DEBUG mode.")),

            ("App::PropertyDistance", "AngularDeflection", "Mesh Conversion",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Smaller values yield a finer, more accurate mesh. Smaller values increase processing time a lot.")),
            ("App::PropertyDistance", "LinearDeflection", "Mesh Conversion",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Smaller values yield a finer, more accurate mesh. Smaller values do not increase processing time much.")),

            ("App::PropertyFloat", "CutterTilt", "Rotation",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Stop index(angle) for rotational scan")),
            ("App::PropertyEnumeration", "DropCutterDir", "Rotation",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Dropcutter lines are created parallel to this axis.")),
            ("App::PropertyVectorDistance", "DropCutterExtraOffset", "Rotation",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Additional offset to the selected bounding box")),
            ("App::PropertyEnumeration", "RotationAxis", "Rotation",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "The model will be rotated around this axis.")),
            ("App::PropertyFloat", "StartIndex", "Rotation",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Start index(angle) for rotational scan")),
            ("App::PropertyFloat", "StopIndex", "Rotation",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Stop index(angle) for rotational scan")),

            ("App::PropertyEnumeration", "ScanType", "Surface",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Planar: Flat, 3D surface scan.  Rotational: 4th-axis rotational scan.")),

            ("App::PropertyInteger", "AvoidLastX_Faces", "Selected Geometry Settings",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Avoid cutting the last 'N' faces in the Base Geometry list of selected faces.")),
            ("App::PropertyBool", "AvoidLastX_InternalFeatures", "Selected Geometry Settings",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Do not cut internal features on avoided faces.")),
            ("App::PropertyDistance", "BoundaryAdjustment", "Selected Geometry Settings",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Positive values push the cutter toward, or beyond, the boundary. Negative values retract the cutter away from the boundary.")),
            ("App::PropertyBool", "BoundaryEnforcement", "Selected Geometry Settings",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "If true, the cutter will remain inside the boundaries of the model or selected face(s).")),
            ("App::PropertyEnumeration", "HandleMultipleFeatures", "Selected Geometry Settings",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Choose how to process multiple Base Geometry features.")),
            ("App::PropertyDistance", "InternalFeaturesAdjustment", "Selected Geometry Settings",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Positive values push the cutter toward, or into, the feature. Negative values retract the cutter away from the feature.")),
            ("App::PropertyBool", "InternalFeaturesCut", "Selected Geometry Settings",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Ignore internal feature areas within a larger selected face.")),

            ("App::PropertyEnumeration", "BoundBox", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Select the overall boundary for the operation.")),
            ("App::PropertyEnumeration", "CutMode", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Set the direction for the cutting tool to engage the material: Climb (ClockWise) or Conventional (CounterClockWise)")),
            ("App::PropertyEnumeration", "CutPattern", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Set the geometric clearing pattern to use for the operation.")),
            ("App::PropertyFloat", "CutPatternAngle", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "The yaw angle used for certain clearing patterns")),
            ("App::PropertyBool", "CutPatternReversed", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Reverse the cut order of the stepover paths. For circular cut patterns, begin at the outside and work toward the center.")),
            ("App::PropertyDistance", "DepthOffset", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Set the Z-axis depth offset from the target surface.")),
            ("App::PropertyEnumeration", "LayerMode", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Complete the operation in a single pass at depth, or mulitiple passes to final depth.")),
            ("App::PropertyVectorDistance", "PatternCenterCustom", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Set the start point for the cut pattern.")),
            ("App::PropertyEnumeration", "PatternCenterAt", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Choose location of the center point for starting the cut pattern.")),
            ("App::PropertyEnumeration", "ProfileEdges", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile the edges of the selection.")),
            ("App::PropertyDistance", "SampleInterval", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Set the sampling resolution. Smaller values quickly increase processing time.")),
            ("App::PropertyPercent", "StepOver", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Set the stepover percentage, based on the tool's diameter.")),

            ("App::PropertyBool", "OptimizeLinearPaths", "Optimization",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable optimization of linear paths (co-linear points). Removes unnecessary co-linear points from G-Code output.")),
            ("App::PropertyBool", "OptimizeStepOverTransitions", "Optimization",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable separate optimization of transitions between, and breaks within, each step over path.")),
            ("App::PropertyBool", "CircularUseG2G3", "Optimization",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Convert co-planar arcs to G2/G3 gcode commands for `Circular` and `CircularZigZag` cut patterns.")),
            ("App::PropertyDistance", "GapThreshold", "Optimization",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Collinear and co-radial artifact gaps that are smaller than this threshold are closed in the path.")),
            ("App::PropertyString", "GapSizes", "Optimization",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Feedback: three smallest gaps identified in the path geometry.")),

            ("App::PropertyVectorDistance", "StartPoint", "Start Point",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "The custom start point for the path of this operation")),
            ("App::PropertyBool", "UseStartPoint", "Start Point",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Make True, if specifying a Start Point"))
        ]

    def propertyEnumerations(self):
        # Enumeration lists for App::PropertyEnumeration properties
        return {
            'BoundBox': ['BaseBoundBox', 'Stock'],
            'PatternCenterAt': ['CenterOfMass', 'CenterOfBoundBox', 'XminYmin', 'Custom'],
            'CutMode': ['Conventional', 'Climb'],
            'CutPattern': ['Line', 'Circular', 'CircularZigZag', 'Offset', 'Spiral', 'ZigZag'],  # Additional goals ['Offset', 'ZigZagOffset', 'Grid', 'Triangle']
            'DropCutterDir': ['X', 'Y'],
            'HandleMultipleFeatures': ['Collectively', 'Individually'],
            'LayerMode': ['Single-pass', 'Multi-pass'],
            'ProfileEdges': ['None', 'Only', 'First', 'Last'],
            'RotationAxis': ['X', 'Y'],
            'ScanType': ['Planar', 'Rotational']
        }

    def setEditorProperties(self, obj):
        # Used to hide inputs in properties list

        P0 = R2 = 0  # 0 = show
        P2 = R0 = 2  # 2 = hide
        if obj.ScanType == 'Planar':
            # if obj.CutPattern in ['Line', 'ZigZag']:
            if obj.CutPattern in ['Circular', 'CircularZigZag', 'Spiral']:
                P0 = 2
                P2 = 0
            elif obj.CutPattern == 'Offset':
                P0 = 2
        elif obj.ScanType == 'Rotational':
            R2 = P0 = P2 = 2
            R0 = 0
        obj.setEditorMode('DropCutterDir', R0)
        obj.setEditorMode('DropCutterExtraOffset', R0)
        obj.setEditorMode('RotationAxis', R0)
        obj.setEditorMode('StartIndex', R0)
        obj.setEditorMode('StopIndex', R0)
        obj.setEditorMode('CutterTilt', R0)
        obj.setEditorMode('CutPattern', R2)
        obj.setEditorMode('CutPatternAngle', P0)
        obj.setEditorMode('PatternCenterAt', P2)
        obj.setEditorMode('PatternCenterCustom', P2)

    def onChanged(self, obj, prop):
        if hasattr(self, 'addedAllProperties'):
            if self.addedAllProperties is True:
                if prop == 'ScanType':
                    self.setEditorProperties(obj)
                if prop == 'CutPattern':
                    self.setEditorProperties(obj)

    def opOnDocumentRestored(self, obj):
        self.initOpProperties(obj, warn=True)

        if PathLog.getLevel(PathLog.thisModule()) != 4:
            obj.setEditorMode('ShowTempObjects', 2)  # hide
        else:
            obj.setEditorMode('ShowTempObjects', 0)  # show

        # Repopulate enumerations in case of changes
        ENUMS = self.propertyEnumerations()
        for n in ENUMS:
            restore = False
            if hasattr(obj, n):
                val = obj.getPropertyByName(n)
                restore = True
            setattr(obj, n, ENUMS[n])
            if restore:
                setattr(obj, n, val)

        self.setEditorProperties(obj)

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj, job) ... initialize defaults'''
        job = PathUtils.findParentJob(obj)

        obj.OptimizeLinearPaths = True
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
        obj.PatternCenterAt = 'CenterOfMass'  # 'CenterOfBoundBox', 'XminYmin', 'Custom'
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
        obj.PatternCenterCustom.x = 0.0
        obj.PatternCenterCustom.y = 0.0
        obj.PatternCenterCustom.z = 0.0
        obj.GapThreshold.Value = 0.005
        obj.AngularDeflection.Value = 0.25
        obj.LinearDeflection.Value = job.GeometryTolerance.Value
        # For debugging
        obj.ShowTempObjects = False

        if job.GeometryTolerance.Value == 0.0:
            PathLog.warning(translate('PathSurface', 'The GeometryTolerance for this Job is 0.0.  Initializing LinearDeflection to 0.0001 mm.'))
            obj.LinearDeflection.Value = 0.0001

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
        if obj.SampleInterval.Value < 0.0001:
            obj.SampleInterval.Value = 0.0001
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
        self.tempGroup = None
        self.CutClimb = False
        self.closedGap = False
        self.tmpCOM = None
        self.gaps = [0.1, 0.2, 0.3]
        CMDS = list()
        modelVisibility = list()
        FCAD = FreeCAD.ActiveDocument

        try:
            dotIdx = __name__.index('.') + 1
        except Exception:
            dotIdx = 0
        self.module = __name__[dotIdx:]

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

        # Identify parent Job
        JOB = PathUtils.findParentJob(obj)
        self.JOB = JOB
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
            self.commandlist.append(Path.Command('N ({})'.format(str(obj.Comment)), {}))
        self.commandlist.append(Path.Command('N ({})'.format(obj.Label), {}))
        self.commandlist.append(Path.Command('N (Tool type: {})'.format(str(obj.ToolController.Tool.ToolType)), {}))
        self.commandlist.append(Path.Command('N (Compensated Tool Path. Diameter: {})'.format(str(obj.ToolController.Tool.Diameter)), {}))
        self.commandlist.append(Path.Command('N (Sample interval: {})'.format(str(obj.SampleInterval.Value)), {}))
        self.commandlist.append(Path.Command('N (Step over %: {})'.format(str(obj.StepOver)), {}))
        self.commandlist.append(Path.Command('N ({})'.format(output), {}))
        self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        if obj.UseStartPoint is True:
            self.commandlist.append(Path.Command('G0', {'X': obj.StartPoint.x, 'Y': obj.StartPoint.y, 'F': self.horizRapid}))

        # Instantiate additional class operation variables
        self.resetOpVariables()

        # Impose property limits
        self.opApplyPropertyLimits(obj)

        # Create temporary group for temporary objects, removing existing
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
        if self.cutter is False:
            PathLog.error(translate('PathSurface', "Canceling 3D Surface operation. Error creating OCL cutter."))
            return
        self.toolDiam = self.cutter.getDiameter()
        self.radius = self.toolDiam / 2.0
        self.cutOut = (self.toolDiam * (float(obj.StepOver) / 100.0))
        self.gaps = [self.toolDiam, self.toolDiam, self.toolDiam]

        # Get height offset values for later use
        self.SafeHeightOffset = JOB.SetupSheet.SafeHeightOffset.Value
        self.ClearHeightOffset = JOB.SetupSheet.ClearanceHeightOffset.Value

        # Calculate default depthparams for operation
        self.depthParams = PathUtils.depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown.Value, 0.0, obj.FinalDepth.Value)
        self.midDep = (obj.StartDepth.Value + obj.FinalDepth.Value) / 2.0

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

        # Begin processing obj.Base data and creating GCode
        PSF = PathSurfaceSupport.ProcessSelectedFaces(JOB, obj)
        PSF.setShowDebugObjects(tempGroup, self.showDebugObjects)
        PSF.radius = self.radius
        PSF.depthParams = self.depthParams
        pPM = PSF.preProcessModel(self.module)
        # Process selected faces, if available
        if pPM is False:
            PathLog.error('Unable to pre-process obj.Base.')
        else:
            (FACES, VOIDS) = pPM
            self.modelSTLs = PSF.modelSTLs
            self.profileShapes = PSF.profileShapes

            # Create OCL.stl model objects
            self._prepareModelSTLs(JOB, obj)

            for m in range(0, len(JOB.Model.Group)):
                Mdl = JOB.Model.Group[m]
                if FACES[m] is False:
                    PathLog.error('No data for model base: {}'.format(JOB.Model.Group[m].Label))
                else:
                    if m > 0:
                        # Raise to clearance between models
                        CMDS.append(Path.Command('N (Transition to base: {}.)'.format(Mdl.Label)))
                        CMDS.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
                        PathLog.info('Working on Model.Group[{}]: {}'.format(m, Mdl.Label))
                    # make stock-model-voidShapes STL model for avoidance detection on transitions
                    self._makeSafeSTL(JOB, obj, m, FACES[m], VOIDS[m])
                    # Process model/faces - OCL objects must be ready
                    CMDS.extend(self._processCutAreas(JOB, obj, m, FACES[m], VOIDS[m]))

            # Save gcode produced
            self.commandlist.extend(CMDS)

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
            if g != self.toolDiam:
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

        execTime = time.time() - startTime
        PathLog.info('Operation time: {} sec.'.format(execTime))

        return True

    # Methods for constructing the cut area
    def _prepareModelSTLs(self, JOB, obj):
        PathLog.debug('_prepareModelSTLs()')
        for m in range(0, len(JOB.Model.Group)):
            M = JOB.Model.Group[m]

            # PathLog.debug(f" -self.modelTypes[{m}] == 'M'")
            if self.modelTypes[m] == 'M':
                # TODO: test if this works
                facets = M.Mesh.Facets.Points
            else:
                facets = Part.getFacets(M.Shape)

            if self.modelSTLs[m] is True:
                stl = ocl.STLSurf()

                for tri in facets:
                    t = ocl.Triangle(ocl.Point(tri[0][0], tri[0][1], tri[0][2]),
                                     ocl.Point(tri[1][0], tri[1][1], tri[1][2]),
                                     ocl.Point(tri[2][0], tri[2][1], tri[2][2]))
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

            if cont:
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

        fused = Part.makeCompound(fuseShapes)

        if self.showDebugObjects:
            T = FreeCAD.ActiveDocument.addObject('Part::Feature', 'safeSTLShape')
            T.Shape = fused
            T.purgeTouched()
            self.tempGroup.addObject(T)

        facets = Part.getFacets(fused)

        stl = ocl.STLSurf()
        for tri in facets:
            t = ocl.Triangle(ocl.Point(tri[0][0], tri[0][1], tri[0][2]),
                             ocl.Point(tri[1][0], tri[1][1], tri[1][2]),
                             ocl.Point(tri[2][0], tri[2][1], tri[2][2]))
            stl.addTriangle(t)

        self.safeSTLs[mdlIdx] = stl

    def _processCutAreas(self, JOB, obj, mdlIdx, FCS, VDS):
        '''_processCutAreas(JOB, obj, mdlIdx, FCS, VDS)...
        This method applies any avoided faces or regions to the selected faces.
        It then calls the correct scan method depending on the ScanType property.'''
        PathLog.debug('_processCutAreas()')

        final = list()

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

            if obj.ScanType == 'Planar':
                final.extend(self._processPlanarOp(JOB, obj, mdlIdx, COMP, 0))
            elif obj.ScanType == 'Rotational':
                final.extend(self._processRotationalOp(JOB, obj, mdlIdx, COMP))

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

                if obj.ScanType == 'Planar':
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

        def getTransition(two):
            first = two[0][0][0]  # [step][item][point]
            safe = obj.SafeHeight.Value + 0.1
            trans = [[FreeCAD.Vector(first.x, first.y, safe)]]
            return trans

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == 'Single-pass':
            depthparams = [obj.FinalDepth.Value]
        elif obj.LayerMode == 'Multi-pass':
            depthparams = [i for i in self.depthParams]
        lenDP = len(depthparams)

        # Prepare PathDropCutter objects with STL data
        pdc = self._planarGetPDC(self.modelSTLs[mdlIdx], depthparams[lenDP - 1], obj.SampleInterval.Value, self.cutter)
        safePDC = self._planarGetPDC(self.safeSTLs[mdlIdx], depthparams[lenDP - 1], obj.SampleInterval.Value, self.cutter)

        profScan = list()
        if obj.ProfileEdges != 'None':
            prflShp = self.profileShapes[mdlIdx][fsi]
            if prflShp is False:
                PathLog.error('No profile shape is False.')
                return list()
            if self.showDebugObjects:
                P = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpNewProfileShape')
                P.Shape = prflShp
                P.purgeTouched()
                self.tempGroup.addObject(P)
            # get offset path geometry and perform OCL scan with that geometry
            pathOffsetGeom = self._offsetFacesToPointData(obj, prflShp)
            if pathOffsetGeom is False:
                PathLog.error('No profile geometry returned.')
                return list()
            profScan = [self._planarPerformOclScan(obj, pdc, pathOffsetGeom, True)]

        geoScan = list()
        if obj.ProfileEdges != 'Only':
            if self.showDebugObjects:
                F = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpCutArea')
                F.Shape = cmpdShp
                F.purgeTouched()
                self.tempGroup.addObject(F)
            # get internal path geometry and perform OCL scan with that geometry
            PGG = PathSurfaceSupport.PathGeometryGenerator(obj, cmpdShp, obj.CutPattern)
            if self.showDebugObjects:
                PGG.setDebugObjectsGroup(self.tempGroup)
            self.tmpCOM = PGG.getCenterOfPattern()
            pathGeom = PGG.generatePathGeometry()
            if pathGeom is False:
                PathLog.error('No path geometry returned.')
                return list()
            if obj.CutPattern == 'Offset':
                useGeom = self._offsetFacesToPointData(obj, pathGeom, profile=False)
                if useGeom is False:
                    PathLog.error('No profile geometry returned.')
                    return list()
                geoScan = [self._planarPerformOclScan(obj, pdc, useGeom, True)]
            else:
                geoScan = self._planarPerformOclScan(obj, pdc, pathGeom, False)

        if obj.ProfileEdges == 'Only':  # ['None', 'Only', 'First', 'Last']
            SCANDATA.extend(profScan)
        if obj.ProfileEdges == 'None':
            SCANDATA.extend(geoScan)
        if obj.ProfileEdges == 'First':
            profScan.append(getTransition(geoScan))
            SCANDATA.extend(profScan)
            SCANDATA.extend(geoScan)
        if obj.ProfileEdges == 'Last':
            SCANDATA.extend(geoScan)
            SCANDATA.extend(profScan)

        if len(SCANDATA) == 0:
            PathLog.error('No scan data to convert to Gcode.')
            return list()

        # Apply depth offset
        if obj.DepthOffset.Value != 0.0:
            self._planarApplyDepthOffset(SCANDATA, obj.DepthOffset.Value)

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

    def _offsetFacesToPointData(self, obj, subShp, profile=True):
        PathLog.debug('_offsetFacesToPointData()')

        offsetLists = list()
        dist = obj.SampleInterval.Value / 5.0
        # defl = obj.SampleInterval.Value / 5.0

        if not profile:
            # Reverse order of wires in each face - inside to outside
            for w in range(len(subShp.Wires) - 1, -1, -1):
                W = subShp.Wires[w]
                PNTS = W.discretize(Distance=dist)
                # PNTS = W.discretize(Deflection=defl)
                if self.CutClimb:
                    PNTS.reverse()
                offsetLists.append(PNTS)
        else:
            # Reference https://forum.freecadweb.org/viewtopic.php?t=28861#p234939
            for fc in subShp.Faces:
                # Reverse order of wires in each face - inside to outside
                for w in range(len(fc.Wires) - 1, -1, -1):
                    W = fc.Wires[w]
                    PNTS = W.discretize(Distance=dist)
                    # PNTS = W.discretize(Deflection=defl)
                    if self.CutClimb:
                        PNTS.reverse()
                    offsetLists.append(PNTS)

        return offsetLists

    def _planarPerformOclScan(self, obj, pdc, pathGeom, offsetPoints=False):
        '''_planarPerformOclScan(obj, pdc, pathGeom, offsetPoints=False)...
        Switching function for calling the appropriate path-geometry to OCL points conversion function
        for the various cut patterns.'''
        PathLog.debug('_planarPerformOclScan()')
        SCANS = list()

        if offsetPoints or obj.CutPattern == 'Offset':
            PNTSET = PathSurfaceSupport.pathGeomToOffsetPointSet(obj, pathGeom)
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
        elif obj.CutPattern in ['Line', 'Spiral', 'ZigZag']:
            stpOvr = list()
            if obj.CutPattern == 'Line':
                PNTSET = PathSurfaceSupport.pathGeomToLinesPointSet(obj, pathGeom, self.CutClimb, self.toolDiam, self.closedGap, self.gaps)
            elif obj.CutPattern == 'ZigZag':
                PNTSET = PathSurfaceSupport.pathGeomToZigzagPointSet(obj, pathGeom, self.CutClimb, self.toolDiam, self.closedGap, self.gaps)
            elif obj.CutPattern == 'Spiral':
                PNTSET = PathSurfaceSupport.pathGeomToSpiralPointSet(obj, pathGeom)

            for STEP in PNTSET:
                for LN in STEP:
                    if LN == 'BRK':
                        stpOvr.append(LN)
                    else:
                        # D format is ((p1, p2), (p3, p4))
                        (A, B) = LN
                        stpOvr.append(self._planarDropCutScan(pdc, A, B))
                SCANS.append(stpOvr)
                stpOvr = list()
        elif obj.CutPattern in ['Circular', 'CircularZigZag']:
            # PNTSET is list, by stepover.
            # Each stepover is a list containing arc/loop descriptions, (sp, ep, cp)
            PNTSET = PathSurfaceSupport.pathGeomToCircularPointSet(obj, pathGeom, self.CutClimb, self.toolDiam, self.closedGap, self.gaps, self.tmpCOM)

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
        # Eif

        return SCANS

    def _planarDropCutScan(self, pdc, A, B):
        #PNTS = list()
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
        PNTS = [FreeCAD.Vector(p.x, p.y, p.z) for p in CLP]
        return PNTS  # pdc.getCLPoints()

    def _planarCircularDropCutScan(self, pdc, Arc, cMode):
        PNTS = list()
        path = ocl.Path()  # create an empty path object
        (sp, ep, cp) = Arc

        # process list of segment tuples (vect, vect)
        p1 = ocl.Point(sp[0], sp[1], 0)   # start point of arc
        p2 = ocl.Point(ep[0], ep[1], 0)   # end point of arc
        C = ocl.Point(cp[0], cp[1], 0)   # center point of arc
        ao = ocl.Arc(p1, p2, C, cMode)     # arc object
        path.append(ao)        # add the arc to the path
        pdc.setPath(path)
        pdc.run()  # run dropcutter algorithm on path
        CLP = pdc.getCLPoints()

        # Convert OCL object data to FreeCAD vectors
        return [FreeCAD.Vector(p.x, p.y, p.z) for p in CLP]

    # Main planar scan functions
    def _planarDropCutSingle(self, JOB, obj, pdc, safePDC, depthparams, SCANDATA):
        PathLog.debug('_planarDropCutSingle()')

        GCODE = [Path.Command('N (Beginning of Single-pass layer.)', {})]
        tolrnc = JOB.GeometryTolerance.Value
        lenSCANDATA = len(SCANDATA)
        gDIR = ['G3', 'G2']

        if self.CutClimb:
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
                    if odd:
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
                        if rtnVal:
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
            if optimize:
                if pnt.isOnLineSegment(prev, nxt):
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
        
        PNTS.pop()  # Remove temp end point

        return output

    def _planarDropCutMulti(self, JOB, obj, pdc, safePDC, depthparams, SCANDATA):
        GCODE = [Path.Command('N (Beginning of Multi-pass layers.)', {})]
        tolrnc = JOB.GeometryTolerance.Value
        lenDP = len(depthparams)
        prevDepth = depthparams[0]
        lenSCANDATA = len(SCANDATA)
        gDIR = ['G3', 'G2']

        if self.CutClimb:
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
        lastPrvStpLast = None
        for lyr in range(0, lenDP):
            odd = True  # ZigZag directional switch
            lyrHasCmds = False
            actvSteps = 0
            LYR = list()
            prvStpFirst = None
            if lyr > 0:
                if prvStpLast is not None:
                    lastPrvStpLast = prvStpLast
            prvStpLast = None
            lyrDep = depthparams[lyr]
            PathLog.debug('Multi-pass lyrDep: {}'.format(round(lyrDep, 4)))

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
                        if brkFlg:
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
                        # PathLog.debug('  stepover index: {}'.format(so))
                        # Control ZigZag direction
                        if obj.CutPattern == 'CircularZigZag':
                            if odd is True:
                                odd = False
                            else:
                                odd = True
                        # Control step over transition
                        if prvStpLast is None:
                            prvStpLast = lastPrvStpLast
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
                        # PathLog.debug('  adj parts index - lenPrt: {} - {}'.format(i, lenPrt))
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
                    # iPOL = prev.isOnLineSegment(nxt, pnt)
                    iPOL = pnt.isOnLineSegment(prev, nxt)
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

        PNTS.pop()  # Remove temp end point

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

    def _planarGetPDC(self, stl, finalDep, SampleInterval, cutter):
        pdc = ocl.PathDropCutter()   # create a pdc [PathDropCutter] object
        pdc.setSTL(stl)  # add stl model
        pdc.setCutter(cutter)  # add cutter
        pdc.setZ(finalDep)  # set minimumZ (final / target depth value)
        pdc.setSampling(SampleInterval)  # set sampling size
        return pdc


    # Main rotational scan functions
    def _processRotationalOp(self, JOB, obj, mdlIdx, compoundFaces=None):
        PathLog.debug('_processRotationalOp(self, JOB, obj, mdlIdx, compoundFaces=None)')

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
            self.holdPoint = FreeCAD.Vector(0.0, 0.0, 0.0)
        if self.layerEndPnt is None:
            self.layerEndPnt = FreeCAD.Vector(0.0, 0.0, 0.0)

        # Avoid division by zero in rotational scan calculations
        if obj.FinalDepth.Value == 0.0:
            zero = obj.SampleInterval.Value  # 0.00001
            self.FinalDepth = zero
            # obj.FinalDepth.Value = 0.0
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

        return self._rotationalDropCutterOp(obj, stl, bb)

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

                # Translate scan to gcode
                sumAdv = begIdx
                for sl in range(0, len(scanLines)):
                    sumAdv += advances[sl]
                    # Translate scan to gcode
                    iSTG = self._indexedScanToGcode(obj, sl, scanLines[sl], sumAdv, prevDepth, layDep, lenDP)
                    commands.extend(iSTG)

                    # Raise cutter to safe height after each index cut
                    commands.append(Path.Command('G0', {'Z': self.clearHeight, 'F': self.vertRapid}))
                # Eol
            else:
                if self.CutClimb is False:
                    advances = invertAdvances(advances)
                    advances.reverse()
                    scanLines.reverse()

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

            prevDepth = layDep
            lCnt += 1  # increment layer count
            PathLog.debug("--Layer " + str(lCnt) + ": " + str(len(advances)) + " OCL scans and gcode in " + str(time.time() - t_before) + " s")
        # Eol

        return commands

    def _indexedDropCutScan(self, obj, stl, advances, xmin, ymin, xmax, ymax, layDep, sample):
        cutterOfst = 0.0
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
            result = pdc.getCLPoints()  # request the list of points

            # Convert list of OCL objects to list of Vectors for faster access and Apply depth offset
            if obj.DepthOffset.Value != 0.0:
                Lines.append([FreeCAD.Vector(p.x, p.y, p.z + obj.DepthOffset.Value) for p in result])
            else:
                Lines.append([FreeCAD.Vector(p.x, p.y, p.z) for p in result])

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
        prev = FreeCAD.Vector(0.0, 0.0, 0.0)
        nxt = FreeCAD.Vector(0.0, 0.0, 0.0)

        # Create first point
        pnt = CLP[0]

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
                nxt = CLP[i + 1]
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
                        self.holdPoint = pnt
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
                    self.holdPoint = FreeCAD.Vector(0.0, 0.0, 0.0)

            if self.onHold is False:
                if not optimize or not pnt.isOnLineSegment(prev, nxt):
                    output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, 'F': self.horizFeed}))

            # Rotate point data
            prev = pnt
            pnt = nxt
        output.append(Path.Command('N (End index angle ' + str(round(idxAng, 4)) + ')', {}))

        # Save layer end point for use in transitioning to next layer
        self.layerEndPnt = pnt

        return output

    def _rotationalScanToGcode(self, obj, RNG, rN, prvDep, layDep, advances):
        '''_rotationalScanToGcode(obj, RNG, rN, prvDep, layDep, advances) ...
        Convert rotational scan data to gcode path commands.'''
        output = []
        nxtAng = 0
        zMax = 0.0
        nxt = FreeCAD.Vector(0.0, 0.0, 0.0)

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
        pnt = RNG[0]

        # Adjust feed rate based on radius/circumference of cutter.
        # Original feed rate based on travel at circumference.
        if rN > 0:
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
                nxt = RNG[i + 1]

            if pnt.z > zMax:
                zMax = pnt.z

            output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'Z': pnt.z, axisOfRot: ang, 'F': self.axialFeed}))
            pnt = nxt
            ang = nxtAng

        # Save layer end point for use in transitioning to next layer
        self.layerEndPnt = RNG[0]
        self.layerEndzMax = zMax

        return output

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

    # Additional support methods
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

    def _getMinSafeTravelHeight(self, pdc, p1, p2, minDep=None):
        A = (p1.x, p1.y)
        B = (p2.x, p2.y)
        LINE = self._planarDropCutScan(pdc, A, B)
        zMax = max([obj.z for obj in LINE])
        if minDep is not None:
            if zMax < minDep:
                zMax = minDep
        return zMax


def SetupProperties():
    ''' SetupProperties() ... Return list of properties required for operation.'''
    setup = ['AvoidLastX_Faces', 'AvoidLastX_InternalFeatures', 'BoundBox']
    setup.extend(['BoundaryAdjustment', 'PatternCenterAt', 'PatternCenterCustom'])
    setup.extend(['CircularUseG2G3', 'InternalFeaturesCut', 'InternalFeaturesAdjustment'])
    setup.extend(['CutMode', 'CutPattern', 'CutPatternAngle', 'CutPatternReversed'])
    setup.extend(['CutterTilt', 'DepthOffset', 'DropCutterDir', 'GapSizes', 'GapThreshold'])
    setup.extend(['HandleMultipleFeatures', 'LayerMode', 'OptimizeStepOverTransitions'])
    setup.extend(['ProfileEdges', 'BoundaryEnforcement', 'RotationAxis', 'SampleInterval'])
    setup.extend(['ScanType', 'StartIndex', 'StartPoint', 'StepOver', 'StopIndex'])
    setup.extend(['UseStartPoint', 'AngularDeflection', 'LinearDeflection', 'ShowTempObjects'])
    return setup


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Surface operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectSurface(obj, name)
    return obj
