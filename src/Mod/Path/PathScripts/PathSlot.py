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
__doc__ = "Class and implementation of Slot operation."
__contributors__ = ""

import FreeCAD
from PySide import QtCore
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
import PathScripts.PathOp as PathOp
import math

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Part = LazyLoader('Part', globals(), 'Part')

if FreeCAD.GuiUp:
    import FreeCADGui

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectSlot(PathOp.ObjectOp):
    '''Proxy object for Surfacing operation.'''

    def opFeatures(self, obj):
        '''opFeatures(obj) ... return all standard features'''
        return PathOp.FeatureTool | PathOp.FeatureDepths \
            | PathOp.FeatureHeights | PathOp.FeatureStepDown \
            | PathOp.FeatureCoolant | PathOp.FeatureBaseVertexes \
            | PathOp.FeatureBaseEdges | PathOp.FeatureBaseFaces

    def initOperation(self, obj):
        '''initOperation(obj) ... Initialize the operation by
        managing property creation and property editor status.'''
        self.propertiesReady = False

        self.initOpProperties(obj)  # Initialize operation-specific properties

        # For debugging
        if PathLog.getLevel(PathLog.thisModule()) != 4:
            obj.setEditorMode('ShowTempObjects', 2)  # hide

        if not hasattr(obj, 'DoNotSetDefaultValues'):
            self.setEditorProperties(obj)

    def initOpProperties(self, obj, warn=False):
        '''initOpProperties(obj) ... create operation specific properties'''
        self.addNewProps = list()

        for (prtyp, nm, grp, tt) in self.opPropertyDefinitions():
            if not hasattr(obj, nm):
                obj.addProperty(prtyp, nm, grp, tt)
                self.addNewProps.append(nm)

        # Set enumeration lists for enumeration properties
        if len(self.addNewProps) > 0:
            ENUMS = self.opPropertyEnumerations()
            for n in ENUMS:
                if n in self.addNewProps:
                    setattr(obj, n, ENUMS[n])

            if warn:
                newPropMsg = translate('PathSlot', 'New property added to')
                newPropMsg += ' "{}": {}'.format(obj.Label, self.addNewProps) + '. '
                newPropMsg += translate('PathSlot', 'Check default value(s).')
                FreeCAD.Console.PrintWarning(newPropMsg + '\n')

        self.propertiesReady = True

    def opPropertyDefinitions(self):
        '''opPropertyDefinitions(obj) ... Store operation specific properties'''

        return [
            ("App::PropertyBool", "ShowTempObjects", "Debug",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Show the temporary path construction objects when module is in DEBUG mode.")),

            ("App::PropertyVectorDistance", "CustomPoint1", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Enter custom start point for slot path.")),
            ("App::PropertyVectorDistance", "CustomPoint2", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Enter custom end point for slot path.")),
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
            ("App::PropertyBool", "ReverseDirection", "Slot",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable to reverse the cut direction of the slot path.")),

            ("App::PropertyVectorDistance", "StartPoint", "Start Point",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "The custom start point for the path of this operation")),
            ("App::PropertyBool", "UseStartPoint", "Start Point",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Make True, if specifying a Start Point"))
        ]

    def opPropertyEnumerations(self):
        # Enumeration lists for App::PropertyEnumeration properties
        return {
            'LayerMode': ['Single-pass', 'Multi-pass'],
            'PathOrientation': ['Start to End', 'Perpendicular'],
            'Reference1': ['Center of Mass', 'Center of BoundBox',
                           'Lowest Point', 'Highest Point', 'Long Edge',
                           'Short Edge', 'Vertex'],
            'Reference2': ['Center of Mass', 'Center of BoundBox',
                           'Lowest Point', 'Highest Point', 'Vertex']
        }

    def opPropertyDefaults(self, obj, job):
        '''opPropertyDefaults(obj, job) ... returns a dictionary of default values
        for the operation's properties.'''
        defaults = {
            'CustomPoint1': FreeCAD.Vector(0.0, 0.0, 0.0),
            'ExtendPathStart': 0.0,
            'Reference1': 'Center of Mass',
            'CustomPoint2': FreeCAD.Vector(10.0, 10.0, 0.0),
            'ExtendPathEnd': 0.0,
            'Reference2': 'Center of Mass',
            'LayerMode': 'Single-pass',
            'PathOrientation': 'Start to End',
            'ReverseDirection': False,

            # For debugging
            'ShowTempObjects': False
        }

        return defaults

    def setEditorProperties(self, obj):
        # Used to hide inputs in properties list
        A = B = 2
        if hasattr(obj, 'Base'):
            enums2 = self.opPropertyEnumerations()['Reference2']
            if obj.Base:
                (base, subsList) = obj.Base[0]
                subCnt = len(subsList)
                if subCnt == 1:
                    # Adjust available enumerations
                    obj.Reference1 = self._getReference1Enums(subsList[0], True)
                    A = 0
                elif subCnt == 2:
                    # Adjust available enumerations
                    obj.Reference1 = self._getReference1Enums(subsList[0])
                    obj.Reference2 = self._getReference2Enums(subsList[1])
                    A = B = 0
            else:
                ENUMS = self.opPropertyEnumerations()
                obj.Reference1 = ENUMS['Reference1']
                obj.Reference2 = ENUMS['Reference2']

        obj.setEditorMode('Reference1', A)
        obj.setEditorMode('Reference2', B)

    def onChanged(self, obj, prop):
        if hasattr(self, 'propertiesReady'):
            if self.propertiesReady:
                if prop in ['Base']:
                    self.setEditorProperties(obj)

    def opOnDocumentRestored(self, obj):
        self.propertiesReady = False
        job = PathUtils.findParentJob(obj)

        self.initOpProperties(obj, warn=True)
        self.opApplyPropertyDefaults(obj, job, self.addNewProps)

        mode = 2 if PathLog.getLevel(PathLog.thisModule()) != 4 else 0
        obj.setEditorMode('ShowTempObjects', mode)

        # Repopulate enumerations in case of changes
        ENUMS = self.opPropertyEnumerations()
        for n in ENUMS:
            restore = False
            if hasattr(obj, n):
                val = obj.getPropertyByName(n)
                restore = True
            setattr(obj, n, ENUMS[n])
            if restore:
                setattr(obj, n, val)

        self.setEditorProperties(obj)

    def opApplyPropertyDefaults(self, obj, job, propList):
        # Set standard property defaults
        PROP_DFLTS = self.opPropertyDefaults(obj, job)
        for n in PROP_DFLTS:
            if n in propList:
                prop = getattr(obj, n)
                val = PROP_DFLTS[n]
                setVal = False
                if hasattr(prop, 'Value'):
                    if isinstance(val, int) or isinstance(val, float):
                        setVal = True
                if setVal:
                    propVal = getattr(prop, 'Value')
                    setattr(prop, 'Value', val)
                else:
                    setattr(obj, n, val)

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj, job) ... initialize defaults'''
        job = PathUtils.findParentJob(obj)

        self.opApplyPropertyDefaults(obj, job, self.addNewProps)

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
        pass

    def opUpdateDepths(self, obj):
        if hasattr(obj, 'Base') and obj.Base:
            base, sublist = obj.Base[0]
            fbb = base.Shape.getElement(sublist[0]).BoundBox
            zmin = fbb.ZMax
            for base, sublist in obj.Base:
                for sub in sublist:
                    try:
                        fbb = base.Shape.getElement(sub).BoundBox
                        zmin = min(zmin, fbb.ZMin)
                    except Part.OCCError as e:
                        PathLog.error(e)
            obj.OpFinalDepth = zmin

    def opExecute(self, obj):
        '''opExecute(obj) ... process surface operation'''
        PathLog.track()

        self.cancelOperation = False
        self.base = None
        self.shape1 = None
        self.shape2 = None
        self.shapeType1 = None
        self.shapeType2 = None
        self.shapeLength1 = None
        self.shapeLength2 = None
        self.dYdX1 = None
        self.dYdX2 = None
        self.bottomEdges = None
        self.isDebug = False if PathLog.getLevel(PathLog.thisModule()) != 4 else True
        self.showTempObjects = obj.ShowTempObjects
        CMDS = list()
        FCAD = FreeCAD.ActiveDocument

        try:
            dotIdx = __name__.index('.') + 1
        except Exception:
            dotIdx = 0
        self.module = __name__[dotIdx:]

        if not self.isDebug:
            self.showTempObjects = False
        if self.showTempObjects:
            for grpNm in ['tmpDebugGrp', 'tmpDebugGrp001']:
                if hasattr(FreeCAD.ActiveDocument, grpNm):
                    for go in FreeCAD.ActiveDocument.getObject(grpNm).Group:
                        FreeCAD.ActiveDocument.removeObject(go.Name)
                    FreeCAD.ActiveDocument.removeObject(grpNm)
            self.tmpGrp = FreeCAD.ActiveDocument.addObject('App::DocumentObjectGroup', 'tmpDebugGrp')
            tmpGrpNm = self.tmpGrp.Name

        # Identify parent Job
        JOB = PathUtils.findParentJob(obj)
        self.JOB = JOB
        if JOB is None:
            PathLog.error(translate('PathSlot', "No JOB"))
            return
        self.stockZMin = JOB.Stock.Shape.BoundBox.ZMin

        # Begin GCode for operation with basic information
        # ... and move cutter to clearance height and startpoint
        tool = obj.ToolController.Tool
        toolType = tool.ToolType if hasattr(tool, 'ToolType') else tool.ShapeName
        output = ''
        if obj.Comment != '':
            self.commandlist.append(Path.Command('N ({})'.format(obj.Comment), {}))
        self.commandlist.append(Path.Command('N ({})'.format(obj.Label), {}))
        self.commandlist.append(Path.Command('N (Tool type: {})'.format(toolType), {}))
        self.commandlist.append(Path.Command('N (Compensated Tool Path. Diameter: {})'.format(tool.Diameter), {}))
        self.commandlist.append(Path.Command('N ({})'.format(output), {}))
        self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        if obj.UseStartPoint is True:
            self.commandlist.append(Path.Command('G0', {'X': obj.StartPoint.x, 'Y': obj.StartPoint.y, 'F': self.horizRapid}))

        # Impose property limits
        self.opApplyPropertyLimits(obj)

        # Calculate default depthparams for operation
        self.depthParams = PathUtils.depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown.Value, 0.0, obj.FinalDepth.Value)

        # ######  MAIN COMMANDS FOR OPERATION ######

        cmds = self._makeOperation(obj)
        if cmds:
            CMDS.extend(cmds)

        # Save gcode produced
        CMDS.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        self.commandlist.extend(CMDS)

        # ######  CLOSING COMMANDS FOR OPERATION ######

        # Hide the temporary objects
        if self.showTempObjects:
            if FreeCAD.GuiUp:
                import FreeCADGui
                FreeCADGui.ActiveDocument.getObject(tmpGrpNm).Visibility = False
            self.tmpGrp.purgeTouched()

        if self.cancelOperation:
            FreeCAD.ActiveDocument.openTransaction(translate("PathSlot", "Canceled the Slot operation."))
            FreeCAD.ActiveDocument.removeObject(obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()

        return True

    # Control method for operation
    def _makeOperation(self, obj):
        """This method controls the overall slot creation process."""
        pnts = False
        featureCnt = 0

        def eLen(E):
            return E.Length

        if not hasattr(obj, 'Base'):
            msg = translate('PathSlot',
                    'No Base Geometry object in the operation.')
            FreeCAD.Console.PrintError(msg + '\n')
            return False

        # Calculate extensions to slot path
        begExt, endExt = 0, 0
        if obj.ExtendPathStart.Value != 0:
            begExt = obj.ExtendPathStart.Value
        if obj.ExtendPathEnd.Value != 0:
            endExt = obj.ExtendPathEnd.Value

        if not obj.Base:
            # Use custom inputs here
            p1 = obj.CustomPoint1
            p2 = obj.CustomPoint2
            if p1.z == p2.z:
                pnts = (p1, p2)
            else:
                msg = translate('PathSlot',
                        'Custom points not at same Z height.')
                FreeCAD.Console.PrintError(msg + '\n')
                return False

        if pnts:
            (p1, p2) = pnts
        else:
            baseGeom = obj.Base[0]
            base, subsList = baseGeom
            self.base = base
            lenSL = len(subsList)
            featureCnt = lenSL
            if lenSL == 1:
                sub1 = subsList[0]
                shape_1 = getattr(base.Shape, sub1)
                self.shape1 = shape_1
                pnts = self._processSingle(obj, shape_1, sub1)
            else:
                sub1 = subsList[0]
                sub2 = subsList[1]
                shape_1 = getattr(base.Shape, sub1)
                shape_2 = getattr(base.Shape, sub2)
                self.shape1 = shape_1
                self.shape2 = shape_2
                pnts = self._processDouble(obj, shape_1, sub1, shape_2, sub2)

        if not pnts:
            return False

        # Apply perpendicular rotation if requested
        perpZero = True
        if obj.PathOrientation == 'Perpendicular':
            if featureCnt == 2:
                if self.shapeType1 == 'Face' and self.shapeType2 == 'Face':
                    if self.bottomEdges:
                        self.bottomEdges.sort(key=lambda edg: edg.Length, reverse=True)
                        BE = self.bottomEdges[0]
                        pnts = self._processSingleVertFace(obj, BE)
                        perpZero = False
            if perpZero:
                (p1, p2) = pnts
                pnts = self._makePerpendicular(p1, p2, 10.0)
        else:
            perpZero = False

        # Reverse direction of path if requested
        if obj.ReverseDirection:
            (p2, p1) = pnts
        else:
            (p1, p2) = pnts

        # Apply extensions to slot path
        if perpZero:
            begExt -= 5.0
            endExt -= 5.0
        pnts = self._extendSlot(p1, p2, begExt, endExt)

        if not pnts:
            return False

        (p1, p2) = pnts
        if self.isDebug:
            PathLog.debug('p1, p2: {}, {}'.format(p1, p2))
            if p1.sub(p2).Length != 0 and self.showTempObjects:
                O = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmp_Path')
                O.Shape = Part.makeLine(p1, p2)
                O.purgeTouched()
                self.tmpGrp.addObject(O)

        if featureCnt:
            obj.CustomPoint1 = p1
            obj.CustomPoint2 = p2

        if self._lineCollisionCheck(obj, p1, p2):
            msg = obj.Label + ' '
            msg += translate('PathSlot',
                    'operation collides with model.')
            FreeCAD.Console.PrintWarning(msg + '\n')

        cmds = self._makeGCode(obj, p1, p2)
        return cmds

    def _makeGCode(self, obj, p1, p2):
        """This method is the last in the overall slot creation process.
        It accepts the operation object and two end points for the path.
        It returns the slot gcode for the operation."""
        CMDS = list()

        def layerPass(p1, p2, depth):
            cmds = list()
            # cmds.append(Path.Command('N (Tool type: {})'.format(toolType), {}))
            cmds.append(Path.Command('G0', {'X': p1.x, 'Y': p1.y, 'F': self.horizRapid}))
            cmds.append(Path.Command('G1', {'Z': depth, 'F': self.vertFeed}))
            cmds.append(Path.Command('G1', {'X': p2.x, 'Y': p2.y, 'F': self.horizFeed}))
            return cmds

        # CMDS.append(Path.Command('N (Tool type: {})'.format(toolType), {}))
        # CMDS.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        if obj.LayerMode == 'Single-pass':
            CMDS.extend(layerPass(p1, p2, obj.FinalDepth.Value))
            CMDS.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
        else:
            prvDep = obj.StartDepth.Value
            for dep in self.depthParams:
                CMDS.extend(layerPass(p1, p2, dep))
                CMDS.append(Path.Command('G0', {'Z': prvDep, 'F': self.vertRapid}))
                prvDep = dep
            CMDS.append(Path.Command('G0', {'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))

        return CMDS

    # Methods for processing single geometry
    def _processSingle(self, obj, shape_1, sub1):
        """This is the control method for slots based on a
        single Base Geometry feature."""
        cmds = False
        make = False
        cat1 = sub1[:4]

        if cat1 == 'Face':
            pnts = False

            norm = shape_1.normalAt(0.0, 0.0)
            PathLog.debug('Face.normalAt(): {}'.format(norm))
            if norm.z == 1 or norm.z == -1:
                pnts = self._processSingleHorizFace(obj, shape_1)
            elif norm.z == 0:
                faceType = self._getVertFaceType(shape_1)
                if faceType:
                    (geo, shp) = faceType
                    if geo == 'Face':
                        pnts = self._processSingleComplexFace(obj, shape_1)
                    if geo == 'Wire':
                        pnts = self._processSingleVertFace(obj, shp)
                    if geo == 'Edge':
                        pnts = self._processSingleVertFace(obj, shp)
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
            V1 = shape_1.Vertexes[0]
            V2 = shape_1.Vertexes[1]
            p1 = FreeCAD.Vector(V1.X, V1.Y, 0.0)
            p2 = FreeCAD.Vector(V2.X, V2.Y, 0.0)
            make = True

        elif cat1 == 'Vert':
            msg = translate('PathSlot',
                'Only a vertex selected. Add another feature to the Base Geometry.')
            FreeCAD.Console.PrintError(msg + '\n')

        if make:
            return (p1, p2)

        return False

    def _processSingleHorizFace(self, obj, shape):
        """Determine slot path endpoints from a single horizontally oriented face."""
        lineTypes = ['Part::GeomLine']

        def getRadians(self, E):
            vect = self._dXdYdZ(E)
            norm = self._normalizeVector(vect)
            rads = self._xyToRadians(norm)
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
                             getRadians(self, shape.Edges[i]))
                            )

        # Sort tuples by edge angle
        eTups.sort(key=lambda tup: tup[2])
        # Identify parallel edges
        pairs = list()
        eCnt = len(shape.Edges)
        lstE = eCnt - 1
        I = [i for i in range(0, eCnt)]
        I.append(0)
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

        if pairCnt == 0:
            msg = translate('PathSlot',
                'No parallel edges identified.')
            FreeCAD.Console.PrintError(msg + '\n')
            return False
        elif pairCnt == 1:
            same = pairs[0]
        else:
            if obj.Reference1 == 'Long Edge':
                same = pairs[0]
            elif obj.Reference1 == 'Short Edge':
                same = pairs[1]
            else:
                msg = 'Reference1 '
                msg += translate('PathSlot',
                    'value error.')
                FreeCAD.Console.PrintError(msg + '\n')
                return False

        (p1, p2) = self._getOppMidPoints(same)
        return (p1, p2)

    def _processSingleComplexFace(self, obj, shape):
        """Determine slot path endpoints from a single complex face."""
        PNTS = list()

        def zVal(V):
            return V.z

        for E in shape.Wires[0].Edges:
            p = self._findLowestEdgePoint(E)
            PNTS.append(p)
        PNTS.sort(key=zVal)
        return (PNTS[0], PNTS[1])

    def _processSingleVertFace(self, obj, shape):
        """Determine slot path endpoints from a single vertically oriented face
        with no single bottom edge."""
        eCnt = len(shape.Edges)
        V0 = shape.Edges[0].Vertexes[0]
        V1 = shape.Edges[eCnt - 1].Vertexes[1]
        v0 = FreeCAD.Vector(V0.X, V0.Y, V0.Z)
        v1 = FreeCAD.Vector(V1.X, V1.Y, V1.Z)

        dX = V1.X - V0.X
        dY = V1.Y - V0.Y
        dZ = V1.Z - V0.Z
        temp = FreeCAD.Vector(dX, dY, dZ)
        slope = self._normalizeVector(temp)
        perpVect = FreeCAD.Vector(-1 * slope.y, slope.x, slope.z)
        perpVect.multiply(self.tool.Diameter / 2.0)

        # Create offset endpoints for raw slot path
        a1 = v0.add(perpVect)
        a2 = v1.add(perpVect)
        b1 = v0.sub(perpVect)
        b2 = v1.sub(perpVect)
        (p1, p2) = self._getCutSidePoints(obj, v0, v1, a1, a2, b1, b2)
        return (p1, p2)

    # Methods for processing double geometry
    def _processDouble(self, obj, shape_1, sub1, shape_2, sub2):
        """This is the control method for slots based on a
        two Base Geometry features."""
        cmds = False
        make = False
        cat2 = sub2[:4]
        p1 = None
        p2 = None
        dYdX1 = None
        dYdX2 = None
        self.bottomEdges = list()

        feature1 = self._processFeature(obj, shape_1, sub1, 1)
        if not feature1:
            msg = translate('PathSlot',
                'Failed to determine point 1 from')
            FreeCAD.Console.PrintError(msg + ' {}.\n'.format(sub1))
            return False
        (p1, dYdX1, shpType) = feature1
        self.shapeType1 = shpType
        if dYdX1:
            self.dYdX1 = dYdX1

        feature2 = self._processFeature(obj, shape_2, sub2, 2)
        if not feature2:
            msg = translate('PathSlot',
                'Failed to determine point 2 from')
            FreeCAD.Console.PrintError(msg + ' {}.\n'.format(sub2))
            return False
        (p2, dYdX2, shpType) = feature2
        self.shapeType2 = shpType
        if dYdX2:
            self.dYdX2 = dYdX2

        # Parallel check for twin face, and face-edge cases
        if dYdX1 and dYdX2:
            if not self._isParallel(dYdX1, dYdX2):
                PathLog.debug('dYdX1, dYdX2: {}, {}'.format(dYdX1, dYdX2))
                msg = translate('PathSlot',
                    'Selected geometry not parallel.')
                FreeCAD.Console.PrintError(msg + '\n')
                return False

        if p2:
            return (p1, p2)

        return False

    # Support methods
    def _dXdYdZ(self, E):
        v1 = E.Vertexes[0]
        v2 = E.Vertexes[1]
        dX = v2.X - v1.X
        dY = v2.Y - v1.Y
        dZ = v2.Z - v1.Z
        return FreeCAD.Vector(dX, dY, dZ)

    def _normalizeVector(self, v):
        posTol = 0.0000000001
        negTol = -1 * posTol
        V = FreeCAD.Vector(v.x, v.y, v.z)
        V.normalize()
        x = V.x
        y = V.y
        z = V.z

        if V.x != 0 and abs(V.x) < posTol:
            x = 0.0
        if V.x != 1 and 1.0 - V.x < posTol:
            x = 1.0
        if V.x != -1 and -1.0 - V.x > negTol:
            x = -1.0

        if V.y != 0 and abs(V.y) < posTol:
            y = 0.0
        if V.y != 1 and 1.0 - V.y < posTol:
            y = 1.0
        if V.y != -1 and -1.0 - V.y > negTol:
            y = -1.0

        if V.z != 0 and abs(V.z) < posTol:
            z = 0.0
        if V.z != 1 and 1.0 - V.z < posTol:
            z = 1.0
        if V.z != -1 and -1.0 - V.z > negTol:
            z = -1.0

        return FreeCAD.Vector(x, y, z)

    def _getLowestPoint(self, shape_1):
        # find lowest vertex
        vMin = shape_1.Vertexes[0]
        zmin = vMin.Z
        same = [vMin]
        for V in shape_1.Vertexes:
            if V.Z < zmin:
                zmin = V.Z
                vMin = V
            elif V.Z == zmin:
                same.append(V)
        if len(same) > 1:
            X = [E.X for E in same]
            Y = [E.Y for E in same]
            avgX = sum(X) / len(X)
            avgY = sum(Y) / len(Y)
            return FreeCAD.Vector(avgX, avgY, zmin)
        else:
            return FreeCAD.Vector(V.X, V.Y, V.Z)

    def _getHighestPoint(self, shape_1):
        # find highest vertex
        vMax = shape_1.Vertexes[0]
        zmax = vMax.Z
        same = [vMax]
        for V in shape_1.Vertexes:
            if V.Z > zmax:
                zmax = V.Z
                vMax = V
            elif V.Z == zmax:
                same.append(V)
        if len(same) > 1:
            X = [E.X for E in same]
            Y = [E.Y for E in same]
            avgX = sum(X) / len(X)
            avgY = sum(Y) / len(Y)
            return FreeCAD.Vector(avgX, avgY, zmax)
        else:
            return FreeCAD.Vector(V.X, V.Y, V.Z)

    def _processFeature(self, obj, shape, sub, pNum):
        p = None
        dYdX = None
        cat = sub[:4]
        Ref = getattr(obj, 'Reference' + str(pNum))
        if cat == 'Face':
            BE = self._getBottomEdge(shape)
            if BE:
                self.bottomEdges.append(BE)
            # calculate slope of face
            V0 = shape.Vertexes[0]
            v1 = shape.CenterOfMass
            temp = FreeCAD.Vector(v1.x - V0.X, v1.y - V0.Y, 0.0)
            dYdX = self._normalizeVector(temp)

            # Determine normal vector for face
            norm = shape.normalAt(0.0, 0.0)
            # FreeCAD.Console.PrintMessage('{} normal {}.\n'.format(sub, norm))
            if norm.z != 0:
                msg = translate('PathSlot',
                    'The selected face is not oriented vertically:')
                FreeCAD.Console.PrintError(msg + ' {}.\n'.format(sub))
                return False

            if Ref == 'Center of Mass':
                comS = shape.CenterOfMass
                p = FreeCAD.Vector(comS.x, comS.y, 0.0)
            elif Ref == 'Center of BoundBox':
                comS = shape.BoundBox.Center
                p = FreeCAD.Vector(comS.x, comS.y, 0.0)
            elif Ref == 'Lowest Point':
                p = self._getLowestPoint(shape)
            elif Ref == 'Highest Point':
                p = self._getHighestPoint(shape)

        elif cat == 'Edge':
            # calculate slope between end vertexes
            v0 = shape.Edges[0].Vertexes[0]
            v1 = shape.Edges[0].Vertexes[1]
            temp = FreeCAD.Vector(v1.X - v0.X, v1.Y - v0.Y, 0.0)
            dYdX = self._normalizeVector(temp)

            if Ref == 'Center of Mass':
                comS = shape.CenterOfMass
                p = FreeCAD.Vector(comS.x, comS.y, 0.0)
            elif Ref == 'Center of BoundBox':
                comS = shape.BoundBox.Center
                p = FreeCAD.Vector(comS.x, comS.y, 0.0)
            elif Ref == 'Lowest Point':
                p = self._findLowestPointOnEdge(shape)
            elif Ref == 'Highest Point':
                p = self._findHighestPointOnEdge(shape)

        elif cat == 'Vert':
            V = shape.Vertexes[0]
            p = FreeCAD.Vector(V.X, V.Y, 0.0)

        if p:
            return (p, dYdX, cat)

        return False

    def _extendSlot(self, p1, p2, begExt, endExt):
        if begExt:
            beg = p1.sub(p2)
            beg.normalize()
            beg.multiply(begExt)
            n1 = p1.add(beg)
        else:
            n1 = p1
        if endExt:
            end = p2.sub(p1)
            end.normalize()
            end.multiply(endExt)
            n2 = p2.add(end)
        else:
            n2 = p2
        return (n1, n2)

    def _getEndMidPoints(self, same):
        # Find mid-points between ends of equal, oppossing edges
        e0va = same[0].Vertexes[0]
        e0vb = same[0].Vertexes[1]
        e1va = same[1].Vertexes[0]
        e1vb = same[1].Vertexes[1]

        if False:
            midX1 = (e0va.X + e0vb.X) / 2.0
            midY1 = (e0va.Y + e0vb.Y) / 2.0
            midX2 = (e1va.X + e1vb.X) / 2.0
            midY2 = (e1va.Y + e1vb.Y) / 2.0
            m1 = FreeCAD.Vector(midX1, midY1, e0va.Z)
            m2 = FreeCAD.Vector(midX2, midY2, e0va.Z)

        p1 = FreeCAD.Vector(e0va.X, e0va.Y, e0va.Z)
        p2 = FreeCAD.Vector(e0vb.X, e0vb.Y, e0vb.Z)
        p3 = FreeCAD.Vector(e1va.X, e1va.Y, e1va.Z)
        p4 = FreeCAD.Vector(e1vb.X, e1vb.Y, e1vb.Z)

        L0 = Part.makeLine(p1, p2)
        L1 = Part.makeLine(p3, p4)
        comL0 = L0.CenterOfMass
        comL1 = L1.CenterOfMass
        m1 = FreeCAD.Vector(comL0.x, comL0.y, 0.0)
        m2 = FreeCAD.Vector(comL1.x, comL1.y, 0.0)

        return (m1, m2)

    def _getOppMidPoints(self, same):
        # Find mid-points between ends of equal, oppossing edges
        v1 = same[0].Vertexes[0]
        v2 = same[0].Vertexes[1]
        a1 = same[1].Vertexes[0]
        a2 = same[1].Vertexes[1]
        midX1 = (v1.X + a2.X) / 2.0
        midY1 = (v1.Y + a2.Y) / 2.0
        midX2 = (v2.X + a1.X) / 2.0
        midY2 = (v2.Y + a1.Y) / 2.0
        p1 = FreeCAD.Vector(midX1, midY1, v1.Z)
        p2 = FreeCAD.Vector(midX2, midY2, v1.Z)
        return (p1, p2)

    def _isParallel(self, dYdX1, dYdX2):
        if dYdX1.add(dYdX2).Length == 0:
            return True
        if ((dYdX1.x + dYdX2.x) / 2.0 == dYdX1.x and
                (dYdX1.y + dYdX2.y) / 2.0 == dYdX1.y):
            return True
        return False

    def _makePerpendicular(self, p1, p2, length):
        line = Part.makeLine(p1, p2)
        midPnt = line.CenterOfMass

        halfDist = length / 2.0
        if self.dYdX1:
            half = FreeCAD.Vector(self.dYdX1.x, self.dYdX1.y, 0.0).multiply(halfDist)
            n1 = midPnt.add(half)
            n2 = midPnt.sub(half)
            return (n1, n2)
        elif self.dYdX2:
            half = FreeCAD.Vector(self.dYdX2.x, self.dYdX2.y, 0.0).multiply(halfDist)
            n1 = midPnt.add(half)
            n2 = midPnt.sub(half)
            return (n1, n2)
        else:
            toEnd = p2.sub(p1)
            factor = halfDist / toEnd.Length
            perp = FreeCAD.Vector(-1 * toEnd.y, toEnd.x, 0.0)
            perp.normalize()
            perp.multiply(halfDist)
            n1 = midPnt.add(perp)
            n2 = midPnt.sub(perp)
            return (n1, n2)

    def _findLowestPointOnEdge(self, E):
        tol = 0.0000001
        zMin = E.BoundBox.ZMin
        # Test first vertex
        v = E.Vertexes[0]
        if abs(v.Z - zMin) < tol:
            return FreeCAD.Vector(v.X, v.Y, v.Z)
        # Test second vertex
        v = E.Vertexes[1]
        if abs(v.Z - zMin) < tol:
            return FreeCAD.Vector(v.X, v.Y, v.Z)
        # Test middle point of edge
        eMidLen = E.Length / 2.0
        eMidPnt = E.valueAt(E.getParameterByLength(eMidLen))
        if abs(eMidPnt.z - zMin) < tol:
            return eMidPnt
        if E.BoundBox.ZLength < 0.000000001:  # roughly horizontal edge
            return eMidPnt
        return self._findLowestEdgePoint(E)

    def _findLowestEdgePoint(self, E):
        zMin = E.BoundBox.ZMin
        eLen = E.Length
        L0 = 0
        L1 = eLen
        p0 = None
        p1 = None
        cnt = 0
        while L1 - L0 > 0.00001 and cnt < 2000:
            adj = (L1 - L0) * 0.1
            # Get points at L0 and L1 along edge
            p0 = E.valueAt(E.getParameterByLength(L0))
            p1 = E.valueAt(E.getParameterByLength(L1))
            # Adjust points based on proximity to target depth
            diff0 = p0.z - zMin
            diff1 = p1.z - zMin
            if diff0 < diff1:
                L1 -= adj
            elif diff0 > diff1:
                L0 += adj
            else:
                L0 += adj
                L1 -= adj
            cnt += 1
        midLen = (L0 + L1) / 2.0
        return E.valueAt(E.getParameterByLength(midLen))

    def _findHighestPointOnEdge(self, E):
        tol = 0.0000001
        zMax = E.BoundBox.ZMax
        # Test first vertex
        v = E.Vertexes[0]
        if abs(zMax - v.Z) < tol:
            return FreeCAD.Vector(v.X, v.Y, v.Z)
        # Test second vertex
        v = E.Vertexes[1]
        if abs(zMax - v.Z) < tol:
            return FreeCAD.Vector(v.X, v.Y, v.Z)
        # Test middle point of edge
        eMidLen = E.Length / 2.0
        eMidPnt = E.valueAt(E.getParameterByLength(eMidLen))
        if abs(zMax - eMidPnt.z) < tol:
            return eMidPnt
        if E.BoundBox.ZLength < 0.000000001:  # roughly horizontal edge
            return eMidPnt
        return self._findHighestEdgePoint(E)

    def _findHighestEdgePoint(self, E):
        zMax = E.BoundBox.ZMax
        eLen = E.Length
        L0 = 0
        L1 = eLen
        p0 = None
        p1 = None
        cnt = 0
        while L1 - L0 > 0.00001 and cnt < 2000:
            adj = (L1 - L0) * 0.1
            # Get points at L0 and L1 along edge
            p0 = E.valueAt(E.getParameterByLength(L0))
            p1 = E.valueAt(E.getParameterByLength(L1))
            # Adjust points based on proximity to target depth
            diff0 = zMax - p0.z
            diff1 = zMax - p1.z
            if diff0 < diff1:
                L1 -= adj
            elif diff0 > diff1:
                L0 += adj
            else:
                L0 += adj
                L1 -= adj
            cnt += 1
        midLen = (L0 + L1) / 2.0
        return E.valueAt(E.getParameterByLength(midLen))

    def _xyToRadians(self, v):
        # Assumes Z value of vector is zero
        halfPi = math.pi / 2

        if v.y == 1 and v.x == 0:
            return halfPi
        if v.y == -1 and v.x == 0:
            return math.pi + halfPi
        if v.y == 0 and v.x == 1:
            return 0.0
        if v.y == 0 and v.x == -1:
            return math.pi

        x = abs(v.x)
        y = abs(v.y)
        rads = math.atan(y/x)
        if v.x > 0:
            if v.y > 0:
                return rads
            else:
                return (2 * math.pi) - rads
        if v.x < 0:
            if v.y > 0:
                return math.pi - rads
            else:
                return math.pi + rads

    def _getCutSidePoints(self, obj, v0, v1, a1, a2, b1, b2):
        ea1 = Part.makeLine(v0, a1)
        ea2 = Part.makeLine(a1, a2)
        ea3 = Part.makeLine(a2, v1)
        ea4 = Part.makeLine(v1, v0)
        boxA = Part.Face(Part.Wire([ea1, ea2, ea3, ea4]))
        cubeA = boxA.extrude(FreeCAD.Vector(0.0, 0.0, 1.0))
        cmnA = self.base.Shape.common(cubeA)
        eb1 = Part.makeLine(v0, b1)
        eb2 = Part.makeLine(b1, b2)
        eb3 = Part.makeLine(b2, v1)
        eb4 = Part.makeLine(v1, v0)
        boxB = Part.Face(Part.Wire([eb1, eb2, eb3, eb4]))
        cubeB = boxB.extrude(FreeCAD.Vector(0.0, 0.0, 1.0))
        cmnB = self.base.Shape.common(cubeB)
        if cmnA.Volume > cmnB.Volume:
            return (b1, b2)
        return (a1, a2)

    def _getBottomEdge(self, shape):
        EDGES = list()
        # Determine if selected face has a single bottom horizontal edge
        eCnt = len(shape.Edges)
        eZMin = shape.BoundBox.ZMin
        for ei in range(0, eCnt):
            E = shape.Edges[ei]
            if abs(E.BoundBox.ZMax - eZMin) < 0.00000001:
                EDGES.append(E)
        if len(EDGES) == 1:  # single bottom horiz. edge
            return EDGES[0]
        return False

    def _getVertFaceType(self, shape):
        wires = list()

        bottomEdge = self._getBottomEdge(shape)
        if bottomEdge:
            return ('Edge', bottomEdge)

        # Extract cross-section of face
        extFwd = (shape.BoundBox.ZLength * 2.2) + 10
        extShp = shape.extrude(FreeCAD.Vector(0.0, 0.0, extFwd))
        sliceZ = shape.BoundBox.ZMin + (extFwd / 2.0)
        slcs = extShp.slice(FreeCAD.Vector(0, 0, 1), sliceZ)
        for i in slcs:
            wires.append(i)
        if len(wires) > 0:
            isFace = False
            csWire = wires[0]
            if wires[0].isClosed():
                csWire = Part.Face(wires[0])
                if csWire.Area > 0:
                    csWire.translate(FreeCAD.Vector(0.0, 0.0, shape.BoundBox.ZMin - csWire.BoundBox.ZMin))
                    return ('Face', csWire)
            return ('Wire', wires[0])
        return False

    def _getReference1Enums(self, sub, single=False):
        # Adjust available enumerations
        enums1 = self.opPropertyEnumerations()['Reference1']
        for ri in removeIndexesFromReference_1(sub, single):
            enums1.pop(ri)
        return enums1

    def _getReference2Enums(self, sub):
        # Adjust available enumerations
        enums2 = self.opPropertyEnumerations()['Reference2']
        for ri in removeIndexesFromReference_2(sub):
            enums2.pop(ri)
        return enums2

    def _lineCollisionCheck(self, obj, p1, p2):
        """Make simple circle with diameter of tool, at start point.
        Extrude it latterally along path.
        Extrude it vertically.
        Check for collision with model."""
        # Make path travel of tool as 3D solid.
        rad = self.tool.Diameter / 2.0

        def getPerp(p1, p2, dist):
            toEnd = p2.sub(p1)
            factor = dist / toEnd.Length
            perp = FreeCAD.Vector(-1 * toEnd.y, toEnd.x, 0.0)
            perp.normalize()
            perp.multiply(dist)
            return perp

        ce1 = Part.Wire(Part.makeCircle(rad, p1).Edges)
        ce2 = Part.Wire(Part.makeCircle(rad, p2).Edges)
        C1 = Part.Face(ce1)
        C2 = Part.Face(ce2)

        zTrans = obj.FinalDepth.Value - C1.BoundBox.ZMin
        C1.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
        zTrans = obj.FinalDepth.Value - C2.BoundBox.ZMin
        C2.translate(FreeCAD.Vector(0.0, 0.0, zTrans))

        extFwd = obj.StartDepth.Value - obj.FinalDepth.Value
        extVect = FreeCAD.Vector(0.0, 0.0, extFwd)
        startShp = C1.extrude(extVect)
        endShp = C2.extrude(extVect)

        perp = getPerp(p1, p2, rad)
        v1 = p1.add(perp)
        v2 = p1.sub(perp)
        v3 = p2.sub(perp)
        v4 = p2.add(perp)
        e1 = Part.makeLine(v1, v2)
        e2 = Part.makeLine(v2, v3)
        e3 = Part.makeLine(v3, v4)
        e4 = Part.makeLine(v4, v1)
        edges = Part.__sortEdges__([e1, e2, e3, e4])
        rectFace = Part.Face(Part.Wire(edges))
        zTrans = obj.FinalDepth.Value - rectFace.BoundBox.ZMin
        rectFace.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
        boxShp = rectFace.extrude(extVect)

        part1 = startShp.fuse(boxShp)
        pathTravel = part1.fuse(endShp)

        if self.showTempObjects:
            O = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmp_PathTravel')
            O.Shape = pathTravel
            O.purgeTouched()
            self.tmpGrp.addObject(O)

        # Check for collision with model
        try:
            cmn = self.base.Shape.common(pathTravel)
            if cmn.Volume > 0.000001:
                return True
        except Exception:
            PathLog.debug('Failed to complete path collision check.')

        return False
# Eclass


# Determine applicable enumerations
def removeIndexesFromReference_1(sub, single=False):
    """Determine which enumerations to remove for Reference1 input
    based upon the feature type(category)."""
    cat = sub[:4]
    remIdxs = [6, 5, 4]
    if cat == 'Face':
        if single:
            remIdxs = [6, 3, 2, 1, 0]
    elif cat == 'Edge':
        if single:
            remIdxs = [6, 5, 3, 2, 1, 0]
    elif cat == 'Vert':
        remIdxs = [5, 4, 3, 2, 1, 0]
    return remIdxs


def removeIndexesFromReference_2(sub):
    """Determine which enumerations to remove for Reference2 input
    based upon the feature type(category)."""
    cat = sub[:4]
    remIdxs = [4]
    # Customize Reference combobox options
    if cat == 'Vert':
        remIdxs = [3, 2, 1, 0]
    return remIdxs



def SetupProperties():
    ''' SetupProperties() ... Return list of properties required for operation.'''
    return [tup[1] for tup in ObjectSlot.opPropertyDefinitions(False)]


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Slot operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectSlot(obj, name)
    return obj
