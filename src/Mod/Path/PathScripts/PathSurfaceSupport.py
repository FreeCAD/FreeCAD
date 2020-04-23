# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 russ4262 <russ4262@gmail.com>                      *
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

__title__ = "Path Surface Support Module"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Support functions and classes for 3D Surface and Waterline operations."
# __name__ = "PathSurfaceSupport"
__contributors__ = ""

import FreeCAD
from PySide import QtCore
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
import math
import Part


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class PathGeometryGenerator:
    '''Creates a path geometry shape from an assigned pattern for conversion to tool paths.
    PathGeometryGenerator(obj, shape, pattern)
    `obj` is the operation object, `shape` is the horizontal planar shape object,
    and `pattern` is the name of the geometric pattern to apply.
    First, call the getCenterOfPattern() method for the CenterOfMass for patterns allowing a custom center.
    Next, call the generatePathGeometry() method to request the path geometry shape.'''

    # Register valid patterns here by name
    # Create a corresponding processing method below. Precede the name with an underscore(_)
    patterns = ('Circular', 'CircularZigZag', 'Line', 'Offset', 'Spiral', 'ZigZag')

    def __init__(self, obj, shape, pattern):
        '''__init__(obj, shape, pattern)... Instantiate PathGeometryGenerator class.
        Required arguments are the operation object, horizontal planar shape, and pattern name.'''
        self.debugObjectsGroup = False
        self.pattern = 'None'
        self.shape = None
        self.pathGeometry = None
        self.rawGeoList = None
        self.centerOfMass = None
        self.centerofPattern = None
        self.deltaX = None
        self.deltaY = None
        self.deltaC = None
        self.halfDiag = None
        self.halfPasses = None
        self.obj = obj
        self.toolDiam = float(obj.ToolController.Tool.Diameter)
        self.cutOut = self.toolDiam * (float(obj.StepOver) / 100.0)
        self.wpc = Part.makeCircle(2.0)  # make circle for workplane

        # validate requested pattern
        if pattern in self.patterns:
            if hasattr(self, '_' + pattern):
                self.pattern = pattern

        if shape.BoundBox.ZMin != 0.0:
            shape.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - shape.BoundBox.ZMin))
        if shape.BoundBox.ZLength == 0.0:
            self.shape = shape
        else:
            PathLog.warning('Shape appears to not be horizontal planar. ZMax is {}.'.format(shape.BoundBox.ZMax))

        self._prepareConstants()

    def _prepareConstants(self):
        # Apply drop cutter extra offset and set the max and min XY area of the operation
        xmin = self.shape.BoundBox.XMin
        xmax = self.shape.BoundBox.XMax
        ymin = self.shape.BoundBox.YMin
        ymax = self.shape.BoundBox.YMax

        # Compute weighted center of mass of all faces combined
        if self.pattern in ['Circular', 'CircularZigZag', 'Spiral']:
            if self.obj.PatternCenterAt == 'CenterOfMass':
                fCnt = 0
                totArea = 0.0
                zeroCOM = FreeCAD.Vector(0.0, 0.0, 0.0)
                for F in self.shape.Faces:
                    comF = F.CenterOfMass
                    areaF = F.Area
                    totArea += areaF
                    fCnt += 1
                    zeroCOM = zeroCOM.add(FreeCAD.Vector(comF.x, comF.y, 0.0).multiply(areaF))
                if fCnt == 0:
                    PathLog.error(translate(self.module, 'Cannot calculate the Center Of Mass. Using Center of Boundbox instead.'))
                    bbC = self.shape.BoundBox.Center
                    zeroCOM = FreeCAD.Vector(bbC.x, bbC.y, 0.0)
                else:
                    avgArea = totArea / fCnt
                    zeroCOM.multiply(1 / fCnt)
                    zeroCOM.multiply(1 / avgArea)
                self.centerOfMass = FreeCAD.Vector(zeroCOM.x, zeroCOM.y, 0.0)
            self.centerOfPattern = self._getPatternCenter()
        else:
            bbC = self.shape.BoundBox.Center
            self.centerOfPattern = FreeCAD.Vector(bbC.x, bbC.y, 0.0)

        # get X, Y, Z spans; Compute center of rotation
        self.deltaX = self.shape.BoundBox.XLength
        self.deltaY = self.shape.BoundBox.YLength
        self.deltaC = self.shape.BoundBox.DiagonalLength  # math.sqrt(self.deltaX**2 + self.deltaY**2)
        lineLen = self.deltaC + (2.0 * self.toolDiam)  # Line length to span boundbox diag with 2x cutter diameter extra on each end
        self.halfDiag = math.ceil(lineLen / 2.0)
        cutPasses = math.ceil(lineLen / self.cutOut) + 1  # Number of lines(passes) required to cover boundbox diagonal
        self.halfPasses = math.ceil(cutPasses / 2.0)

    # Public methods
    def setDebugObjectsGroup(self, tmpGrpObject):
        '''setDebugObjectsGroup(tmpGrpObject)...
        Pass the temporary object group to show temporary construction objects'''
        self.debugObjectsGroup = tmpGrpObject

    def getCenterOfPattern(self):
        '''getCenterOfPattern()...
        Returns the Center Of Mass for the current class instance.'''
        return self.centerOfPattern

    def generatePathGeometry(self):
        '''generatePathGeometry()...
        Call this function to obtain the path geometry shape, generated by this class.'''
        if self.pattern == 'None':
            PathLog.warning('PGG: No pattern set.')
            return False

        if self.shape is None:
            PathLog.warning('PGG: No shape set.')
            return False

        cmd = 'self._' + self.pattern + '()'
        exec(cmd)

        if self.obj.CutPatternReversed is True:
            self.rawGeoList.reverse()

        # Create compound object to bind all lines in Lineset
        geomShape = Part.makeCompound(self.rawGeoList)

        # Position and rotate the Line and ZigZag geometry
        if self.pattern in ['Line', 'ZigZag']:
            if self.obj.CutPatternAngle != 0.0:
                geomShape.Placement.Rotation = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), self.obj.CutPatternAngle)
            bbC = self.shape.BoundBox.Center
            geomShape.Placement.Base = FreeCAD.Vector(bbC.x, bbC.y, 0.0 - geomShape.BoundBox.ZMin)

        if self.debugObjectsGroup:
            F = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpGeometrySet')
            F.Shape = geomShape
            F.purgeTouched()
            self.debugObjectsGroup.addObject(F)

        if self.pattern == 'Offset':
            return geomShape

        # Identify intersection of cross-section face and lineset
        cmnShape = self.shape.common(geomShape)

        if self.debugObjectsGroup:
            F = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpPathGeometry')
            F.Shape = cmnShape
            F.purgeTouched()
            self.debugObjectsGroup.addObject(F)

        return cmnShape

    # Cut pattern methods
    def _Circular(self):
        GeoSet = list()
        radialPasses = self._getRadialPasses()
        minRad = self.toolDiam * 0.45
        siX3 = 3 * self.obj.SampleInterval.Value
        minRadSI = (siX3 / 2.0) / math.pi

        if minRad < minRadSI:
            minRad = minRadSI

        PathLog.debug(' -centerOfPattern: {}'.format(self.centerOfPattern))
        # Make small center circle to start pattern
        if self.obj.StepOver > 50:
            circle = Part.makeCircle(minRad, self.centerOfPattern)
            GeoSet.append(circle)

        for lc in range(1, radialPasses + 1):
            rad = (lc * self.cutOut)
            if rad >= minRad:
                circle = Part.makeCircle(rad, self.centerOfPattern)
                GeoSet.append(circle)
        # Efor
        self.rawGeoList = GeoSet

    def _CircularZigZag(self):
        self._Circular()  # Use _Circular generator

    def _Line(self):
        GeoSet = list()
        centRot = FreeCAD.Vector(0.0, 0.0, 0.0)  # Bottom left corner of face/selection/model
        cAng = math.atan(self.deltaX / self.deltaY)  # BoundaryBox angle

        # Determine end points and create top lines
        x1 = centRot.x - self.halfDiag
        x2 = centRot.x + self.halfDiag
        diag = None
        if self.obj.CutPatternAngle == 0 or self.obj.CutPatternAngle == 180:
            diag = self.deltaY
        elif self.obj.CutPatternAngle == 90 or self.obj.CutPatternAngle == 270:
            diag = self.deltaX
        else:
            perpDist = math.cos(cAng - math.radians(self.obj.CutPatternAngle)) * self.deltaC
            diag = perpDist
        y1 = centRot.y + diag
        # y2 = y1

        # Create end points for set of lines to intersect with cross-section face
        pntTuples = list()
        for lc in range((-1 * (self.halfPasses - 1)), self.halfPasses + 1):
            x1 = centRot.x - self.halfDiag
            x2 = centRot.x + self.halfDiag
            y1 = centRot.y + (lc * self.cutOut)
            # y2 = y1
            p1 = FreeCAD.Vector(x1, y1, 0.0)
            p2 = FreeCAD.Vector(x2, y1, 0.0)
            pntTuples.append((p1, p2))

        # Convert end points to lines
        for (p1, p2) in pntTuples:
            line = Part.makeLine(p1, p2)
            GeoSet.append(line)

        self.rawGeoList = GeoSet

    def _Offset(self):
        self.rawGeoList = self._extractOffsetFaces()

    def _Spiral(self):
        GeoSet = list()
        SEGS = list()
        draw = True
        loopRadians = 0.0  # Used to keep track of complete loops/cycles
        sumRadians = 0.0
        loopCnt = 0
        segCnt = 0
        twoPi = 2.0 * math.pi
        maxDist = math.ceil(self.cutOut * self._getRadialPasses())  # self.halfDiag
        move = self.centerOfPattern  # Use to translate the center of the spiral
        lastPoint = FreeCAD.Vector(0.0, 0.0, 0.0)

        # Set tool properties and calculate cutout
        cutOut = self.cutOut / twoPi
        segLen = self.obj.SampleInterval.Value  # CutterDiameter / 10.0  # SampleInterval.Value
        stepAng = segLen / ((loopCnt + 1) * self.cutOut)  # math.pi / 18.0  # 10 degrees
        stopRadians = maxDist / cutOut

        if self.obj.CutPatternReversed:
            if self.obj.CutMode == 'Conventional':
                getPoint = self._makeOppSpiralPnt
            else:
                getPoint = self._makeRegSpiralPnt

            while draw:
                radAng = sumRadians + stepAng
                p1 = lastPoint
                p2 = getPoint(move, cutOut, radAng)  # cutOut is 'b' in the equation r = b * radAng
                sumRadians += stepAng  # Increment sumRadians
                loopRadians += stepAng  # Increment loopRadians
                if loopRadians > twoPi:
                    loopCnt += 1
                    loopRadians -= twoPi
                    stepAng = segLen / ((loopCnt + 1) * self.cutOut)  # adjust stepAng with each loop/cycle
                segCnt += 1
                lastPoint = p2
                if sumRadians > stopRadians:
                    draw = False
                # Create line and show in Object tree
                lineSeg = Part.makeLine(p2, p1)
                SEGS.append(lineSeg)
            # Ewhile
            SEGS.reverse()
        else:
            if self.obj.CutMode == 'Climb':
                getPoint = self._makeOppSpiralPnt
            else:
                getPoint = self._makeRegSpiralPnt

            while draw:
                radAng = sumRadians + stepAng
                p1 = lastPoint
                p2 = getPoint(move, cutOut, radAng)  # cutOut is 'b' in the equation r = b * radAng
                sumRadians += stepAng  # Increment sumRadians
                loopRadians += stepAng  # Increment loopRadians
                if loopRadians > twoPi:
                    loopCnt += 1
                    loopRadians -= twoPi
                    stepAng = segLen / ((loopCnt + 1) * self.cutOut)  # adjust stepAng with each loop/cycle
                segCnt += 1
                lastPoint = p2
                if sumRadians > stopRadians:
                    draw = False
                # Create line and show in Object tree
                lineSeg = Part.makeLine(p1, p2)
                SEGS.append(lineSeg)
            # Ewhile
        # Eif
        spiral = Part.Wire([ls.Edges[0] for ls in SEGS])
        GeoSet.append(spiral)

        self.rawGeoList = GeoSet

    def _ZigZag(self):
        self._Line()  # Use _Line generator

    # Support methods
    def _getPatternCenter(self):
        centerAt = self.obj.PatternCenterAt

        if centerAt == 'CenterOfMass':
            cntrPnt = FreeCAD.Vector(self.centerOfMass.x, self.centerOfMass.y, 0.0)
        elif centerAt == 'CenterOfBoundBox':
            cent = self.shape.BoundBox.Center
            cntrPnt = FreeCAD.Vector(cent.x, cent.y, 0.0)
        elif centerAt == 'XminYmin':
            cntrPnt = FreeCAD.Vector(self.shape.BoundBox.XMin, self.shape.BoundBox.YMin, 0.0)
        elif centerAt == 'Custom':
            cntrPnt = FreeCAD.Vector(self.obj.PatternCenterCustom.x, self.obj.PatternCenterCustom.y, 0.0)

        # Update centerOfPattern point
        if centerAt != 'Custom':
            self.obj.PatternCenterCustom = cntrPnt
        self.centerOfPattern = cntrPnt

        return cntrPnt

    def _getRadialPasses(self):
        # recalculate number of passes, if need be
        radialPasses = self.halfPasses
        if self.obj.PatternCenterAt != 'CenterOfBoundBox':
            # make 4 corners of boundbox in XY plane, find which is greatest distance to new circular center
            EBB = self.shape.BoundBox
            CORNERS = [
                FreeCAD.Vector(EBB.XMin, EBB.YMin, 0.0),
                FreeCAD.Vector(EBB.XMin, EBB.YMax, 0.0),
                FreeCAD.Vector(EBB.XMax, EBB.YMax, 0.0),
                FreeCAD.Vector(EBB.XMax, EBB.YMin, 0.0),
            ]
            dMax = 0.0
            for c in range(0, 4):
                dist = CORNERS[c].sub(self.centerOfPattern).Length
                if dist > dMax:
                    dMax = dist
            diag = dMax + (2.0 * self.toolDiam)  # Line length to span boundbox diag with 2x cutter diameter extra on each end
            radialPasses = math.ceil(diag / self.cutOut) + 1  # Number of lines(passes) required to cover boundbox diagonal

        return radialPasses

    def _makeRegSpiralPnt(self, move, b, radAng):
        x = b * radAng * math.cos(radAng)
        y = b * radAng * math.sin(radAng)
        return FreeCAD.Vector(x, y, 0.0).add(move)

    def _makeOppSpiralPnt(self, move, b, radAng):
        x = b * radAng * math.cos(radAng)
        y = b * radAng * math.sin(radAng)
        return FreeCAD.Vector(-1 * x, y, 0.0).add(move)

    def _extractOffsetFaces(self):
        PathLog.debug('_extractOffsetFaces()')
        wires = list()
        faces = list()
        ofst = 0.0  # - self.cutOut
        shape = self.shape
        cont = True
        cnt = 0
        while cont:
            ofstArea = self._getFaceOffset(shape, ofst)
            if not ofstArea:
                PathLog.warning('PGG: No offset clearing area returned.')
                cont = False
                break
            for F in ofstArea.Faces:
                faces.append(F)
                for w in F.Wires:
                    wires.append(w)
            shape = ofstArea
            if cnt == 0:
                ofst = 0.0 - self.cutOut
            cnt += 1
        return wires

    def _getFaceOffset(self, shape, offset):
        '''_getFaceOffset(shape, offset) ... internal function.
            Original _buildPathArea() version copied from PathAreaOp.py module.  This version is modified.
            Adjustments made based on notes by @sliptonic at this webpage: https://github.com/sliptonic/FreeCAD/wiki/PathArea-notes.'''
        PathLog.debug('_getFaceOffset()')

        areaParams = {}
        areaParams['Offset'] = offset
        areaParams['Fill'] = 1  # 1
        areaParams['Coplanar'] = 0
        areaParams['SectionCount'] = 1  # -1 = full(all per depthparams??) sections
        areaParams['Reorient'] = True
        areaParams['OpenMode'] = 0
        areaParams['MaxArcPoints'] = 400  # 400
        areaParams['Project'] = True

        area = Path.Area()  # Create instance of Area() class object
        # area.setPlane(PathUtils.makeWorkplane(shape))  # Set working plane
        area.setPlane(PathUtils.makeWorkplane(self.wpc))  # Set working plane to normal at Z=1
        area.add(shape)
        area.setParams(**areaParams)  # set parameters

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

        return ofstFace
