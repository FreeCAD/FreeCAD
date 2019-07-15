# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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
# *   Focus: 4th-axis integration                                           *
# *   by Russell Johnson  <russ4262@gmail.com>                              *
# *                                                                         *
# ***************************************************************************

import ArchPanel
import FreeCAD
import DraftGeomUtils
import Part
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils

from PySide import QtCore
import PathScripts.PathGeom as PathGeom

import math
import Draft
if FreeCAD.GuiUp:
    import FreeCADGui

__title__ = "Path Circular Holes Base Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Base class an implementation for operations on circular holes."
__contributors__ = "russ4262 (Russell Johnson)"
__created__ = "2017"
__scriptVersion__ = "1d testing"
__lastModified__ = "2019-07-12 09:58 CST"


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


class ObjectOp(PathOp.ObjectOp):
    '''Base class for proxy objects of all operations on circular holes.'''
    # These are static while document is open, if it contains a CircularHole Op
    initOpFinalDepth = None
    initOpStartDepth = None
    initWithRotation = False
    defValsSet = False
    docRestored = False

    def opFeatures(self, obj):
        '''opFeatures(obj) ... calls circularHoleFeatures(obj) and ORs in the standard features required for processing circular holes.
        Do not overwrite, implement circularHoleFeatures(obj) instead'''
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureBaseFaces | self.circularHoleFeatures(obj)

    def circularHoleFeatures(self, obj):
        '''circularHoleFeatures(obj) ... overwrite to add operations specific features.
        Can safely be overwritten by subclasses.'''
        # pylint: disable=unused-argument
        return 0

    def initOperation(self, obj):
        '''initOperation(obj) ... adds Disabled properties and calls initCircularHoleOperation(obj).
        Do not overwrite, implement initCircularHoleOperation(obj) instead.'''
        obj.addProperty("App::PropertyStringList", "Disabled", "Base", QtCore.QT_TRANSLATE_NOOP("Path", "List of disabled features"))

        self.initCircularHoleOperation(obj)

    def initCircularHoleOperation(self, obj):
        '''initCircularHoleOperation(obj) ... overwrite if the subclass needs initialisation.
        Can safely be overwritten by subclasses.'''
        pass # pylint: disable=unnecessary-pass

    def baseIsArchPanel(self, obj, base):
        '''baseIsArchPanel(obj, base) ... return true if op deals with an Arch.Panel.'''
        # pylint: disable=unused-argument
        return hasattr(base, "Proxy") and isinstance(base.Proxy, ArchPanel.PanelSheet)

    def getArchPanelEdge(self, obj, base, sub):
        '''getArchPanelEdge(obj, base, sub) ... helper function to identify a specific edge of an Arch.Panel.
        Edges are identified by 3 numbers:
            <holeId>.<wireId>.<edgeId>
        Let's say the edge is specified as "3.2.7", then the 7th edge of the 2nd wire in the 3rd hole returned
        by the panel sheet is the edge returned.
        Obviously this is as fragile as can be, but currently the best we can do while the panel sheets
        hide the actual features from Path and they can't be referenced directly.
        '''
        # pylint: disable=unused-argument
        ids = sub.split(".")
        holeId = int(ids[0])
        wireId = int(ids[1])
        edgeId = int(ids[2])

        for holeNr, hole in enumerate(base.Proxy.getHoles(base, transform=True)):
            if holeNr == holeId:
                for wireNr, wire in enumerate(hole.Wires):
                    if wireNr == wireId:
                        for edgeNr, edge in enumerate(wire.Edges):
                            if edgeNr == edgeId:
                                return edge

    def holeDiameter(self, obj, base, sub):
        '''holeDiameter(obj, base, sub) ... returns the diameter of the specified hole.'''
        if self.baseIsArchPanel(obj, base):
            edge = self.getArchPanelEdge(obj, base, sub)
            return edge.BoundBox.XLength

        try:
            shape = base.Shape.getElement(sub)
            if shape.ShapeType == 'Vertex':
                return 0

            if shape.ShapeType == 'Edge' and type(shape.Curve) == Part.Circle:
                return shape.Curve.Radius * 2

            # for all other shapes the diameter is just the dimension in X
            return shape.BoundBox.XLength
        except Part.OCCError as e:
            PathLog.error(e)

        return 0

    def holePosition(self, obj, base, sub):
        '''holePosition(obj, base, sub) ... returns a Vector for the position defined by the given features.
        Note that the value for Z is set to 0.'''
        if self.baseIsArchPanel(obj, base):
            edge = self.getArchPanelEdge(obj, base, sub)
            center = edge.Curve.Center
            return FreeCAD.Vector(center.x, center.y, 0)

        try:
            shape = base.Shape.getElement(sub)
            if shape.ShapeType == 'Vertex':
                return FreeCAD.Vector(shape.X, shape.Y, 0)

            if shape.ShapeType == 'Edge' and hasattr(shape.Curve, 'Center'):
                return FreeCAD.Vector(shape.Curve.Center.x, shape.Curve.Center.y, 0)

            if shape.ShapeType == 'Face':
                if hasattr(shape.Surface, 'Center'):
                    return FreeCAD.Vector(shape.Surface.Center.x, shape.Surface.Center.y, 0)
                if len(shape.Edges) == 1 and type(shape.Edges[0].Curve) == Part.Circle:
                    return shape.Edges[0].Curve.Center
        except Part.OCCError as e:
            PathLog.error(e)

        PathLog.error(translate("Path", "Feature %s.%s cannot be processed as a circular hole - please remove from Base geometry list.") % (base.Label, sub))
        return None

    def isHoleEnabled(self, obj, base, sub):
        '''isHoleEnabled(obj, base, sub) ... return true if hole is enabled.'''
        name = "%s.%s" % (base.Name, sub)
        return not name in obj.Disabled

    def opExecute(self, obj):
        '''opExecute(obj) ... processes all Base features and Locations and collects
        them in a list of positions and radii which is then passed to circularHoleExecute(obj, holes).
        If no Base geometries and no Locations are present, the job's Base is inspected and all
        drillable features are added to Base. In this case appropriate values for depths are also
        calculated and assigned.
        Do not overwrite, implement circularHoleExecute(obj, holes) instead.'''
        PathLog.track()

        holes = []
        baseSubsTuples = []
        subCount = 0
        allTuples = []
        self.cloneNames = []    # pylint: disable=attribute-defined-outside-init
        self.guiMsgs = []       # pylint: disable=attribute-defined-outside-init
        self.rotateFlag = False # pylint: disable=attribute-defined-outside-init
        self.useTempJobClones('Delete') # pylint: disable=attribute-defined-outside-init
        self.stockBB = PathUtils.findParentJob(obj).Stock.Shape.BoundBox # pylint: disable=attribute-defined-outside-init
        self.clearHeight = obj.ClearanceHeight.Value # pylint: disable=attribute-defined-outside-init
        self.safeHeight = obj.SafeHeight.Value # pylint: disable=attribute-defined-outside-init
        self.axialFeed = 0.0 # pylint: disable=attribute-defined-outside-init
        self.axialRapid = 0.0 # pylint: disable=attribute-defined-outside-init
        trgtDep = None

        def haveLocations(self, obj):
            if PathOp.FeatureLocations & self.opFeatures(obj):
                return len(obj.Locations) != 0
            return False

        if obj.EnableRotation == 'Off':
            # maxDep = self.stockBB.ZMax
            # minDep = self.stockBB.ZMin
            strDep = obj.StartDepth.Value
            finDep = obj.FinalDepth.Value
        else:
            # Calculate operation heights based upon rotation radii
            opHeights = self.opDetermineRotationRadii(obj)
            (self.xRotRad, self.yRotRad, self.zRotRad) = opHeights[0] # pylint: disable=attribute-defined-outside-init
            (clrOfset, safOfst) = opHeights[1]
            PathLog.debug("Exec. opHeights[0]: " + str(opHeights[0]))
            PathLog.debug("Exec. opHeights[1]: " + str(opHeights[1]))

            # Set clearance and safe heights based upon rotation radii
            if obj.EnableRotation == 'A(x)':
                strDep = self.xRotRad
            elif obj.EnableRotation == 'B(y)':
                strDep = self.yRotRad
            else:
                strDep = max(self.xRotRad, self.yRotRad)
            finDep = -1 * strDep

            obj.ClearanceHeight.Value = strDep + clrOfset
            obj.SafeHeight.Value = strDep + safOfst

            # Create visual axes when debugging.
            if PathLog.getLevel(PathLog.thisModule()) == 4:
                self.visualAxis()

            # Set axial feed rates based upon horizontal feed rates
            safeCircum = 2 * math.pi * obj.SafeHeight.Value
            self.axialFeed = 360 / safeCircum * self.horizFeed # pylint: disable=attribute-defined-outside-init
            self.axialRapid = 360 / safeCircum * self.horizRapid # pylint: disable=attribute-defined-outside-init

        # Complete rotational analysis and temp clone creation as needed
        if obj.EnableRotation == 'Off':
            PathLog.debug("Enable Rotation setting is 'Off' for {}.".format(obj.Name))
            stock = PathUtils.findParentJob(obj).Stock
            for (base, subList) in obj.Base:
                baseSubsTuples.append((base, subList, 0.0, 'A', stock))
        else:
            for p in range(0, len(obj.Base)):
                (base, subsList) = obj.Base[p]
                for sub in subsList:
                    if self.isHoleEnabled(obj, base, sub):
                        shape = getattr(base.Shape, sub)
                        rtn = False
                        (norm, surf) = self.getFaceNormAndSurf(shape)
                        (rtn, angle, axis, praInfo) = self.faceRotationAnalysis(obj, norm, surf) # pylint: disable=unused-variable
                        if rtn is True:
                            (clnBase, angle, clnStock, tag) = self.applyRotationalAnalysis(obj, base, angle, axis, subCount)
                            # Verify faces are correctly oriented - InverseAngle might be necessary
                            PathLog.debug("Verifying {} orientation: running faceRotationAnalysis() again.".format(sub))
                            faceIA = getattr(clnBase.Shape, sub)
                            (norm, surf) = self.getFaceNormAndSurf(faceIA)
                            (rtn, praAngle, praAxis, praInfo) = self.faceRotationAnalysis(obj, norm, surf) # pylint: disable=unused-variable
                            if rtn is True:
                                msg = obj.Name + ":: "
                                msg += translate("Path", "{} might be misaligned after initial rotation.".format(sub)) + "  "
                                if obj.AttemptInverseAngle is True and obj.InverseAngle is False:
                                    (clnBase, clnStock, angle) = self.applyInverseAngle(obj, clnBase, clnStock, axis, angle)
                                    msg += translate("Path", "Rotated to 'InverseAngle' to attempt access.")
                                else:
                                    if len(subsList) == 1:
                                        msg += translate("Path", "Consider toggling the 'InverseAngle' property and recomputing.")
                                    else:
                                        msg += translate("Path", "Consider transferring '{}' to independent operation.".format(sub))
                                PathLog.warning(msg)
                                # title = translate("Path", 'Rotation Warning')
                                # self.guiMessage(title, msg, False)
                            else:
                                PathLog.debug("Face appears to be oriented correctly.")

                            cmnt = "{}: {} @ {};  ".format(sub, axis, str(round(angle, 5)))
                            if cmnt not in obj.Comment:
                                obj.Comment += cmnt

                            tup = clnBase, sub, tag, angle, axis, clnStock
                            allTuples.append(tup)
                        else:
                            if self.warnDisabledAxis(obj, axis, sub) is True:
                                pass  # Skip drill feature due to access issue
                            else:
                                PathLog.debug(str(sub) + ": No rotation used")
                                axis = 'X'
                                angle = 0.0
                                tag = base.Name + '_' + axis + str(angle).replace('.', '_')
                                stock = PathUtils.findParentJob(obj).Stock
                                tup = base, sub, tag, angle, axis, stock
                                allTuples.append(tup)
                        # Eif
                    # Eif
                    subCount += 1
                # Efor
            # Efor
            (Tags, Grps) = self.sortTuplesByIndex(allTuples, 2)  # return (TagList, GroupList)
            subList = []
            for o in range(0, len(Tags)):
                PathLog.debug('hTag: {}'.format(Tags[o]))
                subList = []
                for (base, sub, tag, angle, axis, stock) in Grps[o]:
                    subList.append(sub)
                pair = base, subList, angle, axis, stock
                baseSubsTuples.append(pair)
            # Efor

        for base, subs, angle, axis, stock in baseSubsTuples:
            for sub in subs:
                if self.isHoleEnabled(obj, base, sub):
                    pos = self.holePosition(obj, base, sub)
                    if pos:
                        # Default is treat selection as 'Face' shape
                        finDep = base.Shape.getElement(sub).BoundBox.ZMin
                        if base.Shape.getElement(sub).ShapeType == 'Edge':
                            msg = translate("Path", "Verify Final Depth of holes based on edges. {} depth is: {} mm".format(sub, round(finDep, 4))) + "  "
                            msg += translate("Path", "Always select the bottom edge of the hole when using an edge.")
                            PathLog.warning(msg)

                        # If user has not adjusted Final Depth value, attempt to determine from sub
                        if obj.OpFinalDepth.Value == obj.FinalDepth.Value:
                            PathLog.debug(translate('Path', 'Auto detecting Final Depth based on {}.'.format(sub)))
                            trgtDep = finDep
                        else:
                            trgtDep = max(obj.FinalDepth.Value, finDep)

                        holes.append({'x': pos.x, 'y': pos.y, 'r': self.holeDiameter(obj, base, sub),
                                     'angle': angle, 'axis': axis, 'trgtDep': trgtDep,
                                      'stkTop': stock.Shape.BoundBox.ZMax})

        if haveLocations(self, obj):
            for location in obj.Locations:
                # holes.append({'x': location.x, 'y': location.y, 'r': 0, 'angle': 0.0, 'axis': 'X', 'finDep': obj.FinalDepth.Value})
                trgtDep = obj.FinalDepth.Value
                holes.append({'x': location.x, 'y': location.y, 'r': 0,
                             'angle': 0.0, 'axis': 'X', 'trgtDep': trgtDep,
                              'stkTop': PathUtils.findParentJob(obj).stock.Shape.BoundBox.ZMax})

        # If all holes based upon edges, set post-operation Final Depth to highest edge height
        if obj.OpFinalDepth.Value == obj.FinalDepth.Value:
            if len(holes) == 1:
                PathLog.info(translate('Path', "Single-hole operation. Saving Final Depth determined from hole base."))
                obj.FinalDepth.Value = trgtDep

        if len(holes) > 0:
            self.circularHoleExecute(obj, holes)  # circularHoleExecute() located in PathDrilling.py

        self.useTempJobClones('Delete')  # Delete temp job clone group and contents
        self.guiMessage('title', None, show=True)  # Process GUI messages to user
        PathLog.debug("obj.Name: " + str(obj.Name))

    def circularHoleExecute(self, obj, holes):
        '''circularHoleExecute(obj, holes) ... implement processing of holes.
        holes is a list of dictionaries with 'x', 'y' and 'r' specified for each hole.
        Note that for Vertexes, non-circular Edges and Locations r=0.
        Must be overwritten by subclasses.'''
        pass # pylint: disable=unnecessary-pass

    def findAllHoles(self, obj):
        '''findAllHoles(obj) ... find all holes of all base models and assign as features.'''
        PathLog.track()
        if not self.getJob(obj):
            return
        features = []
        if 1 == len(self.model) and self.baseIsArchPanel(obj, self.model[0]):
            panel = self.model[0]
            holeshapes = panel.Proxy.getHoles(panel, transform=True)
            tooldiameter = obj.ToolController.Proxy.getTool(obj.ToolController).Diameter
            for holeNr, hole in enumerate(holeshapes):
                PathLog.debug('Entering new HoleShape')
                for wireNr, wire in enumerate(hole.Wires):
                    PathLog.debug('Entering new Wire')
                    for edgeNr, edge in enumerate(wire.Edges):
                        if PathUtils.isDrillable(panel, edge, tooldiameter):
                            PathLog.debug('Found drillable hole edges: {}'.format(edge))
                            features.append((panel, "%d.%d.%d" % (holeNr, wireNr, edgeNr)))
        else:
            for base in self.model:
                features.extend(self.findHoles(obj, base))
        obj.Base = features
        obj.Disabled = []

    def findHoles(self, obj, baseobject):
        '''findHoles(obj, baseobject) ... inspect baseobject and identify all features that resemble a straight cricular hole.'''
        shape = baseobject.Shape
        PathLog.track('obj: {} shape: {}'.format(obj, shape))
        holelist = []
        features = []
        # tooldiameter = obj.ToolController.Proxy.getTool(obj.ToolController).Diameter
        tooldiameter = None
        PathLog.debug('search for holes larger than tooldiameter: {}: '.format(tooldiameter))
        if DraftGeomUtils.isPlanar(shape):
            PathLog.debug("shape is planar")
            for i in range(len(shape.Edges)):
                candidateEdgeName = "Edge" + str(i + 1)
                e = shape.getElement(candidateEdgeName)
                if PathUtils.isDrillable(shape, e, tooldiameter):
                    PathLog.debug('edge candidate: {} (hash {})is drillable '.format(e, e.hashCode()))
                    x = e.Curve.Center.x
                    y = e.Curve.Center.y
                    diameter = e.BoundBox.XLength
                    holelist.append({'featureName': candidateEdgeName, 'feature': e, 'x': x, 'y': y, 'd': diameter, 'enabled': True})
                    features.append((baseobject, candidateEdgeName))
                    PathLog.debug("Found hole feature %s.%s" % (baseobject.Label, candidateEdgeName))
        else:
            PathLog.debug("shape is not planar")
            for i in range(len(shape.Faces)):
                candidateFaceName = "Face" + str(i + 1)
                f = shape.getElement(candidateFaceName)
                if PathUtils.isDrillable(shape, f, tooldiameter):
                    PathLog.debug('face candidate: {} is drillable '.format(f))
                    if hasattr(f.Surface, 'Center'):
                        x = f.Surface.Center.x
                        y = f.Surface.Center.y
                        diameter = f.BoundBox.XLength
                    else:
                        center = f.Edges[0].Curve.Center
                        x = center.x
                        y = center.y
                        diameter = f.Edges[0].Curve.Radius * 2
                    holelist.append({'featureName': candidateFaceName, 'feature': f, 'x': x, 'y': y, 'd': diameter, 'enabled': True})
                    features.append((baseobject, candidateFaceName))
                    PathLog.debug("Found hole feature %s.%s" % (baseobject.Label, candidateFaceName))

        PathLog.debug("holes found: {}".format(holelist))
        return features

    # Rotation-related methods
    def opDetermineRotationRadii(self, obj):
        '''opDetermineRotationRadii(obj)
            Determine rotational radii for 4th-axis rotations, for clearance/safe heights '''

        parentJob = PathUtils.findParentJob(obj)
        # bb = parentJob.Stock.Shape.BoundBox
        xlim = 0.0
        ylim = 0.0
        zlim = 0.0
        xRotRad = 0.01
        yRotRad = 0.01
        xRotRad = 0.01

        # Determine boundbox radius based upon xzy limits data
        if math.fabs(self.stockBB.ZMin) > math.fabs(self.stockBB.ZMax):
            zlim = self.stockBB.ZMin
        else:
            zlim = self.stockBB.ZMax

        if obj.EnableRotation != 'B(y)':
            # Rotation is around X-axis, cutter moves along same axis
            if math.fabs(self.stockBB.YMin) > math.fabs(self.stockBB.YMax):
                ylim = self.stockBB.YMin
            else:
                ylim = self.stockBB.YMax

        if obj.EnableRotation != 'A(x)':
            # Rotation is around Y-axis, cutter moves along same axis
            if math.fabs(self.stockBB.XMin) > math.fabs(self.stockBB.XMax):
                xlim = self.stockBB.XMin
            else:
                xlim = self.stockBB.XMax

        if ylim != 0.0:
            xRotRad = math.sqrt(ylim**2 + zlim**2)
        if xlim != 0.0:
            yRotRad = math.sqrt(xlim**2 + zlim**2)
        zRotRad = math.sqrt(xlim**2 + ylim**2)

        clrOfst = parentJob.SetupSheet.ClearanceHeightOffset.Value
        safOfst = parentJob.SetupSheet.SafeHeightOffset.Value

        return [(xRotRad, yRotRad, zRotRad), (clrOfst, safOfst)]

    def faceRotationAnalysis(self, obj, norm, surf):
        '''faceRotationAnalysis(obj, norm, surf)
            Determine X and Y independent rotation necessary to make normalAt = Z=1 (0,0,1) '''
        PathLog.track()

        praInfo = "faceRotationAnalysis(): "
        rtn = True
        orientation = 'X'
        angle = 500.0
        precision = 6

        for i in range(0, 13):
            if PathGeom.Tolerance * (i * 10) == 1.0:
                precision = i
                break

        def roundRoughValues(precision, val):
            # Convert VALxe-15 numbers to zero
            if PathGeom.isRoughly(0.0, val) is True:
                return 0.0
            # Convert VAL.99999999 to next integer
            elif math.fabs(val % 1) > 1.0 - PathGeom.Tolerance:
                return round(val)
            else:
                return round(val, precision)

        nX = roundRoughValues(precision, norm.x)
        nY = roundRoughValues(precision, norm.y)
        nZ = roundRoughValues(precision, norm.z)
        praInfo += "\n -normalAt(0,0): " + str(nX) + ", " + str(nY) + ", " + str(nZ)

        saX = roundRoughValues(precision, surf.x)
        saY = roundRoughValues(precision, surf.y)
        saZ = roundRoughValues(precision, surf.z)
        praInfo += "\n -Surface.Axis: " + str(saX) + ", " + str(saY) + ", " + str(saZ)

        # Determine rotation needed and current orientation
        if saX == 0.0:
            if saY == 0.0:
                orientation = "Z"
                if saZ == 1.0:
                    angle = 0.0
                elif saZ == -1.0:
                    angle = -180.0
                else:
                    praInfo += "_else_X" + str(saZ)
            elif saY == 1.0:
                orientation = "Y"
                angle = 90.0
            elif saY == -1.0:
                orientation = "Y"
                angle = -90.0
            else:
                if saZ != 0.0:
                    angle = math.degrees(math.atan(saY / saZ))
                    orientation = "Y"
        elif saY == 0.0:
            if saZ == 0.0:
                orientation = "X"
                if saX == 1.0:
                    angle = -90.0
                elif saX == -1.0:
                    angle = 90.0
                else:
                    praInfo += "_else_X" + str(saX)
            else:
                orientation = "X"
                ratio = saX / saZ
                angle = math.degrees(math.atan(ratio))
                if ratio < 0.0:
                    praInfo += " NEG-ratio"
                    # angle -= 90
                else:
                    praInfo += " POS-ratio"
                    angle = -1 * angle
                    if saX < 0.0:
                        angle = angle + 180.0
        elif saZ == 0.0:
            if saY != 0.0:
                angle = math.degrees(math.atan(saX / saY))
                orientation = "Y"

        if saX + nX == 0.0:
            angle = -1 * angle
        if saY + nY == 0.0:
            angle = -1 * angle
        if saZ + nZ == 0.0:
            angle = -1 * angle

        if saY == -1.0 or saY == 1.0:
            if nX != 0.0:
                angle = -1 * angle

        # Enforce enabled rotation in settings
        praInfo += "\n -Initial orientation:  {}".format(orientation)
        if orientation == 'Y':
            axis = 'X'
            if obj.EnableRotation == 'B(y)':  # Required axis disabled
                if angle == 180.0 or angle == -180.0:
                    axis = 'Y'
                else:
                    rtn = False
        elif orientation == 'X':
            axis = 'Y'
            if obj.EnableRotation == 'A(x)':  # Required axis disabled
                if angle == 180.0 or angle == -180.0:
                    axis = 'X'
                else:
                    rtn = False
        elif orientation == 'Z':
            axis = 'X'

        if math.fabs(angle) == 0.0:
            angle = 0.0
            rtn = False

        if angle == 500.0:
            angle = 0.0
            rtn = False

        if rtn is False:
            if orientation == 'Z' and angle == 0.0 and obj.ReverseDirection is True:
                if obj.EnableRotation == 'B(y)':
                    axis = 'Y'
                rtn = True

        if rtn is True:
            self.rotateFlag = True # pylint: disable=attribute-defined-outside-init
            # rtn = True
            if obj.ReverseDirection is True:
                if angle < 180.0:
                    angle = angle + 180.0
                else:
                    angle = angle - 180.0
            angle = round(angle, precision)

        praInfo += "\n -Rotation analysis:  angle: " + str(angle) + ",   axis: " + str(axis)
        if rtn is True:
            praInfo += "\n - ... rotation triggered"
        else:
            praInfo += "\n - ... NO rotation triggered"

        PathLog.debug("\n" + str(praInfo))

        return (rtn, angle, axis, praInfo)

    def guiMessage(self, title, msg, show=False):
        '''guiMessage(title, msg, show=False)
            Handle op related GUI messages to user'''
        if msg is not None:
            self.guiMsgs.append((title, msg))
        if show is True:
            if len(self.guiMsgs) > 0:
                if FreeCAD.GuiUp:
                    from PySide.QtGui import QMessageBox
                    for entry in self.guiMsgs:
                        (title, msg) = entry
                        QMessageBox.warning(None, title, msg)
                    self.guiMsgs = []  # pylint: disable=attribute-defined-outside-init
                    return True
                else:
                    for entry in self.guiMsgs:
                        (title, msg) = entry
                        PathLog.warning("{}:: {}".format(title, msg))
                    self.guiMsgs = []  # pylint: disable=attribute-defined-outside-init
                    return True
        return False

    def visualAxis(self):
        '''visualAxis()
            Create visual X & Y axis for use in orientation of rotational operations
            Triggered only for PathLog.debug'''

        if not FreeCAD.ActiveDocument.getObject('xAxCyl'):
            xAx = 'xAxCyl'
            yAx = 'yAxCyl'
            FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup", "visualAxis")
            if FreeCAD.GuiUp:
                FreeCADGui.ActiveDocument.getObject('visualAxis').Visibility = False
            vaGrp = FreeCAD.ActiveDocument.getObject("visualAxis")

            FreeCAD.ActiveDocument.addObject("Part::Cylinder", xAx)
            cyl = FreeCAD.ActiveDocument.getObject(xAx)
            cyl.Label = xAx
            cyl.Radius = self.xRotRad
            cyl.Height = 0.01
            cyl.Placement = FreeCAD.Placement(FreeCAD.Vector(0, 0, 0), FreeCAD.Rotation(FreeCAD.Vector(0, 1, 0), 90))
            cyl.purgeTouched()
            if FreeCAD.GuiUp:
                cylGui = FreeCADGui.ActiveDocument.getObject(xAx)
                cylGui.ShapeColor = (0.667, 0.000, 0.000)
                cylGui.Transparency = 85
                cylGui.Visibility = False
            vaGrp.addObject(cyl)

            FreeCAD.ActiveDocument.addObject("Part::Cylinder", yAx)
            cyl = FreeCAD.ActiveDocument.getObject(yAx)
            cyl.Label = yAx
            cyl.Radius = self.yRotRad
            cyl.Height = 0.01
            cyl.Placement = FreeCAD.Placement(FreeCAD.Vector(0, 0, 0), FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), 90))
            cyl.purgeTouched()
            if FreeCAD.GuiUp:
                cylGui = FreeCADGui.ActiveDocument.getObject(yAx)
                cylGui.ShapeColor = (0.000, 0.667, 0.000)
                cylGui.Transparency = 85
                cylGui.Visibility = False
            vaGrp.addObject(cyl)

    def useTempJobClones(self, cloneName):
        '''useTempJobClones(cloneName)
            Manage use of temporary model clones for rotational operation calculations.
            Clones are stored in 'rotJobClones' group.'''
        if FreeCAD.ActiveDocument.getObject('rotJobClones'):
            if cloneName == 'Start':
                if PathLog.getLevel(PathLog.thisModule()) < 4:
                    for cln in FreeCAD.ActiveDocument.getObject('rotJobClones').Group:
                        FreeCAD.ActiveDocument.removeObject(cln.Name)
            elif cloneName == 'Delete':
                if PathLog.getLevel(PathLog.thisModule()) < 4:
                    for cln in FreeCAD.ActiveDocument.getObject('rotJobClones').Group:
                        FreeCAD.ActiveDocument.removeObject(cln.Name)
                    FreeCAD.ActiveDocument.removeObject('rotJobClones')
        else:
            FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup", "rotJobClones")
            if FreeCAD.GuiUp:
                FreeCADGui.ActiveDocument.getObject('rotJobClones').Visibility = False

        if cloneName != 'Start' and cloneName != 'Delete':
            FreeCAD.ActiveDocument.getObject('rotJobClones').addObject(FreeCAD.ActiveDocument.getObject(cloneName))
            if FreeCAD.GuiUp:
                FreeCADGui.ActiveDocument.getObject(cloneName).Visibility = False

    def cloneBaseAndStock(self, obj, base, angle, axis, subCount):
        '''cloneBaseAndStock(obj, base, angle, axis, subCount)
            Method called to create a temporary clone of the base and parent Job stock.
            Clones are destroyed after usage for calculations related to rotational operations.'''
        # Create a temporary clone and stock of model for rotational use.
        rndAng = round(angle, 8)
        if rndAng < 0.0:  # neg sign is converted to underscore in clone name creation.
            tag = axis + '_' + axis + '_' + str(math.fabs(rndAng)).replace('.', '_')
        else:
            tag = axis + str(rndAng).replace('.', '_')
        clnNm = obj.Name + '_base_' + '_' + str(subCount) + '_' + tag
        stckClnNm = obj.Name + '_stock_' + '_' + str(subCount) + '_' + tag
        if clnNm not in self.cloneNames:
            self.cloneNames.append(clnNm)
            self.cloneNames.append(stckClnNm)
            if FreeCAD.ActiveDocument.getObject(clnNm):
                FreeCAD.ActiveDocument.getObject(clnNm).Shape = base.Shape
            else:
                FreeCAD.ActiveDocument.addObject('Part::Feature', clnNm).Shape = base.Shape
                self.useTempJobClones(clnNm)
            if FreeCAD.ActiveDocument.getObject(stckClnNm):
                FreeCAD.ActiveDocument.getObject(stckClnNm).Shape = PathUtils.findParentJob(obj).Stock.Shape
            else:
                FreeCAD.ActiveDocument.addObject('Part::Feature', stckClnNm).Shape = PathUtils.findParentJob(obj).Stock.Shape
                self.useTempJobClones(stckClnNm)
            if FreeCAD.GuiUp:
                FreeCADGui.ActiveDocument.getObject(stckClnNm).Transparency = 90
                FreeCADGui.ActiveDocument.getObject(clnNm).ShapeColor = (1.000, 0.667, 0.000)
        clnBase = FreeCAD.ActiveDocument.getObject(clnNm)
        clnStock = FreeCAD.ActiveDocument.getObject(stckClnNm)
        tag = base.Name + '_' + tag
        return (clnBase, clnStock, tag)

    def getFaceNormAndSurf(self, face):
        '''getFaceNormAndSurf(face)
            Return face.normalAt(0,0) or face.normal(0,0) and face.Surface.Axis vectors
        '''
        norm = FreeCAD.Vector(0.0, 0.0, 0.0)
        surf = FreeCAD.Vector(0.0, 0.0, 0.0)

        if face.ShapeType == 'Edge':
            edgToFace = Part.Face(Part.Wire(Part.__sortEdges__([face])))
            face = edgToFace

        if hasattr(face, 'normalAt'):
            n = face.normalAt(0, 0)
        elif hasattr(face, 'normal'):
            n = face.normal(0, 0)
        if hasattr(face.Surface, 'Axis'):
            s = face.Surface.Axis
        else:
            s = n

        norm.x = n.x
        norm.y = n.y
        norm.z = n.z
        surf.x = s.x
        surf.y = s.y
        surf.z = s.z
        return (norm, surf)

    def applyRotationalAnalysis(self, obj, base, angle, axis, subCount):
        '''applyRotationalAnalysis(obj, base, angle, axis, subCount)
            Create temp clone and stock and apply rotation to both.
            Return new rotated clones
        '''
        if axis == 'X':
            vect = FreeCAD.Vector(1, 0, 0)
        elif axis == 'Y':
            vect = FreeCAD.Vector(0, 1, 0)

        if obj.InverseAngle is True:
            angle = -1 * angle
            if math.fabs(angle) == 0.0:
                angle = 0.0

        # Create a temporary clone of model for rotational use.
        (clnBase, clnStock, tag) = self.cloneBaseAndStock(obj, base, angle, axis, subCount)

        # Rotate base to such that Surface.Axis of pocket bottom is Z=1
        clnBase = Draft.rotate(clnBase, angle, center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect, copy=False)
        clnStock = Draft.rotate(clnStock, angle, center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect, copy=False)

        clnBase.purgeTouched()
        clnStock.purgeTouched()
        return (clnBase, angle, clnStock, tag)

    def applyInverseAngle(self, obj, clnBase, clnStock, axis, angle):
        '''applyInverseAngle(obj, clnBase, clnStock, axis, angle)
            Apply rotations to incoming base and stock objects.'''
        if axis == 'X':
            vect = FreeCAD.Vector(1, 0, 0)
        elif axis == 'Y':
            vect = FreeCAD.Vector(0, 1, 0)
        # Rotate base to inverse of original angle
        clnBase = Draft.rotate(clnBase, (-2 * angle), center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect, copy=False)
        clnStock = Draft.rotate(clnStock, (-2 * angle), center=FreeCAD.Vector(0.0, 0.0, 0.0), axis=vect, copy=False)
        clnBase.purgeTouched()
        clnStock.purgeTouched()
        # Update property and angle values
        obj.InverseAngle = True
        obj.AttemptInverseAngle = False
        angle = -1 * angle
        return (clnBase, clnStock, angle)

    def calculateStartFinalDepths(self, obj, shape, stock):
        '''calculateStartFinalDepths(obj, shape, stock)
            Calculate correct start and final depths for the shape(face) object provided.'''
        finDep = max(obj.FinalDepth.Value, shape.BoundBox.ZMin)
        stockTop = stock.Shape.BoundBox.ZMax
        if obj.EnableRotation == 'Off':
            strDep = obj.StartDepth.Value
            if strDep <= finDep:
                strDep = stockTop
        else:
            strDep = min(obj.StartDepth.Value, stockTop)
            if strDep <= finDep:
                strDep = stockTop
                msg = translate('Path', "Start depth <= face depth.\nIncreased to stock top.")
                PathLog.error(msg)
        return (strDep, finDep)

    def sortTuplesByIndex(self, TupleList, tagIdx):
        '''sortTuplesByIndex(TupleList, tagIdx)
            sort list of tuples based on tag index provided
            return (TagList, GroupList)
        '''
        # Separate elements, regroup by orientation (axis_angle combination)
        TagList = ['X34.2']
        GroupList = [[(2.3, 3.4, 'X')]]
        for tup in TupleList:
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

    def warnDisabledAxis(self, obj, axis, sub=''):
        '''warnDisabledAxis(self, obj, axis)
            Provide user feedback if required axis is disabled'''
        if axis == 'X' and obj.EnableRotation == 'B(y)':
            msg = translate('Path', "{}:: {} is inaccessible.".format(obj.Name, sub)) + "  "
            msg += translate('Path', "Selected feature(s) require 'Enable Rotation: A(x)' for access.")
            PathLog.warning(msg)
            return True
        elif axis == 'Y' and obj.EnableRotation == 'A(x)':
            msg = translate('Path', "{}:: {} is inaccessible.".format(obj.Name, sub)) + "  "
            msg += translate('Path', "Selected feature(s) require 'Enable Rotation: B(y)' for access.")
            PathLog.warning(msg)
            return True
        else:
            return False
