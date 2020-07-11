# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

__title__ = "Path Profile Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = ("Path Profile operation based on entire model,"
           " selected faces or selected edges.")
__contributors__ = "Schildkroet"

# Standard
# Third-party
# FreeCAD
import PathScripts.PathOpBase as PathOpBase


OpFeatures = PathOpBase.OpFeatures
LazyLoader = PathOpBase.LazyLoader
# Add pointers to existing imports in PathOpBase
FreeCAD   = PathOpBase.FreeCAD
Path      = PathOpBase.Path
PathLog   = PathOpBase.PathLog
PathUtils = PathOpBase.PathUtils
QtCore    = PathOpBase.QtCore
# lazily loaded modules
ArchPanel = LazyLoader('ArchPanel', globals(), 'ArchPanel')
numpy = LazyLoader('numpy', globals(), 'numpy')
Part = LazyLoader('Part', globals(), 'Part')
PathSurfaceSupport = \
    LazyLoader('PathScripts.path_operations.surface_support',
               globals(), 'PathScripts.path_operations.surface_support')
Rotation = \
    LazyLoader('PathScripts.path_support.rotation',
               globals(), 'PathScripts.path_support.rotation')
Profile_OpenEdge = \
    LazyLoader('PathScripts.path_operations.profile_openedge',
               globals(), 'PathScripts.path_operations.profile_openedge')


# Qt translation handling
translate = PathOpBase.translate


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Error message
noSelfMsg = translate('profile', 'No "self" argument passed.')