# Eclass


class ProcessSelectedFaces:
    """ProcessSelectedFaces(JOB, obj) class.
    This class processes the `obj.Base` object for selected geometery.
    Calling the preProcessModel(module) method returns
    two compound objects as a tuple: (FACES, VOIDS) or False."""

    def __init__(self, JOB, obj):
        self.modelSTLs = list()
        self.profileShapes = list()
        self.tempGroup = False
        self.showDebugObjects = False
        self.checkBase = False
        self.module = None
        self.radius = None
        self.depthParams = None
        self.msgNoFaces = translate(self.module, 'Face selection is unavailable for Rotational scans.  Ignoring selected faces.')
        self.JOB = JOB
        self.obj = obj
        self.profileEdges = 'None'

        if hasattr(obj, 'ProfileEdges'):
            self.profileEdges = obj.ProfileEdges

        # Setup STL, model type, and bound box containers for each model in Job
        for m in range(0, len(JOB.Model.Group)):
            M = JOB.Model.Group[m]
            self.modelSTLs.append(False)
            self.profileShapes.append(False)

        # make circle for workplane
        self.wpc = Part.makeCircle(2.0)

    def PathSurface(self):
        if self.obj.Base:
            if len(self.obj.Base) > 0:
                self.checkBase = True
                if self.obj.ScanType == 'Rotational':
                    self.checkBase = False
                    PathLog.warning(self.msgNoFaces)

    def PathWaterline(self):
        if self.obj.Base:
            if len(self.obj.Base) > 0:
                self.checkBase = True
                if self.obj.Algorithm in ['OCL Dropcutter', 'Experimental']:
                    self.checkBase = False
                    PathLog.warning(self.msgNoFaces)

    # public class methods
    def setShowDebugObjects(self, grpObj, val):
        self.tempGroup = grpObj
        self.showDebugObjects = val

    def preProcessModel(self, module):
        PathLog.debug('preProcessModel()')

        if not self._isReady(module):
            return False

        FACES = list()
        VOIDS = list()
        fShapes = list()
        vShapes = list()
        GRP = self.JOB.Model.Group
        lenGRP = len(GRP)

        # Crete place holders for each base model in Job
        for m in range(0, lenGRP):
            FACES.append(False)
            VOIDS.append(False)
            fShapes.append(False)
            vShapes.append(False)

        # The user has selected subobjects from the base.  Pre-Process each.
        if self.checkBase:
            PathLog.debug(' -obj.Base exists. Pre-processing for selected faces.')

            # (FACES, VOIDS) = self._identifyFacesAndVoids(FACES, VOIDS)
            (F, V) = self._identifyFacesAndVoids(FACES, VOIDS)

            # Cycle through each base model, processing faces for each
            for m in range(0, lenGRP):
                base = GRP[m]
                (mFS, mVS, mPS) = self._preProcessFacesAndVoids(base, m, FACES, VOIDS)
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
                if self.obj.BoundBox == 'BaseBoundBox':
                    base = GRP[m]
                elif self.obj.BoundBox == 'Stock':
                    base = self.JOB.Stock

                pPEB = self._preProcessEntireBase(base, m)
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

    # private class methods
    def _isReady(self, module):
        '''_isReady(module)... Internal method.
        Checks if required attributes are available for processing obj.Base (the Base Geometry).'''
        if hasattr(self, module):
            self.module = module
            modMethod = getattr(self, module)  # gets the attribute only
            modMethod()  # executes as method
        else:
            return False

        if not self.radius:
            return False

        if not self.depthParams:
            return False

        return True

    def _identifyFacesAndVoids(self, F, V):
        TUPS = list()
        GRP = self.JOB.Model.Group
        lenGRP = len(GRP)

        # Separate selected faces into (base, face) tuples and flag model(s) for STL creation
        for (bs, SBS) in self.obj.Base:
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
        add = faceCnt - self.obj.AvoidLastX_Faces
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

    def _preProcessFacesAndVoids(self, base, m, FACES, VOIDS):
        mFS = False
        mVS = False
        mPS = False
        mIFS = list()

        if FACES[m] is not False:
            isHole = False
            if self.obj.HandleMultipleFeatures == 'Collectively':
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
                        if self.obj.InternalFeaturesCut is False:
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
                if cont is True and self.profileEdges != 'None':
                    ofstVal = self._calculateOffsetValue(isHole)
                    psOfst = extractFaceOffset(cfsL, ofstVal, self.wpc)
                    if psOfst is not False:
                        mPS = [psOfst]
                        if self.profileEdges == 'Only':
                            mFS = True
                            cont = False
                    else:
                        PathLog.error(' -Failed to create profile geometry for selected faces.')
                        cont = False

                if cont:
                    if self.showDebugObjects:
                        T = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpCollectiveShape')
                        T.Shape = cfsL
                        T.purgeTouched()
                        self.tempGroup.addObject(T)

                    ofstVal = self._calculateOffsetValue(isHole)
                    faceOfstShp = extractFaceOffset(cfsL, ofstVal, self.wpc)
                    if faceOfstShp is False:
                        PathLog.error(' -Failed to create offset face.')
                        cont = False

                if cont:
                    lenIfL = len(ifL)
                    if self.obj.InternalFeaturesCut is False:
                        if lenIfL == 0:
                            PathLog.debug(' -No internal features saved.')
                        else:
                            if lenIfL == 1:
                                casL = ifL[0]
                            else:
                                casL = Part.makeCompound(ifL)
                            if self.showDebugObjects:
                                C = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpCompoundIntFeat')
                                C.Shape = casL
                                C.purgeTouched()
                                self.tempGroup.addObject(C)
                            ofstVal = self._calculateOffsetValue(isHole=True)
                            intOfstShp = extractFaceOffset(casL, ofstVal, self.wpc)
                            mIFS.append(intOfstShp)
                            # faceOfstShp = faceOfstShp.cut(intOfstShp)

                    mFS = [faceOfstShp]
                # Eif

            elif self.obj.HandleMultipleFeatures == 'Individually':
                for (fcshp, fcIdx) in FACES[m]:
                    cont = True
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
                        if self.obj.InternalFeaturesCut is False:
                            if intWires is not False:
                                for (iFace, rsd) in intWires:
                                    ifL.append(iFace)

                    if outerFace is not False:
                        PathLog.debug('Attempting to create offset face of Face{}'.format(fNum))

                        if self.profileEdges != 'None':
                            ofstVal = self._calculateOffsetValue(isHole)
                            psOfst = extractFaceOffset(outerFace, ofstVal, self.wpc)
                            if psOfst is not False:
                                if mPS is False:
                                    mPS = list()
                                mPS.append(psOfst)
                                if self.profileEdges == 'Only':
                                    if mFS is False:
                                        mFS = list()
                                    mFS.append(True)
                                    cont = False
                            else:
                                PathLog.error(' -Failed to create profile geometry for Face{}.'.format(fNum))
                                cont = False

                        if cont:
                            ofstVal = self._calculateOffsetValue(isHole)
                            faceOfstShp = extractFaceOffset(outerFace, ofstVal, self.wpc)

                            lenIfl = len(ifL)
                            if self.obj.InternalFeaturesCut is False and lenIfl > 0:
                                if lenIfl == 1:
                                    casL = ifL[0]
                                else:
                                    casL = Part.makeCompound(ifL)

                                ofstVal = self._calculateOffsetValue(isHole=True)
                                intOfstShp = extractFaceOffset(casL, ofstVal, self.wpc)
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
                    if self.obj.AvoidLastX_InternalFeatures is False:
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

                if self.showDebugObjects:
                    PathLog.debug('*** tmpAvoidArea')
                    P = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpVoidEnvelope')
                    P.Shape = avoid
                    P.purgeTouched()
                    self.tempGroup.addObject(P)

            if cont:
                if self.showDebugObjects:
                    PathLog.debug('*** tmpVoidCompound')
                    P = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpVoidCompound')
                    P.Shape = avoid
                    P.purgeTouched()
                    self.tempGroup.addObject(P)
                ofstVal = self._calculateOffsetValue(isHole, isVoid=True)
                avdOfstShp = extractFaceOffset(avoid, ofstVal, self.wpc)
                if avdOfstShp is False:
                    PathLog.error('Failed to create collective offset avoid face.')
                    cont = False

            if cont:
                avdShp = avdOfstShp

                if self.obj.AvoidLastX_InternalFeatures is False and len(intFEAT) > 0:
                    if len(intFEAT) > 1:
                        ifc = Part.makeCompound(intFEAT)
                    else:
                        ifc = intFEAT[0]
                    ofstVal = self._calculateOffsetValue(isHole=True)
                    ifOfstShp = extractFaceOffset(ifc, ofstVal, self.wpc)
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
        warnFinDep = translate(self.module, 'Final Depth might need to be lower. Internal features detected in Face')

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

    def _preProcessEntireBase(self, base, m):
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

        if cont:
            csFaceShape = getShapeSlice(baseEnv)
            if csFaceShape is False:
                PathLog.debug('getShapeSlice(baseEnv) failed')
                csFaceShape = getCrossSection(baseEnv)
                if csFaceShape is False:
                    PathLog.debug('getCrossSection(baseEnv) failed')
                    csFaceShape = getSliceFromEnvelope(baseEnv)
            if csFaceShape is False:
                PathLog.error('Failed to slice baseEnv shape.')
                cont = False

        if cont is True and self.profileEdges != 'None':
            PathLog.debug(' -Attempting profile geometry for model base.')
            ofstVal = self._calculateOffsetValue(isHole)
            psOfst = extractFaceOffset(csFaceShape, ofstVal, self.wpc)
            if psOfst is not False:
                if self.profileEdges == 'Only':
                    return (True, psOfst)
                prflShp = psOfst
            else:
                PathLog.error(' -Failed to create profile geometry.')
                cont = False

        if cont:
            ofstVal = self._calculateOffsetValue(isHole)
            faceOffsetShape = extractFaceOffset(csFaceShape, ofstVal, self.wpc)
            if faceOffsetShape is False:
                PathLog.error('extractFaceOffset() failed.')
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

            if cont:
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
                owLen = fc.OuterWire.Length
                wLen = W.Length
                if abs(owLen - wLen) > 0.0000001:
                    OW = Part.Wire(Part.__sortEdges__(fc.OuterWire.Edges))
                    return [(OW, False), (W, raised)]
                else:
                    return [(W, raised)]
            else:
                sortedWIRES = sorted(WIRES, key=index0, reverse=True)
                WRS = [(W, raised) for (area, W, raised) in sortedWIRES]  # outer, then inner by area size
                # Check if OuterWire is larger than largest in WRS list
                (W, raised) = WRS[0]
                owLen = fc.OuterWire.Length
                wLen = W.Length
                if abs(owLen - wLen) > 0.0000001:
                    OW = Part.Wire(Part.__sortEdges__(fc.OuterWire.Edges))
                    WRS.insert(0, (OW, False))
                return WRS

        return False

    def _calculateOffsetValue(self, isHole, isVoid=False):
        '''_calculateOffsetValue(self.obj, isHole, isVoid) ... internal function.
        Calculate the offset for the Path.Area() function.'''
        self.JOB = PathUtils.findParentJob(self.obj)
        tolrnc = self.JOB.GeometryTolerance.Value

        if isVoid is False:
            if isHole is True:
                offset = -1 * self.obj.InternalFeaturesAdjustment.Value
                offset += self.radius + (tolrnc / 10.0)
            else:
                offset = -1 * self.obj.BoundaryAdjustment.Value
                if self.obj.BoundaryEnforcement is True:
                    offset += self.radius + (tolrnc / 10.0)
                else:
                    offset -= self.radius + (tolrnc / 10.0)
                offset = 0.0 - offset
        else:
            offset = -1 * self.obj.BoundaryAdjustment.Value
            offset += self.radius + (tolrnc / 10.0)

        return offset

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
        ext = getExtrudedShape(nWire)
        if ext is False:
            PathLog.debug('getExtrudedShape() failed')
        else:
            slc = getShapeSlice(ext)
            if slc is not False:
                return slc
            cs = getCrossSection(ext, True)
            if cs is not False:
                return cs

        # Attempt creating an envelope, and then try a manual slice and then cross-section
        env = getShapeEnvelope(nWire)
        if env is False:
            PathLog.debug('getShapeEnvelope() failed')
        else:
            slc = getShapeSlice(env)
            if slc is not False:
                return slc
            cs = getCrossSection(env, True)
            if cs is not False:
                return cs

        # Attempt creating a projection
        slc = getProjectedFace(self.tempGroup, nWire)
        if slc is False:
            PathLog.debug('getProjectedFace() failed')
        else:
            return slc

        return False
