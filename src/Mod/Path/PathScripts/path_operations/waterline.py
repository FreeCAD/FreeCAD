# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 Russell Johnson (russ4262) <russ4262@gmail.com>    *
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

__title__ = "Path Waterline Operation Strategy"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and strategy implementation of Waterline operation."
__contributors__ = ""

from PySide import QtCore
import ocl

import FreeCAD
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
import PathScripts.PathOp as PathOp
import time
import math

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Part = LazyLoader('Part', globals(), 'Part')
path_surface_support = LazyLoader('PathScripts.path_operations.surface_support',
                                globals(),
                                'PathScripts.path_operations.surface_support')


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule()


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class WaterlineOperation:
    '''Given an operation object and its proxy (parent) class,
    produces a Waterline path.
    '''

    def __init__(self, obj, parent, initialize=True):
        '''__init__(obj, parent) ... Constructor for Waterline operation class.'''
        PathLog.debug('WaterlineOperation.__init__()')
        # Required operation instance attributes
        self.parent = parent
        self.obj = obj

        self.job = parent.job
        self.propertyEnumerations = None
        self.propertyEditorModes = None
        self.isDebug = parent.isDebug  # Debug attribute
        self.showDebugObjects = parent.showDebugObjects  # Debug attribute
        self.tmpDebugGrp = None
        self.hasProperties = False
        self.newPropsDict = None
        self.propertiesReady = False

        # Place operation-specific attributes in this method
        self._initOperationAttributes()

        # Perform a initial Base Geometry analysis
        self._analyzeBaseGeometry()

        if initialize:
            self.initializeProperties()

    # Required initialization and maintenance methods
    def _initOperationAttributes(self):
        '''Operation-specific instance attributes - modify as needed'''
        self.ClearHeightOffset = None
        self.CutClimb = False
        self.SafeHeightOffset = None
        self.avoidShapes = list()
        self.boundBoxes = list()
        self.closedGap = False
        self.collectiveShapes = list()
        self.commandlist = None
        self.cutOut = 0.0
        self.cutter = None
        self.depthParams = None
        self.fullSTL = None
        self.gaps = [0.1, 0.2, 0.3]
        self.geoTlrnc = None
        self.individualShapes = list()
        self.midDep = None
        self.modelSTLs = list()
        self.modelTypes = list()
        self.module = None
        self.profileShapes = list()
        self.radius = None
        self.safeSTLs = list()
        self.stl = None
        self.tmpCOM = None
        self.toolDiam = None
        self.useTiltCutter = False

        # Instantiate additional class operation variables
        self._resetOpVariables()

    def _resetAnalysisAttributes(self):
        self.isReady = False
        self.processType = None  # CustomPoints, Single, Double
        self._base = None
        self._subsList = list()
        self._shapes = list()
        self._shapeTypes = list()
        self._ZMin = -99999999

    def _analyzeBaseGeometry(self):
        PathLog.debug('_analyzeBaseGeometry()')
        # Reset analysis attributes
        self._resetAnalysisAttributes()
        useBoundBox = True

        if hasattr(self.obj, 'Base'):
            # PathLog.debug(" -hasattr(self.obj, 'Base')")
            if self.obj.Base:
                # PathLog.debug(' -self.obj.Base')
                base, sublist = self.obj.Base[0]
                fbb = base.Shape.getElement(sublist[0]).BoundBox
                zmin = fbb.ZMax
                for base, sublist in self.obj.Base:
                    for sub in sublist:
                        try:
                            fbb = base.Shape.getElement(sub).BoundBox
                            zmin = min(zmin, fbb.ZMin)
                        except Part.OCCError as e:
                            PathLog.error(e)
                self._ZMin = zmin
                useBoundBox = False

        if useBoundBox and self.job:
            # PathLog.debug(' -self.job')
            if hasattr(self.obj, 'BoundBox'):
                # PathLog.debug(" -hasattr(self.obj, 'BoundBox')")
                if self.obj.BoundBox == 'BaseBoundBox':
                    models = self.job.Model.Group
                    zmin = models[0].Shape.BoundBox.ZMax
                    for M in models:
                        zmin = min(zmin, M.Shape.BoundBox.ZMin)
                    self._ZMin = zmin
                elif self.obj.BoundBox == 'Stock':
                    self._ZMin = self.job.Stock.Shape.BoundBox.ZMin

        PathLog.debug(' -_ZMin: {}'.format(self._ZMin))
        self.isReady = True

    def _revisePropertyEnumerations(self):
        '''
        Method returns dictionary of property enumerations based on
        active conditions in the operation.
        Use one internal function here pertaining to each property for
        which you wish to modify its enumerations.  The enumerations
        maniplated here need to be syncronized with the class defaults
        returned from `getInitialPropertyEnumerations()`.        
        '''

        # Retrieve full enumeration lists
        initEnums = self.getInitialPropertyEnumerations()

        # The class instance attribute is updated.
        return initEnums

    def _updatePropertyEditorModes(self, apply=False):
        self.propertyEditorModes = self.getPropertyEditorModes()
        if apply:
            for prop, mode in self.propertyEditorModes.items():
                self.obj.setEditorMode(prop, mode)

    def _updatePropertyEnumerations(self, apply=False):
        # initialEnums = self.getInitialPropertyEnumerations()
        # Enumeration lists for App::PropertyEnumeration properties
        self.propertyEnumerations = \
            self._revisePropertyEnumerations()
        if apply:
            self.parent.updateEnumerations(self.obj, self.propertyEnumerations)

    # Public methods to manage operation features and properties
    @staticmethod
    def getFeatures():
        '''getFeatures() ... Return list of standard features by name'''
        return [
            'FeatureTool', 'FeatureDepths', 'FeatureHeights',
            'FeatureStepDown', 'FeatureCoolant'  # , 'FeatureBaseFaces'
        ]

    @staticmethod
    def getPropertyDefinitions():
        '''getPropertyDefinitions() ...
        This method returns initial property definitions as a list of tuples.
        '''
        # Definition format: (prop_type, name, group, tooltip)
        return [
            ("App::PropertyBool", "ShowDebugObjects", "Debug",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Show the temporary path construction objects when module is in DEBUG mode.")),

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
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Select the algorithm to use: OCL Dropcutter*, or Experimental (Not OCL based).")),
            ("App::PropertyEnumeration", "BoundBox", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Select the overall boundary for the operation.")),
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
            ("App::PropertyDistance", "IgnoreOuterAbove", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Ignore outer waterlines above this height.")),
            ("App::PropertyEnumeration", "LayerMode", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Complete the operation in a single pass at depth, or mulitiple passes to final depth.")),
            ("App::PropertyVectorDistance", "PatternCenterCustom", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Set the start point for the cut pattern.")),
            ("App::PropertyEnumeration", "PatternCenterAt", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Choose location of the center point for starting the cut pattern.")),
            ("App::PropertyDistance", "SampleInterval", "Clearing Options",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Set the sampling resolution. Smaller values quickly increase processing time.")),
            ("App::PropertyFloat", "StepOver", "Clearing Options",
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

    @staticmethod
    def getInitialPropertyEnumerations():
        '''getInitialPropertyEnumerations() ...
        This method returns initial property enumeration lists used to
        initialize the properties.

        Once properties are created, calling `updateParamaters()` will
        update the enumeration lists per logic in the
        `_revisePropertyEnumerations()` method. The updated enumerations will
        be applied to the properties and stored in the
        `self.propertyEnumerations` attribute.
        '''
        # Enumeration lists for App::PropertyEnumeration properties
        return {
            'Algorithm': ['OCL Dropcutter', 'Experimental'],
            'BoundBox': ['BaseBoundBox', 'Stock'],
            'PatternCenterAt': ['CenterOfMass', 'CenterOfBoundBox', 'XminYmin', 'Custom'],
            'ClearLastLayer': ['Off', 'Circular', 'CircularZigZag', 'Line', 'Offset', 'Spiral', 'ZigZag'],
            'CutMode': ['Conventional', 'Climb'],
            'CutPattern': ['None', 'Circular', 'CircularZigZag', 'Line', 'Offset', 'Spiral', 'ZigZag'],  # Additional goals ['Offset', 'Spiral', 'ZigZagOffset', 'Grid', 'Triangle']
            'HandleMultipleFeatures': ['Collectively', 'Individually'],
            'LayerMode': ['Single-pass', 'Multi-pass'],
        }

    def getPropertyDefaultValues(self):
        '''getPropertyDefaultValues() ...
        This method returns initial property default values if not called
        from a class instance.  If called from a class instance, then the
        default values will reflect operation and job object influences.
        '''
        PathLog.debug('getPropertyDefaultValues()')
        initialValues = {
            'Algorithm': 'OCL Dropcutter',
            'AvoidLastX_Faces': 0,
            'AvoidLastX_InternalFeatures': True,
            'BoundBox': 'BaseBoundBox',
            'BoundaryAdjustment': 0.0,
            'BoundaryEnforcement': True,
            'ClearLastLayer': 'Off',
            'CutMode': 'Conventional',
            'CutPattern': 'None',
            'CutPatternAngle': 0.0,
            'CutPatternReversed': False,
            'DepthOffset': 0.0,
            'DropCutterDir': 'X',
            'DropCutterExtraOffset': FreeCAD.Vector(0.0, 0.0, 0.0),
            'GapSizes': 'No gaps identified.',
            'GapThreshold': 0.005,
            'HandleMultipleFeatures': 'Collectively',
            'IgnoreOuterAbove': 99999999999,
            'InternalFeaturesAdjustment': 0.0,
            'InternalFeaturesCut': True,
            'LayerMode': 'Single-pass',
            # LinearDeflection: Reasonable compromise between speed & precision
            'LinearDeflection': 0.001,
            'OptimizeLinearPaths': True,
            'OptimizeStepOverTransitions': False,
            'PatternCenterAt': 'CenterOfMass',
            'PatternCenterCustom': FreeCAD.Vector(0.0, 0.0, 0.0),
            'SampleInterval': 1.0,
            'ScanType': 'Planar',
            'StartIndex': 0.0,
            'StartPoint': FreeCAD.Vector(0.0, 0.0, 0.0),
            'StepOver': 100.0,
            'UseStartPoint': False
        }

        if not self:
            FreeCAD.Console.PrintWarning(noSelfMsg + ' <> 3\n')
            return initialValues

        initialValues['StartPoint'].z = self.obj.ClearanceHeight.Value
        initialValues['IgnoreOuterAbove'] = self.obj.StartDepth.Value + 0.00001

        warn = True
        if hasattr(self.job, 'GeometryTolerance'):
            if self.job.GeometryTolerance.Value != 0.0:
                warn = False
                # Tessellation precision dictates the offsets we need to add to
                # avoid false collisions with the model mesh, so make sure we
                # default to tessellating with greater precision than the target
                # GeometryTolerance.
                initialValues['LinearDeflection'] = self.job.GeometryTolerance.Value / 4.0
        if warn:
            msg = translate('waterline',
                            'The GeometryTolerance for this Job is 0.0.')
            msg += translate('waterline',
                             'Initializing LinearDeflection to 0.001 mm.')
            FreeCAD.Console.PrintWarning(msg + '\n')

        return initialValues

    def getPropertyEditorModes(self):
        '''getPropertyEditorModes() ...
        Use logic to set a mode variable assigned to one or more properties.
        Enter the property and its mode variable in the return dictionary.
        This method will also be used by the GUI module to set visibility
        in both the Data tab of the Property View and the Tasks editor window.

        Must not be called before properties are created.
        If so, error will be returned.
        '''
        obj = self.obj

        ocl_mode = G = 0
        show = hide = exp_mode = A = B = C = IOA = 2

        if obj.Algorithm == 'OCL Dropcutter':
            pass
        elif obj.Algorithm == 'Experimental':
            exp_mode = A = B = C = IOA = 0
            ocl_mode = G = show = hide = 2

            cutPattern = obj.CutPattern
            if obj.ClearLastLayer != 'Off':
                cutPattern = obj.ClearLastLayer

            if cutPattern == 'None':
                show = hide = A = B = 2
            elif cutPattern in ['Line', 'ZigZag']:
                show = 0
            elif cutPattern in ['Circular', 'CircularZigZag']:
                show = 2  # hide
                hide = 0  # show
            elif cutPattern == 'Spiral':
                G = hide = 0

        # format is {'property': value (fixed integer, or variable)}
        editorModes = {
            'AvoidLastX_Faces': 2,
            'AvoidLastX_InternalFeatures': 2,
            'BoundaryAdjustment': exp_mode,
            'BoundaryEnforcement': 2,
            'ClearLastLayer': C,
            'CutPattern': C,
            'CutPatternAngle': show,
            'CutPatternReversed': A,
            'GapSizes': 2,
            'GapThreshold': 2,
            'HandleMultipleFeatures': 2,
            'IgnoreOuterAbove': IOA,
            'InternalFeaturesAdjustment': 2,
            'InternalFeaturesCut': 2,
            'LinearDeflection': ocl_mode,
            'OptimizeLinearPaths': 2,
            'OptimizeStepOverTransitions': 2,
            'PatternCenterAt': hide,
            'PatternCenterCustom': hide,
            'SampleInterval': G,
            'StepOver': B
        }
        if hasattr(obj, 'EnableRotation'):
            editorModes['EnableRotation'] = 2

        return editorModes

    def initializeProperties(self):
        '''initializeProperties(self) ...
        Purpose is to create properties, set standard enumerations,
        set default values, and set initial editor modes.

        This method should only be called once.
        '''
        PathLog.debug('initializeProperties()')

        # Create properties, set enumerations, and set default values
        definitions = self.getPropertyDefinitions()
        enumerations = self.getInitialPropertyEnumerations()
        createOpProperties = self.parent.createOperationProperties
        self.newPropsDict = createOpProperties(self.obj, definitions,
                                               enumerations, warn=False)
        self.hasProperties = True

    def finishPropertySetup(self, job, obj):
        '''finishPropertySetup(self) ...
        Purpose is to add default values to existing properties,
        and set the initial editor modes.

        This method should only be called once.
        '''
        PathLog.debug('finishPropertySetup()')
        self.job = job
        self.obj = obj

        # Set default values for all new properties created
        if not self.hasProperties:
            # Error, no properties available
            return

        if self.newPropsDict:
            defaultValues = self.getPropertyDefaultValues()
            setDefVals = self.parent.setDefaultValuesForProperties
            setDefVals(self.obj, self.newPropsDict, defaultValues)

        # Set editor modes
        self._updatePropertyEditorModes(apply=True)

        # Re-analyze Base Geometry since defaults are available
        self._analyzeBaseGeometry()

        self.propertiesReady = True

    def onChanged(self, prop):
        # PathLog.info('onChanged() prop: {}'.format(prop))
        if prop in ['Algorithm', 'CutPattern', 'BoundBox']:
            self.updateParameters()

    def updateParameters(self, apply=False):
        '''updateParameters()
        This method:
            - Re-analyzes Base Geometry
            - Updates the property enumerations accordingly
            - Updates property editor modes accordingly
            - Stores updated enumerations to self.propertyEnumerations
            - Stores updated editor modes to self.propertyEditorModes

        This method must not be called before initializing the operation
        by calling the `initializeProperties()` method.
        '''
        # Perform current Base Geometry analysis
        self._analyzeBaseGeometry()
        self._updatePropertyEnumerations(apply)
        self._updatePropertyEditorModes(apply)

    # Primary public method to generate Waterline path data
    def execute(self):
        '''execute() ...
        Execute the Waterline operation.
        Returns a list of path commands.
        '''
        PathLog.track()

        # Update and connect parent attributes
        self.job = self.parent.job
        self.vertFeed = self.parent.vertFeed
        self.vertRapid = self.parent.vertRapid
        self.horizFeed = self.parent.horizFeed
        self.horizRapid = self.parent.horizRapid
        self.axialFeed = self.parent.axialFeed
        self.axialRapid = self.parent.axialRapid
        self.commandlist = self.parent.commandlist
        self.isDebug = self.parent.isDebug  # Debug attribute
        self.showDebugObjects = self.parent.showDebugObjects  # Debug attribute
        if self.isDebug:
            self.tmpDebugGrp = self.parent.tmpDebugGrp

        # Update Base Geometry analysis and parameters
        self.updateParameters(apply=True)

        if not self.isReady:
            return False

        rtn = self._makePath(self.obj)
        if rtn:
            g0 = {'Z': self.obj.ClearanceHeight.Value, 'F': self.parent.vertRapid}
            self.commandlist.append(Path.Command('G0', g0))

        return self.commandlist

    # Initial point of exection, called by `execute()` in PathOpBase
    def _makePath(self, obj):
        '''_makePath(obj) ... process waterline operation'''
        PathLog.track()

        CMDS = list()
        FCAD = FreeCAD.ActiveDocument

        self.module = 'PathWaterline'

        # make circle for workplane
        self.wpc = Part.makeCircle(2.0)

        # mark beginning of operation and identify parent Job
        PathLog.info('\nBegin Waterline operation...')
        startTime = time.time()

        # set cut mode; reverse as needed
        if obj.CutMode == 'Climb':
            self.CutClimb = True
        if obj.CutPatternReversed is True:
            if self.CutClimb is True:
                self.CutClimb = False
            else:
                self.CutClimb = True

        # Setup cutter for OCL and cutout value for operation - based on tool controller properties
        oclTool = path_surface_support.OCL_Tool(ocl, obj)
        self.cutter = oclTool.getOclTool()
        if not self.cutter:
            PathLog.error(translate('PathWaterline', "Canceling Waterline operation. Error creating OCL cutter."))
            return
        self.toolDiam = self.cutter.getDiameter()
        self.radius = self.toolDiam / 2.0
        self.cutOut = (self.toolDiam * (float(obj.StepOver) / 100.0))
        self.gaps = [self.toolDiam, self.toolDiam, self.toolDiam]

        # Begin GCode for operation with basic information
        # ... and move cutter to clearance height and startpoint
        if obj.Comment != '':
            self.commandlist.append(
                Path.Command('N ({})'.format(str(obj.Comment)), {}))
        self.commandlist.extend([
            Path.Command('N ({})'.format(obj.Label), {}),
            Path.Command('N (Tool type: {})'.format(oclTool.toolType), {}),
            Path.Command('N (Compensated Tool Path. Diameter: {})'.format(oclTool.diameter), {}),
            Path.Command('N (Sample interval: '
                         '{})'.format(obj.SampleInterval.Value), {}),
            Path.Command('N (Step over %: {})'.format(obj.StepOver), {}),
            Path.Command('G0', {'Z': obj.ClearanceHeight.Value,
                                'F': self.vertRapid})
        ])
        if obj.UseStartPoint:
            self.commandlist.append(Path.Command('G0', {'X': obj.StartPoint.x,
                                                        'Y': obj.StartPoint.y,
                                                        'F': self.horizRapid}))

        # Impose property limits
        self._opApplyPropertyLimits(obj)

        # Get height offset values for later use
        self.SafeHeightOffset = \
            self.job.SetupSheet.SafeHeightOffset.Value
        self.ClearHeightOffset = \
            self.job.SetupSheet.ClearanceHeightOffset.Value

        # Set deflection values for mesh generation
        useDGT = False
        try:  # try/except is for Path Jobs created before GeometryTolerance
            self.geoTlrnc = self.job.GeometryTolerance.Value
            if self.geoTlrnc == 0.0:
                useDGT = True
        except AttributeError as ee:
            msg = '{}\n'
            msg += translate('waterline',
                             ('Please set Job.GeometryTolerance'
                              ' to an acceptable value. Using'
                              ' PathPreferences.defaultGeometryTolerance().'))
            PathLog.warning(msg)
            useDGT = True
        if useDGT:
            import PathScripts.PathPreferences as PathPreferences
            self.geoTlrnc = PathPreferences.defaultGeometryTolerance()

        # Calculate default depthparams for operation
        self.depthParams = \
            PathUtils.depth_params(obj.ClearanceHeight.Value,
                                   obj.SafeHeight.Value,
                                   obj.StartDepth.Value,
                                   obj.StepDown.Value,
                                   0.0,
                                   obj.FinalDepth.Value)
        self.midDep = (obj.StartDepth.Value + obj.FinalDepth.Value) / 2.0

        # Setup STL, model type, and bound box containers for each model in Job
        for m in range(0, len(self.job.Model.Group)):
            M = self.job.Model.Group[m]
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
                self.boundBoxes.append(self.job.Stock.Shape.BoundBox)

        # ######  MAIN COMMANDS FOR OPERATION ######

        # Begin processing obj.Base data and creating GCode
        PSF = path_surface_support.ProcessSelectedFaces(self.job, obj)
        if self.showDebugObjects:
            PSF.setShowDebugObjects(self.tmpDebugGrp, self.showDebugObjects)
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

            for m in range(0, len(self.job.Model.Group)):
                # Create OCL.stl model objects
                if obj.Algorithm == 'OCL Dropcutter':
                    path_surface_support._prepareModelSTLs(self, self.job,
                                                         obj, m, ocl)

                Mdl = self.job.Model.Group[m]
                if FACES[m] is False:
                    lbl = self.job.Model.Group[m].Label
                    PathLog.error('No data for model base: {}'.format(lbl))
                else:
                    if m > 0:
                        # Raise to clearance between models
                        CMDS.append(Path.Command(('N (Transition to base: '
                                                  '{}.)'.format(Mdl.Label))))
                        cmdDict = {'Z': obj.ClearanceHeight.Value,
                                   'F': self.vertRapid}
                        CMDS.append(Path.Command('G0', cmdDict))
                        PathLog.info('Working on Model.Group[{}]: {}'.format(m, Mdl.Label))
                    # make stock-model-voidShapes STL model
                    #     for avoidance detection on transitions
                    if obj.Algorithm == 'OCL Dropcutter':
                        path_surface_support._makeSafeSTL(self, self.job, obj,
                                                        m, FACES[m], VOIDS[m],
                                                        ocl)
                    # Process model/faces - OCL objects must be ready
                    CMDS.extend(self._processWaterlineAreas(self.job, obj,
                                                            m, FACES[m],
                                                            VOIDS[m]))

            # Save gcode produced
            self.commandlist.extend(CMDS)

        # ######  CLOSING COMMANDS FOR OPERATION ######

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

        execTime = time.time() - startTime
        msg = translate('PathWaterline', 'operation time is')
        PathLog.info('Waterline ' + msg + ' {} sec.'.format(execTime))

        return True

    def _opApplyPropertyLimits(self, obj):
        '''_opApplyPropertyLimits(obj) ... Apply necessary limits to user input property values before performing main operation.'''
        # Limit sample interval
        if obj.SampleInterval.Value < 0.0001:
            obj.SampleInterval.Value = 0.0001
            PathLog.error(translate('PathWaterline', 'Sample interval limits are 0.0001 to 25.4 millimeters.'))
        if obj.SampleInterval.Value > 25.4:
            obj.SampleInterval.Value = 25.4
            PathLog.error(translate('PathWaterline', 'Sample interval limits are 0.0001 to 25.4 millimeters.'))

        # Limit cut pattern angle
        if obj.CutPatternAngle < -360.0:
            obj.CutPatternAngle = 0.0
            PathLog.error(translate('PathWaterline', 'Cut pattern angle limits are +-360 degrees.'))
        if obj.CutPatternAngle >= 360.0:
            obj.CutPatternAngle = 0.0
            PathLog.error(translate('PathWaterline', 'Cut pattern angle limits are +- 360 degrees.'))

        # Limit StepOver to natural number percentage
        if obj.StepOver > 100.0:
            obj.StepOver = 100.0
        if obj.StepOver < 1.0:
            obj.StepOver = 1.0

        # Limit AvoidLastX_Faces to zero and positive values
        if obj.AvoidLastX_Faces < 0:
            obj.AvoidLastX_Faces = 0
            PathLog.error(translate('PathWaterline', 'AvoidLastX_Faces: Only zero or positive values permitted.'))
        if obj.AvoidLastX_Faces > 100:
            obj.AvoidLastX_Faces = 100
            PathLog.error(translate('PathWaterline', 'AvoidLastX_Faces: Avoid last X faces count limited to 100.'))

    # Methods for constructing the cut area and creating path geometry
    def _processWaterlineAreas(self, JOB, obj, mdlIdx, FCS, VDS):
        '''_processWaterlineAreas(JOB, obj, mdlIdx, FCS, VDS)...
        This method applies any avoided faces or regions to the selected faces.
        It then calls the correct method.'''
        PathLog.debug('_processWaterlineAreas()')

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

            final.append(Path.Command('G0', {'Z': obj.SafeHeight.Value,
                                             'F': self.vertRapid}))
            # set independent method for Waterline
            if obj.Algorithm == 'OCL Dropcutter':
                final.extend(self._oclWaterlineOp(JOB, obj, mdlIdx, COMP))
            else:
                final.extend(self._experimentalWaterlineOp(JOB,
                                                           obj, mdlIdx, COMP))

        elif obj.HandleMultipleFeatures == 'Individually':
            for fsi in range(0, len(FCS)):
                fShp = FCS[fsi]
                self._resetOpVariables()

                if fShp is True:
                    COMP = False
                else:
                    ADD = Part.makeCompound([fShp])
                    if VDS is not False:
                        DEL = Part.makeCompound(VDS)
                        COMP = ADD.cut(DEL)
                    else:
                        COMP = ADD

                final.append(Path.Command('G0', {'Z': obj.SafeHeight.Value,
                                                 'F': self.vertRapid}))
                # set independent method for Waterline
                if obj.Algorithm == 'OCL Dropcutter':
                    final.extend(self._oclWaterlineOp(JOB, obj, mdlIdx, COMP))
                else:
                    final.extend(self._experimentalWaterlineOp(JOB,
                                                               obj,
                                                               mdlIdx,
                                                               COMP))
                COMP = None
        # Eif

        return final

    def _getExperimentalWaterlinePaths(self, PNTSET, csHght, cutPattern):
        '''_getExperimentalWaterlinePaths(PNTSET, csHght, cutPattern)...
        Switching function for calling the appropriate path-geometry to OCL points conversion function
        for the various cut patterns.'''
        PathLog.debug('_getExperimentalWaterlinePaths()')
        SCANS = list()

        # PNTSET is list, by stepover.
        if cutPattern in ['Line', 'Spiral', 'ZigZag']:
            stpOvr = list()
            for STEP in PNTSET:
                for SEG in STEP:
                    if SEG == 'BRK':
                        stpOvr.append(SEG)
                    else:
                        (A, B) = SEG  # format is ((p1, p2), (p3, p4))
                        P1 = FreeCAD.Vector(A[0], A[1], csHght)
                        P2 = FreeCAD.Vector(B[0], B[1], csHght)
                        stpOvr.append((P1, P2))
                SCANS.append(stpOvr)
                stpOvr = list()
        elif cutPattern in ['Circular', 'CircularZigZag']:
            # Each stepover is a list containing arc/loop descriptions as:
            # (sp, ep, cp)
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
                            stpOvr.append(scan)
                if erFlg is False:
                    SCANS.append(stpOvr)

        return SCANS

    # Main planar scan functions
    def _stepTransitionCmds(self, obj, cutPattern,
                            lstPnt, first, minSTH, tolrnc):
        cmds = list()
        rtpd = False
        horizGC = 'G0'
        hSpeed = self.horizRapid
        height = obj.SafeHeight.Value

        if cutPattern in ['Line', 'Circular', 'Spiral']:
            if obj.OptimizeStepOverTransitions is True:
                height = minSTH + 2.0
        elif cutPattern in ['ZigZag', 'CircularZigZag']:
            if obj.OptimizeStepOverTransitions is True:
                zChng = first.z - lstPnt.z
                if abs(zChng) < tolrnc:  # transitions to same Z height
                    if (minSTH - first.z) > tolrnc:
                        height = minSTH + 2.0
                    else:
                        horizGC = 'G1'
                        height = first.z
                elif (minSTH + (2.0 * tolrnc)) >= max(first.z, lstPnt.z):
                    height = False  # allow end of Zig to cut to start of Zag

        # Create raise, shift, and optional lower commands
        if height is not False:
            cmds.append(Path.Command('G0', {'Z': height, 'F': self.vertRapid}))
            cmds.append(Path.Command(horizGC, {'X': first.x,
                                               'Y': first.y,
                                               'F': hSpeed}))
        if rtpd is not False:  # ReturnToPreviousDepth
            cmds.append(Path.Command('G0', {'Z': rtpd, 'F': self.vertRapid}))

        return cmds

    def _breakCmds(self, obj, cutPattern, lstPnt, first, minSTH, tolrnc):
        cmds = list()
        rtpd = False
        horizGC = 'G0'
        hSpeed = self.horizRapid
        height = obj.SafeHeight.Value

        if cutPattern in ['Line', 'Circular', 'Spiral']:
            if obj.OptimizeStepOverTransitions is True:
                height = minSTH + 2.0
        elif cutPattern in ['ZigZag', 'CircularZigZag']:
            if obj.OptimizeStepOverTransitions is True:
                zChng = first.z - lstPnt.z
                if abs(zChng) < tolrnc:  # transitions to same Z height
                    if (minSTH - first.z) > tolrnc:
                        height = minSTH + 2.0
                    else:
                        height = first.z + 2.0  # first.z

        cmds.append(Path.Command('G0', {'Z': height, 'F': self.vertRapid}))
        cmds.append(Path.Command(horizGC, {'X': first.x,
                                           'Y': first.y,
                                           'F': hSpeed}))
        if rtpd is not False:  # ReturnToPreviousDepth
            cmds.append(Path.Command('G0', {'Z': rtpd, 'F': self.vertRapid}))

        return cmds

    def _planarGetPDC(self, stl, finalDep, SampleInterval, cutter):
        pdc = ocl.PathDropCutter()   # create a pdc [PathDropCutter] object
        pdc.setSTL(stl)  # add stl model
        pdc.setCutter(cutter)  # add cutter
        pdc.setZ(finalDep)  # set minimumZ (final / target depth value)
        pdc.setSampling(SampleInterval)  # set sampling size
        return pdc

    # OCL Dropcutter waterline functions
    def _oclWaterlineOp(self, JOB, obj, mdlIdx, subShp=None):
        '''_oclWaterlineOp(obj, base) ...
        Main waterline function to perform waterline extraction from model.
        '''
        commands = []

        base = JOB.Model.Group[mdlIdx]
        bb = self.boundBoxes[mdlIdx]
        stl = self.modelSTLs[mdlIdx]
        depOfst = obj.DepthOffset.Value

        # Prepare global holdpoint and layerEndPnt containers
        if self.holdPoint is None:
            self.holdPoint = FreeCAD.Vector(0.0, 0.0, 0.0)
        if self.layerEndPnt is None:
            self.layerEndPnt = FreeCAD.Vector(0.0, 0.0, 0.0)

        # Set extra offset to diameter of cutter to allow cutter
        # to move around perimeter of model
        toolDiam = self.cutter.getDiameter()

        if subShp is None:
            # Get correct boundbox
            if obj.BoundBox == 'Stock':
                BS = JOB.Stock
                bb = BS.Shape.BoundBox
            elif obj.BoundBox == 'BaseBoundBox':
                BS = base
                bb = base.Shape.BoundBox

            xmin = bb.XMin
            xmax = bb.XMax
            ymin = bb.YMin
            ymax = bb.YMax
        else:
            xmin = subShp.BoundBox.XMin
            xmax = subShp.BoundBox.XMax
            ymin = subShp.BoundBox.YMin
            ymax = subShp.BoundBox.YMax

        smplInt = obj.SampleInterval.Value
        minSampInt = 0.001  # value is mm
        if smplInt < minSampInt:
            smplInt = minSampInt

        # Determine bounding box length for the OCL scan
        bbLength = math.fabs(ymax - ymin)
        numScanLines = int(math.ceil(bbLength / smplInt) + 1)  # line count

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == 'Single-pass':
            depthparams = [obj.FinalDepth.Value]
        else:
            depthparams = [dp for dp in self.depthParams]
        lenDP = len(depthparams)

        # Scan the piece to depth at smplInt
        oclScan = []
        oclScan = self._waterlineDropCutScan(stl, smplInt, xmin, xmax, ymin,
                                             depthparams[lenDP - 1],
                                             numScanLines)
        oclScan = [FreeCAD.Vector(P.x, P.y, P.z + depOfst) for P in oclScan]
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
        msg = "--OCL scan: " + str(lenSL * pntsPerLine) + " points, with "
        msg += str(numScanLines) + " lines and "
        msg += str(pntsPerLine) + " pts/line"
        PathLog.debug(msg)

        # Extract Wl layers per depthparams
        lyr = 0
        cmds = []
        layTime = time.time()
        self.topoMap = []
        for layDep in depthparams:
            cmds = self._getWaterline(obj, scanLines, layDep,
                                      lyr, lenSL, pntsPerLine)
            commands.extend(cmds)
            lyr += 1
        PathLog.debug("--All layer scans combined took " +
                      str(time.time() - layTime) + " s")
        return commands

    def _waterlineDropCutScan(self, stl, smplInt, xmin,
                              xmax, ymin, fd, numScanLines):
        '''
        _waterlineDropCutScan(stl, smplInt, xmin, xmax,
                              ymin, fd, numScanLines) ...
        Perform OCL scan for waterline purpose.
        '''
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

        # return the list of points
        return pdc.getCLPoints()

    def _getWaterline(self, obj, scanLines, layDep, lyr, lenSL, pntsPerLine):
        '''
        _getWaterline(obj, scanLines, layDep, lyr, lenSL, pntsPerLine) ...
        Get waterline.
        '''
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

        prev = FreeCAD.Vector(2135984513.165, -58351896873.17455, 13838638431.861)
        nxt = FreeCAD.Vector(0.0, 0.0, 0.0)

        # Create first point
        pnt = FreeCAD.Vector(loop[0].x, loop[0].y, layDep)

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

            output.append(Path.Command('G1', {'X': pnt.x, 'Y': pnt.y, 'F': self.horizFeed}))

            # Rotate point data
            prev = pnt
            pnt = nxt

        # Save layer end point for use in transitioning to next layer
        self.layerEndPnt = pnt

        return output

    # Experimental waterline functions
    def _experimentalWaterlineOp(self, JOB, obj, mdlIdx, subShp=None):
        '''_waterlineOp(JOB, obj, mdlIdx, subShp=None) ...
        Main waterline function to perform waterline extraction from model.'''
        PathLog.debug('_experimentalWaterlineOp()')

        commands = []
        t_begin = time.time()
        base = JOB.Model.Group[mdlIdx]
        # safeSTL = self.safeSTLs[mdlIdx]
        self.endVector = None

        finDep = obj.FinalDepth.Value + (self.geoTlrnc / 10.0)
        depthParams = PathUtils.depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown.Value, 0.0, finDep)

        # Compute number and size of stepdowns, and final depth
        if obj.LayerMode == 'Single-pass':
            depthparams = [finDep]
        else:
            depthparams = [dp for dp in depthParams]
        PathLog.debug('Experimental Waterline depthparams:\n{}'.format(depthparams))

        # Prepare PathDropCutter objects with STL data
        # safePDC = self._planarGetPDC(safeSTL, depthparams[lenDP - 1], obj.SampleInterval.Value, self.cutter)

        buffer = self.cutter.getDiameter() * 10.0
        borderFace = Part.Face(self._makeExtendedBoundBox(JOB.Stock.Shape.BoundBox, buffer, 0.0))

        # Get correct boundbox
        if obj.BoundBox == 'Stock':
            stockEnv = path_surface_support.getShapeEnvelope(JOB.Stock.Shape)
            bbFace = path_surface_support.getCrossSection(stockEnv)  # returned at Z=0.0
        elif obj.BoundBox == 'BaseBoundBox':
            baseEnv = path_surface_support.getShapeEnvelope(base.Shape)
            bbFace = path_surface_support.getCrossSection(baseEnv)  # returned at Z=0.0

        trimFace = borderFace.cut(bbFace)
        self.parent._addDebugObject(trimFace, 'TrimFace')

        # Cycle through layer depths
        CUTAREAS = self._getCutAreas(base.Shape, depthparams, bbFace, trimFace, borderFace)
        if not CUTAREAS:
            PathLog.error('No cross-section cut areas identified.')
            return commands

        caCnt = 0
        ofst = -1 * obj.BoundaryAdjustment.Value
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
                self.parent._addDebugObject(area, 'CutArea_{}'.format(caCnt))
            else:
                data = FreeCAD.Units.Quantity(csHght, FreeCAD.Units.Length).UserString
                PathLog.debug('Cut area at {} is zero.'.format(data))

            # get offset wire(s) based upon cross-section cut area
            if cont:
                area.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - area.BoundBox.ZMin))
                activeArea = area.cut(trimFace)
                activeAreaWireCnt = len(activeArea.Wires)  # first wire is boundFace wire
                self.parent._addDebugObject(activeArea, 'ActiveArea_{}'.format(caCnt))
                ofstArea = path_surface_support.extractFaceOffset(activeArea, ofst, self.wpc, makeComp=False)
                if not ofstArea:
                    data = FreeCAD.Units.Quantity(csHght, FreeCAD.Units.Length).UserString
                    PathLog.debug('No offset area returned for cut area depth at {}.'.format(data))
                    cont = False

            if cont:
                # Identify solid areas in the offset data
                if obj.CutPattern == 'Offset' or obj.CutPattern == 'None':
                    ofstSolidFacesList = self._getSolidAreasFromPlanarFaces(ofstArea)
                    if ofstSolidFacesList:
                        clearArea = Part.makeCompound(ofstSolidFacesList)
                        self.parent._addDebugObject(clearArea, 'ClearArea_{}'.format(caCnt))
                    else:
                        cont = False
                        data = FreeCAD.Units.Quantity(csHght, FreeCAD.Units.Length).UserString
                        PathLog.error('Could not determine solid faces at {}.'.format(data))
                else:
                    clearArea = activeArea

            if cont:
                data = FreeCAD.Units.Quantity(csHght, FreeCAD.Units.Length).UserString
                PathLog.debug('... Clearning area at {}.'.format(data))
                # Make waterline path for current CUTAREA depth (csHght)
                commands.extend(self._wiresToWaterlinePath(obj, clearArea, csHght))
                clearArea.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - clearArea.BoundBox.ZMin))
                lastClearArea = clearArea
                lastCsHght = csHght

                # Clear layer as needed
                (clrLyr, clearLastLayer) = self._clearLayer(obj, ca, lastCA, clearLastLayer)
                if clrLyr == 'Offset':
                    commands.extend(self._makeOffsetLayerPaths(obj, clearArea, csHght))
                elif clrLyr:
                    cutPattern = obj.CutPattern
                    if clearLastLayer is False:
                        cutPattern = obj.ClearLastLayer
                    commands.extend(self._makeCutPatternLayerPaths(JOB, obj, clearArea, csHght, cutPattern))
        # Efor

        if clearLastLayer and obj.ClearLastLayer != 'Off':
            PathLog.debug('... Clearning last layer')
            (clrLyr, cLL) = self._clearLayer(obj, 1, 1, False)
            lastClearArea.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - lastClearArea.BoundBox.ZMin))
            if clrLyr == 'Offset':
                commands.extend(self._makeOffsetLayerPaths(obj, lastClearArea, lastCsHght))
            elif clrLyr:
                commands.extend(self._makeCutPatternLayerPaths(JOB, obj, lastClearArea, lastCsHght, obj.ClearLastLayer))

        return commands

    def _getCutAreas(self, shape, depthparams, bbFace, trimFace, borderFace):
        '''_getCutAreas(JOB, shape, depthparams, bbFace, borderFace) ...
        Takes shape, depthparams and base-envelope-cross-section, and
        returns a list of cut areas - one for each depth.'''
        PathLog.debug('_getCutAreas()')

        CUTAREAS = list()
        isFirst = True
        lenDP = len(depthparams)
        
        # Cycle through layer depths
        for dp in range(0, lenDP):
            csHght = depthparams[dp]
            # PathLog.debug('Depth {} is {}'.format(dp + 1, csHght))

            # Get slice at depth of shape
            csFaces = self._getModelCrossSection(shape, csHght)  # returned at Z=0.0
            if not csFaces:
                data = FreeCAD.Units.Quantity(csHght, FreeCAD.Units.Length).UserString
            else:
                if len(csFaces) > 0:
                    useFaces = self._getSolidAreasFromPlanarFaces(csFaces)
                else:
                    useFaces = False

                if useFaces:
                    compAdjFaces = Part.makeCompound(useFaces)
                    self.parent._addDebugObject(compAdjFaces, 'Solids_{}'.format(dp + 1))
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
        self.parent._addDebugObject(ofstPlnrShp, 'WaterlinePathArea_{}'.format(round(csHght, 2)))

        commands.append(Path.Command('N (Cut Area {}.)'.format(round(csHght, 2))))
        start = 1
        if csHght < obj.IgnoreOuterAbove:
            start = 0
        for w in range(start, len(ofstPlnrShp.Wires)):
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

    def _makeCutPatternLayerPaths(self, JOB, obj, clrAreaShp, csHght, cutPattern):
        PathLog.debug('_makeCutPatternLayerPaths()')
        commands = []

        clrAreaShp.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - clrAreaShp.BoundBox.ZMin))

        # Convert pathGeom to gcode more efficiently
        if cutPattern == 'Offset':
            commands.extend(self._makeOffsetLayerPaths(obj, clrAreaShp, csHght))
        else:
            # Request path geometry from external support class
            PGG = path_surface_support.PathGeometryGenerator(obj, clrAreaShp, cutPattern)
            if self.showDebugObjects:
                PGG.setDebugObjectsGroup(self.tmpDebugGrp)
            self.tmpCOM = PGG.getCenterOfPattern()
            pathGeom = PGG.generatePathGeometry()
            if not pathGeom:
                PathLog.warning('No path geometry generated.')
                return commands
            pathGeom.translate(FreeCAD.Vector(0.0, 0.0, csHght - pathGeom.BoundBox.ZMin))

            self.parent._addDebugObject(pathGeom, 'PathGeom_{}'.format(round(csHght, 2)))

            if cutPattern == 'Line':
                pntSet = path_surface_support.pathGeomToLinesPointSet(obj, pathGeom, self.CutClimb, self.toolDiam, self.closedGap, self.gaps)
            elif cutPattern == 'ZigZag':
                pntSet = path_surface_support.pathGeomToZigzagPointSet(obj, pathGeom, self.CutClimb, self.toolDiam, self.closedGap, self.gaps)
            elif cutPattern in ['Circular', 'CircularZigZag']:
                pntSet = path_surface_support.pathGeomToCircularPointSet(obj, pathGeom, self.CutClimb, self.toolDiam, self.closedGap, self.gaps, self.tmpCOM)
            elif cutPattern == 'Spiral':
                pntSet = path_surface_support.pathGeomToSpiralPointSet(obj, pathGeom)

            stpOVRS = self._getExperimentalWaterlinePaths(pntSet, csHght, cutPattern)
            safePDC = False
            cmds = self._clearGeomToPaths(JOB, obj, safePDC, stpOVRS, cutPattern)
            commands.extend(cmds)

        return commands

    def _makeOffsetLayerPaths(self, obj, clrAreaShp, csHght):
        PathLog.debug('_makeOffsetLayerPaths()')
        cmds = list()
        ofst = 0.0 - self.cutOut
        shape = clrAreaShp
        cont = True
        cnt = 0
        while cont:
            ofstArea = path_surface_support.extractFaceOffset(shape, ofst, self.wpc, makeComp=True)
            if not ofstArea:
                break
            for F in ofstArea.Faces:
                cmds.extend(self._wiresToWaterlinePath(obj, F, csHght))
            shape = ofstArea
            if cnt == 0:
                ofst = 0.0 - self.cutOut
            cnt += 1
        PathLog.debug(' -Offset path count: {} at height: {}'.format(cnt, round(csHght, 2)))

        return cmds

    def _clearGeomToPaths(self, JOB, obj, safePDC, stpOVRS, cutPattern):
        PathLog.debug('_clearGeomToPaths()')

        GCODE = [Path.Command('N (Beginning of Single-pass layer.)', {})]
        tolrnc = JOB.GeometryTolerance.Value
        lenstpOVRS = len(stpOVRS)
        lstSO = lenstpOVRS - 1
        lstStpOvr = False
        gDIR = ['G3', 'G2']

        if self.CutClimb is True:
            gDIR = ['G2', 'G3']

        # Send cutter to x,y position of first point on first line
        first = stpOVRS[0][0][0]  # [step][item][point]
        GCODE.append(Path.Command('G0', {'X': first.x, 'Y': first.y, 'F': self.horizRapid}))

        # Cycle through step-over sections (line segments or arcs)
        odd = True
        lstStpEnd = None
        for so in range(0, lenstpOVRS):
            cmds = list()
            PRTS = stpOVRS[so]
            lenPRTS = len(PRTS)
            first = PRTS[0][0]  # first point of arc/line stepover group
            last = None
            cmds.append(Path.Command('N (Begin step {}.)'.format(so), {}))
            if so == lstSO:
                lstStpOvr = True

            if so > 0:
                if cutPattern == 'CircularZigZag':
                    if odd:
                        odd = False
                    else:
                        odd = True
                # minTrnsHght = self._getMinSafeTravelHeight(safePDC, lstStpEnd, first)  # Check safe travel height against fullSTL
                minTrnsHght = obj.SafeHeight.Value
                # cmds.append(Path.Command('N (Transition: last, first: {}, {}:  minSTH: {})'.format(lstStpEnd, first, minTrnsHght), {}))
                cmds.extend(self._stepTransitionCmds(obj, cutPattern, lstStpEnd, first, minTrnsHght, tolrnc))

            # Cycle through current step-over parts
            for i in range(0, lenPRTS):
                prt = PRTS[i]
                # PathLog.debug('prt: {}'.format(prt))
                if prt == 'BRK':
                    nxtStart = PRTS[i + 1][0]
                    # minSTH = self._getMinSafeTravelHeight(safePDC, last, nxtStart)  # Check safe travel height against fullSTL
                    minSTH = obj.SafeHeight.Value
                    cmds.append(Path.Command('N (Break)', {}))
                    cmds.extend(self._breakCmds(obj, cutPattern, last, nxtStart, minSTH, tolrnc))
                else:
                    cmds.append(Path.Command('N (part {}.)'.format(i + 1), {}))
                    if cutPattern in ['Line', 'ZigZag', 'Spiral']:
                        start, last = prt
                        cmds.append(Path.Command('G1', {'X': start.x, 'Y': start.y, 'Z': start.z, 'F': self.horizFeed}))
                        cmds.append(Path.Command('G1', {'X': last.x, 'Y': last.y, 'F': self.horizFeed}))
                    elif cutPattern in ['Circular', 'CircularZigZag']:
                        # isCircle = True if lenPRTS == 1 else False
                        isZigZag = True if cutPattern == 'CircularZigZag' else False
                        PathLog.debug('so, isZigZag, odd, cMode: {}, {}, {}, {}'.format(so, isZigZag, odd, prt[3]))
                        gcode = self._makeGcodeArc(prt, gDIR, odd, isZigZag)
                        cmds.extend(gcode)
            cmds.append(Path.Command('N (End of step {}.)'.format(so), {}))
            GCODE.extend(cmds)  # save line commands
            lstStpEnd = last
        # Efor

        # Raise to safe height after clearing
        GCODE.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))

        return GCODE

    def _getSolidAreasFromPlanarFaces(self, csFaces):
        PathLog.debug('_getSolidAreasFromPlanarFaces()')
        holds = list()
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
            
            for af in range(lenCsF - 1, -1, -1):  # cycle from last item toward first
                prnt = pIds[af]
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

            for af in range(0, lenCsF):
                pFc = cIds[af]
                if pFc == -1:
                    # Simple, independent region
                    holds[af] = csFaces[af]  # place face in hold
                else:
                    # Compound region
                    cnt = len(pFc)
                    if cnt % 2.0 == 0.0:
                        # even is donut cut
                        inr = pFc[cnt - 1]
                        otr = pFc[cnt - 2]
                        holds[otr] = holds[otr].cut(csFaces[inr])
                    else:
                        # odd is floating solid
                        holds[af] = csFaces[af]

            for af in range(0, lenCsF):
                if holds[af]:
                    useFaces.append(holds[af])  # save independent solid
        # Eif

        if len(useFaces) > 0:
            return useFaces

        return False

    def _getModelCrossSection(self, shape, csHght):
        PathLog.debug('_getModelCrossSection()')
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

        return pIds

    def _wireToPath(self, obj, wire, startVect):
        '''_wireToPath(obj, wire, startVect) ... wire to path.'''
        PathLog.track()

        paths = []
        pathParams = {} # pylint: disable=assignment-from-no-return

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

    def _makeGcodeArc(self, prt, gDIR, odd, isZigZag):
        cmds = list()
        strtPnt, endPnt, cntrPnt, cMode = prt
        gdi = 0
        if odd:
            gdi = 1
        else:
            if not cMode and isZigZag:
                gdi = 1
        gCmd = gDIR[gdi]

        # ijk = self.tmpCOM - strtPnt
        # ijk = self.tmpCOM.sub(strtPnt)  # vector from start to center
        ijk = cntrPnt.sub(strtPnt)  # vector from start to center
        xyz = endPnt
        cmds.append(Path.Command('G1', {'X': strtPnt.x, 'Y': strtPnt.y, 'Z': strtPnt.z, 'F': self.horizFeed}))
        cmds.append(Path.Command(gCmd, {'X': xyz.x, 'Y': xyz.y, 'Z': xyz.z,
                                            'I': ijk.x, 'J': ijk.y, 'K': ijk.z,  # leave same xyz.z height
                                            'F': self.horizFeed}))
        cmds.append(Path.Command('G1', {'X': endPnt.x, 'Y': endPnt.y, 'Z': endPnt.z, 'F': self.horizFeed}))

        return cmds

    def _clearLayer(self, obj, ca, lastCA, clearLastLayer):
        PathLog.debug('_clearLayer()')
        clrLyr = False

        if obj.ClearLastLayer == 'Off':
            if obj.CutPattern != 'None':
                clrLyr = obj.CutPattern
        else:
            obj.CutPattern = 'None'
            if ca == lastCA:  # if current iteration is last layer
                PathLog.debug('... Clearing bottom layer.')
                clrLyr = obj.ClearLastLayer
                clearLastLayer = False

        return (clrLyr, clearLastLayer)

    # Support methods
    def _resetOpVariables(self):
        '''_resetOpVariables() ... Reset class variables used for instance of operation.'''
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
# Eclass