class ProfileOperation:
    '''Given an operation object and its proxy (parent) class,
    produces a list of shape detail tuples to be processed
    by Path.Area into gcode commands.
    '''

    def __init__(self, obj, parent, initialize=True):
        '''__init__(obj, parent) ...
        Constructor for Profile operation class.
        '''
        PathLog.debug('ProfileOperation.__init__()')
        # Required operation instance attributes
        self.parent = parent
        self.obj = obj

        self.job = parent.job
        self.propertyEditorModes = None
        self.propertyEnumerations = None
        self.isDebug = parent.isDebug  # Debug attribute
        self.showDebugObjects = parent.showDebugObjects  # Debug attribute
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
        self.commandlist = None  # post-execution
        self.depthparams = None  # post-execution
        self.expandProfile = None  # post-execution
        self.offsetExtra = None  # post-execution
        self.ofstRadius = None  # post-execution
        self.inaccessibleMsg = \
            translate('PathProfile', 'The selected edge(s) are inaccessible.' +
                      ' ' + 'If multiple, re-ordering selection might work.')
        self.useComp = False
        # make circle for workplane
        self.workPlaneRef = Part.makeCircle(2.0)

    def _resetAnalysisAttributes(self):
        self.isReady = False
        self._processTypes = list()  # Model, Face, ClosedEdge, OpenEdge
        self._Bases = list()
        self._ZMin = None

    def _analyzeBaseGeometry(self):
        PathLog.debug('_analyzeBaseGeometry()')

        # Reset analysis attributes
        self._resetAnalysisAttributes()

        if not hasattr(self.obj, 'Base'):
            msg = translate('PathSlot',
                            'No Base Geometry object in the operation.')
            PathLog.debug(msg + '\n')
            return

        if len(self.obj.Base) == 0:
            self._processTypes.append('Model')
            self.isReady = True
            if self.parent.job:
                self._ZMin = self.parent.job.Stock.Shape.BoundBox.ZMin
            return

        # Set initial zmin value
        firstTup = self.obj.Base[0]
        firstFeature = firstTup[0].Shape.getElement(firstTup[1][0])
        zmin = firstFeature.BoundBox.ZMax

        # Process Base Geometry, sorting and separating per base and feature
        for (base, featList) in self.obj.Base:
            # instantiate blank group containers
            faces = {'subs': list(), 'shapes': list()}
            edges = {'subs': list(), 'shapes': list()}

            # Group features by type, and determine absolute ZMin.
            for feat in featList:
                featType = feat[:4]
                subShp = getattr(base.Shape, feat)
                if featType == 'Face':
                    group = faces
                elif featType == 'Edge':
                    group = edges
                group['subs'].append(feat)
                group['shapes'].append(subShp)
                try:
                    fbb = subShp.BoundBox
                    zmin = min(zmin, fbb.ZMin)
                except Part.OCCError as e:
                    PathLog.error(e)

            if len(edges['subs']) > 0:
                wireList = [Part.Wire(e) for e in Part.sortEdges(edges['shapes'])]
                for wire in wireList:
                    prcsType = 'OpenEdge'  # default to open edge(wire)
                    if wire.isClosed():
                        prcsType = 'ClosedEdge'  # change to closed edge(wire)
                    item = {
                        'base': base,
                        'subsList': edges['subs'],
                        'zMin': wire.BoundBox.ZMin,
                        'shapes': [wire],
                        'processType': prcsType
                    }
                    self._Bases.append(item)
                    self._processTypes.append(prcsType)

            if len(faces['subs']) > 0:
                comp = Part.makeCompound(faces['shapes'])
                item = {
                    'base': base,
                    'subsList': faces['subs'],
                    'zMin': comp.BoundBox.ZMin,
                    'shapes': faces['shapes'],
                    'processType': 'Face'
                }
                self._Bases.append(item)
                self._processTypes.append('Face')

        self.isReady = True
        self._ZMin = zmin

    def _revisePropertyEnumerations(self):
        '''
        Method returns dictionary of property enumerations based on
        active conditions in the operation.
        Use one internal function here pertaining to each property for
        which you wish to modify its enumerations.  The enumerations
        maniplated here need to be syncronized with the class defaults
        returned from `getInitialPropertyEnumerations()`.
        '''

        # Internal function to adjust property enumeration list
        def _updateProperty1():
            '''Customize `Property1` enumerations based on feature type.
            For example:
            An argument might references a feature type: Face, Edge, Vertex.
            That feature type could be used to determine which enumeration
            choices are available for this property. The updated enumeration
            list would be returned from this function.
            See PathSlotOperation module for a practical example.'''
            pass

        # Retrieve full enumeration lists
        enums = self.getInitialPropertyEnumerations()

        # Place 'enums' modification logic here.

        # The class instance attribute is updated.
        return enums

    def _getOperationType(self):
        if not hasattr(self.obj, 'Base'):
            return 'Contour'

        if len(self.obj.Base) == 0:
            return 'Contour'

        # return first geometry type selected
        (base, subsList) = self.obj.Base[0]
        return subsList[0][:4]

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
        return ['FeatureBasePanels', 'FeatureBaseEdges', 'FeatureBaseFaces']

    @staticmethod
    def getPropertyDefinitions():
        '''getPropertyDefinitions() ...
        This method returns initial property definitions as a list of tuples.
        '''
        # Definition format: (prop_type, name, group, tooltip)
        return [
            ("App::PropertyEnumeration", "Direction", "Profile",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction that the toolpath should go around the part ClockWise (CW) or CounterClockWise (CCW)")),
            ("App::PropertyLength", "ExpandProfile", "Profile",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Extend the profile clearing beyond the Extra Offset.")),
            ("App::PropertyPercent", "ExpandProfileStepOver", "Profile",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Set the stepover percentage, based on the tool's diameter.")),
            ("App::PropertyEnumeration", "HandleMultipleFeatures", "Profile",
                QtCore.QT_TRANSLATE_NOOP("PathPocket", "Choose how to process multiple Base Geometry features.")),
            ("App::PropertyEnumeration", "JoinType", "Profile",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Controls how tool moves around corners. Default=Round")),
            ("App::PropertyFloat", "MiterLimit", "Profile",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Maximum distance before a miter join is truncated")),
            ("App::PropertyDistance", "OffsetExtra", "Profile",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Extra value to stay away from final profile- good for roughing toolpath")),
            ("App::PropertyBool", "processHoles", "Profile",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile holes as well as the outline")),
            ("App::PropertyBool", "processPerimeter", "Profile",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile the outline")),
            ("App::PropertyBool", "processCircles", "Profile",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile round holes")),
            ("App::PropertyEnumeration", "Side", "Profile",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Side of edge that tool should cut")),
            ("App::PropertyBool", "UseComp", "Profile",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Make True, if using Cutter Radius Compensation")),

            ("App::PropertyBool", "ReverseDirection", "Rotation",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Reverse direction of pocket operation.")),
            ("App::PropertyBool", "InverseAngle", "Rotation",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Inverse the angle. Example: -22.5 -> 22.5 degrees.")),
            ("App::PropertyBool", "AttemptInverseAngle", "Rotation",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Attempt the inverse angle for face access if original rotation fails.")),
            ("App::PropertyBool", "LimitDepthToFace", "Rotation",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Enforce the Z-depth of the selected face as the lowest value for final depth. Higher user values will be observed."))
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
            # this is the direction that the profile runs
            'Direction': ['CW', 'CCW'],
            'HandleMultipleFeatures': ['Collectively', 'Individually'],
            'JoinType': ['Round', 'Square', 'Miter'],
            # side that cutter is on in relation to the profile
            'Side': ['Outside', 'Inside'],
        }

    def getPropertyDefaultValues(self):
        '''getPropertyDefaultValues() ...
        This method returns initial property default values if not called
        from a class instance.  If called from a class instance, then the
        default values will reflect operation and job object influences.
        '''
        PathLog.debug('getPropertyDefaultValues()')

        initialValues = {
            'AttemptInverseAngle': True,
            'Direction': 'CW',
            'ExpandProfile': 0.0,
            'ExpandProfileStepOver': 100,
            'HandleMultipleFeatures': 'Individually',
            'InverseAngle': False,
            'JoinType': 'Round',
            'LimitDepthToFace': True,
            'MiterLimit': 0.1,
            'OffsetExtra': 0.0,
            'ReverseDirection': False,
            'Side': 'Outside',
            'UseComp': True,
            'processCircles': False,
            'processHoles': False,
            'processPerimeter': True
        }

        if not self:
            FreeCAD.Console.PrintWarning(noSelfMsg + ' <> 3\n')
            return initialValues

        """
        Insert modifications here to change initial values in default dict.
        The source operation (object) is accessible by self.obj.
        The source operation class (proxy) is accessible by self.parent
        """

        return initialValues

    def getPropertyEditorModes(self):
        '''
        getPropertyEditorModes() ...
        Use logic to set a mode variable assigned to one or more properties.
        Enter the property and its mode variable in the return dictionary.
        This method will also be used by the GUI module to set visibility
        in both the Data tab of the Property View and the Tasks editor window.

        Must be called after proeperties are created.
        '''

        if not self.hasProperties:
            msg = translate('profile', 'No operation-specific properties.')
            FreeCAD.Console.PrintError(msg + '\n')
            return {}

        fc = 2
        # ml = 0 if self.obj.JoinType == 'Miter' else 2
        rotation = 2 if self.obj.EnableRotation == 'Off' else 0
        side = 0 if self.obj.UseComp else 2
        opType = self._getOperationType()

        if opType == 'Contour':
            side = 2
        elif opType == 'Face':
            fc = 0
        elif opType == 'Edge':
            pass

        # format is {'property': value (fixed integer, or variable)}
        return {
            'JoinType': 2,  # 2 = Hide
            'MiterLimit': 2,  # ml
            'Side': side,
            'HandleMultipleFeatures': fc,
            'processCircles': fc,
            'processHoles': fc,
            'processPerimeter': fc,
            'ReverseDirection': rotation,
            'InverseAngle': rotation,
            'AttemptInverseAngle': rotation,
            'LimitDepthToFace': rotation
        }

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
        if prop in ['UseComp', 'JoinType', 'EnableRotation', 'Base']:
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
        # Update Base Geometry analysis
        self._analyzeBaseGeometry()
        self._updatePropertyEnumerations(apply)
        self._updatePropertyEditorModes(apply)

    # Primary public method to generate shape tuples to be processed into paths
    def execute(self, parent, obj):
        '''execute() ...
        Execute the Profile operation.
        Returns a list of tuples formatted as follows:
            (shape,
             isHole,
             'string_operation_detail_or_control',
             angle,
             axis,
             start depth
             final depth)

        These tuples are intended to be post-processed in
        the areaOp_base class. There the shape and included details
        are converted to paths.
        '''
        PathLog.track()
        self.parent = parent
        self.job = self.parent.job
        self.obj = obj

        # Update Base Geometry analysis and parameters
        self.updateParameters(apply=True)

        if not self.isReady:
            msg = translate('PathProfile', "Profile operation not ready.")
            FreeCAD.Console.PrintError(msg + '\n')
            return False

        # Set pointers to parent-class execute() values
        self.commandlist = self.parent.commandlist
        self.depthparams = self.parent.depthparams
        self.offsetExtra = obj.OffsetExtra.Value
        self.ofstRadius = self.offsetExtra
        if obj.UseComp:
            self.useComp = True
        if obj.ExpandProfile.Value != 0.0:
            self.expandProfile = True

        shapeTuples = self._getShapeTuples()

        return shapeTuples

    # Main control methods for operation begin here
    def _getShapeTuples(self):
        '''_getShapeTuples() ...
        Control the process for the creating the shape detail tuples
        that will be further processed by Path.Area().
        '''
        PathLog.track()

        shapeTuples = list()

        if self.useComp:
            self.ofstRadius = self.parent.radius + self.offsetExtra
            cmdTxt = "(Compensated Tool Path. Diameter: " + \
                str(self.parent.radius * 2) + ")"
            self.commandlist.append(Path.Command(cmdTxt))
        else:
            self.commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        # Process entire model if no Base Geometry
        PathLog.debug('_processTypes: {}'.format(self._processTypes))
        if self._processTypes[0] == 'Model':
            # Try to build targets from the job models
            shapeTuples.extend(self._contourTheModel())
            return shapeTuples

        # Process self._Bases items that are pre-packed in dictionaries
        # The items are packed in the '_analyzeBaseGeometry()' method.
        for itemDict in self._Bases:
            processType = itemDict['processType']

            if processType == 'Face':
                shapeTuples.extend(self._processItem_Face(itemDict))

            elif processType == 'ClosedEdge':
                base = itemDict['base']
                for wire in itemDict['shapes']:
                    closedEdgeTup = self._processItem_ClosedEdge(base, wire)
                    if closedEdgeTup:
                        shapeTuples.append(closedEdgeTup)

            elif processType == 'OpenEdge':
                base = itemDict['base']
                for wire in itemDict['shapes']:
                    openEdgeTup = self._processItem_OpenEdge(base, wire)
                    if openEdgeTup:
                        shapeTuples.append(openEdgeTup)
        # Efor

        return shapeTuples

    # Face:
    def _processItem_Face(self, itemDict):
        PathLog.debug('_processItem_Face()')
        shapeTuples = list()
        holes = list()
        faces = list()
        processInternals = False
        baseSubsTuples = self._preProcessFacesForRoation(itemDict)
        finDep = self.obj.FinalDepth.Value
        strDep = self.obj.StartDepth.Value

        if self.obj.processCircles or self.obj.processHoles:
            processInternals = True

        """At this point, the `base` and `subsList` below is not
        the same as in the `self._Bases` items above.  This set
        has been adjusted for rotation.  Consequently, the base might
        be a new rotational clone base."""
        for itemTuple in baseSubsTuples:
            (base, subsList, angle, axis, stock) = itemTuple
            details = (angle, axis, strDep, finDep)
            # Cycle through faces in subsList, verifying orientation, and
            # identifying internal holes (including circles) to be processed,
            # and identify the face for perimeter processing.
            for sub in subsList:
                shape = getattr(base.Shape, sub)
                faces.append(shape)
                if processInternals:
                    if numpy.isclose(abs(shape.normalAt(0, 0).z), 1):
                        # is horizontal face
                        for wire in shape.Wires[1:]:
                            holes.append((base.Shape, wire))

            # Process circles and holes as requested by the user
            if holes:
                shapeTuples.extend(self._processFaceInternals(details, holes))

            # Process the perimeter as requested by the user
            if self.obj.processPerimeter:
                shapeTuples.extend(self._processFacePerimeter(details, faces))
        return shapeTuples

    def _preProcessFacesForRoation(self, itemDict):
        PathLog.debug('_preProcessFacesForRoation()')
        baseSubsTuples = list()
        allTuples = list()
        subCount = 0
        base = itemDict['base']
        subsList = itemDict['subsList']
        shapes = itemDict['shapes']

        if self.obj.EnableRotation != 'Off':
            for i in range(0, len(shapes)):
                shp = itemDict['shapes'][i]
                sub = itemDict['subsList'][i]
                subCount += 1
                tup = Rotation.analyzeFace(self, base, sub, shp, subCount)
                allTuples.append(tup)

            # Issue warning about Final Depth and multiple features
            if (subCount > 1 and
                    self.obj.HandleMultipleFeatures == 'Collectively'):
                msg = translate('PathProfile',
                                "Multiple faces in Base Geometry.") + "  "
                msg += translate('PathProfile',
                                 ("Depth settings will be applied"
                                  " to all faces."))
                FreeCAD.Console.PrintWarning(msg + '\n')

            # return (TagList, GroupList)
            (Tags, Grps) = Rotation.sortTuplesByIndex(self, allTuples, 2)
            subsList = []
            for o in range(0, len(Tags)):
                subsList = []
                for (base, sub, tag, angle, axis, stock) in Grps[o]:
                    subsList.append(sub)

                pair = base, subsList, angle, axis, stock
                baseSubsTuples.append(pair)
            # Efor
        else:
            stock = self.job.Stock
            for (base, subsList) in self.obj.Base:
                baseSubsTuples.append((base, subsList, 0.0, 'X', stock))

        return baseSubsTuples

    def _processFaceInternals(self, details, holes):
        PathLog.debug('_processFaceInternals()')
        (angle, axis, strDep, finDep) = details
        shapeTuples = list()

        for baseShape, wire in holes:
            cont = False
            intWireFace = Part.makeFace(wire, 'Part::FaceMakerSimple')
            tVect = FreeCAD.Vector(0.0, 0.0, finDep - wire.BoundBox.ZMin)
            intWireFace.translate(tVect)
            drillable = PathUtils.isDrillable(baseShape, wire)
            ot = self._openingType(baseShape, intWireFace, strDep, finDep)

            if self.obj.processCircles:
                if drillable:
                    if ot < 1:
                        cont = True
            if self.obj.processHoles:
                if not drillable:
                    if ot < 1:
                        cont = True
            if cont:
                PathLog.debug(' -processing internal')
                self.parent._addDebugObject(intWireFace, 'IntWireFace')
                if self.expandProfile:
                    startDep = self.obj.StartDepth.Value
                    shapeEnv = \
                        self._getExpandedProfileEnvelope(intWireFace,
                                                         True,
                                                         startDep,
                                                         finDep)
                else:
                    dp = self.depthparams
                    PathLog.debug(' -depthparams: {}'.format([i for i in dp]))
                    PathLog.debug(' -self.obj.StepDown: {}'.format(self.obj.StepDown))
                    shapeEnv = PathUtils.getEnvelope(intWireFace,
                                                     depthparams=dp)

                if shapeEnv:
                    self.parent._addDebugObject(shapeEnv, 'HoleShapeEnvelope')
                    tup = (shapeEnv, True, 'pathProfile',
                           angle, axis, strDep, finDep)
                    shapeTuples.append(tup)
        # for

        return shapeTuples

    def _processFacePerimeter(self, details, faces):
        PathLog.debug('_processFacePerimeter()')
        (angle, axis, strDep, finDep) = details
        shapeTuples = list()

        # Process selected faces Collectively
        if self.obj.HandleMultipleFeatures == 'Collectively':
            custDeps = self.depthparams
            cont = True

            if len(faces) > 0:
                profileshape = Part.makeCompound(faces)

            if (self.obj.LimitDepthToFace is True and
                    self.obj.EnableRotation != 'Off'):
                if profileshape.BoundBox.ZMin > self.obj.FinalDepth.Value:
                    finDep = profileshape.BoundBox.ZMin
                    custDeps = self._customDepthParams(self.obj,
                                                       strDep + 0.5,
                                                       finDep)

            try:
                if self.expandProfile:
                    startDep = self.obj.StartDepth.Value
                    shapeEnv = \
                        self._getExpandedProfileEnvelope(shape,
                                                         False,
                                                         startDep,
                                                         finDep)
                else:
                    shapeEnv = PathUtils.getEnvelope(profileshape,
                                                     depthparams=custDeps)
            except Exception as ee:
                # PathUtils.getEnvelope() failed to return an object.
                msg = translate('Path', 'Unable to create path for face(s).')
                PathLog.error(msg + '\n{}'.format(ee))
                cont = False

            if cont:
                self.parent._addDebugObject(shapeEnv, 'CollectCutShapeEnv')
                tup = (shapeEnv, False, 'pathProfile',
                       angle, axis, strDep, finDep)
                shapeTuples.append(tup)

        # Process selected faces Individually
        elif self.obj.HandleMultipleFeatures == 'Individually':
            for shape in faces:
                self.parent._addDebugObject(shape, 'FaceShape')
                finalDep = self.obj.FinalDepth.Value
                custDeps = self.depthparams

                if self.obj.Side == 'Inside':
                    if finalDep < shape.BoundBox.ZMin:
                        # Recalculate depthparams
                        finalDep = shape.BoundBox.ZMin
                        custDeps = self._customDepthParams(self.obj,
                                                           strDep + 0.5,
                                                           finalDep)

                if self.expandProfile:
                    startDep = self.obj.StartDepth.Value
                    shapeEnv = self._getExpandedProfileEnvelope(shape,
                                                                False,
                                                                startDep,
                                                                finalDep)
                else:
                    PathLog.debug(' -depthparams: {}'.format([i for i in custDeps]))
                    PathLog.debug(' -self.obj.StepDown: {}'.format(self.obj.StepDown))
                    shapeEnv = \
                        PathUtils.getEnvelope(shape, depthparams=custDeps)

                if shapeEnv:
                    self.parent._addDebugObject(shapeEnv, 'IndivPerimeterEnv')
                    tup = (shapeEnv, False, 'pathProfile',
                           angle, axis, strDep, finalDep)
                    shapeTuples.append(tup)
        # Eif

        return shapeTuples

    # ClosedEdge:
    def _processItem_ClosedEdge(self, base, wire):
        PathLog.debug('_processItem_ClosedEdge()')
        # Attempt to profile a closed wire
        shapeTuples = list()

        # Original face creation for closed wires is commented out below.
        # It might fail on non-planar closed wires.
        # f = Part.makeFace(wire, 'Part::FaceMakerSimple')
        _fW = self._flattenWire(wire, self.obj.FinalDepth.Value)
        (origWire, flatWire) = _fW
        f = origWire.Wires[0]

        if not f:
            PathLog.error(self.inaccessibleMsg)
            return False

        # shift the compound to the bottom of the base object
        #    for proper sectioning
        ezMin = base.Shape.BoundBox.ZMin
        zShift = ezMin - f.BoundBox.ZMin
        newPlace = FreeCAD.Placement(FreeCAD.Vector(0, 0, zShift),
                                     f.Placement.Rotation)
        f.Placement = newPlace

        if self.expandProfile:
            shapeEnv = \
                self._getExpandedProfileEnvelope(Part.Face(f), False,
                                                 self.obj.StartDepth.Value,
                                                 ezMin)
        else:
            shapeEnv = PathUtils.getEnvelope(base.Shape, subshape=f,
                                             depthparams=self.depthparams)

        if shapeEnv:
            tup = (shapeEnv, False, 'Profile', 0.0, 'X',
                   self.obj.StartDepth.Value,
                   self.obj.FinalDepth.Value)
            return tup

        msg = translate('PathProfile', 'Failed to create closed edge shape.')
        PathLog.debug(msg)
        return False

    # OpenEdge:
    def _processItem_OpenEdge(self, base, wire):
        PathLog.debug('_processItem_OpenEdge()')
        # Attempt open-edges profile
        ezMin = None
        cutWireObjs = False
        passOffsets = [self.ofstRadius]
        stepOver = float(self.obj.ExpandProfileStepOver)
        self.cutOut = self.parent.tool.Diameter * (stepOver / 100.0)

        # Verify Geometry Tolerance is set in Job
        if self.job.GeometryTolerance.Value == 0.0:
            msg = self.job.Label + '.GeometryTolerance = 0.0.'
            msg += translate('PathProfile',
                             ('Please set to an acceptable value'
                              ' greater than zero.'))
            PathLog.error(msg)
            return False

        # Flatten the edge(s)
        flattened = self._flattenWire(wire, self.obj.FinalDepth.Value)
        if not flattened:
            PathLog.error(self.inaccessibleMsg)
            return False

        (origWire, flatWire) = flattened
        self.parent._addDebugObject(flatWire, 'FlatWire')

        # Identify list of pass offset values for expanded profile paths
        if self.expandProfile:
            regularOfst = self.ofstRadius
            targetOfst = regularOfst + self.obj.ExpandProfile.Value
            while regularOfst < targetOfst:
                regularOfst += self.cutOut
                passOffsets.insert(0, regularOfst)

        # Attempt to profile the open edge for each pass offset
        OE = Profile_OpenEdge.OpenEdge(self.obj, self, self.parent)
        openEdgesList = OE.getOpenEdgeShape(passOffsets,
                                            base, origWire, flatWire)

        return (openEdgesList, False, 'OpenEdge', 0.0,
                'X', self.obj.StartDepth.Value, self.obj.FinalDepth.Value)

    # Model: (profile entire model)
    def _contourTheModel(self):
        PathLog.debug('_contourTheModel()')
        shapeTuples = list()
        modelGroup = self.parent.model

        # No base geometry selected, so treating operation like
        #    an exterior contour operation
        self.parent.areaOpUpdateDepths(self.obj)
        self.obj.Side = 'Outside'  # Force outside for whole model profile

        if 1 == len(modelGroup) and hasattr(modelGroup[0], "Proxy"):
            # process the sheet
            if isinstance(modelGroup[0].Proxy, ArchPanel.PanelSheet):
                # Cancel ExpandProfile feature. Unavailable for ArchPanels.
                if self.obj.ExpandProfile.Value != 0.0:
                    self.obj.ExpandProfile.Value == 0.0
                    msg = translate('PathProfile',
                                    ('No ExpandProfile support'
                                     ' for ArchPanel models.'))
                    FreeCAD.Console.PrintWarning(msg + '\n')
                modelProxy = modelGroup[0].Proxy
                # Process circles and holes if requested by user
                if self.obj.processCircles or self.obj.processHoles:
                    for shape in modelProxy.getHoles(modelGroup[0],
                                                     transform=True):
                        for wire in shape.Wires:
                            drillable = PathUtils.isDrillable(modelProxy, wire)
                            if ((drillable and self.obj.processCircles) or
                                    (not drillable and self.obj.processHoles)):
                                f = Part.makeFace(wire,
                                                  'Part::FaceMakerSimple')
                                shp = modelGroup[0].Shape
                                dp = self.depthparams
                                env = PathUtils.getEnvelope(shp, subshape=f,
                                                            depthparams=dp)
                                tup = (env, True, 'pathProfile', 0.0, 'X',
                                       self.obj.StartDepth.Value,
                                       self.obj.FinalDepth.Value)
                                shapeTuples.append(tup)

                # Process perimeter if requested by user
                if self.obj.processPerimeter:
                    for shape in modelProxy.getOutlines(modelGroup[0],
                                                        transform=True):
                        for wire in shape.Wires:
                            f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                            dp = self.depthparams
                            env = PathUtils.getEnvelope(modelGroup[0].Shape,
                                                        subshape=f,
                                                        depthparams=dp)
                            tup = (env, False, 'pathProfile', 0.0, 'X',
                                   self.obj.StartDepth.Value,
                                   self.obj.FinalDepth.Value)
                            shapeTuples.append(tup)
            else:
                PathLog.debug('Single model processed.')
                shapeTuples.extend(self._processEachModel())
        else:
            shapeTuples.extend(self._processEachModel())

        return shapeTuples

    def _processEachModel(self):
        shapeTups = list()
        for base in self.parent.model:
            if hasattr(base, 'Shape'):
                env = PathUtils.getEnvelope(partshape=base.Shape,
                                            subshape=None,
                                            depthparams=self.depthparams)
                if self.expandProfile:
                    eSlice = PathSurfaceSupport.getCrossSection(env)
                    transDist = base.Shape.BoundBox.ZMin - env.BoundBox.ZMin
                    eSlice.translate(FreeCAD.Vector(0.0, 0.0, transDist))
                    self.parent._addDebugObject(eSlice, 'ModelSlice')
                    startDep = self.obj.StartDepth.Value
                    finalDep = self.obj.FinalDepth.Value
                    shapeEnv = \
                        self._getExpandedProfileEnvelope(eSlice, False,
                                                         startDep, finalDep)
                else:
                    shapeEnv = env

                if shapeEnv:
                    shapeTups.append((shapeEnv, False))
        return shapeTups

    # General support methods.
    # Additional support methods in slot_support module
    def _openingType(self, baseShape, face, strDep, finDep):
        # Test if solid geometry above opening
        extDistPos = strDep - face.BoundBox.ZMin
        if extDistPos > 0:
            extFacePos = face.extrude(FreeCAD.Vector(0.0, 0.0, extDistPos))
            cmnPos = baseShape.common(extFacePos)
            if cmnPos.Volume > 0:
                # Signifies solid protrusion above,
                # or overhang geometry above opening
                return 1
        # Test if solid geometry below opening
        extDistNeg = finDep - face.BoundBox.ZMin
        if extDistNeg < 0:
            extFaceNeg = face.extrude(FreeCAD.Vector(0.0, 0.0, extDistNeg))
            cmnNeg = baseShape.common(extFaceNeg)
            if cmnNeg.Volume == 0:
                # No volume below signifies
                # an unobstructed/nonconstricted opening through baseShape
                return 0
            else:
                # Could be a pocket,
                # or a constricted/narrowing hole through baseShape
                return -1
        msg = translate('PathProfile', 'failed to return opening type.')
        PathLog.debug('_openingType() ' + msg)
        return -2

    def _getExpandedProfileEnvelope(self, faceShape, isHole, strDep, finalDep):
        shapeZ = faceShape.BoundBox.ZMin

        def calculateOffsetValue(obj, isHole):
            offset = obj.ExpandProfile.Value + obj.OffsetExtra.Value  # 0.0
            if obj.UseComp:
                offset = obj.OffsetExtra.Value + self.parent.tool.Diameter
                offset += obj.ExpandProfile.Value
            if isHole:
                if obj.Side == 'Outside':
                    offset = 0 - offset
            else:
                if obj.Side == 'Inside':
                    offset = 0 - offset
            return offset

        faceEnv = PathSurfaceSupport.getShapeEnvelope(faceShape)

        newFace = PathSurfaceSupport.getShapeSlice(faceEnv)

        # Compute necessary offset using internal function
        offsetVal = calculateOffsetValue(self.obj, isHole)
        expandedFace = PathSurfaceSupport.extractFaceOffset(newFace,
                                                            offsetVal,
                                                            newFace)
        if expandedFace:
            if shapeZ != 0.0:
                expandedFace.translate(FreeCAD.Vector(0.0, 0.0, shapeZ))
                newFace.translate(FreeCAD.Vector(0.0, 0.0, shapeZ))

            if isHole:
                if self.obj.Side == 'Outside':
                    newFace = newFace.cut(expandedFace)
                else:
                    newFace = expandedFace.cut(newFace)
            else:
                if self.obj.Side == 'Inside':
                    newFace = newFace.cut(expandedFace)
                else:
                    newFace = expandedFace.cut(newFace)

            if finalDep - shapeZ != 0:
                newFace.translate(FreeCAD.Vector(0.0, 0.0, finalDep - shapeZ))

            if strDep - finalDep != 0:
                if newFace.Area > 0:
                    return newFace.extrude(FreeCAD.Vector(0.0,
                                                          0.0,
                                                          strDep - finalDep))
                else:
                    PathLog.debug('No expanded profile face shape.\n')
                    return False
        else:
            msg = translate('PathProfile',
                            ('Failed to extract offset(s)'
                             ' for expanded profile.'))
            PathLog.debug(msg)

        PathLog.debug(translate('PathProfile',
                                'Failed to expand profile.'))
        return False

    def _flattenWire(self, wire, trgtDep):
        '''_flattenWire(wire, trgtDep)...
        Returns a tuple containing the original wire
        and a flattened version of the wire, in that order.
        '''
        PathLog.debug('_flattenWire()')
        wBB = wire.BoundBox

        if wBB.ZLength > 0.0:
            PathLog.debug('Wire is not horizontally co-planar. Flattening it.')

            # Extrude non-horizontal wire
            extFwdLen = (wBB.ZLength + 2.0) * 2.0
            mbbEXT = wire.extrude(FreeCAD.Vector(0, 0, extFwdLen))

            # Create cross-section of shape and translate
            sliceZ = wire.BoundBox.ZMin + (extFwdLen / 2)
            crsectFaceShp = self._makeCrossSection(mbbEXT, sliceZ, trgtDep)
            if crsectFaceShp is not False:
                return (wire, crsectFaceShp)
            else:
                return False
        else:
            srtWire = Part.Wire(Part.__sortEdges__(wire.Edges))
            srtWire.translate(FreeCAD.Vector(0.0,
                                             0.0,
                                             trgtDep - srtWire.BoundBox.ZMin))

        return (wire, srtWire)

    def _makeCrossSection(self, shape, sliceZ, zHghtTrgt=False):
        '''_makeCrossSection(shape, sliceZ, zHghtTrgt=None)...
        Creates cross-section object from shape at sliceZ height.
        Translates cross-section to zHghtTrgt if available.
        Makes face shape from cross-section object.
        Returns face shape at zHghtTrgt.
        '''
        PathLog.debug('_makeCrossSection()')
        # Create cross-section of shape and translate
        wires = list()
        slcs = shape.slice(FreeCAD.Vector(0, 0, 1), sliceZ)
        if len(slcs) > 0:
            for i in slcs:
                wires.append(i)
            comp = Part.Compound(wires)
            if zHghtTrgt is not False:
                comp.translate(FreeCAD.Vector(0.0,
                                              0.0,
                                              zHghtTrgt - comp.BoundBox.ZMin))
            return comp

        return False
# Eclass