# Eclass


# Functions for getting a shape envelope and cross-section
def getExtrudedShape(wire):
    PathLog.debug('getExtrudedShape()')
    wBB = wire.BoundBox
    extFwd = math.floor(2.0 * wBB.ZLength) + 10.0

    try:
        shell = wire.extrude(FreeCAD.Vector(0.0, 0.0, extFwd))
    except Exception as ee:
        PathLog.error(' -extrude wire failed: \n{}'.format(ee))
        return False

    SHP = Part.makeSolid(shell)
    return SHP

def getShapeSlice(shape):
    PathLog.debug('getShapeSlice()')

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
            return comp

    # PathLog.debug(' -slcArea !< midArea')
    # PathLog.debug(' -slcShp.Edges count: {}.  Might be a vertically oriented face.'.format(len(slcShp.Edges)))
    return False

def getProjectedFace(tempGroup, wire):
    import Draft
    PathLog.debug('getProjectedFace()')
    F = FreeCAD.ActiveDocument.addObject('Part::Feature', 'tmpProjectionWire')
    F.Shape = wire
    F.purgeTouched()
    tempGroup.addObject(F)
    try:
        prj = Draft.makeShape2DView(F, FreeCAD.Vector(0, 0, 1))
        prj.recompute()
        prj.purgeTouched()
        tempGroup.addObject(prj)
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

