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

__title__ = "Path Slot Operation"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Class and strategy implementation of Slot operation."
__contributors__ = ""

# Standard
import math
# Third-party
from PySide import QtCore
# FreeCAD
import FreeCAD
import Path
import PathScripts.PathLog as PathLog
import PathScripts.path_operations.slot_support as SlotUtils

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Part = LazyLoader('Part', globals(), 'Part')
if FreeCAD.GuiUp:
    FreeCADGui = LazyLoader('FreeCADGui', globals(), 'FreeCADGui')

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


noSelfMsg = translate('slot', 'No "self" argument passed.')


class SlotOperation:
    '''Given an operation object and its proxy (parent) class,
    produces a Slot path.
    '''

    def __init__(self, obj, parent, initialize=True):
        '''__init__(obj, parent) ... Constructor for Slot operation class.'''
        PathLog.debug('SlotOperation.__init__()')
        # Required operation instance attributes
        self.parent = parent
        self.obj = obj

        self.job = parent.job
        self.propertyEnumerations = None
        self.propertyEditorModes = None
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
        self.dYdX1 = None
        self.dYdX2 = None
        self.bottomEdges = None
        self.isArc = 0
        self.arcCenter = None
        self.arcMidPnt = None
        self.arcRadius = 0.0
        self.newRadius = 0.0

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

        if not hasattr(self.obj, 'Base'):
            msg = translate('PathSlot',
                    'No Base Geometry object in the operation.')
            PathLog.debug(msg + '\n')
            return

        if len(self.obj.Base) == 0:
            self.processType = 'CustomPoints'
            self.isReady = True
            return

        (base, subsList) = self.obj.Base[0]
        self._base = base
        self._subsList = subsList

        # Set processType for the strategy
        subCnt = len(subsList)
        if subCnt == 1:
            self.processType = 'Single'
        elif subCnt == 2:
            self.processType = 'Double'
        else:
            # Send message to user about excessive selections
            subCnt = 2
            self.processType = 'Double'

        # Record shapes, shape types, and determine ZMin of geometry
        fbb = base.Shape.getElement(subsList[0]).BoundBox
        zmin = fbb.ZMax
        for s in range(0, subCnt):
            sub = subsList[s]
            self._shapes.append(getattr(base.Shape, sub))
            self._shapeTypes.append(sub[:4])
            try:
                fbb = base.Shape.getElement(sub).BoundBox
                zmin = min(zmin, fbb.ZMin)
            except Part.OCCError as e:
                PathLog.error(e)

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
        def _updateRef1Enums(cat, single=False):
            '''Customize Reference1 enumerations based on feature type.'''
            if single:
                if cat == 'Face':
                    return ['Long Edge', 'Short Edge']
                elif cat == 'Edge':
                    return ['Long Edge']
                elif cat == 'Vert':
                    return ['Vertex']
            elif cat == 'Vert':
                return ['Vertex']

            return ['Center of Mass', 'Center of BoundBox',
                'Lowest Point', 'Highest Point']

        # Internal function to adjust property enumeration list
        def _updateRef2Enums(cat):
            '''Customize Reference2 enumerations based on feature type.'''
            if cat == 'Vert':
                return ['Vertex']
            return ['Center of Mass', 'Center of BoundBox',
                'Lowest Point', 'Highest Point']

        # Retrieve full enumeration lists
        initEnums = self.getInitialPropertyEnumerations()

        # This is the enumeration list modification logic.
        subCnt = len(self._shapes)
        if subCnt == 0:
            pass
        elif subCnt == 1:
            # Adjust available enumerations
            initEnums['Reference1'] = _updateRef1Enums(self._shapeTypes[0], True)
        elif subCnt == 2:
            # Adjust available enumerations
            initEnums['Reference1'] = _updateRef1Enums(self._shapeTypes[0])
            initEnums['Reference2'] = _updateRef2Enums(self._shapeTypes[1])

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
        return ['FeatureTool', 'FeatureDepths', 'FeatureHeights',
                'FeatureStepDown', 'FeatureCoolant', 'FeatureBaseVertexes',
                'FeatureBaseEdges', 'FeatureBaseFaces']

    @staticmethod
    def getPropertyDefinitions():
        '''getPropertyDefinitions() ...
        This method returns initial property definitions as a list of tuples.
        '''
        # Definition format: (prop_type, name, group, tooltip)
        return [
            ("App::PropertyVectorDistance", "CustomPoint1", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Enter custom start point for slot path.")),
            ("App::PropertyVectorDistance", "CustomPoint2", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Enter custom end point for slot path.")),
            ("App::PropertyEnumeration", "CutPattern", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Set the geometric clearing pattern to use for the operation.")),
            ("App::PropertyDistance", "ExtendPathStart", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Positive extends the beginning of the path, negative shortens.")),
            ("App::PropertyDistance", "ExtendPathEnd", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Positive extends the end of the path, negative shortens.")),
            ("App::PropertyEnumeration", "LayerMode", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Complete the operation in a single pass at depth, or mulitiple passes to final depth.")),
            ("App::PropertyEnumeration", "PathOrientation", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Choose the path orientation with regard to the feature(s) selected.")),
            ("App::PropertyEnumeration", "Reference1", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Choose what point to use on the first selected feature.")),
            ("App::PropertyEnumeration", "Reference2", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Choose what point to use on the second selected feature.")),
            ("App::PropertyDistance", "ExtendRadius", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "For arcs/circlular edges, offset the radius for the path.")),
            ("App::PropertyBool", "ReverseDirection", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable to reverse the cut direction of the slot path.")),

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
            'CutPattern': ['Line', 'ZigZag'],
            'LayerMode': ['Single-pass', 'Multi-pass'],
            'PathOrientation': ['Start to End', 'Perpendicular'],
            'Reference1': ['Center of Mass', 'Center of BoundBox',
                            'Lowest Point', 'Highest Point', 'Long Edge',
                            'Short Edge', 'Vertex'],
            'Reference2': ['Center of Mass', 'Center of BoundBox',
                            'Lowest Point', 'Highest Point', 'Vertex']
        }

    def getPropertyDefaultValues(self):
        '''getPropertyDefaultValues() ...
        This method returns initial property default values if not called
        from a class instance.  If called from a class instance, then the
        default values will reflect operation and job object influences.
        '''
        PathLog.debug('getPropertyDefaultValues()')
        initialValues = {
            'CustomPoint1': FreeCAD.Vector(0.0, 0.0, 0.0),
            'ExtendPathStart': 0.0,
            'Reference1': 'Center of Mass',
            'CustomPoint2': FreeCAD.Vector(10.0, 10.0, 0.0),
            'ExtendPathEnd': 0.0,
            'Reference2': 'Center of Mass',
            'LayerMode': 'Multi-pass',
            'CutPattern': 'ZigZag',
            'PathOrientation': 'Start to End',
            'ExtendRadius': 0.0,
            'ReverseDirection': False,
            'StartPoint': FreeCAD.Vector(0.0, 0.0, 0.0),
            'UseStartPoint': False
        }

        if not self:
            FreeCAD.Console.PrintWarning(noSelfMsg + ' <> 3\n')
            return initialValues

        """
        Insert modifications here to change `initialValues` dictionary.
        The source operation (object) is accessible by self.obj.
        The source operation class (proxy) is accessible by self.parent
        """
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

        if not self.hasProperties:
            msg = translate('slot', 'No operation-specific properties.')
            FreeCAD.Console.PrintError(msg + '\n')
            return {}

        A = B = 2
        C = D = 0
        hideRef = True
        subCnt = len(self._shapes)

        if subCnt == 1:
            hideRef = False
            if self._shapeTypes[0] == 'Edge':
                if self._shapes[0].Curve.TypeId == 'Part::GeomCircle':
                    D = 2
                else:
                    C = 2
            A = 0
        elif subCnt == 2:
            hideRef = False
            A = B = 0
            C = 2

        if hideRef:
            A = B = C = D = 2

        # format is {'property': value (fixed integer, or variable)}
        return {
            'Reference1': A,
            'Reference2': B,
            'ExtendRadius': C,
            'PathOrientation': D
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
        if prop in ['Base']:
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

    # Primary public method to generate Slot path data
    def execute(self, parent, obj):
        '''execute() ...
        Execute the Slot operation.
        Returns a list of path commands.
        '''
        PathLog.track()
        self.parent = parent
        self.job = self.parent.job
        self.obj = obj
        CMDS = list()

        # Update Base Geometry analysis and parameters
        self.updateParameters(apply=True)

        if not self.isReady:
            return False

        # Begin GCode for operation with basic information
        # ... and move cutter to clearance height and startpoint
        tool = obj.ToolController.Tool
        toolType = tool.ToolType \
            if hasattr(tool, 'ToolType') else tool.ShapeName
        output = ''
        if obj.Comment != '':
            pathCmd = Path.Command('N ({})'.format(obj.Comment), {})
            CMDS.append(pathCmd)
        CMDS.extend([
            Path.Command('N ({})'.format(obj.Label), {}),
            Path.Command('N (Tool type: {})'.format(toolType), {}),
            Path.Command(('N (Compensated Tool Path. Diameter: '
                          '{})'.format(tool.Diameter)), {}),
            Path.Command('N ({})'.format(output), {}),
            Path.Command('G0', {'Z': obj.ClearanceHeight.Value,
                                'F': self.parent.vertRapid})
        ])
        if obj.UseStartPoint is True:
            cmdDict = {'X': obj.StartPoint.x,
                       'Y': obj.StartPoint.y,
                       'F': self.parent.horizRapid}
            CMDS.append(Path.Command('G0', cmdDict))

        cmds = self._makePath()
        if cmds:
            CMDS.extend(cmds)
            g0 = {'Z': obj.ClearanceHeight.Value, 'F': self.parent.vertRapid}
            CMDS.append(Path.Command('G0', g0))

        return CMDS

    # Control methods for operation
    def _makePath(self):
        '''_makePath(obj) ...
        Control the process for the Slot operation.
        '''
        pnts = False

        if self.processType == 'CustomPoints':
            # Use custom inputs here
            p1 = self.obj.CustomPoint1
            p2 = self.obj.CustomPoint2
            if p1.z != p2.z:
                msg = translate('PathSlot',
                        'Custom points not at same Z height.')
                FreeCAD.Console.PrintError(msg + '\n')
                return False

            cmds = self._finishLine((p1, p2), 1)
            if not cmds:
                return False
            return cmds
        elif self.processType == 'Single':
            pnts = self._processSingle()
        elif self.processType == 'Double':
            pnts = self._processDouble()

        if not pnts:
            return False

        subCnt = len(self._shapes)
        if self.isArc:
            cmds = self._finishArc(pnts, subCnt)
        else:
            cmds = self._finishLine(pnts, subCnt)

        if cmds:
            return cmds

        return False

    # Methods for processing single reference geometry
    def _processSingle(self):
        '''This is the control method for slots based on a
        single Base Geometry feature.'''
        shape = self._shapes[0]
        sub = self._subsList[0]
        cat1 = self._shapeTypes[0]
        cmds = False
        make = False

        PathLog.debug('Reference 1: {}'.format(self.obj.Reference1))
        if cat1 == 'Face':
            pnts = False
            norm = shape.normalAt(0.0, 0.0)
            PathLog.debug('{}.normalAt(): {}'.format(sub, norm))
            if norm.z == 1 or norm.z == -1:
                pnts = self._processSingleHorizFace(shape)
            elif norm.z == 0:
                faceType = SlotUtils._getVertFaceType(shape)
                if faceType:
                    (geo, shp) = faceType
                    if geo == 'Face':
                        pnts = self._processSingleComplexFace(shp)
                    if geo == 'Wire':
                        pnts = self._processSingleVertFace(shp)
                    if geo == 'Edge':
                        pnts = self._processSingleVertFace(shp)
            else:
                msg = translate('PathSlot',
                    'The selected face is not oriented horizontally or vertically.')
                FreeCAD.Console.PrintError(msg + '\n')
                return False

            if pnts:
                (p1, p2) = pnts
                make = True

        elif cat1 == 'Edge':
            PathLog.debug('Single edge')
            pnts = self._processSingleEdge(shape)
            if pnts:
                (p1, p2) = pnts
                make = True

        elif cat1 == 'Vert':
            msg = translate('PathSlot',
                'Only a vertex selected. Add another feature to the Base Geometry.')
            FreeCAD.Console.PrintError(msg + '\n')

        if make:
            return (p1, p2)

        return False

    def _processSingleHorizFace(self, shape):
        '''Determine slot path endpoints from a single horizontally oriented face.'''
        PathLog.debug('_processSingleHorizFace()')
        lineTypes = ['Part::GeomLine']

        def halfCircleAngle(self, E):
            vect = SlotUtils._dXdYdZ(E)
            norm = SlotUtils._normalizeVector(vect)
            rads = SlotUtils._xyToRadians(norm)
            deg = math.degrees(rads)
            if deg >= 180.0:
                deg -= 180.0
            return deg

        # Reject triangular faces
        if len(shape.Edges) < 4:
            msg = translate('PathSlot',
                'A single selected face must have four edges minimum.')
            FreeCAD.Console.PrintError(msg + '\n')
            return False

        # Create tuples as (edge index, length, angle)
        eTups = list()
        for i in range(0, 4):
            eTups.append((i,
                          shape.Edges[i].Length,
                          halfCircleAngle(self, shape.Edges[i]))
                        )

        # Sort tuples by edge angle
        eTups.sort(key=lambda tup: tup[2])
        # Identify parallel edges
        pairs = list()
        eCnt = len(shape.Edges)
        lstE = eCnt - 1
        for i in range(0, eCnt):
            if i < lstE:
                ni = i + 1
                A = eTups[i]
                B = eTups[ni]
                if abs(A[2] - B[2]) < 0.00000001:  # test slopes(yaw angles)
                    debug = False
                    eA = shape.Edges[A[0]]
                    eB = shape.Edges[B[0]]
                    if eA.Curve.TypeId not in lineTypes:
                        debug = eA.Curve.TypeId
                    if not debug:
                        if eB.Curve.TypeId not in lineTypes:
                            debug = eB.Curve.TypeId
                        else:
                            pairs.append((eA, eB))
                    if debug:
                        msg = 'Erroneous Curve.TypeId: {}'.format(debug)
                        PathLog.debug(msg)

        pairCnt = len(pairs)
        if pairCnt > 1:
            pairs.sort(key=lambda tup: tup[0].Length, reverse=True)

        if self.isDebug:
            PathLog.debug(' -pairCnt: {}'.format(pairCnt))
            for (a, b) in pairs:
                PathLog.debug(' -pair: {}, {}'.format(round(a.Length, 4), round(b.Length,4)))

        if pairCnt == 0:
            msg = translate('PathSlot',
                'No parallel edges identified.')
            FreeCAD.Console.PrintError(msg + '\n')
            return False
        elif pairCnt == 1:
            same = pairs[0]
        else:
            if self.obj.Reference1 == 'Long Edge':
                same = pairs[1]
            elif self.obj.Reference1 == 'Short Edge':
                same = pairs[0]
            else:
                msg = 'Reference1 '
                msg += translate('PathSlot',
                    'value error.')
                FreeCAD.Console.PrintError(msg + '\n')
                return False

        (p1, p2) = SlotUtils._getOppMidPoints(same)
        return (p1, p2)

    def _processSingleComplexFace(self, shape):
        '''Determine slot path endpoints from a single complex face.'''
        PathLog.debug('_processSingleComplexFace()')
        PNTS = list()

        def zVal(V):
            return V.z

        for E in shape.Wires[0].Edges:
            p = SlotUtils._findLowestEdgePoint(E)
            PNTS.append(p)
        PNTS.sort(key=zVal)
        return (PNTS[0], PNTS[1])

    def _processSingleVertFace(self, shape):
        '''Determine slot path endpoints from a single vertically oriented face
        with no single bottom edge.'''
        PathLog.debug('_processSingleVertFace()')
        eCnt = len(shape.Edges)
        V0 = shape.Edges[0].Vertexes[0]
        V1 = shape.Edges[eCnt - 1].Vertexes[1]
        v0 = FreeCAD.Vector(V0.X, V0.Y, V0.Z)
        v1 = FreeCAD.Vector(V1.X, V1.Y, V1.Z)

        dX = V1.X - V0.X
        dY = V1.Y - V0.Y
        dZ = V1.Z - V0.Z
        temp = FreeCAD.Vector(dX, dY, dZ)
        slope = SlotUtils._normalizeVector(temp)
        perpVect = FreeCAD.Vector(-1 * slope.y, slope.x, slope.z)
        perpVect.multiply(self.parent.tool.Diameter / 2.0)

        # Create offset endpoints for raw slot path
        a1 = v0.add(perpVect)
        a2 = v1.add(perpVect)
        b1 = v0.sub(perpVect)
        b2 = v1.sub(perpVect)
        (p1, p2) = SlotUtils._getCutSidePoints(self._base, v0, v1, a1, a2, b1, b2)
        return (p1, p2)

    def _processSingleEdge(self, edge):
        '''Determine slot path endpoints from a single horizontally oriented face.'''
        PathLog.debug('_processSingleEdge()')
        tolrnc = 0.0000001
        lineTypes = ['Part::GeomLine']
        curveTypes = ['Part::GeomCircle']

        def oversizedTool(holeDiam):
            # Test if tool larger than opening
            if self.parent.tool.Diameter > holeDiam:
                msg = translate('PathSlot',
                        'Current tool larger than arc diameter.')
                FreeCAD.Console.PrintError(msg + '\n')
                return True
            return False

        def isHorizontal(z1, z2, z3):
            # Check that all Z values are equal (isRoughly same)
            if (abs(z1 - z2) > tolrnc or
                abs(z1 - z3) > tolrnc or
                abs(z2 - z3) > tolrnc):
                return False
            return True

        def circleCentFrom3Points(P1, P2, P3):
            # Source code for this function copied from:
            # https://wiki.freecadweb.org/Macro_Draft_Circle_3_Points_3D
            P1P2 = (P2 - P1).Length
            P2P3 = (P3 - P2).Length
            P3P1 = (P1 - P3).Length

            # Circle radius.
            l = ((P1 - P2).cross(P2 - P3)).Length
            try:
                r = P1P2 * P2P3 * P3P1 / 2 / l
            except:
                PathLog.error("The three points are aligned.")
                return False
            else:
                # Sphere center.
                a = P2P3**2 * (P1 - P2).dot(P1 - P3) / 2 / l**2
                b = P3P1**2 * (P2 - P1).dot(P2 - P3) / 2 / l**2
                c = P1P2**2 * (P3 - P1).dot(P3 - P2) / 2 / l**2
                P1.multiply(a)
                P2.multiply(b)
                P3.multiply(c)
                PC = P1 + P2 + P3
                return PC

        # Process edge based on curve type
        if edge.Curve.TypeId in lineTypes:
            V1 = edge.Vertexes[0]
            V2 = edge.Vertexes[1]
            p1 = FreeCAD.Vector(V1.X, V1.Y, 0.0)
            p2 = FreeCAD.Vector(V2.X, V2.Y, 0.0)
            return (p1, p2)
        elif edge.Curve.TypeId in curveTypes:
            if len(edge.Vertexes) == 1:
                # Circle edge
                PathLog.debug('Arc with single vertex.')
                if oversizedTool(edge.BoundBox.XLength):
                    return False

                self.isArc = 1
                V1 = edge.Vertexes[0]
                tp1 = edge.valueAt(edge.getParameterByLength(edge.Length * 0.33))
                tp2 = edge.valueAt(edge.getParameterByLength(edge.Length * 0.66))
                if not isHorizontal(V1.Z, tp1.z, tp2.z):
                    return False

                cent = edge.BoundBox.Center
                self.arcCenter = FreeCAD.Vector(cent.x, cent.y, 0.0)
                midPnt = edge.valueAt(edge.getParameterByLength(edge.Length / 2.0))
                self.arcMidPnt = FreeCAD.Vector(midPnt.x, midPnt.y, 0.0)
                self.arcRadius = edge.BoundBox.XLength / 2.0
                p1 = FreeCAD.Vector(V1.X, V1.Y, 0.0)
                p2 = FreeCAD.Vector(V1.X, V1.Y, 0.0)
            else:
                # Arc edge
                PathLog.debug('Arc with multiple vertices.')
                self.isArc = 2
                V1 = edge.Vertexes[0]
                V2 = edge.Vertexes[1]
                midPnt = edge.valueAt(edge.getParameterByLength(edge.Length / 2.0))
                if not isHorizontal(V1.Z, V2.Z, midPnt.z):
                    return False

                p1 = FreeCAD.Vector(V1.X, V1.Y, 0.0)
                p2 = FreeCAD.Vector(V2.X, V2.Y, 0.0)
                # Duplicate points required because
                #   circleCentFrom3Points() alters original arguments
                pA = FreeCAD.Vector(V1.X, V1.Y, 0.0)
                pB = FreeCAD.Vector(V2.X, V2.Y, 0.0)
                pC = FreeCAD.Vector(midPnt.x, midPnt.y, 0.0)
                cCF3P = circleCentFrom3Points(pA, pB, pC)
                if not cCF3P:
                    return False
                self.arcMidPnt = FreeCAD.Vector(midPnt.x, midPnt.y, 0.0)
                self.arcCenter = cCF3P
                self.arcRadius = p1.sub(cCF3P).Length

                if oversizedTool(self.arcRadius * 2.0):
                    return False

            return (p1, p2)

    # Methods for processing double reference geometry
    def _processDouble(self):
        PathLog.debug('_processDouble()')
        '''This is the control method for slots based on a
        two Base Geometry features.'''
        shape_1 = self._shapes[0]
        sub1 = self._subsList[0]
        shape_2 = self._shapes[1]
        sub2 = self._subsList[1]
        cmds = False
        make = False
        cat2 = sub2[:4]
        p1 = None
        p2 = None
        dYdX1 = None
        dYdX2 = None
        self.bottomEdges = list()

        PathLog.debug('Reference 1: {}'.format(self.obj.Reference1))
        PathLog.debug('Reference 2: {}'.format(self.obj.Reference2))

        # Process first feature
        feature1 = SlotUtils._processFeatureForDouble(self, shape_1, sub1, 1)
        if not feature1:
            msg = translate('PathSlot',
                'Failed to determine point 1 from')
            FreeCAD.Console.PrintError(msg + ' {}.\n'.format(sub1))
            return False
        (p1, dYdX1, shpType) = feature1
        if dYdX1:
            self.dYdX1 = dYdX1

        # Process second feature
        feature2 = SlotUtils._processFeatureForDouble(self, shape_2, sub2, 2)
        if not feature2:
            msg = translate('PathSlot',
                'Failed to determine point 2 from')
            FreeCAD.Console.PrintError(msg + ' {}.\n'.format(sub2))
            return False
        (p2, dYdX2, shpType) = feature2
        if dYdX2:
            self.dYdX2 = dYdX2

        # Parallel check for twin face, and face-edge cases
        if dYdX1 and dYdX2:
            if not SlotUtils._isParallel(dYdX1, dYdX2):
                PathLog.debug('dYdX1, dYdX2: {}, {}'.format(dYdX1, dYdX2))
                msg = translate('PathSlot',
                    'Selected geometry not parallel.')
                FreeCAD.Console.PrintError(msg + '\n')
                return False

        if p2:
            return (p1, p2)

        return False

    # Finishing methods based on slot type: Line or Arc
    def _finishArc(self, pnts, featureCnt):
        '''This method finishes an Arc Slot operation.'''
        PathLog.debug('arc center: {}'.format(self.arcCenter))
        self.parent._addDebugObject(Part.makeLine(self.arcCenter, self.arcMidPnt), 'CentToMidPnt')

        # PathLog.debug('Pre-offset points are:\np1 = {}\np2 = {}'.format(p1, p2))
        if self.obj.ExtendRadius.Value != 0:
            # verify offset does not force radius < 0
            newRadius = self.arcRadius + self.obj.ExtendRadius.Value
            PathLog.debug('arc radius: {};  offset radius: {}'.format(self.arcRadius, newRadius))
            if newRadius <= 0:
                msg = translate('PathSlot',
                        'Current offset value is not possible.')
                FreeCAD.Console.PrintError(msg + '\n')
                return False
            else:
                (p1, p2) = pnts
                pnts = SlotUtils._makeOffsetArc(p1, p2, self.arcCenter, newRadius)
                self.newRadius = newRadius
        else:
            PathLog.debug('arc radius: {}'.format(self.arcRadius))
            self.newRadius = self.arcRadius

        # Apply path extension for arcs
        # PathLog.debug('Pre-extension points are:\np1 = {}\np2 = {}'.format(p1, p2))
        if self.isArc == 1:
            # Complete circle
            if (self.obj.ExtendPathStart.Value != 0 or
                self.obj.ExtendPathEnd.Value != 0):
                msg = translate('PathSlot',
                        'No path extensions available for full circles.')
                FreeCAD.Console.PrintWarning(msg + '\n')
        else:
            # Arc segment
            # Apply extensions to slot path
            (p1, p2) = pnts
            begExt = self.obj.ExtendPathStart.Value
            endExt = self.obj.ExtendPathEnd.Value
            debugMethod = self.parent._addDebugObject
            pnts = SlotUtils._extendArcSlot(debugMethod, p1, p2, self.arcCenter, self.newRadius, begExt, endExt)

        if not pnts:
            return False

        (p1, p2) = pnts
        # PathLog.error('Post-offset points are:\np1 = {}\np2 = {}'.format(p1, p2))
        if self.isDebug:
            PathLog.debug('Path Points are:\np1 = {}\np2 = {}'.format(p1, p2))
            if p1.sub(p2).Length != 0:
                self.parent._addDebugObject(Part.makeLine(p1, p2), 'Path')

        if featureCnt:
            self.obj.CustomPoint1 = p1
            self.obj.CustomPoint2 = p2

        if SlotUtils._arcCollisionCheck(self, p1, p2, self.arcCenter, self.newRadius):
            msg = self.obj.Label + ' '
            msg += translate('PathSlot',
                    'operation collides with model.')
            FreeCAD.Console.PrintError(msg + '\n')

        # PathLog.warning('Unable to create G-code.  _makeArcGCode() is incomplete.')
        cmds = self._makeArcGCode(p1, p2)
        return cmds

    def _makeArcGCode(self, p1, p2):
        '''This method is the last in the overall slot creation process.
        It accepts the operation object and two end points for the path.
        It returns the slot gcode for the operation.'''
        CMDS = list()
        PATHS = [(p1, p2, 'G2'), (p2, p1, 'G3')]

        def arcPass(PNTS, depth):
            cmds = list()
            (p1, p2, cmd) = PNTS
            # cmds.append(Path.Command('N (Tool type: {})'.format(toolType), {}))
            cmds.append(Path.Command('G0', {'X': p1.x, 'Y': p1.y, 'F': self.parent.horizRapid}))
            cmds.append(Path.Command('G1', {'Z': depth, 'F': self.parent.vertFeed}))
            vtc = self.arcCenter.sub(p1)  # vector to center
            cmds.append(
                Path.Command(cmd,
                    {'X': p2.x, 'Y': p2.y, 'I': vtc.x,
                        'J': vtc.y, 'F': self.parent.horizFeed
                     }))
            return cmds

        if self.obj.LayerMode == 'Single-pass':
            PNTS = PATHS[0]
            if self.obj.ReverseDirection:
                PNTS = PATHS[1]
            CMDS.extend(arcPass(PNTS, self.obj.FinalDepth.Value))
        else:
            if self.obj.CutPattern == 'Line':
                PNTS = PATHS[0]
                if self.obj.ReverseDirection:
                    PNTS = PATHS[1]
                for dep in self.parent.depthParams:
                    CMDS.extend(arcPass(PNTS, dep))
                    CMDS.append(Path.Command('G0', {'Z': self.obj.SafeHeight.Value, 'F': self.parent.vertRapid}))
            elif self.obj.CutPattern == 'ZigZag':
                i = 0
                for dep in self.parent.depthParams:
                    if i % 2.0 == 0:  # even
                        CMDS.extend(arcPass(PATHS[0], dep))
                    else:  # odd
                        CMDS.extend(arcPass(PATHS[1], dep))
                    i += 1
        # Raise to SafeHeight when finished
        CMDS.append(Path.Command('G0', {'Z': self.obj.SafeHeight.Value, 'F': self.parent.vertRapid}))

        return CMDS

    def _finishLine(self, pnts, featureCnt):
        '''This method finishes an Line Slot operation.'''
        # Apply perpendicular rotation if requested
        perpZero = True
        if self.obj.PathOrientation == 'Perpendicular':
            if featureCnt == 2:
                if self._shapeTypes[0] == 'Face' and self._shapeTypes[1] == 'Face':
                    if self.bottomEdges:
                        self.bottomEdges.sort(key=lambda edg: edg.Length, reverse=True)
                        BE = self.bottomEdges[0]
                        pnts = self._processSingleVertFace(BE)
                        perpZero = False
            if perpZero:
                (p1, p2) = pnts
                pnts = SlotUtils._makePerpendicular(p1, p2, 10.0, self.dYdX1, self.dYdX2)  # 10.0 offset below
        else:
            perpZero = False

        # Reverse direction of path if requested
        if self.obj.ReverseDirection:
            (p2, p1) = pnts
        else:
            (p1, p2) = pnts

        # Apply extensions to slot path
        begExt = self.obj.ExtendPathStart.Value
        endExt = self.obj.ExtendPathEnd.Value
        if perpZero:
            # Offsets for 10.0 value above in _makePerpendicular()
            begExt -= 5.0
            endExt -= 5.0
        pnts = SlotUtils._extendLineSlot(p1, p2, begExt, endExt)

        if not pnts:
            return False

        (p1, p2) = pnts
        if self.isDebug:
            PathLog.debug('Path Points are:\np1 = {}\np2 = {}'.format(p1, p2))
            if p1.sub(p2).Length != 0:
                self.parent._addDebugObject(Part.makeLine(p1, p2), 'Path')

        if featureCnt:
            self.obj.CustomPoint1 = p1
            self.obj.CustomPoint2 = p2

        if SlotUtils._lineCollisionCheck(self, p1, p2):
            msg = self.obj.Label + ' '
            msg += translate('PathSlot',
                    'operation collides with model.')
            FreeCAD.Console.PrintWarning(msg + '\n')

        cmds = self._makeLineGCode(p1, p2)
        return cmds

    def _makeLineGCode(self, p1, p2):
        '''This method is the last in the overall slot creation process.
        It accepts the operation object and two end points for the path.
        It returns the slot gcode for the operation.'''
        CMDS = list()

        def linePass(p1, p2, depth):
            cmds = list()
            # cmds.append(Path.Command('N (Tool type: {})'.format(toolType), {}))
            cmds.append(Path.Command('G0', {'X': p1.x, 'Y': p1.y, 'F': self.parent.horizRapid}))
            cmds.append(Path.Command('G1', {'Z': depth, 'F': self.parent.vertFeed}))
            cmds.append(Path.Command('G1', {'X': p2.x, 'Y': p2.y, 'F': self.parent.horizFeed}))
            return cmds

        # CMDS.append(Path.Command('N (Tool type: {})'.format(toolType), {}))
        if self.obj.LayerMode == 'Single-pass':
            CMDS.extend(linePass(p1, p2, self.obj.FinalDepth.Value))
            CMDS.append(Path.Command('G0', {'Z': self.obj.SafeHeight.Value, 'F': self.parent.vertRapid}))
        else:
            if self.obj.CutPattern == 'Line':
                for dep in self.parent.depthParams:
                    CMDS.extend(linePass(p1, p2, dep))
                    CMDS.append(Path.Command('G0', {'Z': self.obj.SafeHeight.Value, 'F': self.parent.vertRapid}))
            elif self.obj.CutPattern == 'ZigZag':
                CMDS.append(Path.Command('G0', {'X': p1.x, 'Y': p1.y, 'F': self.parent.horizRapid}))
                i = 0
                for dep in self.parent.depthParams:
                    if i % 2.0 == 0:  # even
                        CMDS.append(Path.Command('G1', {'Z': dep, 'F': self.parent.vertFeed}))
                        CMDS.append(Path.Command('G1', {'X': p2.x, 'Y': p2.y, 'F': self.parent.horizFeed}))
                    else:  # odd
                        CMDS.append(Path.Command('G1', {'Z': dep, 'F': self.parent.vertFeed}))
                        CMDS.append(Path.Command('G1', {'X': p1.x, 'Y': p1.y, 'F': self.parent.horizFeed}))
                    i += 1
            CMDS.append(Path.Command('G0', {'Z': self.obj.SafeHeight.Value, 'F': self.parent.vertRapid}))

        return CMDS

    # Support methods located in PathSlotUtils module
    # Additional class methods located in PathSlotUtils module
# Eclass
