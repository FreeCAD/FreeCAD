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
    Frist, call the getCenterOfMass() method for the CenterOfMass for patterns allowing a custom center.
    Next, call the getPathGeometryGenerator() method to request the path geometry shape.'''

    # Register valid patterns here by name
    # Create a corresponding processing method below. Precede the name with an underscore(_)
    patterns = ('Circular', 'CircularZigZag', 'Line', 'Offset', 'Spiral', 'ZigZag')

    def __init__(self, obj, shape, pattern):
        '''__init__(obj, shape, pattern)... Instantiate PathGeometryGenerator class.
        Required arguments are the operation object, horizontal planar shape, and pattern name.'''
        self.debugObjectsGroup = False
        self.pattern = None
        self.shape = None
        self.pathGeometry = None
        self.rawGeoList = None
        self.centerOfMass = None
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
        if shape.BoundBox.ZMax == 0.0:
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
            PathLog.error(translate('PathSurface', 'Cannot calculate the Center Of Mass. Using Center of Boundbox.'))
            zeroCOM = FreeCAD.Vector((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, 0.0)
        else:
            avgArea = totArea / fCnt
            zeroCOM.multiply(1 / fCnt)
            zeroCOM.multiply(1 / avgArea)
        self.centerOfMass = FreeCAD.Vector(zeroCOM.x, zeroCOM.y, 0.0)

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

    def getCenterOfMass(self):
        '''getCenterOfMass()...
        Returns the Center Of Mass for the current class instance.'''
        return self.centerOfMass

    def getPathGeometryGenerator(self):
        '''getPathGeometryGenerator()...
        Call this function to obtain the path geometry shape, generated by this class.'''
        if self.pattern is None:
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
            F = FreeCAD.ActiveDocument.addObject('Part::Feature','tmpGeometrySet')
            F.Shape = geomShape
            F.purgeTouched()
            self.debugObjectsGroup.addObject(F)

        if self.pattern == 'Offset':
            return geomShape

        # Identify intersection of cross-section face and lineset
        cmnShape = self.shape.common(geomShape)

        if self.debugObjectsGroup:
            F = FreeCAD.ActiveDocument.addObject('Part::Feature','tmpPathGeometry')
            F.Shape = cmnShape
            F.purgeTouched()
            self.debugObjectsGroup.addObject(F)

        self.tmpCOM = FreeCAD.Vector(self.centerOfMass.x, self.centerOfMass.y, 0.0)
        return cmnShape

    # Cut pattern methods
    def _Circular(self):
        GeoSet = list()
        zTgt = 0.0  # self.shape.BoundBox.ZMin
        centerAt = self.obj.CircularCenterAt
        cntr = FreeCAD.Placement()

        if centerAt == 'CenterOfMass':
            cntrPnt = FreeCAD.Vector(self.centerOfMass.x, self.centerOfMass.y, zTgt)  # self.centerOfMass  # Use center of Mass
        elif centerAt == 'CenterOfBoundBox':
            cent = self.shape.BoundBox.Center
            cntrPnt = FreeCAD.Vector(cent.x, cent.y, zTgt)  
        elif centerAt == 'XminYmin':
            cntrPnt = FreeCAD.Vector(self.shape.BoundBox.XMin, self.shape.BoundBox.YMin, zTgt)
        elif centerAt == 'Custom':
            newCent = FreeCAD.Vector(self.obj.CircularCenterCustom.x, self.obj.CircularCenterCustom.y, zTgt)
            cntrPnt = newCent

        # recalculate number of passes, if need be
        radialPasses = self.halfPasses
        if centerAt != 'CenterOfBoundBox':
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
                dist = CORNERS[c].sub(cntrPnt).Length
                if dist > dMax:
                    dMax = dist
            diag = dMax + (2.0 * self.toolDiam)  # Line length to span boundbox diag with 2x cutter diameter extra on each end
            radialPasses = math.ceil(diag / self.cutOut) + 1  # Number of lines(passes) required to cover boundbox diagonal

        # Update self.centerOfMass point and current CircularCenter
        if centerAt != 'Custom':
            self.obj.CircularCenterCustom = cntrPnt

        minRad = self.toolDiam * 0.45
        siX3 = 3 * self.obj.SampleInterval.Value
        minRadSI = (siX3 / 2.0) / math.pi
        if minRad < minRadSI:
            minRad = minRadSI

        # Make small center circle to start pattern
        if self.obj.StepOver > 50:
            circle = Part.makeCircle(minRad, cntrPnt)
            GeoSet.append(circle)

        for lc in range(1, radialPasses + 1):
            rad = (lc * self.cutOut)
            if rad >= minRad:
                circle = Part.makeCircle(rad, cntrPnt)
                GeoSet.append(circle)
        # Efor
        self.centerOfMass = cntrPnt
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
            pntTuples.append( (p1, p2) )

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
        maxDist = self.halfDiag
        move = self.centerOfMass  # FreeCAD.Vector(0.0, 0.0, 0.0)  # Use to translate the center of the spiral
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