def getCrossSection(shape, withExtrude=False):
    PathLog.debug('getCrossSection()')
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
            ext = getExtrudedShape(csWire)
            CS = getShapeSlice(ext)
            if CS is False:
                return False
        else:
            CS = Part.Face(csWire)
        CS.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - CS.BoundBox.ZMin))
        return CS
    else:
        PathLog.debug(' -No wires from .slice() method')

    return False

def getShapeEnvelope(shape):
    PathLog.debug('getShapeEnvelope()')

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

def getSliceFromEnvelope(env):
    PathLog.debug('getSliceFromEnvelope()')
    eBB = env.BoundBox
    extFwd = eBB.ZLength + 10.0
    maxz = eBB.ZMin + extFwd

    emax = math.floor(maxz - 1.0)
    E = list()
    for e in range(0, len(env.Edges)):
        emin = env.Edges[e].BoundBox.ZMin
        if emin > emax:
            E.append(env.Edges[e])
    tf = Part.Face(Part.Wire(Part.__sortEdges__(E)))
    tf.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - tf.BoundBox.ZMin))

    return tf


# Function to extract offset face from shape
def extractFaceOffset(fcShape, offset, wpc, makeComp=True):
    '''extractFaceOffset(fcShape, offset) ... internal function.
        Original _buildPathArea() version copied from PathAreaOp.py module.  This version is modified.
        Adjustments made based on notes by @sliptonic at this webpage: https://github.com/sliptonic/FreeCAD/wiki/PathArea-notes.'''
    PathLog.debug('extractFaceOffset()')

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

    area = Path.Area()  # Create instance of Area() class object
    # area.setPlane(PathUtils.makeWorkplane(fcShape))  # Set working plane
    area.setPlane(PathUtils.makeWorkplane(wpc))  # Set working plane to normal at Z=1
    area.add(fcShape)
    area.setParams(**areaParams)  # set parameters

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


