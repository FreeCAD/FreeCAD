# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Russell Johnson (russ4262) <russ4262@gmail.com>    *
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
import PathScripts.PathOp as PathOp

from PySide import QtCore
import time
import math

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
MeshPart = LazyLoader('MeshPart', globals(), 'MeshPart')
Draft = LazyLoader('Draft', globals(), 'Draft')
Part = LazyLoader('Part', globals(), 'Part')

if FreeCAD.GuiUp:
    import FreeCADGui

__title__ = "Path Waterline Operation"
__author__ = "russ4262 (Russell Johnson), sliptonic (Brad Collette)"
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
        translate("Path_Waterline", "This operation requires OpenCamLib to be installed.") + "\n")
    import sys
    sys.exit(translate("Path_Waterline", "This operation requires OpenCamLib to be installed."))


class ObjectWaterline(PathOp.ObjectOp):
    '''Proxy object for Surfacing operation.'''

    def baseObject(self):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        return super(self.__class__, self)

    def opFeatures(self, obj):
        '''opFeatures(obj) ... return all standard features and edges based geomtries'''
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureStepDown | PathOp.FeatureCoolant | PathOp.FeatureBaseFaces

    def initOperation(self, obj):
        '''initPocketOp(obj) ...
        Initialize the operation - property creation and property editor status.'''
        self.initOpProperties(obj)

        # For debugging
        if PathLog.getLevel(PathLog.thisModule()) != 4:
            obj.setEditorMode('ShowTempObjects', 2)  # hide

        if not hasattr(obj, 'DoNotSetDefaultValues'):
            self.setEditorProperties(obj)

    def initOpProperties(self, obj):
        '''initOpProperties(obj) ... create operation specific properties'''
        PROPS = [
            ("App::PropertyBool", "ShowTempObjects", "Debug",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Show the temporary path construction objects when module is in DEBUG mode.")),

            ("App::PropertyDistance", "AngularDeflection", "Mesh Conversion",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Smaller values yield a finer, more accurate the mesh. Smaller values increase processing time a lot.")),
            ("App::PropertyDistance", "LinearDeflection", "Mesh Conversion",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Smaller values yield a finer, more accurate the mesh. Smaller values do not increase processing time much.")),

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

            ("App::PropertyEnumeration", "Algorithm", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Select the algorithm to use: OCL Dropcutter*, or Experimental.")),
            ("App::PropertyEnumeration", "BoundBox", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Select the overall boundary for the operation. ")),
            ("App::PropertyVectorDistance", "CircularCenterCustom", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Set the start point for circular cut patterns.")),
            ("App::PropertyEnumeration", "CircularCenterAt", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Choose location of the center point for starting the circular pattern.")),
            ("App::PropertyEnumeration", "ClearLastLayer", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Set to clear last layer in a `Multi-pass` operation.")),
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
            ("App::PropertyDistance", "GapThreshold", "Optimization",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Collinear and co-radial artifact gaps that are smaller than this threshold are closed in the path.")),
            ("App::PropertyString", "GapSizes", "Optimization",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Feedback: three smallest gaps identified in the path geometry.")),

            ("App::PropertyVectorDistance", "StartPoint", "Start Point",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "The custom start point for the path of this operation")),
            ("App::PropertyBool", "UseStartPoint", "Start Point",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Make True, if specifying a Start Point"))
        ]

        missing = list()
        for (prtyp, nm, grp, tt) in PROPS:
            if not hasattr(obj, nm):
                obj.addProperty(prtyp, nm, grp, tt)
                missing.append(nm)

        # Set enumeration lists for enumeration properties
        if len(missing) > 0:
            ENUMS = self._propertyEnumerations()
            for n in ENUMS:
                if n in missing:
                    cmdStr = 'obj.{}={}'.format(n, ENUMS[n])
                    exec(cmdStr)

        self.addedAllProperties = True

    def _propertyEnumerations(self):
        # Enumeration lists for App::PropertyEnumeration properties
        return {
            'Algorithm': ['OCL Dropcutter', 'Experimental'],
            'BoundBox': ['BaseBoundBox', 'Stock'],
            'CircularCenterAt': ['CenterOfMass', 'CenterOfBoundBox', 'XminYmin', 'Custom'],
            'ClearLastLayer': ['Off', 'Line', 'Circular', 'CircularZigZag', 'Offset', 'ZigZag'],
            'CutMode': ['Conventional', 'Climb'],
            'CutPattern': ['None', 'Line', 'Circular', 'CircularZigZag', 'Offset', 'ZigZag'],  # Additional goals ['Offset', 'Spiral', 'ZigZagOffset', 'Grid', 'Triangle']
            'HandleMultipleFeatures': ['Collectively', 'Individually'],
            'LayerMode': ['Single-pass', 'Multi-pass'],
            'ProfileEdges': ['None', 'Only', 'First', 'Last'],
        }

    def setEditorProperties(self, obj):
        # Used to hide inputs in properties list
        show = 0
        hide = 2
        cpShow = 0
        expMode = 0
        obj.setEditorMode('BoundaryEnforcement', hide)
        obj.setEditorMode('ProfileEdges', hide)
        obj.setEditorMode('InternalFeaturesAdjustment', hide)
        obj.setEditorMode('InternalFeaturesCut', hide)
        obj.setEditorMode('GapSizes', hide)
        obj.setEditorMode('GapThreshold', hide)
        obj.setEditorMode('AvoidLastX_Faces', hide)
        obj.setEditorMode('AvoidLastX_InternalFeatures', hide)
        obj.setEditorMode('BoundaryAdjustment', hide)
        obj.setEditorMode('HandleMultipleFeatures', hide)
        if hasattr(obj, 'EnableRotation'):
            obj.setEditorMode('EnableRotation', hide)
        if obj.CutPattern == 'None':
            show = 2
            hide = 2
            cpShow = 2
        # elif obj.CutPattern in ['Line', 'ZigZag']:
        #    show = 0
        #    hide = 2
        elif obj.CutPattern in ['Circular', 'CircularZigZag']:
            show = 2  # hide
            hide = 0  # show
        # obj.setEditorMode('StepOver', cpShow)
        obj.setEditorMode('CutPatternAngle', show)
        obj.setEditorMode('CircularCenterAt', hide)
        obj.setEditorMode('CircularCenterCustom', hide)
        if obj.Algorithm == 'Experimental':
            expMode = 2
        obj.setEditorMode('SampleInterval', expMode)
        obj.setEditorMode('LinearDeflection', expMode)
        obj.setEditorMode('AngularDeflection', expMode)

    def onChanged(self, obj, prop):
        if hasattr(self, 'addedAllProperties'):
            if self.addedAllProperties is True:
                if prop in ['Algorithm', 'CutPattern']:
                    self.setEditorProperties(obj)

    def opOnDocumentRestored(self, obj):
        self.initOpProperties(obj)

        if PathLog.getLevel(PathLog.thisModule()) != 4:
            obj.setEditorMode('ShowTempObjects', 2)  # hide
        else:
            obj.setEditorMode('ShowTempObjects', 0)  # show

        self.setEditorProperties(obj)

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj, job) ... initialize defaults'''
        job = PathUtils.findParentJob(obj)

        obj.OptimizeLinearPaths = True
        obj.InternalFeaturesCut = True
        obj.OptimizeStepOverTransitions = False
        obj.BoundaryEnforcement = True
        obj.UseStartPoint = False
        obj.AvoidLastX_InternalFeatures = True
        obj.CutPatternReversed = False
        obj.StartPoint.x = 0.0
        obj.StartPoint.y = 0.0
        obj.StartPoint.z = obj.ClearanceHeight.Value
        obj.Algorithm = 'OCL Dropcutter'
        obj.ProfileEdges = 'None'
        obj.LayerMode = 'Single-pass'
        obj.CutMode = 'Conventional'
        obj.CutPattern = 'None'
        obj.HandleMultipleFeatures = 'Collectively'  # 'Individually'
        obj.CircularCenterAt = 'CenterOfMass'  # 'CenterOfBoundBox', 'XminYmin', 'Custom'
        obj.GapSizes = 'No gaps identified.'
        obj.ClearLastLayer = 'Off'
        obj.StepOver = 100
        obj.CutPatternAngle = 0.0
        obj.DepthOffset.Value = 0.0
        obj.SampleInterval.Value = 1.0
        obj.BoundaryAdjustment.Value = 0.0
        obj.InternalFeaturesAdjustment.Value = 0.0
        obj.AvoidLastX_Faces = 0
        obj.CircularCenterCustom.x = 0.0
        obj.CircularCenterCustom.y = 0.0
        obj.CircularCenterCustom.z = 0.0
        obj.GapThreshold.Value = 0.005
        obj.LinearDeflection.Value = 0.0001
        obj.AngularDeflection.Value = 0.25
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
        # Limit sample interval
        if obj.SampleInterval.Value < 0.001:
            obj.SampleInterval.Value = 0.001
            PathLog.error(translate('PathWaterline', 'Sample interval limits are 0.001 to 25.4 millimeters.'))
        if obj.SampleInterval.Value > 25.4:
            obj.SampleInterval.Value = 25.4
            PathLog.error(translate('PathWaterline', 'Sample interval limits are 0.001 to 25.4 millimeters.'))

        # Limit cut pattern angle
        if obj.CutPatternAngle < -360.0:
            obj.CutPatternAngle = 0.0
            PathLog.error(translate('PathWaterline', 'Cut pattern angle limits are +-360 degrees.'))
        if obj.CutPatternAngle >= 360.0:
            obj.CutPatternAngle = 0.0
            PathLog.error(translate('PathWaterline', 'Cut pattern angle limits are +- 360 degrees.'))

        # Limit StepOver to natural number percentage
        if obj.StepOver > 100:
            obj.StepOver = 100
        if obj.StepOver < 1:
            obj.StepOver = 1

        # Limit AvoidLastX_Faces to zero and positive values
        if obj.AvoidLastX_Faces < 0:
            obj.AvoidLastX_Faces = 0
            PathLog.error(translate('PathWaterline', 'AvoidLastX_Faces: Only zero or positive values permitted.'))
        if obj.AvoidLastX_Faces > 100:
            obj.AvoidLastX_Faces = 100
            PathLog.error(translate('PathWaterline', 'AvoidLastX_Faces: Avoid last X faces count limited to 100.'))

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
        self.geoTlrnc = None
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
        PathLog.info('\nBegin Waterline operation...')
        startTime = time.time()

        # Identify parent Job
        JOB = PathUtils.findParentJob(obj)
        if JOB is None:
            PathLog.error(translate('PathWaterline', "No JOB"))
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
        # if self.showDebugObjects is True:
        tempGroupName = 'tempPathWaterlineGroup'
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
            PathLog.error(translate('PathWaterline', "Canceling Waterline operation. Error creating OCL cutter."))
            return
        toolDiam = self.cutter.getDiameter()
        self.cutOut = (toolDiam * (float(obj.StepOver) / 100.0))
        self.radius = toolDiam / 2.0
        self.gaps = [toolDiam, toolDiam, toolDiam]

        # Get height offset values for later use
        self.SafeHeightOffset = JOB.SetupSheet.SafeHeightOffset.Value
        self.ClearHeightOffset = JOB.SetupSheet.ClearanceHeightOffset.Value

        # Set deflection values for mesh generation
        useDGT = False
        try:  # try/except is for Path Jobs created before GeometryTolerance
            self.geoTlrnc = JOB.GeometryTolerance.Value
            if self.geoTlrnc == 0.0:
                useDGT = True
        except AttributeError as ee:
            PathLog.warning('{}\nPlease set Job.GeometryTolerance to an acceptable value. Using PathPreferences.defaultGeometryTolerance().'.format(ee))
            useDGT = True
        if useDGT:
            import PathScripts.PathPreferences as PathPreferences
            self.geoTlrnc = PathPreferences.defaultGeometryTolerance()

        # Calculate default depthparams for operation
        self.depthParams = PathUtils.depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown.Value, 0.0, obj.FinalDepth.Value)
        self.midDep = (obj.StartDepth.Value + obj.FinalDepth.Value) / 2.0

        # make circle for workplane
        self.wpc = Part.makeCircle(2.0)

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
        # Process selected faces, if available
        pPM = self._preProcessModel(JOB, obj)
        if pPM is False:
            PathLog.error('Unable to pre-process obj.Base.')
        else:
            (FACES, VOIDS) = pPM

            # Create OCL.stl model objects
            if obj.Algorithm == 'OCL Dropcutter':
                self._prepareModelSTLs(JOB, obj)
                PathLog.debug('obj.LinearDeflection.Value: {}'.format(obj.LinearDeflection.Value))
                PathLog.debug('obj.AngularDeflection.Value: {}'.format(obj.AngularDeflection.Value))

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
                    if obj.Algorithm == 'OCL Dropcutter':
                        self._makeSafeSTL(JOB, obj, m, FACES[m], VOIDS[m])
                    # time.sleep(0.2)
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
        preProcEr = translate('PathWaterline', 'Error pre-processing Face')
        warnFinDep = translate('PathWaterline', 'Final Depth might need to be lower. Internal features detected in Face')
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
        # preProcEr = translate('PathWaterline', 'Error pre-processing Face')
        warnFinDep = translate('PathWaterline', 'Final Depth might need to be lower. Internal features detected in Face')

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
        # time.sleep(0.2)

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

    def _extractFaceOffset(self, obj, fcShape, offset, makeComp=True):
        '''_extractFaceOffset(fcShape, offset) ... internal function.
            Original _buildPathArea() version copied from PathAreaOp.py module.  This version is modified.
            Adjustments made based on notes by @sliptonic at this webpage: https://github.com/sliptonic/FreeCAD/wiki/PathArea-notes.'''
        PathLog.debug('_extractFaceOffset()')

        if fcShape.BoundBox.ZMin != 0.0:
            fcShape.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - fcShape.BoundBox.ZMin))

        areaParams = {}
        areaParams['Offset'] = offset
        areaParams['Fill'] = 1  # 1
        areaParams['Coplanar'] = 0
        areaParams['SectionCount'] = 1  # -1 = full(all per depthparams??) sections
        areaParams['Reorient'] = True
        areaParams['OpenMode'] = 0
        areaParams['MaxArcPoints'] = 400  # 400
        areaParams['Project'] = True
        # areaParams['Tolerance'] = 0.001

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
            if not makeComp:
                ofstFace = [ofstFace]
        else:
            W = list()
            for wr in offsetShape.Wires:
                W.append(Part.Face(wr))
            if makeComp:
                ofstFace = Part.makeCompound(W)
            else:
                ofstFace = W

        return ofstFace  # offsetShape

    def _isPocket(self, b, f, w):
        '''_isPocket(b, f, w)... 
        Attempts to determine if the wire(w) in face(f) of base(b) is a pocket or raised protrusion.
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
                #TODO: test if this works
                facets = M.Mesh.Facets.Points
            else:
                facets = Path.getFacets(M.Shape)

            if self.modelSTLs[m] is True:
                stl = ocl.STLSurf()

                for tri in facets:
                    t = ocl.Triangle(ocl.Point(tri[0][0], tri[0][1], tri[0][2] + obj.DepthOffset.Value),
                            ocl.Point(tri[1][0], tri[1][1], tri[1][2] + obj.DepthOffset.Value),
                            ocl.Point(tri[2][0], tri[2][1], tri[2][2] + obj.DepthOffset.Value))
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
            # time.sleep(0.3)

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

        if self.showDebugObjects is True:
            T = FreeCAD.ActiveDocument.addObject('Part::Feature', 'safeSTLShape')
            T.Shape = fused
            T.purgeTouched()
            self.tempGroup.addObject(T)

        facets = Path.getFacets(fused)

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
        It then calls the correct method.'''
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
            if obj.Algorithm == 'OCL Dropcutter':
                final.extend(self._oclWaterlineOp(JOB, obj, mdlIdx, COMP))  # independent method set for Waterline
            else:
                final.extend(self._experimentalWaterlineOp(JOB, obj, mdlIdx, COMP))  # independent method set for Waterline

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
                if obj.Algorithm == 'OCL Dropcutter':
                    final.extend(self._oclWaterlineOp(JOB, obj, mdlIdx, COMP))  # independent method set for Waterline
                else:
                    final.extend(self._experimentalWaterlineOp(JOB, obj, mdlIdx, COMP))  # independent method set for Waterline
                COMP = None
        # Eif

        return final

    # Methods for creating path geometry
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
            iC = sp.isOnLineSegment(ep, cp)
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
            iC = sp.isOnLineSegment(ep, cp)
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
                # PathLog.debug("SO[0] == 'Loop'")
                lei = SO[1]  # loop Edges index
                v1 = compGeoShp.Edges[lei].Vertexes[0]

                # space = obj.SampleInterval.Value / 2.0
                space = 0.0000001

                # p1 = FreeCAD.Vector(v1.X, v1.Y, v1.Z)
                p1 = FreeCAD.Vector(v1.X, v1.Y, 0.0)
                rad = p1.sub(COM).Length
                spcRadRatio = space/rad
                if spcRadRatio < 1.0:
                    tolrncAng = math.asin(spcRadRatio)
                else:
                    tolrncAng = 0.9999998 * math.pi
                EX = COM.x + (rad * math.cos(tolrncAng))
                EY = v1.Y - space  # rad * math.sin(tolrncAng)

                sp = (v1.X, v1.Y, 0.0)
                ep = (EX, EY, 0.0)
                cp = (COM.x, COM.y, 0.0)
                if dirFlg == 1:
                    arc = (sp, ep, cp)
                else:
                    arc = (ep, sp, cp)  # OCL.Arc(firstPnt, lastPnt, centerPnt, dir=True(CCW direction))
                ARCS.append(('L', dirFlg, [arc]))
            else:  # SO[0] == 'A'    A = Arc
                # PathLog.debug("SO[0] == 'Arc'")
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
                    if iEi > iSi:
                        EI.pop(iEi)
                        EI.pop(iSi)
                    else:
                        EI.pop(iSi)
                        EI.pop(iEi)
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

    def _getExperimentalWaterlinePaths(self, obj, PNTSET, csHght):
        '''_getExperimentalWaterlinePaths(obj, PNTSET, csHght)...
        Switching function for calling the appropriate path-geometry to OCL points conversion function
        for the various cut patterns.'''
        PathLog.debug('_getExperimentalWaterlinePaths()')
        SCANS = list()

        if obj.CutPattern == 'Line':
            stpOvr = list()
            for D in PNTSET:
                for SEG in D:
                    if SEG == 'BRK':
                        stpOvr.append(SEG)
                    else:
                        # D format is ((p1, p2), (p3, p4))
                        (A, B) = SEG
                        P1 = FreeCAD.Vector(A[0], A[1], csHght)
                        P2 = FreeCAD.Vector(B[0], B[1], csHght)
                        stpOvr.append((P1, P2))
                SCANS.append(stpOvr)
                stpOvr = list()
        elif obj.CutPattern == 'ZigZag':
            stpOvr = list()
            for (dirFlg, LNS) in PNTSET:
                for SEG in LNS:
                    if SEG == 'BRK':
                        stpOvr.append(SEG)
                    else:
                        # D format is ((p1, p2), (p3, p4))
                        (A, B) = SEG
                        P1 = FreeCAD.Vector(A[0], A[1], csHght)
                        P2 = FreeCAD.Vector(B[0], B[1], csHght)
                        stpOvr.append((P1, P2))
                SCANS.append(stpOvr)
                stpOvr = list()
        elif obj.CutPattern in ['Circular', 'CircularZigZag']:
            # PNTSET is list, by stepover.
            # Each stepover is a list containing arc/loop descriptions, (sp, ep, cp)
            for so in range(0, len(PNTSET)):
                stpOvr = list()
                erFlg = False
                (aTyp, dirFlg, ARCS) = PNTSET[so]

                if dirFlg == 1:  # 1
                    cMode = True  # Climb mode
                else:
                    cMode = False

                for a in range(0, len(ARCS)):
                    Arc = ARCS[a]
                    if Arc == 'BRK':
                        stpOvr.append('BRK')
                    else:
                        (sp, ep, cp) = Arc
                        S = FreeCAD.Vector(sp[0], sp[1], csHght)
                        E = FreeCAD.Vector(ep[0], ep[1], csHght)
                        C = FreeCAD.Vector(cp[0], cp[1], csHght)
                        scan = (S, E, C, cMode)
                        if scan is False:
                            erFlg = True
                        else:
                            ##if aTyp == 'L':
                            ##    stpOvr.append(FreeCAD.Vector(scan[0][0].x, scan[0][0].y, scan[0][0].z))
                            stpOvr.append(scan)
                if erFlg is False:
                    SCANS.append(stpOvr)

        return SCANS

    # Main planar scan functions
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
                    # PathLog.debug('abs(zChng) < tolrnc')
                    if (minSTH - first.z) > tolrnc:
                        # PathLog.debug('(minSTH - first.z) > tolrnc')
                        height = minSTH + 2.0
                    else:
                        # PathLog.debug('ELSE (minSTH - first.z) > tolrnc')
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
    def _oclWaterlineOp(self, JOB, obj, mdlIdx, subShp=None):
        '''_oclWaterlineOp(obj, base) ... Main waterline function to perform waterline extraction from model.'''
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

            if not optimize or not FreeCAD.Vector(prev.x, prev.y, prev.z).isOnLineSegment(FreeCAD.Vector(nxt.x, nxt.y, nxt.z), FreeCAD.Vector(pnt.x, pnt.y, pnt.z)):
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

    # Main waterline functions
    def _experimentalWaterlineOp(self, JOB, obj, mdlIdx, subShp=None):
        '''_waterlineOp(JOB, obj, mdlIdx, subShp=None) ...
        Main waterline function to perform waterline extraction from model.'''
        PathLog.debug('_experimentalWaterlineOp()')

        msg = translate('PathWaterline', 'Experimental Waterline does not currently support selected faces.')
        PathLog.info('\n..... ' + msg)

        commands = []
        t_begin = time.time()
        base = JOB.Model.Group[mdlIdx]
        bb = self.boundBoxes[mdlIdx]
        stl = self.modelSTLs[mdlIdx]
        safeSTL = self.safeSTLs[mdlIdx]
        self.endVector = None

        finDep = obj.FinalDepth.Value + (self.geoTlrnc / 10.0)
        depthParams = PathUtils.depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown.Value, 0.0, finDep)

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == 'Single-pass':
            depthparams = [finDep]
        else:
            depthparams = [dp for dp in depthParams]
        lenDP = len(depthparams)
        PathLog.debug('Experimental Waterline depthparams:\n{}'.format(depthparams))

        # Prepare PathDropCutter objects with STL data
        # safePDC = self._planarGetPDC(safeSTL, depthparams[lenDP - 1], obj.SampleInterval.Value, useSafeCutter=False)

        buffer = self.cutter.getDiameter() * 2.0
        borderFace = Part.Face(self._makeExtendedBoundBox(JOB.Stock.Shape.BoundBox, buffer, 0.0))

        # Get correct boundbox
        if obj.BoundBox == 'Stock':
            stockEnv = self._getShapeEnvelope(JOB.Stock.Shape)
            bbFace = self._getCrossSection(stockEnv)  # returned at Z=0.0
        elif obj.BoundBox == 'BaseBoundBox':
            baseEnv = self._getShapeEnvelope(base.Shape)
            bbFace = self._getCrossSection(baseEnv)  # returned at Z=0.0

        trimFace = borderFace.cut(bbFace)
        if self.showDebugObjects is True:
            TF = FreeCAD.ActiveDocument.addObject('Part::Feature', 'trimFace')
            TF.Shape = trimFace
            TF.purgeTouched()
            self.tempGroup.addObject(TF)

        # Cycle through layer depths
        CUTAREAS = self._getCutAreas(base.Shape, depthparams, bbFace, trimFace, borderFace)
        if not CUTAREAS:
            PathLog.error('No cross-section cut areas identified.')
            return commands

        caCnt = 0
        ofst = obj.BoundaryAdjustment.Value
        ofst -= self.radius  # (self.radius + (tolrnc / 10.0))
        caLen = len(CUTAREAS)
        lastCA = caLen - 1
        lastClearArea = None
        lastCsHght = None
        clearLastLayer = True
        for ca in range(0, caLen):
            area = CUTAREAS[ca]
            csHght = area.BoundBox.ZMin
            csHght += obj.DepthOffset.Value
            cont = False
            caCnt += 1
            if area.Area > 0.0:
                cont = True
                caWireCnt = len(area.Wires) - 1  # first wire is boundFace wire
                PathLog.debug('cutAreaWireCnt: {}'.format(caWireCnt))
                if self.showDebugObjects is True:
                    CA = FreeCAD.ActiveDocument.addObject('Part::Feature', 'cutArea_{}'.format(caCnt))
                    CA.Shape = area
                    CA.purgeTouched()
                    self.tempGroup.addObject(CA)
            else:
                PathLog.error('Cut area at {} is zero.'.format(round(csHght, 4)))

            # get offset wire(s) based upon cross-section cut area
            if cont:
                area.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - area.BoundBox.ZMin))
                activeArea = area.cut(trimFace)
                activeAreaWireCnt = len(activeArea.Wires)  # first wire is boundFace wire
                PathLog.debug('activeAreaWireCnt: {}'.format(activeAreaWireCnt))
                if self.showDebugObjects is True:
                    CA = FreeCAD.ActiveDocument.addObject('Part::Feature', 'activeArea_{}'.format(caCnt))
                    CA.Shape = activeArea
                    CA.purgeTouched()
                    self.tempGroup.addObject(CA)
                ofstArea = self._extractFaceOffset(obj, activeArea, ofst, makeComp=False)
                if not ofstArea:
                    PathLog.error('No offset area returned for cut area depth: {}'.format(csHght))
                    cont = False

            if cont:
                # Identify solid areas in the offset data
                ofstSolidFacesList = self._getSolidAreasFromPlanarFaces(ofstArea)
                if ofstSolidFacesList:
                    clearArea = Part.makeCompound(ofstSolidFacesList)
                    if self.showDebugObjects is True:
                        CA = FreeCAD.ActiveDocument.addObject('Part::Feature', 'clearArea_{}'.format(caCnt))
                        CA.Shape = clearArea
                        CA.purgeTouched()
                        self.tempGroup.addObject(CA)
                else:
                    cont = False
                    PathLog.error('ofstSolids is False.')

            if cont:
                # Make waterline path for current CUTAREA depth (csHght)
                commands.extend(self._wiresToWaterlinePath(obj, clearArea, csHght))
                clearArea.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - clearArea.BoundBox.ZMin))
                lastClearArea = clearArea
                lastCsHght = csHght

                # Clear layer as needed
                (useOfst, usePat, clearLastLayer) = self._clearLayer(obj, ca, lastCA, clearLastLayer)
                ##if self.showDebugObjects is True and (usePat or useOfst):
                ##    OA = FreeCAD.ActiveDocument.addObject('Part::Feature', 'clearPatternArea_{}'.format(round(csHght, 2)))
                ##    OA.Shape = clearArea
                ##    OA.purgeTouched()
                ##    self.tempGroup.addObject(OA)
                if usePat:
                    commands.extend(self._makeCutPatternLayerPaths(JOB, obj, clearArea, csHght))
                if useOfst:
                    commands.extend(self._makeOffsetLayerPaths(JOB, obj, clearArea, csHght))
        # Efor

        if clearLastLayer:
            (useOfst, usePat, cLL) = self._clearLayer(obj, 1, 1, False)
            clearArea.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - lastClearArea.BoundBox.ZMin))
            if usePat:
                commands.extend(self._makeCutPatternLayerPaths(JOB, obj, lastClearArea, lastCsHght))

            if useOfst:
                commands.extend(self._makeOffsetLayerPaths(JOB, obj, lastClearArea, lastCsHght))

        PathLog.info("Waterline: All layer scans combined took " + str(time.time() - t_begin) + " s")
        return commands

    def _getCutAreas(self, shape, depthparams, bbFace, trimFace, borderFace):
        '''_getCutAreas(JOB, shape, depthparams, bbFace, borderFace) ...
        Takes shape, depthparams and base-envelope-cross-section, and
        returns a list of cut areas - one for each depth.'''
        PathLog.debug('_getCutAreas()')

        CUTAREAS = list()
        lastLayComp = None
        isFirst = True
        lenDP = len(depthparams)
        
        # Cycle through layer depths
        for dp in range(0, lenDP):
            csHght = depthparams[dp]
            PathLog.debug('Depth {} is {}'.format(dp + 1, csHght))

            # Get slice at depth of shape
            csFaces = self._getModelCrossSection(shape, csHght)  # returned at Z=0.0
            if not csFaces:
                PathLog.error('No cross-section wires at {}'.format(csHght))
            else:
                PathLog.debug('cross-section face count {}'.format(len(csFaces)))
                if len(csFaces) > 0:
                    useFaces = self._getSolidAreasFromPlanarFaces(csFaces)
                else:
                    useFaces = False

                if useFaces:
                    PathLog.debug('useFacesCnt: {}'.format(len(useFaces)))
                    compAdjFaces = Part.makeCompound(useFaces)

                    if self.showDebugObjects is True:
                        CA = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpSolids_{}'.format(dp + 1))
                        CA.Shape = compAdjFaces
                        CA.purgeTouched()
                        self.tempGroup.addObject(CA)

                    if isFirst:
                        allPrevComp = compAdjFaces
                        cutArea = borderFace.cut(compAdjFaces)
                    else:
                        preCutArea = borderFace.cut(compAdjFaces)
                        cutArea = preCutArea.cut(allPrevComp)  # cut out higher layers to avoid cutting recessed areas
                        allPrevComp = allPrevComp.fuse(compAdjFaces)
                    cutArea.translate(FreeCAD.Vector(0.0, 0.0, csHght - cutArea.BoundBox.ZMin))
                    CUTAREAS.append(cutArea)
                    isFirst = False
                else:
                    PathLog.error('No waterline at depth: {} mm.'.format(csHght))
        # Efor

        if len(CUTAREAS) > 0:
            return CUTAREAS

        return False

    def _wiresToWaterlinePath(self, obj, ofstPlnrShp, csHght):
        PathLog.debug('_wiresToWaterlinePath()')
        commands = list()

        # Translate path geometry to layer height
        ofstPlnrShp.translate(FreeCAD.Vector(0.0, 0.0, csHght - ofstPlnrShp.BoundBox.ZMin))
        if self.showDebugObjects is True:
            OA = FreeCAD.ActiveDocument.addObject('Part::Feature', 'waterlinePathArea_{}'.format(round(csHght, 2)))
            OA.Shape = ofstPlnrShp
            OA.purgeTouched()
            self.tempGroup.addObject(OA)

        commands.append(Path.Command('N (Cut Area {}.)'.format(round(csHght, 2))))
        for w in range(0, len(ofstPlnrShp.Wires)):
            wire = ofstPlnrShp.Wires[w]
            V = wire.Vertexes
            if obj.CutMode == 'Climb':
                lv = len(V) - 1
                startVect = FreeCAD.Vector(V[lv].X, V[lv].Y, V[lv].Z)
            else:
                startVect = FreeCAD.Vector(V[0].X, V[0].Y, V[0].Z)

            commands.append(Path.Command('N (Wire {}.)'.format(w)))
            (cmds, endVect) = self._wireToPath(obj, wire, startVect)
            commands.extend(cmds)
            commands.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))

        return commands

    def _makeCutPatternLayerPaths(self, JOB, obj, clrAreaShp, csHght):
        PathLog.debug('_makeCutPatternLayerPaths()')
        commands = []

        clrAreaShp.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - clrAreaShp.BoundBox.ZMin))
        pathGeom = self._planarMakePathGeom(obj, clrAreaShp)
        pathGeom.translate(FreeCAD.Vector(0.0, 0.0, csHght - pathGeom.BoundBox.ZMin))
        # clrAreaShp.translate(FreeCAD.Vector(0.0, 0.0, csHght - clrAreaShp.BoundBox.ZMin))

        if self.showDebugObjects is True:
            OA = FreeCAD.ActiveDocument.addObject('Part::Feature', 'pathGeom_{}'.format(round(csHght, 2)))
            OA.Shape = pathGeom
            OA.purgeTouched()
            self.tempGroup.addObject(OA)

        # Convert pathGeom to gcode more efficiently
        if True:
            if obj.CutPattern == 'Offset':
                commands.extend(self._makeOffsetLayerPaths(JOB, obj, clrAreaShp, csHght))
            else:
                clrAreaShp.translate(FreeCAD.Vector(0.0, 0.0, csHght - clrAreaShp.BoundBox.ZMin))
                if obj.CutPattern == 'Line':
                    pntSet = self._pathGeomToLinesPointSet(obj, pathGeom)
                elif obj.CutPattern == 'ZigZag':
                    pntSet = self._pathGeomToZigzagPointSet(obj, pathGeom)
                elif obj.CutPattern in ['Circular', 'CircularZigZag']:
                    pntSet = self._pathGeomToArcPointSet(obj, pathGeom)
                stpOVRS = self._getExperimentalWaterlinePaths(obj, pntSet, csHght)
                # PathLog.debug('stpOVRS:\n{}'.format(stpOVRS))
                safePDC = False
                cmds = self._clearGeomToPaths(JOB, obj, safePDC, stpOVRS, csHght)
                commands.extend(cmds)
        else:
            # Use Path.fromShape() to convert edges to paths
            for w in range(0, len(pathGeom.Edges)):
                wire = pathGeom.Edges[w]
                V = wire.Vertexes
                if obj.CutMode == 'Climb':
                    lv = len(V) - 1
                    startVect = FreeCAD.Vector(V[lv].X, V[lv].Y, V[lv].Z)
                else:
                    startVect = FreeCAD.Vector(V[0].X, V[0].Y, V[0].Z)

                commands.append(Path.Command('N (Wire {}.)'.format(w)))
                (cmds, endVect) = self._wireToPath(obj, wire, startVect)
                commands.extend(cmds)
                commands.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))

        return commands

    def _makeOffsetLayerPaths(self, JOB, obj, clrAreaShp, csHght):
        PathLog.debug('_makeOffsetLayerPaths()')
        PathLog.warning('Using `Offset` for clearing bottom layer.')
        cmds = list()
        # ofst = obj.BoundaryAdjustment.Value
        ofst = 0.0 - self.cutOut  # - self.cutter.getDiameter()  # (self.radius + (tolrnc / 10.0))
        shape = clrAreaShp
        cont = True
        cnt = 0
        while cont:
            ofstArea = self._extractFaceOffset(obj, shape, ofst, makeComp=True)
            if not ofstArea:
                PathLog.warning('No offset clearing area returned.')
                break
            for F in ofstArea.Faces:
                cmds.extend(self._wiresToWaterlinePath(obj, F, csHght))
            shape = ofstArea
            if cnt == 0:
                ofst = 0.0 - self.cutOut  # self.cutter.Diameter()
            cnt += 1
        return cmds

    def _clearGeomToPaths(self, JOB, obj, safePDC, SCANDATA, csHght):
        PathLog.debug('_clearGeomToPaths()')

        GCODE = [Path.Command('N (Beginning of Single-pass layer.)', {})]
        tolrnc = JOB.GeometryTolerance.Value
        prevDepth = obj.SafeHeight.Value
        lenSCANDATA = len(SCANDATA)
        gDIR = ['G3', 'G2']

        if self.CutClimb is True:
            gDIR = ['G2', 'G3']

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
                # minTrnsHght = self._getMinSafeTravelHeight(safePDC, lstStpEnd, first)  # Check safe travel height against fullSTL
                minTrnsHght = obj.SafeHeight.Value
                # cmds.append(Path.Command('N (Transition: last, first: {}, {}:  minSTH: {})'.format(lstStpEnd, first, minTrnsHght), {}))
                cmds.extend(self._stepTransitionCmds(obj, lstStpEnd, first, minTrnsHght, tolrnc))

            # Cycle through current step-over parts
            for i in range(0, lenPRTS):
                prt = PRTS[i]
                lenPrt = len(prt)
                # PathLog.debug('prt: {}'.format(prt))
                if prt == 'BRK':
                    nxtStart = PRTS[i + 1][0]
                    # minSTH = self._getMinSafeTravelHeight(safePDC, last, nxtStart)  # Check safe travel height against fullSTL
                    minSTH = obj.SafeHeight.Value
                    cmds.append(Path.Command('N (Break)', {}))
                    cmds.extend(self._breakCmds(obj, last, nxtStart, minSTH, tolrnc))
                else:
                    cmds.append(Path.Command('N (part {}.)'.format(i + 1), {}))
                    if obj.CutPattern in ['Line', 'ZigZag']:
                        start, last = prt
                        cmds.append(Path.Command('G1', {'X': start.x, 'Y': start.y, 'Z': start.z, 'F': self.horizFeed}))
                        cmds.append(Path.Command('G1', {'X': last.x, 'Y': last.y, 'F': self.horizFeed}))
                    elif obj.CutPattern in ['Circular', 'CircularZigZag']:
                        start, last, centPnt, cMode = prt
                        gcode = self._makeGcodeArc(start, last, odd, gDIR, tolrnc)
                        cmds.extend(gcode)
            cmds.append(Path.Command('N (End of step {}.)'.format(so), {}))
            GCODE.extend(cmds)  # save line commands
            lstStpEnd = last
        # Efor

        return GCODE

    def _getSolidAreasFromPlanarFaces(self, csFaces):
        PathLog.debug('_getSolidAreasFromPlanarFaces()')
        holds = list()
        cutFaces = list()
        useFaces = list()
        lenCsF = len(csFaces)
        PathLog.debug('lenCsF: {}'.format(lenCsF))

        if lenCsF == 1:
            useFaces = csFaces
        else:
            fIds = list()
            aIds = list()
            pIds = list()
            cIds = list()

            for af in range(0, lenCsF):
                fIds.append(af)  # face ids
                aIds.append(af)  # face ids
                pIds.append(-1)  # parent ids
                cIds.append(False)  # cut ids
                holds.append(False)

            while len(fIds) > 0:
                li = fIds.pop()
                low = csFaces[li]  # senior face
                pIds = self._idInternalFeature(csFaces, fIds, pIds, li, low)
            # Ewhile
            ##PathLog.info('fIds: {}'.format(fIds))
            ##PathLog.info('pIds: {}'.format(pIds))
            
            for af in range(lenCsF - 1, -1, -1):  # cycle from last item toward first
                ##PathLog.info('af: {}'.format(af))
                prnt = pIds[af]
                ##PathLog.info('prnt: {}'.format(prnt))
                if prnt == -1:
                    stack = -1
                else:
                    stack = [af]
                    # get_face_ids_to_parent
                    stack.insert(0, prnt)
                    nxtPrnt = pIds[prnt]
                    # find af value for nxtPrnt
                    while nxtPrnt != -1:
                        stack.insert(0, nxtPrnt)
                        nxtPrnt = pIds[nxtPrnt]
                cIds[af] = stack
            # PathLog.debug('cIds: {}\n'.format(cIds))

            for af in range(0, lenCsF):
                # PathLog.debug('af is {}'.format(af))
                pFc = cIds[af]
                if pFc == -1:
                    # Simple, independent region
                    holds[af] = csFaces[af]  # place face in hold
                    # PathLog.debug('pFc == -1')
                else:
                    # Compound region
                    # PathLog.debug('pFc is not -1')
                    cnt = len(pFc)
                    if cnt % 2.0 == 0.0:
                        # even is donut cut
                        # PathLog.debug('cnt is even')
                        inr = pFc[cnt - 1]
                        otr = pFc[cnt - 2]
                        # PathLog.debug('inr / otr: {} / {}'.format(inr, otr))
                        holds[otr] = holds[otr].cut(csFaces[inr])
                    else:
                        # odd is floating solid
                        # PathLog.debug('cnt is ODD')
                        holds[af] = csFaces[af]
            # Efor

            for af in range(0, lenCsF):
                if holds[af]:
                    useFaces.append(holds[af])  # save independent solid
                    
        # Eif

        if len(useFaces) > 0:
            return useFaces

        return False

    def _getModelCrossSection(self, shape, csHght):
        PathLog.debug('_getCrossSection()')
        wires = list()

        def byArea(fc):
            return fc.Area

        for i in shape.slice(FreeCAD.Vector(0, 0, 1), csHght):
            wires.append(i)

        if len(wires) > 0:
            for w in wires:
                if w.isClosed() is False:
                    return False
            FCS = list()
            for w in wires:
                w.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - w.BoundBox.ZMin))
                FCS.append(Part.Face(w))
            FCS.sort(key=byArea, reverse=True)
            return FCS
        else:
            PathLog.debug(' -No wires from .slice() method')

        return False

    def _isInBoundBox(self, outShp, inShp):
        obb = outShp.BoundBox
        ibb = inShp.BoundBox

        if obb.XMin < ibb.XMin:
            if obb.XMax > ibb.XMax:
                if obb.YMin < ibb.YMin:
                    if obb.YMax > ibb.YMax:
                        return True
        return False

    def _idInternalFeature(self, csFaces, fIds, pIds, li, low):
        Ids = list()
        for i in fIds:
            Ids.append(i)
        while len(Ids) > 0:
            hi = Ids.pop()
            high = csFaces[hi]
            if self._isInBoundBox(high, low):
                cmn = high.common(low)
                if cmn.Area > 0.0:
                    pIds[li] = hi
                    break
        # Ewhile
        return pIds

    def _wireToPath(self, obj, wire, startVect):
        '''_wireToPath(obj, wire, startVect) ... wire to path.'''
        PathLog.track()

        paths = []
        pathParams = {} # pylint: disable=assignment-from-no-return
        V = wire.Vertexes

        pathParams['shapes'] = [wire]
        pathParams['feedrate'] = self.horizFeed
        pathParams['feedrate_v'] = self.vertFeed
        pathParams['verbose'] = True
        pathParams['resume_height'] = obj.SafeHeight.Value
        pathParams['retraction'] = obj.ClearanceHeight.Value
        pathParams['return_end'] = True
        # Note that emitting preambles between moves breaks some dressups and prevents path optimization on some controllers
        pathParams['preamble'] = False
        pathParams['start'] = startVect

        (pp, end_vector) = Path.fromShapes(**pathParams)
        paths.extend(pp.Commands)
        # PathLog.debug('pp: {}, end vector: {}'.format(pp, end_vector))

        self.endVector = end_vector # pylint: disable=attribute-defined-outside-init

        return (paths, end_vector)

    def _makeExtendedBoundBox(self, wBB, bbBfr, zDep):
        pl = FreeCAD.Placement()
        pl.Rotation = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0)
        pl.Base = FreeCAD.Vector(0, 0, 0)

        p1 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMin - bbBfr, zDep)
        p2 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMin - bbBfr, zDep)
        p3 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMax + bbBfr, zDep)
        p4 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMax + bbBfr, zDep)
        bb = Part.makePolygon([p1, p2, p3, p4, p1])

        return bb

    def _makeGcodeArc(self, strtPnt, endPnt, odd, gDIR, tolrnc):
        cmds = list()
        isCircle = False
        inrPnt = None
        gdi = 0
        if odd is True:
            gdi = 1

        # Test if pnt set is circle
        if abs(strtPnt.x - endPnt.x) < tolrnc:
            if abs(strtPnt.y - endPnt.y) < tolrnc:
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
            # ijk = self.tmpCOM - strtPnt
            ijk = self.tmpCOM.sub(strtPnt)  # vector from start to center
            xyz = endPnt
            cmds.append(Path.Command('G1', {'X': strtPnt.x, 'Y': strtPnt.y, 'Z': strtPnt.z, 'F': self.horizFeed}))
            cmds.append(Path.Command(gDIR[gdi], {'X': xyz.x, 'Y': xyz.y, 'Z': xyz.z,
                                                'I': ijk.x, 'J': ijk.y, 'K': ijk.z,  # leave same xyz.z height
                                                'F': self.horizFeed}))
            cmds.append(Path.Command('G1', {'X': endPnt.x, 'Y': endPnt.y, 'Z': endPnt.z, 'F': self.horizFeed}))

        return cmds

    def _clearLayer(self, obj, ca, lastCA, clearLastLayer):
        PathLog.debug('_clearLayer()')
        usePat = False
        useOfst = False

        if obj.ClearLastLayer == 'Off':
            if obj.CutPattern != 'None':
                usePat = True
        else:
            if ca == lastCA:
                PathLog.debug('... Clearing bottom layer.')
                if obj.ClearLastLayer == 'Offset':
                    obj.CutPattern = 'None'
                    useOfst = True
                else:
                    obj.CutPattern = obj.ClearLastLayer
                    usePat = True
                clearLastLayer = False

        return (useOfst, usePat, clearLastLayer)

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


