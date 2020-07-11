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

__title__ = "Profile Operation: OpenEdge Class"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Open edge support class for Profile operation."
__contributors__ = ""

# Standard
import math
# Third-party
from PySide import QtCore
# FreeCAD
import FreeCAD
import PathScripts.PathLog as PathLog

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Part = LazyLoader('Part', globals(), 'Part')
Path = LazyLoader('Path', globals(), 'Path')


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule()


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class OpenEdge:

    def __init__(self, obj, parent, baseClass):
        '''__init__(obj, baseClass) ... Constructor for Profile operation class.'''
        PathLog.debug('SlotOperation.__init__()')
        # Required operation instance attributes
        self.baseClass = baseClass
        self.parent = parent
        self.obj = obj
        self.propertyEditorModes = None
        self.propEnumerations = None
        self.isDebug = baseClass.isDebug  # Debug attribute
        self.showDebugObjects = baseClass.showDebugObjects  # Debug attribute

    # Public method
    def getOpenEdgeShape(self, passOffsets, base, origWire, flatWire):
        openEdges = list()
        for po in passOffsets:
            self.ofstRadius = po

            cutShp = self._getCutAreaCrossSection(base, origWire, flatWire)
            if cutShp:
                cutWireObjs = self._extractPathWire(base, flatWire, cutShp)

            if cutWireObjs:
                for cW in cutWireObjs:
                    openEdges.append(cW)
            else:
                PathLog.error(self.parent.inaccessibleMsg)

        return openEdges

    # Private methods
    def _getCutAreaCrossSection(self, base, origWire, flatWire):
        PathLog.debug('_getCutAreaCrossSection()')
        FCAD = FreeCAD.ActiveDocument
        tolerance = self.parent.job.GeometryTolerance.Value
        toolDiam = 2 * self.baseClass.radius
        minBfr = toolDiam * 1.25
        bbBfr = (self.parent.ofstRadius * 2) * 1.25
        if bbBfr < minBfr:
            bbBfr = minBfr
        fwBB = flatWire.BoundBox
        wBB = origWire.BoundBox
        minArea = (self.parent.ofstRadius - tolerance)**2 * math.pi

        useWire = origWire.Wires[0]
        numOrigEdges = len(useWire.Edges)
        sdv = wBB.ZMax
        fdv = self.obj.FinalDepth.Value
        extLenFwd = sdv - fdv
        if extLenFwd <= 0.0:
            msg = translate('PathProfile',
                            'For open edges, verify Final Depth for this operation.')
            FreeCAD.Console.PrintError(msg + '\n')
            # return False
            extLenFwd = 0.1
        WIRE = flatWire.Wires[0]
        numEdges = len(WIRE.Edges)

        # Identify first/last edges and first/last vertex on wire
        begE = WIRE.Edges[0]  # beginning edge
        endE = WIRE.Edges[numEdges - 1]  # ending edge
        blen = begE.Length
        elen = endE.Length
        Vb = begE.Vertexes[0]  # first vertex of wire
        Ve = endE.Vertexes[1]  # last vertex of wire
        pb = FreeCAD.Vector(Vb.X, Vb.Y, fdv)
        pe = FreeCAD.Vector(Ve.X, Ve.Y, fdv)

        # Identify endpoints connecting circle center and diameter
        vectDist = pe.sub(pb)
        diam = vectDist.Length
        cntr = vectDist.multiply(0.5).add(pb)
        R = diam / 2

        # Obtain beginning point perpendicular points
        if blen > 0.1:
            bcp = begE.valueAt(begE.getParameterByLength(0.1))  # point returned 0.1 mm along edge
        else:
            bcp = FreeCAD.Vector(begE.Vertexes[1].X, begE.Vertexes[1].Y, fdv)
        if elen > 0.1:
            ecp = endE.valueAt(endE.getParameterByLength(elen - 0.1))  # point returned 0.1 mm along edge
        else:
            ecp = FreeCAD.Vector(endE.Vertexes[1].X, endE.Vertexes[1].Y, fdv)

        # Create intersection tags for determining which side of wire to cut
        (begInt, begExt, iTAG, eTAG) = self._makeIntersectionTags(useWire, numOrigEdges, fdv)
        if not begInt or not begExt:
            return False
        self.iTAG = iTAG
        self.eTAG = eTAG

        # Create extended wire boundbox, and extrude
        extBndbox = self._makeExtendedBoundBox(wBB, bbBfr, fdv)
        extBndboxEXT = extBndbox.extrude(FreeCAD.Vector(0, 0, extLenFwd))

        # Cut model(selected edges) from extended edges boundbox
        cutArea = extBndboxEXT.cut(base.Shape)
        self.baseClass._addDebugObject(cutArea, 'CutArea')

        # Get top and bottom faces of cut area (CA), and combine faces when necessary
        topFc = list()
        botFc = list()
        bbZMax = cutArea.BoundBox.ZMax
        bbZMin = cutArea.BoundBox.ZMin
        for f in range(0, len(cutArea.Faces)):
            FcBB = cutArea.Faces[f].BoundBox
            if abs(FcBB.ZMax - bbZMax) < tolerance and abs(FcBB.ZMin - bbZMax) < tolerance:
                topFc.append(f)
            if abs(FcBB.ZMax - bbZMin) < tolerance and abs(FcBB.ZMin - bbZMin) < tolerance:
                botFc.append(f)
        if len(topFc) == 0:
            PathLog.error('Failed to identify top faces of cut area.')
            return False
        topComp = Part.makeCompound([cutArea.Faces[f] for f in topFc])
        topComp.translate(FreeCAD.Vector(0, 0, fdv - topComp.BoundBox.ZMin))  # Translate face to final depth
        if len(botFc) > 1:
            # PathLog.debug('len(botFc) > 1')
            bndboxFace = Part.Face(extBndbox.Wires[0])
            tmpFace = Part.Face(extBndbox.Wires[0])
            for f in botFc:
                Q = tmpFace.cut(cutArea.Faces[f])
                tmpFace = Q
            botComp = bndboxFace.cut(tmpFace)
        else:
            botComp = Part.makeCompound([cutArea.Faces[f] for f in botFc])  # Part.makeCompound([CA.Shape.Faces[f] for f in botFc])
        botComp.translate(FreeCAD.Vector(0, 0, fdv - botComp.BoundBox.ZMin))  # Translate face to final depth

        # Make common of the two
        comFC = topComp.common(botComp)

        # Determine with which set of intersection tags the model intersects
        (cmnIntArea, cmnExtArea) = self._checkTagIntersection(iTAG, eTAG, 'QRY', comFC)
        if cmnExtArea > cmnIntArea:
            PathLog.debug('Cutting on Ext side.')
            self.cutSide = 'E'
            self.cutSideTags = eTAG
            tagCOM = begExt.CenterOfMass
        else:
            PathLog.debug('Cutting on Int side.')
            self.cutSide = 'I'
            self.cutSideTags = iTAG
            tagCOM = begInt.CenterOfMass

        # Make two beginning style(oriented) 'L' shape stops
        begStop = self._makeStop('BEG', bcp, pb, 'BegStop')
        altBegStop = self._makeStop('END', bcp, pb, 'BegStop')

        # Identify to which style 'L' stop the beginning intersection tag is closest,
        # and create partner end 'L' stop geometry, and save for application later
        lenBS_extETag = begStop.CenterOfMass.sub(tagCOM).Length
        lenABS_extETag = altBegStop.CenterOfMass.sub(tagCOM).Length
        if lenBS_extETag < lenABS_extETag:
            endStop = self._makeStop('END', ecp, pe, 'EndStop')
            pathStops = Part.makeCompound([begStop, endStop])
        else:
            altEndStop = self._makeStop('BEG', ecp, pe, 'EndStop')
            pathStops = Part.makeCompound([altBegStop, altEndStop])
        pathStops.translate(FreeCAD.Vector(0, 0, fdv - pathStops.BoundBox.ZMin))

        # Identify closed wire in cross-section that corresponds to user-selected edge(s)
        workShp = comFC
        fcShp = workShp
        wire = origWire
        WS = workShp.Wires
        lenWS = len(WS)
        if lenWS < 3:
            wi = 0
        else:
            wi = None
            for wvt in wire.Vertexes:
                for w in range(0, lenWS):
                    twr = WS[w]
                    for v in range(0, len(twr.Vertexes)):
                        V = twr.Vertexes[v]
                        if abs(V.X - wvt.X) < tolerance:
                            if abs(V.Y - wvt.Y) < tolerance:
                                # Same vertex found.  This wire to be used for offset
                                wi = w
                                break
            # Efor

            if wi is None:
                PathLog.error('The cut area cross-section wire does not coincide with selected edge. Wires[] index is None.')
                return False
            else:
                PathLog.debug('Cross-section Wires[] index is {}.'.format(wi))

            nWire = Part.Wire(Part.__sortEdges__(workShp.Wires[wi].Edges))
            fcShp = Part.Face(nWire)
            fcShp.translate(FreeCAD.Vector(0, 0, fdv - workShp.BoundBox.ZMin))
        # Eif

        # verify that wire chosen is not inside the physical model
        if wi > 0:  # and isInterior is False:
            PathLog.debug('Multiple wires in cut area. First choice is not 0. Testing.')
            testArea = fcShp.cut(base.Shape)

            isReady = self._checkTagIntersection(iTAG, eTAG, self.cutSide, testArea)
            PathLog.debug('isReady {}.'.format(isReady))

            if isReady is False:
                PathLog.debug('Using wire index {}.'.format(wi - 1))
                pWire = Part.Wire(Part.__sortEdges__(workShp.Wires[wi - 1].Edges))
                pfcShp = Part.Face(pWire)
                pfcShp.translate(FreeCAD.Vector(0, 0, fdv - workShp.BoundBox.ZMin))
                workShp = pfcShp.cut(fcShp)

            if testArea.Area < minArea:
                PathLog.debug('offset area is less than minArea of {}.'.format(minArea))
                PathLog.debug('Using wire index {}.'.format(wi - 1))
                pWire = Part.Wire(Part.__sortEdges__(workShp.Wires[wi - 1].Edges))
                pfcShp = Part.Face(pWire)
                pfcShp.translate(FreeCAD.Vector(0, 0, fdv - workShp.BoundBox.ZMin))
                workShp = pfcShp.cut(fcShp)
        # Eif

        # Add path stops at ends of wire
        cutShp = workShp.cut(pathStops)
        self.baseClass._addDebugObject(cutShp, 'CutShape')

        return cutShp

    def _checkTagIntersection(self, iTAG, eTAG, cutSide, tstObj):
        PathLog.debug('_checkTagIntersection()')
        # Identify intersection of Common area and Interior Tags
        intCmn = tstObj.common(iTAG)

        # Identify intersection of Common area and Exterior Tags
        extCmn = tstObj.common(eTAG)

        # Calculate common intersection (solid model side, or the non-cut side) area with tags, to determine physical cut side
        cmnIntArea = intCmn.Area
        cmnExtArea = extCmn.Area
        if cutSide == 'QRY':
            return (cmnIntArea, cmnExtArea)

        if cmnExtArea > cmnIntArea:
            PathLog.debug('Cutting on Ext side.')
            if cutSide == 'E':
                return True
        else:
            PathLog.debug('Cutting on Int side.')
            if cutSide == 'I':
                return True
        return False

    def _extractPathWire(self, base, flatWire, cutShp):
        PathLog.debug('_extractPathWire()')

        subLoops = list()
        rtnWIRES = list()
        osWrIdxs = list()
        subDistFactor = 1.0  # Raise to include sub wires at greater distance from original
        fdv = self.obj.FinalDepth.Value
        wire = flatWire
        lstVrtIdx = len(wire.Vertexes) - 1
        lstVrt = wire.Vertexes[lstVrtIdx]
        frstVrt = wire.Vertexes[0]
        cent0 = FreeCAD.Vector(frstVrt.X, frstVrt.Y, fdv)
        cent1 = FreeCAD.Vector(lstVrt.X, lstVrt.Y, fdv)

        # Calculate offset shape, containing cut region
        ofstShp = self._extractFaceOffset(cutShp, False)

        # CHECK for ZERO area of offset shape
        try:
            osArea = ofstShp.Area
        except Exception as ee:
            PathLog.error('No area to offset shape returned.\n{}'.format(ee))
            return False

        self.baseClass._addDebugObject(ofstShp, 'OffsetShape')

        numOSWires = len(ofstShp.Wires)
        for w in range(0, numOSWires):
            osWrIdxs.append(w)

        # Identify two vertexes for dividing offset loop
        NEAR0 = self._findNearestVertex(ofstShp,  cent0)
        min0i = 0
        min0 = NEAR0[0][4]
        for n in range(0, len(NEAR0)):
            N = NEAR0[n]
            if N[4] < min0:
                min0 = N[4]
                min0i = n
        (w0, vi0, pnt0, vrt0, d0) = NEAR0[0]  # min0i
        near0Shp = Part.makeLine(cent0, pnt0)
        self.baseClass._addDebugObject(near0Shp, 'Near0')

        NEAR1 = self._findNearestVertex(ofstShp,  cent1)
        min1i = 0
        min1 = NEAR1[0][4]
        for n in range(0, len(NEAR1)):
            N = NEAR1[n]
            if N[4] < min1:
                min1 = N[4]
                min1i = n
        (w1, vi1, pnt1, vrt1, d1) = NEAR1[0]  # min1i
        near1Shp = Part.makeLine(cent1, pnt1)
        self.baseClass._addDebugObject(near1Shp, 'Near1')

        if w0 != w1:
            PathLog.warning('Offset wire endpoint indexes are not equal - w0, w1: {}, {}'.format(w0, w1))

        if self.isDebug and False:
            PathLog.debug('min0i is {}.'.format(min0i))
            PathLog.debug('min1i is {}.'.format(min1i))
            PathLog.debug('NEAR0[{}] is {}.'.format(w0, NEAR0[w0]))
            PathLog.debug('NEAR1[{}] is {}.'.format(w1, NEAR1[w1]))
            PathLog.debug('NEAR0 is {}.'.format(NEAR0))
            PathLog.debug('NEAR1 is {}.'.format(NEAR1))

        mainWire = ofstShp.Wires[w0]

        # Check for additional closed loops in offset wire by checking distance to iTAG or eTAG elements
        if numOSWires > 1:
            # check all wires for proximity(children) to intersection tags
            tagsComList = list()
            for T in self.cutSideTags.Faces:
                tcom = T.CenterOfMass
                tv = FreeCAD.Vector(tcom.x, tcom.y, 0.0)
                tagsComList.append(tv)
            subDist = self.parent.ofstRadius * subDistFactor
            for w in osWrIdxs:
                if w != w0:
                    cutSub = False
                    VTXS = ofstShp.Wires[w].Vertexes
                    for V in VTXS:
                        v = FreeCAD.Vector(V.X, V.Y, 0.0)
                        for t in tagsComList:
                            if t.sub(v).Length < subDist:
                                cutSub = True
                                break
                        if cutSub is True:
                            break
                    if cutSub is True:
                        sub = Part.Wire(Part.__sortEdges__(ofstShp.Wires[w].Edges))
                        subLoops.append(sub)
                # Eif

        # Break offset loop into two wires - one of which is the desired profile path wire.
        try:
            (edgeIdxs0, edgeIdxs1) = self._separateWireAtVertexes(mainWire, mainWire.Vertexes[vi0], mainWire.Vertexes[vi1])
        except Exception as ee:
            PathLog.error('Failed to identify offset edge.\n{}'.format(ee))
            return False
        edgs0 = list()
        edgs1 = list()
        for e in edgeIdxs0:
            edgs0.append(mainWire.Edges[e])
        for e in edgeIdxs1:
            edgs1.append(mainWire.Edges[e])
        part0 = Part.Wire(Part.__sortEdges__(edgs0))
        part1 = Part.Wire(Part.__sortEdges__(edgs1))

        # Determine which part is nearest original edge(s)
        distToPart0 = self._distMidToMid(wire.Wires[0], part0.Wires[0])
        distToPart1 = self._distMidToMid(wire.Wires[0], part1.Wires[0])
        if distToPart0 < distToPart1:
            rtnWIRES.append(part0)
        else:
            rtnWIRES.append(part1)
        rtnWIRES.extend(subLoops)

        return rtnWIRES

    def _extractFaceOffset(self, fcShape, isHole):
        '''_extractFaceOffset(fcShape, isHole) ... internal function.
            Original _buildPathArea() version copied from PathAreaOp.py module.  This version is modified.
            Adjustments made based on notes by @sliptonic - https://github.com/sliptonic/FreeCAD/wiki/PathArea-notes.'''
        PathLog.debug('_extractFaceOffset()')

        areaParams = {}
        offset = self.parent.ofstRadius

        if isHole is False:
            offset = 0 - offset

        areaParams['Offset'] = offset
        areaParams['Fill'] = 1
        areaParams['Coplanar'] = 0
        areaParams['SectionCount'] = 1  # -1 = full sections
        areaParams['Reorient'] = True
        areaParams['OpenMode'] = 0
        areaParams['MaxArcPoints'] = 400  # 400
        areaParams['Project'] = True
        # areaParams['JoinType'] = 1

        area = Path.Area()  # Create instance of Area() class object
        area.setPlane(self.parent.workPlaneRef)  # Set working plane
        area.add(fcShape)
        area.setParams(**areaParams)  # set parameters

        return area.getShape()

    def _findNearestVertex(self, shape, point):
        PathLog.debug('_findNearestVertex()')
        PT = FreeCAD.Vector(point.x, point.y, 0.0)

        def sortDist(tup):
            return tup[4]

        PNTS = list()
        for w in range(0, len(shape.Wires)):
            WR = shape.Wires[w]
            V = WR.Vertexes[0]
            P = FreeCAD.Vector(V.X, V.Y, 0.0)
            dist = P.sub(PT).Length
            vi = 0
            pnt = P
            vrt = V
            for v in range(0, len(WR.Vertexes)):
                V = WR.Vertexes[v]
                P = FreeCAD.Vector(V.X, V.Y, 0.0)
                d = P.sub(PT).Length
                if d < dist:
                    dist = d
                    vi = v
                    pnt = P
                    vrt = V
            PNTS.append((w, vi, pnt, vrt, dist))
        PNTS.sort(key=sortDist)
        return PNTS

    def _separateWireAtVertexes(self, wire, VV1, VV2):
        PathLog.debug('_separateWireAtVertexes()')
        tolerance = self.parent.job.GeometryTolerance.Value
        grps = [[], []]
        wireIdxs = [[], []]
        V1 = FreeCAD.Vector(VV1.X, VV1.Y, VV1.Z)
        V2 = FreeCAD.Vector(VV2.X, VV2.Y, VV2.Z)

        lenE = len(wire.Edges)
        FLGS = list()
        for e in range(0, lenE):
            FLGS.append(0)

        chk4 = False
        for e in range(0, lenE):
            v = 0
            E = wire.Edges[e]
            fv0 = FreeCAD.Vector(E.Vertexes[0].X, E.Vertexes[0].Y, E.Vertexes[0].Z)
            fv1 = FreeCAD.Vector(E.Vertexes[1].X, E.Vertexes[1].Y, E.Vertexes[1].Z)

            if fv0.sub(V1).Length < tolerance:
                v = 1
                if fv1.sub(V2).Length < tolerance:
                    v += 3
                    chk4 = True
            elif fv1.sub(V1).Length < tolerance:
                v = 1
                if fv0.sub(V2).Length < tolerance:
                    v += 3
                    chk4 = True

            if fv0.sub(V2).Length < tolerance:
                v = 3
                if fv1.sub(V1).Length < tolerance:
                    v += 1
                    chk4 = True
            elif fv1.sub(V2).Length < tolerance:
                v = 3
                if fv0.sub(V1).Length < tolerance:
                    v += 1
                    chk4 = True
            FLGS[e] += v
        # Efor

        # PathLog.debug('_separateWireAtVertexes() FLGS: {}'.format(FLGS))

        PRE = list()
        POST = list()
        IDXS = list()
        IDX1 = list()
        IDX2 = list()
        for e in range(0, lenE):
            f = FLGS[e]
            PRE.append(f)
            POST.append(f)
            IDXS.append(e)
            IDX1.append(e)
            IDX2.append(e)

        PRE.extend(FLGS)
        PRE.extend(POST)
        lenFULL = len(PRE)
        IDXS.extend(IDX1)
        IDXS.extend(IDX2)

        if chk4 is True:
            # find beginning 1 edge
            begIdx = None
            begFlg = False
            for e in range(0, lenFULL):
                f = PRE[e]
                i = IDXS[e]
                if f == 4:
                    begIdx = e
                    grps[0].append(f)
                    wireIdxs[0].append(i)
                    break
            # find first 3 edge
            endIdx = None
            for e in range(begIdx + 1, lenE + begIdx):
                f = PRE[e]
                i = IDXS[e]
                grps[1].append(f)
                wireIdxs[1].append(i)
        else:
            # find beginning 1 edge
            begIdx = None
            begFlg = False
            for e in range(0, lenFULL):
                f = PRE[e]
                if f == 1:
                    if not begFlg:
                        begFlg = True
                    else:
                        begIdx = e
                        break
            # find first 3 edge and group all first wire edges
            endIdx = None
            for e in range(begIdx, lenE + begIdx):
                f = PRE[e]
                i = IDXS[e]
                if f == 3:
                    grps[0].append(f)
                    wireIdxs[0].append(i)
                    endIdx = e
                    break
                else:
                    grps[0].append(f)
                    wireIdxs[0].append(i)
            # Collect remaining edges
            for e in range(endIdx + 1, lenFULL):
                f = PRE[e]
                i = IDXS[e]
                if f == 1:
                    grps[1].append(f)
                    wireIdxs[1].append(i)
                    break
                else:
                    wireIdxs[1].append(i)
                    grps[1].append(f)
            # Efor
        # Eif

        # Remove `and False` when debugging open edges, as needed
        if self.isDebug and False:
            PathLog.debug('grps[0]: {}'.format(grps[0]))
            PathLog.debug('grps[1]: {}'.format(grps[1]))
            PathLog.debug('wireIdxs[0]: {}'.format(wireIdxs[0]))
            PathLog.debug('wireIdxs[1]: {}'.format(wireIdxs[1]))
            PathLog.debug('PRE: {}'.format(PRE))
            PathLog.debug('IDXS: {}'.format(IDXS))

        return (wireIdxs[0], wireIdxs[1])

    def _makeExtendedBoundBox(self, wBB, bbBfr, zDep):
        PathLog.debug('_makeExtendedBoundBox()')
        p1 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMin - bbBfr, zDep)
        p2 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMin - bbBfr, zDep)
        p3 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMax + bbBfr, zDep)
        p4 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMax + bbBfr, zDep)

        L1 = Part.makeLine(p1, p2)
        L2 = Part.makeLine(p2, p3)
        L3 = Part.makeLine(p3, p4)
        L4 = Part.makeLine(p4, p1)

        return Part.Face(Part.Wire([L1, L2, L3, L4]))

    def _makeIntersectionTags(self, useWire, numOrigEdges, fdv):
        PathLog.debug('_makeIntersectionTags()')
        # Create circular probe tags around perimiter of wire
        extTags = list()
        intTags = list()
        tagRad = (self.baseClass.radius / 2)
        tagCnt = 0
        begInt = False
        begExt = False
        for e in range(0, numOrigEdges):
            E = useWire.Edges[e]
            edgLen = E.Length
            if edgLen > (self.baseClass.radius * 2):
                nt = math.ceil(edgLen / (tagRad * math.pi))
            else:
                nt = 4  # desired + 1
            mid = edgLen / nt
            spc = self.baseClass.radius / 10
            for i in range(0, nt):
                if i == 0:
                    if e == 0:
                        if edgLen > 0.2:
                            aspc = 0.1
                        else:
                            aspc = edgLen * 0.75
                        cp1 = E.valueAt(E.getParameterByLength(0))
                        cp2 = E.valueAt(E.getParameterByLength(aspc))
                        txt = 'BeginEdge[{}]_'.format(e)
                        (intTag, extTag) = self._makeOffsetCircleTag(cp1, cp2,
                                                                     tagRad,
                                                                     fdv, txt)
                        if intTag and extTag:
                            begInt = intTag
                            begExt = extTag
                else:
                    d = i * mid
                    negTestLen = d - spc
                    if negTestLen < 0:
                        negTestLen = d - (edgLen * 0.25)
                    posTestLen = d + spc
                    if posTestLen > edgLen:
                        posTestLen = d + (edgLen * 0.25)
                    cp1 = E.valueAt(E.getParameterByLength(negTestLen))
                    cp2 = E.valueAt(E.getParameterByLength(posTestLen))
                    txt = 'Edge[{}]_'.format(e)
                    (intTag, extTag) = self._makeOffsetCircleTag(cp1, cp2,
                                                                 tagRad, fdv,
                                                                 txt)
                    if intTag and extTag:
                        tagCnt += nt
                        intTags.append(intTag)
                        extTags.append(extTag)
        tagArea = math.pi * tagRad**2 * tagCnt
        iTAG = Part.makeCompound(intTags)
        eTAG = Part.makeCompound(extTags)

        return (begInt, begExt, iTAG, eTAG)

    def _makeOffsetCircleTag(self, p1, p2, cutterRad,
                             depth, lbl, reverse=False):
        # PathLog.debug('_makeOffsetCircleTag()')
        pb = FreeCAD.Vector(p1.x, p1.y, 0.0)
        pe = FreeCAD.Vector(p2.x, p2.y, 0.0)

        toMid = pe.sub(pb).multiply(0.5)
        lenToMid = toMid.Length
        if lenToMid == 0.0:
            # Probably a vertical line segment
            return (False, False)

        cutFactor = (cutterRad / 2.1) / lenToMid  # = 2 is tangent to wire; > 2 allows tag to overlap wire; < 2 pulls tag away from wire
        perpE = FreeCAD.Vector(-1 * toMid.y, toMid.x, 0.0).multiply(-1 * cutFactor)  # exterior tag
        extPnt = pb.add(toMid.add(perpE))

        # make exterior tag
        eCntr = extPnt.add(FreeCAD.Vector(0, 0, depth))
        ecw = Part.Wire(Part.makeCircle((cutterRad / 2), eCntr).Edges[0])
        extTag = Part.Face(ecw)

        # make interior tag
        perpI = FreeCAD.Vector(-1 * toMid.y, toMid.x, 0.0).multiply(cutFactor)  # interior tag
        intPnt = pb.add(toMid.add(perpI))
        iCntr = intPnt.add(FreeCAD.Vector(0, 0, depth))
        icw = Part.Wire(Part.makeCircle((cutterRad / 2), iCntr).Edges[0])
        intTag = Part.Face(icw)

        return (intTag, extTag)

    def _makeStop(self, sType, pA, pB, lbl):
        # PathLog.debug('_makeStop()')
        rad = self.baseClass.radius
        ofstRad = self.parent.ofstRadius
        extra = self.baseClass.radius / 10
        offsetExtra = self.parent.offsetExtra

        E = FreeCAD.Vector(pB.x, pB.y, 0)  # endpoint
        C = FreeCAD.Vector(pA.x, pA.y, 0)  # checkpoint
        lenEC = E.sub(C).Length

        if (self.parent.useComp is True or
                (self.parent.useComp is False and ofstExt != 0)):
            # 'L' stop shape and edge map
            # --1--
            # |   |
            # 2   6
            # |   |
            # |   ----5----|
            # |            4
            # -----3-------|
            # positive dist in _makePerp2DVector() is CCW rotation
            p1 = E
            if sType == 'BEG':
                p2 = self._makePerp2DVector(C, E, -0.25)  # E1
                p3 = self._makePerp2DVector(p1, p2, ofstRad + 1.0 + extra)
                p4 = self._makePerp2DVector(p2, p3, 0.25 + ofstRad + extra)
                p5 = self._makePerp2DVector(p3, p4, 1.0 + extra)  # E4
                p6 = self._makePerp2DVector(p4, p5, ofstRad + extra)  # E5
            elif sType == 'END':
                p2 = self._makePerp2DVector(C, E, 0.25)  # E1
                p3 = self._makePerp2DVector(p1, p2,
                                            -1 * (ofstRad + 1.0 + extra))
                p4 = self._makePerp2DVector(p2, p3,
                                            -1 * (0.25 + ofstRad + extra))
                p5 = self._makePerp2DVector(p3, p4, -1 * (1.0 + extra))  # E4
                p6 = self._makePerp2DVector(p4, p5, -1 * (ofstRad + extra))
            p7 = E  # E6
            L1 = Part.makeLine(p1, p2)
            L2 = Part.makeLine(p2, p3)
            L3 = Part.makeLine(p3, p4)
            L4 = Part.makeLine(p4, p5)
            L5 = Part.makeLine(p5, p6)
            L6 = Part.makeLine(p6, p7)
            wire = Part.Wire([L1, L2, L3, L4, L5, L6])
        else:
            # 'L' stop shape and edge map
            # :
            # |----2-------|
            # 3            1
            # |-----4------|
            # positive dist in _makePerp2DVector() is CCW rotation
            p1 = E
            if sType == 'BEG':
                p2 = self._makePerp2DVector(C, E, -1 * (0.25 + abs(ofstExt)))
                p3 = self._makePerp2DVector(p1, p2, 0.25 + abs(ofstExt))
                p4 = self._makePerp2DVector(p2, p3, (0.5 + abs(ofstExt)))
                p5 = self._makePerp2DVector(p3, p4, 0.25 + abs(ofstExt))
            elif sType == 'END':
                p2 = self._makePerp2DVector(C, E, (0.25 + abs(ofstExt)))
                p3 = self._makePerp2DVector(p1, p2, -1 * (0.25 + abs(ofstExt)))
                p4 = self._makePerp2DVector(p2, p3, -1 * (0.5 + abs(ofstExt)))
                p5 = self._makePerp2DVector(p3, p4, -1 * (0.25 + abs(ofstExt)))
            p6 = p1  # E4
            L1 = Part.makeLine(p1, p2)
            L2 = Part.makeLine(p2, p3)
            L3 = Part.makeLine(p3, p4)
            L4 = Part.makeLine(p4, p5)
            L5 = Part.makeLine(p5, p6)
            wire = Part.Wire([L1, L2, L3, L4, L5])
        # Eif
        face = Part.Face(wire)
        self.baseClass._addDebugObject(face, lbl)

        return face

    def _makePerp2DVector(self, v1, v2, dist):
        p1 = FreeCAD.Vector(v1.x, v1.y, 0.0)
        p2 = FreeCAD.Vector(v2.x, v2.y, 0.0)
        toEnd = p2.sub(p1)
        factor = dist / toEnd.Length
        perp = FreeCAD.Vector(-1 * toEnd.y, toEnd.x, 0.0).multiply(factor)
        return p1.add(toEnd.add(perp))

    def _distMidToMid(self, wireA, wireB):
        mpA = self._findWireMidpoint(wireA)
        mpB = self._findWireMidpoint(wireB)
        return mpA.sub(mpB).Length

    def _findWireMidpoint(self, wire):
        midPnt = None
        dist = 0.0
        wL = wire.Length
        midW = wL / 2

        for e in range(0, len(wire.Edges)):
            E = wire.Edges[e]
            elen = E.Length
            d_ = dist + elen
            if dist < midW and midW <= d_:
                dtm = midW - dist
                midPnt = E.valueAt(E.getParameterByLength(dtm))
                break
            else:
                dist += elen
        return midPnt
# Eclass