# Functions to convert path geometry into line/arc segments for OCL input or directly to g-code
def pathGeomToLinesPointSet(obj, compGeoShp, cutClimb, toolDiam, closedGap, gaps):
    '''pathGeomToLinesPointSet(obj, compGeoShp)...
    Convert a compound set of sequential line segments to directionally-oriented collinear groupings.'''
    PathLog.debug('pathGeomToLinesPointSet()')
    # Extract intersection line segments for return value as list()
    LINES = list()
    inLine = list()
    chkGap = False
    lnCnt = 0
    ec = len(compGeoShp.Edges)
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
        # iC = sp.isOnLineSegment(ep, cp)
        iC = cp.isOnLineSegment(sp, ep)
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
                closedGap = True
            else:
                # PathLog.debug('---- Gap: {} mm'.format(gap))
                gap = round(gap, 6)
                if gap < gaps[0]:
                    gaps.insert(0, gap)
                    gaps.pop()
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

def pathGeomToZigzagPointSet(obj, compGeoShp, cutClimb, toolDiam, closedGap, gaps):
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

    if cutClimb is True:
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

    for ei in range(1, ec):
        edg = compGeoShp.Edges[ei]
        v1 = (edg.Vertexes[0].X, edg.Vertexes[0].Y)
        v2 = (edg.Vertexes[1].X, edg.Vertexes[1].Y)

        cp = FreeCAD.Vector(v1[0], v1[1], 0.0)  # check point (start point of segment)
        ep = FreeCAD.Vector(v2[0], v2[1], 0.0)  # end point
        # iC = sp.isOnLineSegment(ep, cp)
        iC = cp.isOnLineSegment(sp, ep)
        if iC is True:
            inLine.append('BRK')
            chkGap = True
            gap = abs(toolDiam - lst.sub(cp).Length)
        else:
            chkGap = False
            if dirFlg == -1:
                inLine.reverse()
            # LINES.append((dirFlg, inLine))
            LINES.append(inLine)
            lnCnt += 1
            dirFlg = -1 * dirFlg  # Change zig to zag
            inLine = list()  # reset collinear container
            sp = cp  # FreeCAD.Vector(v1[0], v1[1], 0.0)

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
                    tup = (tup[0], vB)
                closedGap = True
            else:
                gap = round(gap, 6)
                if gap < gaps[0]:
                    gaps.insert(0, gap)
                    gaps.pop()
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
            if cutClimb is True:
                dirFlg = -1 * dirFlg

    if obj.CutPatternReversed:
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

        if not obj.CutPatternReversed:
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

        # LINES.append((dirFlg, rev))
        LINES.append(rev)
    else:
        # LINES.append((dirFlg, inLine))
        LINES.append(inLine)

    return LINES