def SetupProperties():
    ''' SetupProperties() ... Return list of properties required for operation.'''
    setup = []
    setup.append('Algorithm')
    setup.append('AngularDeflection')
    setup.append('AvoidLastX_Faces')
    setup.append('AvoidLastX_InternalFeatures')
    setup.append('BoundBox')
    setup.append('BoundaryAdjustment')
    setup.append('CircularCenterAt')
    setup.append('CircularCenterCustom')
    setup.append('ClearLastLayer')
    setup.append('CutMode')
    setup.append('CutPattern')
    setup.append('CutPatternAngle')
    setup.append('CutPatternReversed')
    setup.append('DepthOffset')
    setup.append('GapSizes')
    setup.append('GapThreshold')
    setup.append('HandleMultipleFeatures')
    setup.append('InternalFeaturesCut')
    setup.append('InternalFeaturesAdjustment')
    setup.append('LayerMode')
    setup.append('LinearDeflection')
    setup.append('OptimizeStepOverTransitions')
    setup.append('ProfileEdges')
    setup.append('BoundaryEnforcement')
    setup.append('SampleInterval')
    setup.append('StartPoint')
    setup.append('StepOver')
    setup.append('UseStartPoint')
    # For debugging
    setup.append('ShowTempObjects')
    return setup


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Waterline operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectWaterline(obj, name)
    return obj
