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

import FreeCAD
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathProfileBase as PathProfileBase
import PathScripts.PathUtils as PathUtils

import math
import PySide

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Part = LazyLoader('Part', globals(), 'Part')
DraftGeomUtils = LazyLoader('DraftGeomUtils', globals(), 'DraftGeomUtils')

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return PySide.QtCore.QCoreApplication.translate(context, text, disambig)


__title__ = "Path Profile Edges Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Path Profile operation based on edges."
__contributors__ = "russ4262 (Russell Johnson)"


class ObjectProfile(PathProfileBase.ObjectProfile):
    '''Proxy object for Profile operations based on edges.'''

    def baseObject(self):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        return super(self.__class__, self)

    def areaOpFeatures(self, obj):
        '''areaOpFeatures(obj) ... add support for edge base geometry.'''
        return PathOp.FeatureBaseEdges

    def areaOpShapes(self, obj):
        '''areaOpShapes(obj) ... returns envelope for all wires formed by the base edges.'''
        PathLog.track()

        inaccessible = translate('PathProfileEdges', 'The selected edge(s) are inaccessible. If multiple, re-ordering selection might work.')
        if PathLog.getLevel(PathLog.thisModule()) == 4:
            self.tmpGrp = FreeCAD.ActiveDocument.addObject('App::DocumentObjectGroup', 'tmpDebugGrp')
            tmpGrpNm = self.tmpGrp.Name
        self.JOB = PathUtils.findParentJob(obj)

        self.offsetExtra = abs(obj.OffsetExtra.Value)

        if obj.UseComp:
            self.useComp = True
            self.ofstRadius = self.radius + self.offsetExtra
            self.commandlist.append(Path.Command("(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"))
        else:
            self.useComp = False
            self.ofstRadius = self.offsetExtra
            self.commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        shapes = []
        if obj.Base:
            basewires = []

            zMin = None
            for b in obj.Base:
                edgelist = []
                for sub in b[1]:
                    edgelist.append(getattr(b[0].Shape, sub))
                basewires.append((b[0], DraftGeomUtils.findWires(edgelist)))
                if zMin is None or b[0].Shape.BoundBox.ZMin < zMin:
                    zMin = b[0].Shape.BoundBox.ZMin

            PathLog.debug('PathProfileEdges areaOpShapes():: len(basewires) is {}'.format(len(basewires)))
            for base, wires in basewires:
                for wire in wires:
                    if wire.isClosed() is True:
                        # f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                        # if planar error, Comment out previous line, uncomment the next two
                        (origWire, flatWire) = self._flattenWire(obj, wire, obj.FinalDepth.Value)
                        f = origWire.Wires[0]
                        if f is not False:
                            # shift the compound to the bottom of the base object for proper sectioning
                            zShift = zMin - f.BoundBox.ZMin
                            newPlace = FreeCAD.Placement(FreeCAD.Vector(0, 0, zShift), f.Placement.Rotation)
                            f.Placement = newPlace
                            env = PathUtils.getEnvelope(base.Shape, subshape=f, depthparams=self.depthparams)
                            shapes.append((env, False))
                        else:
                            PathLog.error(inaccessible)
                    else:
                        if self.JOB.GeometryTolerance.Value == 0.0:
                            msg = self.JOB.Label + '.GeometryTolerance = 0.0.'
                            msg += translate('PathProfileEdges', 'Please set to an acceptable value greater than zero.')
                            PathLog.error(msg)
                        else:
                            cutWireObjs = False
                            flattened = self._flattenWire(obj, wire, obj.FinalDepth.Value)
                            if flattened:
                                (origWire, flatWire) = flattened
                                if PathLog.getLevel(PathLog.thisModule()) == 4:
                                    os = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpFlatWire')
                                    os.Shape = flatWire
                                    os.purgeTouched()
                                    self.tmpGrp.addObject(os)
                                cutShp = self._getCutAreaCrossSection(obj, base, origWire, flatWire)
                                if cutShp is not False:
                                    cutWireObjs = self._extractPathWire(obj, base, flatWire, cutShp)

                                if cutWireObjs is not False:
                                    for cW in cutWireObjs:
                                        shapes.append((cW, False))
                                        self.profileEdgesIsOpen = True
                                else:
                                    PathLog.error(inaccessible)
                            else:
                                PathLog.error(inaccessible)

            # Delete the temporary objects
            if PathLog.getLevel(PathLog.thisModule()) == 4:
                if FreeCAD.GuiUp:
                    import FreeCADGui
                    FreeCADGui.ActiveDocument.getObject(tmpGrpNm).Visibility = False
                self.tmpGrp.purgeTouched()

        return shapes

    def _flattenWire(self, obj, wire, trgtDep):
        '''_flattenWire(obj, wire)... Return a flattened version of the wire'''
        PathLog.debug('_flattenWire()')
        wBB = wire.BoundBox

        if wBB.ZLength > 0.0:
            PathLog.debug('Wire is not horizontally co-planar. Flattening it.')

            # Extrude non-horizontal wire
            extFwdLen = wBB.ZLength * 2.2
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
            srtWire.translate(FreeCAD.Vector(0, 0, trgtDep - srtWire.BoundBox.ZMin))

        return (wire, srtWire)

    # Open-edges methods
    def _getCutAreaCrossSection(self, obj, base, origWire, flatWire):
        PathLog.debug('_getCutAreaCrossSection()')
        FCAD = FreeCAD.ActiveDocument
        tolerance = self.JOB.GeometryTolerance.Value
        toolDiam = 2 * self.radius  # self.radius defined in PathAreaOp or PathProfileBase modules
        minBfr = toolDiam * 1.25
        bbBfr = (self.ofstRadius * 2) * 1.25
        if bbBfr < minBfr:
            bbBfr = minBfr
        fwBB = flatWire.BoundBox
        wBB = origWire.BoundBox
        minArea = (self.ofstRadius - tolerance)**2 * math.pi

        useWire = origWire.Wires[0]
        numOrigEdges = len(useWire.Edges)
        sdv = wBB.ZMax
        fdv = obj.FinalDepth.Value
        extLenFwd = sdv - fdv
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

        pl = FreeCAD.Placement()
        pl.Rotation = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0)
        pl.Base = FreeCAD.Vector(0, 0, 0)

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
        if PathLog.getLevel(PathLog.thisModule()) == 4:
            CA = FCAD.addObject('Part::Feature', 'tmpCutArea')
            CA.Shape = cutArea
            CA.recompute()
            CA.purgeTouched()
            self.tmpGrp.addObject(CA)


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
            PathLog.debug('len(botFc) > 1')
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
        return cutShp

    def _checkTagIntersection(self, iTAG, eTAG, cutSide, tstObj):
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

    def _extractPathWire(self, obj, base, flatWire, cutShp):
        PathLog.debug('_extractPathWire()')

        subLoops = list()
        rtnWIRES = list()
        osWrIdxs = list()
        subDistFactor = 1.0  # Raise to include sub wires at greater distance from original
        fdv = obj.FinalDepth.Value
        wire = flatWire
        lstVrtIdx = len(wire.Vertexes) - 1
        lstVrt = wire.Vertexes[lstVrtIdx]
        frstVrt = wire.Vertexes[0]
        cent0 = FreeCAD.Vector(frstVrt.X, frstVrt.Y, fdv)
        cent1 = FreeCAD.Vector(lstVrt.X, lstVrt.Y, fdv)

        pl = FreeCAD.Placement()
        pl.Rotation = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0)
        pl.Base = FreeCAD.Vector(0, 0, 0)

        # Calculate offset shape, containing cut region
        ofstShp = self._extractFaceOffset(obj, cutShp, False)

        # CHECK for ZERO area of offset shape
        try:
            osArea = ofstShp.Area
        except Exception as ee:
            PathLog.error('No area to offset shape returned.')
            return False

        if PathLog.getLevel(PathLog.thisModule()) == 4:
            os = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpOffsetShape')
            os.Shape = ofstShp
            os.recompute()
            os.purgeTouched()
            self.tmpGrp.addObject(os)

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
        if PathLog.getLevel(PathLog.thisModule()) == 4:
            near0 = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpNear0')
            near0.Shape = Part.makeLine(cent0, pnt0)
            near0.recompute()
            near0.purgeTouched()
            self.tmpGrp.addObject(near0)

        NEAR1 = self._findNearestVertex(ofstShp,  cent1)
        min1i = 0
        min1 = NEAR1[0][4]
        for n in range(0, len(NEAR1)):
            N = NEAR1[n]
            if N[4] < min1:
                min1 = N[4]
                min1i = n
        (w1, vi1, pnt1, vrt1, d1) = NEAR1[0]  # min1i
        if PathLog.getLevel(PathLog.thisModule()) == 4:
            near1 = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpNear1')
            near1.Shape = Part.makeLine(cent1, pnt1)
            near1.recompute()
            near1.purgeTouched()
            self.tmpGrp.addObject(near1)

        if w0 != w1:
            PathLog.warning('Offset wire endpoint indexes are not equal - w0, w1: {}, {}'.format(w0, w1))

        if PathLog.getLevel(PathLog.thisModule()) == 4:
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
            subDist = self.ofstRadius * subDistFactor
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
        (edgeIdxs0, edgeIdxs1) = self._separateWireAtVertexes(mainWire, mainWire.Vertexes[vi0], mainWire.Vertexes[vi1])
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

    def _extractFaceOffset(self, obj, fcShape, isHole):
        '''_extractFaceOffset(obj, fcShape, isHole) ... internal function.
            Original _buildPathArea() version copied from PathAreaOp.py module.  This version is modified.
            Adjustments made based on notes by @sliptonic - https://github.com/sliptonic/FreeCAD/wiki/PathArea-notes.'''
        PathLog.debug('_extractFaceOffset()')

        areaParams = {}
        JOB = PathUtils.findParentJob(obj)
        tolrnc = JOB.GeometryTolerance.Value
        if self.useComp is True:
            offset = self.ofstRadius  # + tolrnc
        else:
            offset = self.offsetExtra  # + tolrnc

        if isHole is False:
            offset = 0 - offset

        areaParams['Offset'] = offset
        areaParams['Fill'] = 1
        areaParams['Coplanar'] = 0
        areaParams['SectionCount'] = 1  # -1 = full(all per depthparams??) sections
        areaParams['Reorient'] = True
        areaParams['OpenMode'] = 0
        areaParams['MaxArcPoints'] = 400  # 400
        areaParams['Project'] = True
        # areaParams['JoinType'] = 1

        area = Path.Area()  # Create instance of Area() class object
        area.setPlane(PathUtils.makeWorkplane(fcShape))  # Set working plane
        area.add(fcShape)  # obj.Shape to use for extracting offset
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
        tolerance = self.JOB.GeometryTolerance.Value
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
        PathLog.debug('_separateWireAtVertexes() FLGS: \n{}'.format(FLGS))

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
                    if begFlg is False:
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

        if PathLog.getLevel(PathLog.thisModule()) != 4:
            PathLog.debug('grps[0]: {}'.format(grps[0]))
            PathLog.debug('grps[1]: {}'.format(grps[1]))
            PathLog.debug('wireIdxs[0]: {}'.format(wireIdxs[0]))
            PathLog.debug('wireIdxs[1]: {}'.format(wireIdxs[1]))
            PathLog.debug('PRE: {}'.format(PRE))
            PathLog.debug('IDXS: {}'.format(IDXS))

        return (wireIdxs[0], wireIdxs[1])

    def _makeCrossSection(self, shape, sliceZ, zHghtTrgt=False):
        '''_makeCrossSection(shape, sliceZ, zHghtTrgt=None)... 
        Creates cross-section objectc from shape.  Translates cross-section to zHghtTrgt if available.
        Makes face shape from cross-section object. Returns face shape at zHghtTrgt.'''
        # Create cross-section of shape and translate
        wires = list()
        slcs = shape.slice(FreeCAD.Vector(0, 0, 1), sliceZ)
        if len(slcs) > 0:
            for i in slcs:
                wires.append(i)
            comp = Part.Compound(wires)
            if zHghtTrgt is not False:
                comp.translate(FreeCAD.Vector(0, 0, zHghtTrgt - comp.BoundBox.ZMin))
            return comp

        return False

    def _makeExtendedBoundBox(self, wBB, bbBfr, zDep):
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
        # Create circular probe tags around perimiter of wire
        extTags = list()
        intTags = list()
        tagRad = (self.radius / 2)
        tagCnt = 0
        begInt = False
        begExt = False
        for e in range(0, numOrigEdges):
            E = useWire.Edges[e]
            LE = E.Length
            if LE > (self.radius * 2):
                nt = math.ceil(LE / (tagRad * math.pi))  # (tagRad * 2 * math.pi) is circumference
            else:
                nt = 4  # desired + 1
            mid = LE / nt
            spc = self.radius / 10
            for i in range(0, nt):
                if i == 0:
                    if e == 0:
                        if LE > 0.2:
                            aspc = 0.1
                        else:
                            aspc = LE * 0.75
                        cp1 = E.valueAt(E.getParameterByLength(0))
                        cp2 = E.valueAt(E.getParameterByLength(aspc))
                        (intTObj, extTObj) = self._makeOffsetCircleTag(cp1, cp2, tagRad, fdv, 'BeginEdge[{}]_'.format(e))
                        if intTObj and extTObj:
                            begInt = intTObj
                            begExt = extTObj
                else:
                    d = i * mid
                    cp1 = E.valueAt(E.getParameterByLength(d - spc))
                    cp2 = E.valueAt(E.getParameterByLength(d + spc))
                    (intTObj, extTObj) = self._makeOffsetCircleTag(cp1, cp2, tagRad, fdv, 'Edge[{}]_'.format(e))
                    if intTObj and extTObj:
                        tagCnt += nt
                        intTags.append(intTObj)
                        extTags.append(extTObj)
        tagArea = math.pi * tagRad**2 * tagCnt
        iTAG = Part.makeCompound(intTags)
        eTAG = Part.makeCompound(extTags)

        return (begInt, begExt, iTAG, eTAG)

    def _makeOffsetCircleTag(self, p1, p2, cutterRad, depth, lbl, reverse=False):
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

        pl = FreeCAD.Placement()
        pl.Rotation = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0)
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
        rad = self.radius
        ofstRad = self.ofstRadius
        extra = self.radius / 10

        pl = FreeCAD.Placement()
        pl.Rotation = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0)
        pl.Base = FreeCAD.Vector(0, 0, 0)

        E = FreeCAD.Vector(pB.x, pB.y, 0)  # endpoint
        C = FreeCAD.Vector(pA.x, pA.y, 0)  # checkpoint
        lenEC = E.sub(C).Length

        if self.useComp is True or (self.useComp is False and self.offsetExtra != 0):
            # 'L' stop shape and edge legend
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
                p3 = self._makePerp2DVector(p1, p2, ofstRad + 1 + extra)  # E2
                p4 = self._makePerp2DVector(p2, p3, 0.25 + ofstRad + extra)  # E3
                p5 = self._makePerp2DVector(p3, p4, 1 + extra)  # E4
                p6 = self._makePerp2DVector(p4, p5, ofstRad + extra)  # E5
            elif sType == 'END':
                p2 = self._makePerp2DVector(C, E, 0.25)  # E1
                p3 = self._makePerp2DVector(p1, p2, -1 * (ofstRad + 1 + extra))  # E2
                p4 = self._makePerp2DVector(p2, p3, -1 * (0.25 + ofstRad + extra))  # E3
                p5 = self._makePerp2DVector(p3, p4, -1 * (1 + extra))  # E4
                p6 = self._makePerp2DVector(p4, p5, -1 * (ofstRad + extra))  # E5
            p7 = E  # E6
            L1 = Part.makeLine(p1, p2)
            L2 = Part.makeLine(p2, p3)
            L3 = Part.makeLine(p3, p4)
            L4 = Part.makeLine(p4, p5)
            L5 = Part.makeLine(p5, p6)
            L6 = Part.makeLine(p6, p7)
            wire = Part.Wire([L1, L2, L3, L4, L5, L6])
        else:
            # 'L' stop shape and edge legend
            # :
            # |----2-------|
            # 3            1
            # |-----4------|
            # positive dist in _makePerp2DVector() is CCW rotation
            p1 = E
            if sType == 'BEG':
                p2 = self._makePerp2DVector(C, E, -1 * (0.25 + abs(self.offsetExtra)))  # left, 0.25
                p3 = self._makePerp2DVector(p1, p2, 0.25 + abs(self.offsetExtra))
                p4 = self._makePerp2DVector(p2, p3, (0.5 + abs(self.offsetExtra)))  #      FIRST POINT
                p5 = self._makePerp2DVector(p3, p4, 0.25 + abs(self.offsetExtra))  # E1                SECOND
            elif sType == 'END':
                p2 = self._makePerp2DVector(C, E, (0.25 + abs(self.offsetExtra)))  # left, 0.25
                p3 = self._makePerp2DVector(p1, p2, -1 * (0.25 + abs(self.offsetExtra)))
                p4 = self._makePerp2DVector(p2, p3, -1 * (0.5 + abs(self.offsetExtra)))  #      FIRST POINT
                p5 = self._makePerp2DVector(p3, p4, -1 * (0.25 + abs(self.offsetExtra)))  # E1                SECOND
            p6 = p1  # E4
            L1 = Part.makeLine(p1, p2)
            L2 = Part.makeLine(p2, p3)
            L3 = Part.makeLine(p3, p4)
            L4 = Part.makeLine(p4, p5)
            L5 = Part.makeLine(p5, p6)
            wire = Part.Wire([L1, L2, L3, L4, L5])
        # Eif
        face = Part.Face(wire)
        if PathLog.getLevel(PathLog.thisModule()) == 4:
            os = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmp' + lbl)
            os.Shape = face
            os.recompute()
            os.purgeTouched()
            self.tmpGrp.addObject(os)

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


def SetupProperties():
    return PathProfileBase.SetupProperties()


def Create(name, obj = None):
    '''Create(name) ... Creates and returns a Profile based on edges operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectProfile(obj, name)
    return obj