def pathGeomToCircularPointSet(obj, compGeoShp, cutClimb, toolDiam, closedGap, gaps, COM):
    '''pathGeomToCircularPointSet(obj, compGeoShp)...
    Convert a compound set of arcs/circles to a set of directionally-oriented arc end points
    and the corresponding center point.'''
    # Extract intersection line segments for return value as list()
    PathLog.debug('pathGeomToCircularPointSet()')
    ARCS = list()
    stpOvrEI = list()
    segEI = list()
    isSame = False
    sameRad = None
    ec = len(compGeoShp.Edges)

    def gapDist(sp, ep):
        X = (ep[0] - sp[0])**2
        Y = (ep[1] - sp[1])**2
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
    if not cutClimb:  # True yields Climb when set to Conventional
        dirFlg = -1

    # Cycle through stepOver data
    for so in range(0, len(stpOvrEI)):
        SO = stpOvrEI[so]
        if SO[0] == 'L':  # L = Loop/Ring/Circle
            # PathLog.debug("SO[0] == 'Loop'")
            lei = SO[1]  # loop Edges index
            v1 = compGeoShp.Edges[lei].Vertexes[0]

            # space = obj.SampleInterval.Value / 10.0
            # space = 0.000001
            space = toolDiam * 0.005  # If too small, OCL will fail to scan the loop

            # p1 = FreeCAD.Vector(v1.X, v1.Y, v1.Z)
            p1 = FreeCAD.Vector(v1.X, v1.Y, 0.0)  # z=0.0 for waterline; z=v1.Z for 3D Surface
            rad = p1.sub(COM).Length
            spcRadRatio = space/rad
            if spcRadRatio < 1.0:
                tolrncAng = math.asin(spcRadRatio)
            else:
                tolrncAng = 0.99999998 * math.pi
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
                        PRTS.pop()  # pop off 'BRK' marker
                        (vA, vB, vC) = PRTS.pop()  # pop off previous arc segment for combining with current
                        arc = (vA, arc[1], vC)
                        closedGap = True
                    else:
                        # PathLog.debug('---- Gap: {} mm'.format(gap))
                        gap = round(gap, 6)
                        if gap < gaps[0]:
                            gaps.insert(0, gap)
                            gaps.pop()
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

def pathGeomToSpiralPointSet(obj, compGeoShp):
    '''_pathGeomToSpiralPointSet(obj, compGeoShp)...
    Convert a compound set of sequential line segments to directional, connected groupings.'''
    PathLog.debug('_pathGeomToSpiralPointSet()')
    # Extract intersection line segments for return value as list()
    LINES = list()
    inLine = list()
    lnCnt = 0
    ec = len(compGeoShp.Edges)
    start = 2

    if obj.CutPatternReversed:
        edg1 = compGeoShp.Edges[0]  # Skip first edge, as it is the closing edge: center to outer tail
        ec -= 1
        start = 1
    else:
        edg1 = compGeoShp.Edges[1]  # Skip first edge, as it is the closing edge: center to outer tail
    p1 = FreeCAD.Vector(edg1.Vertexes[0].X, edg1.Vertexes[0].Y, 0.0)
    p2 = FreeCAD.Vector(edg1.Vertexes[1].X, edg1.Vertexes[1].Y, 0.0)
    tup = ((p1.x, p1.y), (p2.x, p2.y))
    inLine.append(tup)
    lst = p2

    for ei in range(start, ec):  # Skipped first edge, started with second edge above as edg1
        edg = compGeoShp.Edges[ei]  # Get edge for vertexes
        sp = FreeCAD.Vector(edg.Vertexes[0].X, edg.Vertexes[0].Y, 0.0)  # check point (first / middle point)
        ep = FreeCAD.Vector(edg.Vertexes[1].X, edg.Vertexes[1].Y, 0.0)  # end point
        tup = ((sp.x, sp.y), (ep.x, ep.y))

        if sp.sub(p2).Length < 0.000001:
            inLine.append(tup)
        else:
            LINES.append(inLine)  # Save inLine segments
            lnCnt += 1
            inLine = list()  # reset container
            inLine.append(tup)
        p1 = sp
        p2 = ep
    # Efor

    lnCnt += 1
    LINES.append(inLine)  # Save inLine segments

    return LINES

def pathGeomToOffsetPointSet(obj, compGeoShp):
    '''pathGeomToOffsetPointSet(obj, compGeoShp)...
    Convert a compound set of 3D profile segmented wires to 2D segments, applying linear optimization.'''
    PathLog.debug('pathGeomToOffsetPointSet()')

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
            if optimize:
                # iPOL = prev.isOnLineSegment(nxt, pnt)
                iPOL = pnt.isOnLineSegment(prev, nxt)
                if iPOL:
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
        if iPOL:
            tup = ((prev.x, prev.y), (pnt.x, pnt.y))
            LINES.append(tup)
    # Efor

    return [LINES]