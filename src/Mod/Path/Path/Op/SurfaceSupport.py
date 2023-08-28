# -*- coding: utf-8 -*-
# ***************************************************************************
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


__title__ = "Path Surface Support Module"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecad.org"
__doc__ = "Support functions and classes for 3D Surface and Waterline operations."
__contributors__ = ""

import FreeCAD
import Path
import Path.Op.Util as PathOpUtil
import PathScripts.PathUtils as PathUtils
import math

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

# MeshPart = LazyLoader('MeshPart', globals(), 'MeshPart')
Part = LazyLoader("Part", globals(), "Part")


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


class PathGeometryGenerator:
    """Creates a path geometry shape from an assigned pattern for conversion to tool paths.
    PathGeometryGenerator(obj, shape, pattern)
    `obj` is the operation object, `shape` is the horizontal planar shape object,
    and `pattern` is the name of the geometric pattern to apply.
    First, call the getCenterOfPattern() method for the CenterOfMass for patterns allowing a custom center.
    Next, call the generatePathGeometry() method to request the path geometry shape."""

    # Register valid patterns here by name
    # Create a corresponding processing method below. Precede the name with an underscore(_)
    patterns = ("Circular", "CircularZigZag", "Line", "Offset", "Spiral", "ZigZag")

    def __init__(self, obj, shape, pattern):
        """__init__(obj, shape, pattern)... Instantiate PathGeometryGenerator class.
        Required arguments are the operation object, horizontal planar shape, and pattern name."""
        self.debugObjectsGroup = False
        self.pattern = "None"
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
            if hasattr(self, "_" + pattern):
                self.pattern = pattern

        if shape.BoundBox.ZMin != 0.0:
            shape.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - shape.BoundBox.ZMin))
        if shape.BoundBox.ZLength > 1.0e-8:
            msg = translate(
                "PathSurfaceSupport", "Shape appears to not be horizontal planar."
            )
            msg += " ZMax == {} mm.\n".format(shape.BoundBox.ZMax)
            FreeCAD.Console.PrintWarning(msg)
        else:
            self.shape = shape
            self._prepareConstants()

    def _prepareConstants(self):
        # Compute weighted center of mass of all faces combined
        if self.pattern in ["Circular", "CircularZigZag", "Spiral"]:
            if self.obj.PatternCenterAt == "CenterOfMass":
                fCnt = 0
                totArea = 0.0
                zeroCOM = FreeCAD.Vector(0.0, 0.0, 0.0)
                for F in self.shape.Faces:
                    comF = F.CenterOfMass
                    areaF = F.Area
                    totArea += areaF
                    fCnt += 1
                    zeroCOM = zeroCOM.add(
                        FreeCAD.Vector(comF.x, comF.y, 0.0).multiply(areaF)
                    )
                if fCnt == 0:
                    msg = translate(
                        "PathSurfaceSupport", "Cannot calculate the Center Of Mass."
                    )
                    msg += (
                        " "
                        + translate(
                            "PathSurfaceSupport", "Using Center of Boundbox instead."
                        )
                        + "\n"
                    )
                    FreeCAD.Console.PrintError(msg)
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
        self.deltaC = (
            self.shape.BoundBox.DiagonalLength
        )  # math.sqrt(self.deltaX**2 + self.deltaY**2)
        lineLen = self.deltaC + (
            2.0 * self.toolDiam
        )  # Line length to span boundbox diag with 2x cutter diameter extra on each end
        self.halfDiag = math.ceil(lineLen / 2.0)
        cutPasses = (
            math.ceil(lineLen / self.cutOut) + 1
        )  # Number of lines(passes) required to cover boundbox diagonal
        self.halfPasses = math.ceil(cutPasses / 2.0)

    # Public methods
    def setDebugObjectsGroup(self, tmpGrpObject):
        """setDebugObjectsGroup(tmpGrpObject)...
        Pass the temporary object group to show temporary construction objects"""
        self.debugObjectsGroup = tmpGrpObject

    def getCenterOfPattern(self):
        """getCenterOfPattern()...
        Returns the Center Of Mass for the current class instance."""
        return self.centerOfPattern

    def generatePathGeometry(self):
        """generatePathGeometry()...
        Call this function to obtain the path geometry shape, generated by this class."""
        if self.pattern == "None":
            return False

        if self.shape is None:
            return False

        cmd = "self._" + self.pattern + "()"
        exec(cmd)

        if self.obj.CutPatternReversed is True:
            self.rawGeoList.reverse()

        # Create compound object to bind all lines in Lineset
        geomShape = Part.makeCompound(self.rawGeoList)

        # Position and rotate the Line and ZigZag geometry
        if self.pattern in ["Line", "ZigZag"]:
            if self.obj.CutPatternAngle != 0.0:
                geomShape.Placement.Rotation = FreeCAD.Rotation(
                    FreeCAD.Vector(0, 0, 1), self.obj.CutPatternAngle
                )
            bbC = self.shape.BoundBox.Center
            geomShape.Placement.Base = FreeCAD.Vector(
                bbC.x, bbC.y, 0.0 - geomShape.BoundBox.ZMin
            )

        if self.debugObjectsGroup:
            F = FreeCAD.ActiveDocument.addObject("Part::Feature", "tmpGeometrySet")
            F.Shape = geomShape
            F.purgeTouched()
            self.debugObjectsGroup.addObject(F)

        if self.pattern == "Offset":
            return geomShape

        # Identify intersection of cross-section face and lineset
        cmnShape = self.shape.common(geomShape)

        if self.debugObjectsGroup:
            F = FreeCAD.ActiveDocument.addObject("Part::Feature", "tmpPathGeometry")
            F.Shape = cmnShape
            F.purgeTouched()
            self.debugObjectsGroup.addObject(F)

        return cmnShape

    # Cut pattern methods
    def _Circular(self):
        GeoSet = []
        radialPasses = self._getRadialPasses()
        minRad = self.toolDiam * 0.45
        siX3 = 3 * self.obj.SampleInterval.Value
        minRadSI = (siX3 / 2.0) / math.pi

        if minRad < minRadSI:
            minRad = minRadSI

        Path.Log.debug(" -centerOfPattern: {}".format(self.centerOfPattern))
        # Make small center circle to start pattern
        if self.obj.StepOver > 50:
            circle = Part.makeCircle(minRad, self.centerOfPattern)
            GeoSet.append(circle)

        for lc in range(1, radialPasses + 1):
            rad = lc * self.cutOut
            if rad >= minRad:
                circle = Part.makeCircle(rad, self.centerOfPattern)
                GeoSet.append(circle)
        # Efor
        self.rawGeoList = GeoSet

    def _CircularZigZag(self):
        self._Circular()  # Use _Circular generator

    def _Line(self):
        GeoSet = []
        centRot = FreeCAD.Vector(
            0.0, 0.0, 0.0
        )  # Bottom left corner of face/selection/model

        # Create end points for set of lines to intersect with cross-section face
        pntTuples = []
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
        GeoSet = []
        SEGS = []
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
        segLen = (
            self.obj.SampleInterval.Value
        )  # CutterDiameter / 10.0  # SampleInterval.Value
        stepAng = segLen / ((loopCnt + 1) * self.cutOut)  # math.pi / 18.0  # 10 degrees
        stopRadians = maxDist / cutOut

        if self.obj.CutPatternReversed:
            if self.obj.CutMode == "Conventional":
                getPoint = self._makeOppSpiralPnt
            else:
                getPoint = self._makeRegSpiralPnt

            while draw:
                radAng = sumRadians + stepAng
                p1 = lastPoint
                p2 = getPoint(
                    move, cutOut, radAng
                )  # cutOut is 'b' in the equation r = b * radAng
                sumRadians += stepAng  # Increment sumRadians
                loopRadians += stepAng  # Increment loopRadians
                if loopRadians > twoPi:
                    loopCnt += 1
                    loopRadians -= twoPi
                    stepAng = segLen / (
                        (loopCnt + 1) * self.cutOut
                    )  # adjust stepAng with each loop/cycle
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
            if self.obj.CutMode == "Climb":
                getPoint = self._makeOppSpiralPnt
            else:
                getPoint = self._makeRegSpiralPnt

            while draw:
                radAng = sumRadians + stepAng
                p1 = lastPoint
                p2 = getPoint(
                    move, cutOut, radAng
                )  # cutOut is 'b' in the equation r = b * radAng
                sumRadians += stepAng  # Increment sumRadians
                loopRadians += stepAng  # Increment loopRadians
                if loopRadians > twoPi:
                    loopCnt += 1
                    loopRadians -= twoPi
                    stepAng = segLen / (
                        (loopCnt + 1) * self.cutOut
                    )  # adjust stepAng with each loop/cycle
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

        if centerAt == "CenterOfMass":
            cntrPnt = FreeCAD.Vector(self.centerOfMass.x, self.centerOfMass.y, 0.0)
        elif centerAt == "CenterOfBoundBox":
            cent = self.shape.BoundBox.Center
            cntrPnt = FreeCAD.Vector(cent.x, cent.y, 0.0)
        elif centerAt == "XminYmin":
            cntrPnt = FreeCAD.Vector(
                self.shape.BoundBox.XMin, self.shape.BoundBox.YMin, 0.0
            )
        elif centerAt == "Custom":
            cntrPnt = FreeCAD.Vector(
                self.obj.PatternCenterCustom.x, self.obj.PatternCenterCustom.y, 0.0
            )

        # Update centerOfPattern point
        if centerAt != "Custom":
            self.obj.PatternCenterCustom = cntrPnt
        self.centerOfPattern = cntrPnt

        return cntrPnt

    def _getRadialPasses(self):
        # recalculate number of passes, if need be
        radialPasses = self.halfPasses
        if self.obj.PatternCenterAt != "CenterOfBoundBox":
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
            diag = dMax + (
                2.0 * self.toolDiam
            )  # Line length to span boundbox diag with 2x cutter diameter extra on each end
            radialPasses = (
                math.ceil(diag / self.cutOut) + 1
            )  # Number of lines(passes) required to cover boundbox diagonal

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
        Path.Log.debug("_extractOffsetFaces()")
        wires = []
        shape = self.shape
        offset = 0.0  # Start right at the edge of cut area
        direction = 0
        loop_cnt = 0

        def _get_direction(w):
            if PathOpUtil._isWireClockwise(w):
                return 1
            return -1

        def _reverse_wire(w):
            rev_list = []
            for e in w.Edges:
                rev_list.append(PathUtils.reverseEdge(e))
            rev_list.reverse()
            # return Part.Wire(Part.__sortEdges__(rev_list))
            return Part.Wire(rev_list)

        while True:
            offsetArea = PathUtils.getOffsetArea(shape, offset, plane=self.wpc)
            if not offsetArea:
                # Area fully consumed
                break

            # set initial cut direction
            if direction == 0:
                first_face_wire = offsetArea.Faces[0].Wires[0]
                direction = _get_direction(first_face_wire)
                if self.obj.CutMode == "Climb":
                    if direction == 1:
                        direction = -1
                else:
                    if direction == -1:
                        direction = 1

            # Correct cut direction for `Conventional` cuts
            if self.obj.CutMode == "Conventional":
                if loop_cnt == 1:
                    direction = direction * -1

            # process each wire within face
            for f in offsetArea.Faces:
                wire_cnt = 0
                for w in f.Wires:
                    use_direction = direction
                    if wire_cnt > 0:
                        # swap direction for internal features
                        use_direction = direction * -1
                    wire_direction = _get_direction(w)
                    # Process wire
                    if wire_direction == use_direction:
                        # direction is correct
                        wires.append(w)
                    else:
                        # incorrect direction, so reverse wire
                        rw = _reverse_wire(w)
                        wires.append(rw)

            offset -= self.cutOut
            loop_cnt += 1
        return wires


# Eclass


class ProcessSelectedFaces:
    """ProcessSelectedFaces(JOB, obj) class.
    This class processes the `obj.Base` object for selected geometery.
    Calling the preProcessModel(module) method returns
    two compound objects as a tuple: (FACES, VOIDS) or False."""

    def __init__(self, JOB, obj):
        self.modelSTLs = []
        self.profileShapes = []
        self.tempGroup = False
        self.showDebugObjects = False
        self.checkBase = False
        self.module = None
        self.radius = None
        self.depthParams = None
        self.msgNoFaces = (
            translate(
                "PathSurfaceSupport",
                "Face selection is unavailable for Rotational scans.",
            )
            + "\n"
        )
        self.msgNoFaces += (
            " " + translate("PathSurfaceSupport", "Ignoring selected faces.") + "\n"
        )
        self.JOB = JOB
        self.obj = obj
        self.profileEdges = "None"

        if hasattr(obj, "ProfileEdges"):
            self.profileEdges = obj.ProfileEdges

        # Setup STL, model type, and bound box containers for each model in Job
        for m in range(0, len(JOB.Model.Group)):
            self.modelSTLs.append(False)
            self.profileShapes.append(False)

        # make circle for workplane
        self.wpc = Part.makeCircle(2.0)

    def PathSurface(self):
        if self.obj.Base:
            if len(self.obj.Base) > 0:
                self.checkBase = True
                if self.obj.ScanType == "Rotational":
                    self.checkBase = False
                    FreeCAD.Console.PrintWarning(self.msgNoFaces)

    def PathWaterline(self):
        if self.obj.Base:
            if len(self.obj.Base) > 0:
                self.checkBase = True
                if self.obj.Algorithm in ["OCL Dropcutter", "Experimental"]:
                    self.checkBase = False
                    FreeCAD.Console.PrintWarning(self.msgNoFaces)

    # public class methods
    def setShowDebugObjects(self, grpObj, val):
        self.tempGroup = grpObj
        self.showDebugObjects = val

    def preProcessModel(self, module):
        Path.Log.debug("preProcessModel()")

        if not self._isReady(module):
            return False

        FACES = []
        VOIDS = []
        fShapes = []
        vShapes = []
        GRP = self.JOB.Model.Group
        lenGRP = len(GRP)
        proceed = False

        # Crete place holders for each base model in Job
        for m in range(0, lenGRP):
            FACES.append(False)
            VOIDS.append(False)
            fShapes.append(False)
            vShapes.append(False)

        # The user has selected subobjects from the base.  Pre-Process each.
        if self.checkBase:
            Path.Log.debug(" -obj.Base exists. Pre-processing for selected faces.")

            (hasFace, hasVoid) = self._identifyFacesAndVoids(
                FACES, VOIDS
            )  # modifies FACES and VOIDS
            hasGeometry = True if hasFace or hasVoid else False

            # Cycle through each base model, processing faces for each
            for m in range(0, lenGRP):
                base = GRP[m]
                (mFS, mVS, mPS) = self._preProcessFacesAndVoids(
                    base, FACES[m], VOIDS[m]
                )
                fShapes[m] = mFS
                vShapes[m] = mVS
                self.profileShapes[m] = mPS
                if mFS or mVS:
                    proceed = True
            if hasGeometry and not proceed:
                return False
        else:
            Path.Log.debug(" -No obj.Base data.")
            for m in range(0, lenGRP):
                self.modelSTLs[m] = True

        # Process each model base, as a whole, as needed
        for m in range(0, lenGRP):
            if self.modelSTLs[m] and not fShapes[m]:
                Path.Log.debug(" -Pre-processing {} as a whole.".format(GRP[m].Label))
                if self.obj.BoundBox == "BaseBoundBox":
                    base = GRP[m]
                elif self.obj.BoundBox == "Stock":
                    base = self.JOB.Stock

                pPEB = self._preProcessEntireBase(base, m)
                if pPEB is False:
                    msg = (
                        translate(
                            "PathSurfaceSupport",
                            "Failed to pre-process base as a whole.",
                        )
                        + "\n"
                    )
                    FreeCAD.Console.PrintError(msg)
                else:
                    (fcShp, prflShp) = pPEB
                    if fcShp:
                        if fcShp is True:
                            Path.Log.debug(" -fcShp is True.")
                            fShapes[m] = True
                        else:
                            fShapes[m] = [fcShp]
                    if prflShp:
                        if fcShp:
                            Path.Log.debug("vShapes[{}]: {}".format(m, vShapes[m]))
                            if vShapes[m]:
                                Path.Log.debug(
                                    " -Cutting void from base profile shape."
                                )
                                adjPS = prflShp.cut(vShapes[m][0])
                                self.profileShapes[m] = [adjPS]
                            else:
                                Path.Log.debug(" -vShapes[m] is False.")
                                self.profileShapes[m] = [prflShp]
                        else:
                            Path.Log.debug(" -Saving base profile shape.")
                            self.profileShapes[m] = [prflShp]
                        Path.Log.debug(
                            "self.profileShapes[{}]: {}".format(
                                m, self.profileShapes[m]
                            )
                        )
        # Efor

        return (fShapes, vShapes)

    # private class methods
    def _isReady(self, module):
        """_isReady(module)... Internal method.
        Checks if required attributes are available for processing obj.Base (the Base Geometry)."""
        Path.Log.debug("ProcessSelectedFaces _isReady({})".format(module))
        modMethodName = module.replace("Op.", "Path")
        if hasattr(self, modMethodName):
            self.module = module
            modMethod = getattr(self, modMethodName)  # gets the attribute only
            modMethod()  # executes as method
        else:
            Path.Log.error('PSF._isReady() no "{}" method.'.format(module))
            return False

        if not self.radius:
            Path.Log.error("PSF._isReady() no cutter radius available.")
            return False

        if not self.depthParams:
            Path.Log.error("PSF._isReady() no depth params available.")
            return False

        return True

    def _identifyFacesAndVoids(self, F, V):
        TUPS = []
        GRP = self.JOB.Model.Group
        lenGRP = len(GRP)
        hasFace = False
        hasVoid = False

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
                        F[m] = []
                    F[m].append((shape, faceIdx))
                    Path.Log.debug(".. Cutting {}".format(sub))
                    hasFace = True
                else:
                    if V[m] is False:
                        V[m] = []
                    V[m].append((shape, faceIdx))
                    Path.Log.debug(".. Avoiding {}".format(sub))
                    hasVoid = True
        return (hasFace, hasVoid)

    def _preProcessFacesAndVoids(self, base, FCS, VDS):
        mFS = False
        mVS = False
        mPS = False
        mIFS = []

        if FCS:
            isHole = False
            if self.obj.HandleMultipleFeatures == "Collectively":
                cont = True
                Path.Log.debug("Attempting to get cross-section of collective faces.")
                outFCS, ifL = self.findUnifiedRegions(FCS)
                if self.obj.InternalFeaturesCut and ifL:
                    ifL = []  # clear avoid shape list

                if len(outFCS) == 0:
                    msg = "PathSurfaceSupport \n Cannot process selected faces. Check horizontal \n surface exposure.\n"
                    FreeCAD.Console.PrintError(msg)
                    cont = False
                else:
                    cfsL = Part.makeCompound(outFCS)

                # Handle profile edges request
                if cont and self.profileEdges != "None":
                    Path.Log.debug(".. include Profile Edge")
                    ofstVal = self._calculateOffsetValue(isHole)
                    psOfst = PathUtils.getOffsetArea(cfsL, ofstVal, plane=self.wpc)
                    if psOfst:
                        mPS = [psOfst]
                        if self.profileEdges == "Only":
                            mFS = True
                            cont = False
                    else:
                        cont = False

                if cont:
                    if self.showDebugObjects:
                        T = FreeCAD.ActiveDocument.addObject(
                            "Part::Feature", "tmpCollectiveShape"
                        )
                        T.Shape = cfsL
                        T.purgeTouched()
                        self.tempGroup.addObject(T)

                    ofstVal = self._calculateOffsetValue(isHole)
                    faceOfstShp = PathUtils.getOffsetArea(cfsL, ofstVal, plane=self.wpc)
                    if not faceOfstShp:
                        msg = "Failed to create offset face."
                        FreeCAD.Console.PrintError(msg)
                        cont = False

                if cont:
                    lenIfL = len(ifL)
                    if not self.obj.InternalFeaturesCut:
                        if lenIfL == 0:
                            Path.Log.debug(" -No internal features saved.")
                        else:
                            if lenIfL == 1:
                                casL = ifL[0]
                            else:
                                casL = Part.makeCompound(ifL)
                            if self.showDebugObjects:
                                C = FreeCAD.ActiveDocument.addObject(
                                    "Part::Feature", "tmpCompoundIntFeat"
                                )
                                C.Shape = casL
                                C.purgeTouched()
                                self.tempGroup.addObject(C)
                            ofstVal = self._calculateOffsetValue(isHole=True)
                            intOfstShp = PathUtils.getOffsetArea(
                                casL, ofstVal, plane=self.wpc
                            )
                            mIFS.append(intOfstShp)

                    mFS = [faceOfstShp]
                # Eif

            elif self.obj.HandleMultipleFeatures == "Individually":
                for (fcshp, fcIdx) in FCS:
                    cont = True
                    fNum = fcIdx + 1
                    outerFace = False

                    gUR, ifL = self.findUnifiedRegions(FCS)
                    if len(gUR) > 0:
                        outerFace = gUR[0]
                    if self.obj.InternalFeaturesCut:
                        ifL = []  # avoid shape list

                    if outerFace:
                        Path.Log.debug(
                            "Attempting to create offset face of Face{}".format(fNum)
                        )

                        if self.profileEdges != "None":
                            ofstVal = self._calculateOffsetValue(isHole)
                            psOfst = PathUtils.getOffsetArea(
                                outerFace, ofstVal, plane=self.wpc
                            )
                            if psOfst:
                                if mPS is False:
                                    mPS = []
                                mPS.append(psOfst)
                                if self.profileEdges == "Only":
                                    if mFS is False:
                                        mFS = []
                                    mFS.append(True)
                                    cont = False
                            else:
                                cont = False

                        if cont:
                            ofstVal = self._calculateOffsetValue(isHole)
                            faceOfstShp = PathUtils.getOffsetArea(
                                outerFace, ofstVal, plane=self.wpc
                            )

                            lenIfl = len(ifL)
                            if self.obj.InternalFeaturesCut is False and lenIfl > 0:
                                if lenIfl == 1:
                                    casL = ifL[0]
                                else:
                                    casL = Part.makeCompound(ifL)

                                ofstVal = self._calculateOffsetValue(isHole=True)
                                intOfstShp = PathUtils.getOffsetArea(
                                    casL, ofstVal, plane=self.wpc
                                )
                                mIFS.append(intOfstShp)
                                # faceOfstShp = faceOfstShp.cut(intOfstShp)

                            if mFS is False:
                                mFS = []
                            mFS.append(faceOfstShp)
                    # Eif
                # Efor
            # Eif
        # Eif

        if len(mIFS) > 0:
            if mVS is False:
                mVS = []
            for ifs in mIFS:
                mVS.append(ifs)

        if VDS:
            Path.Log.debug("Processing avoid faces.")
            cont = True
            isHole = False

            outFCS, intFEAT = self.findUnifiedRegions(VDS)
            if self.obj.InternalFeaturesCut:
                intFEAT = []

            lenOtFcs = len(outFCS)
            if lenOtFcs == 0:
                cont = False
            else:
                if lenOtFcs == 1:
                    avoid = outFCS[0]
                else:
                    avoid = Part.makeCompound(outFCS)

                if self.showDebugObjects:
                    P = FreeCAD.ActiveDocument.addObject(
                        "Part::Feature", "tmpVoidEnvelope"
                    )
                    P.Shape = avoid
                    P.purgeTouched()
                    self.tempGroup.addObject(P)

            if cont:
                if self.showDebugObjects:
                    P = FreeCAD.ActiveDocument.addObject(
                        "Part::Feature", "tmpVoidCompound"
                    )
                    P.Shape = avoid
                    P.purgeTouched()
                    self.tempGroup.addObject(P)
                ofstVal = self._calculateOffsetValue(isHole, isVoid=True)
                avdOfstShp = PathUtils.getOffsetArea(avoid, ofstVal, plane=self.wpc)
                if avdOfstShp is False:
                    msg = "Failed to create collective offset avoid face.\n"
                    FreeCAD.Console.PrintError(msg)
                    cont = False

            if cont:
                avdShp = avdOfstShp

                if not self.obj.AvoidLastX_InternalFeatures and len(intFEAT) > 0:
                    if len(intFEAT) > 1:
                        ifc = Part.makeCompound(intFEAT)
                    else:
                        ifc = intFEAT[0]
                    ofstVal = self._calculateOffsetValue(isHole=True)
                    ifOfstShp = PathUtils.getOffsetArea(ifc, ofstVal, plane=self.wpc)
                    if ifOfstShp is False:
                        msg = "Failed to create collective offset avoid internal features.\n"
                        FreeCAD.Console.PrintError(msg)
                    else:
                        avdShp = avdOfstShp.cut(ifOfstShp)

                if mVS is False:
                    mVS = []
                mVS.append(avdShp)

        return (mFS, mVS, mPS)

    def _preProcessEntireBase(self, base, m):
        cont = True
        isHole = False
        prflShp = False
        # Create envelope, extract cross-section and make offset co-planar shape
        # baseEnv = PathUtils.getEnvelope(base.Shape, subshape=None, depthparams=self.depthParams)

        try:
            baseEnv = PathUtils.getEnvelope(
                partshape=base.Shape, subshape=None, depthparams=self.depthParams
            )  # Produces .Shape
        except Exception as ee:
            Path.Log.error(str(ee))
            shell = base.Shape.Shells[0]
            solid = Part.makeSolid(shell)
            try:
                baseEnv = PathUtils.getEnvelope(
                    partshape=solid, subshape=None, depthparams=self.depthParams
                )  # Produces .Shape
            except Exception as eee:
                Path.Log.error(str(eee))
                cont = False

        if cont:
            csFaceShape = getShapeSlice(baseEnv)
            if csFaceShape is False:
                csFaceShape = getCrossSection(baseEnv)
                if csFaceShape is False:
                    csFaceShape = getSliceFromEnvelope(baseEnv)
            if csFaceShape is False:
                Path.Log.debug("Failed to slice baseEnv shape.")
                cont = False

        if cont and self.profileEdges != "None":
            Path.Log.debug(" -Attempting profile geometry for model base.")
            ofstVal = self._calculateOffsetValue(isHole)
            psOfst = PathUtils.getOffsetArea(csFaceShape, ofstVal, plane=self.wpc)
            if psOfst:
                if self.profileEdges == "Only":
                    return (True, psOfst)
                prflShp = psOfst
            else:
                cont = False

        if cont:
            ofstVal = self._calculateOffsetValue(isHole)
            faceOffsetShape = PathUtils.getOffsetArea(
                csFaceShape, ofstVal, plane=self.wpc
            )
            if faceOffsetShape is False:
                Path.Log.debug("getOffsetArea() failed for entire base.")
            else:
                faceOffsetShape.translate(
                    FreeCAD.Vector(0.0, 0.0, 0.0 - faceOffsetShape.BoundBox.ZMin)
                )
                return (faceOffsetShape, prflShp)
        return False

    def _calculateOffsetValue(self, isHole, isVoid=False):
        """_calculateOffsetValue(self.obj, isHole, isVoid) ... internal function.
        Calculate the offset for the Path.Area() function."""
        self.JOB = PathUtils.findParentJob(self.obj)
        # We need to offset by at least our linear tessellation deflection
        # (default GeometryTolerance / 4) to avoid false retracts at the
        # boundaries.
        tolrnc = max(
            self.JOB.GeometryTolerance.Value / 10.0, self.obj.LinearDeflection.Value
        )

        if isVoid is False:
            if isHole is True:
                offset = -1 * self.obj.InternalFeaturesAdjustment.Value
                offset += self.radius + tolrnc
            else:
                offset = -1 * self.obj.BoundaryAdjustment.Value
                if self.obj.BoundaryEnforcement is True:
                    offset += self.radius + tolrnc
                else:
                    offset -= self.radius + tolrnc
                offset = 0.0 - offset
        else:
            offset = -1 * self.obj.BoundaryAdjustment.Value
            offset += self.radius + tolrnc

        return offset

    def findUnifiedRegions(self, shapeAndIndexTuples, useAreaImplementation=True):
        """Wrapper around area and wire based region unification
        implementations."""
        Path.Log.debug("findUnifiedRegions()")
        # Allow merging of faces within the LinearDeflection tolerance.
        tolerance = self.obj.LinearDeflection.Value
        # Default: normal to Z=1 (XY plane), at Z=0
        try:
            # Use Area based implementation
            shapes = Part.makeCompound([t[0] for t in shapeAndIndexTuples])
            outlineShape = PathUtils.getOffsetArea(
                shapes,
                # Make the outline very slightly smaller, to avoid creating
                # small edges in the cut with the hole-preserving projection.
                0.0 - tolerance / 10,
                removeHoles=True,  # Outline has holes filled in
                tolerance=tolerance,
                plane=self.wpc,
            )
            projectionShape = PathUtils.getOffsetArea(
                shapes,
                # Make the projection very slightly larger
                tolerance / 10,
                removeHoles=False,  # Projection has holes preserved
                tolerance=tolerance,
                plane=self.wpc,
            )
            internalShape = outlineShape.cut(projectionShape)
            # Filter out tiny faces, usually artifacts around the perimeter of
            # the cut.
            minArea = (10 * tolerance) ** 2
            internalFaces = [f for f in internalShape.Faces if f.Area > minArea]
            if internalFaces:
                internalFaces = Part.makeCompound(internalFaces)
            return ([outlineShape], [internalFaces])
        except Exception as e:
            Path.Log.warning(
                "getOffsetArea failed: {}; Using FindUnifiedRegions.".format(e)
            )
        # Use face-unifying class
        FUR = FindUnifiedRegions(shapeAndIndexTuples, tolerance)
        if self.showDebugObjects:
            FUR.setTempGroup(self.tempGroup)
        return (FUR.getUnifiedRegions(), FUR.getInternalFeatures)


# Eclass


# Functions for getting a shape envelope and cross-section
def getExtrudedShape(wire):
    Path.Log.debug("getExtrudedShape()")
    wBB = wire.BoundBox
    extFwd = math.floor(2.0 * wBB.ZLength) + 10.0

    try:
        shell = wire.extrude(FreeCAD.Vector(0.0, 0.0, extFwd))
    except Exception as ee:
        Path.Log.error(" -extrude wire failed: \n{}".format(ee))
        return False

    SHP = Part.makeSolid(shell)
    return SHP


def getShapeSlice(shape):
    Path.Log.debug("getShapeSlice()")

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
                Path.Log.debug(" -wire.isClosed() is False")
                return False
        if len(slcShp.Wires) == 1:
            wire = slcShp.Wires[0]
            slc = Part.Face(wire)
            slc.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - slc.BoundBox.ZMin))
            return slc
        else:
            fL = []
            for W in slcShp.Wires:
                slc = Part.Face(W)
                slc.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - slc.BoundBox.ZMin))
                fL.append(slc)
            comp = Part.makeCompound(fL)
            return comp

    return False


def getProjectedFace(tempGroup, wire):
    import Draft

    Path.Log.debug("getProjectedFace()")
    F = FreeCAD.ActiveDocument.addObject("Part::Feature", "tmpProjectionWire")
    F.Shape = wire
    F.purgeTouched()
    tempGroup.addObject(F)
    try:
        prj = Draft.makeShape2DView(F, FreeCAD.Vector(0, 0, 1))
        prj.recompute()
        prj.purgeTouched()
        tempGroup.addObject(prj)
    except Exception as ee:
        Path.Log.error(str(ee))
        return False
    else:
        pWire = Part.Wire(prj.Shape.Edges)
        if pWire.isClosed() is False:
            return False
        slc = Part.Face(pWire)
        slc.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - slc.BoundBox.ZMin))
        return slc


def getCrossSection(shape):
    Path.Log.debug("getCrossSection()")
    wires = []
    bb = shape.BoundBox
    mid = (bb.ZMin + bb.ZMax) / 2.0

    for i in shape.slice(FreeCAD.Vector(0, 0, 1), mid):
        wires.append(i)

    if len(wires) > 0:
        comp = Part.Compound(wires)  # produces correct cross-section wire !
        comp.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - comp.BoundBox.ZMin))
        csWire = comp.Wires[0]
        if csWire.isClosed() is False:
            Path.Log.debug(" -comp.Wires[0] is not closed")
            return False
        CS = Part.Face(csWire)
        CS.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - CS.BoundBox.ZMin))
        return CS
    else:
        Path.Log.debug(" -No wires from .slice() method")

    return False


def getShapeEnvelope(shape):
    Path.Log.debug("getShapeEnvelope()")

    wBB = shape.BoundBox
    extFwd = wBB.ZLength + 10.0
    minz = wBB.ZMin
    maxz = wBB.ZMin + extFwd
    stpDwn = (maxz - minz) / 4.0
    dep_par = PathUtils.depth_params(maxz + 5.0, maxz + 3.0, maxz, stpDwn, 0.0, minz)

    try:
        env = PathUtils.getEnvelope(
            partshape=shape, depthparams=dep_par
        )  # Produces .Shape
    except Exception as ee:
        FreeCAD.Console.PrintError("PathUtils.getEnvelope() failed.\n" + str(ee) + "\n")
        return False
    else:
        return env


def getSliceFromEnvelope(env):
    Path.Log.debug("getSliceFromEnvelope()")
    eBB = env.BoundBox
    extFwd = eBB.ZLength + 10.0
    maxz = eBB.ZMin + extFwd

    emax = math.floor(maxz - 1.0)
    E = []
    for e in range(0, len(env.Edges)):
        emin = env.Edges[e].BoundBox.ZMin
        if emin > emax:
            E.append(env.Edges[e])
    tf = Part.Face(Part.Wire(Part.__sortEdges__(E)))
    tf.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - tf.BoundBox.ZMin))

    return tf


def _prepareModelSTLs(self, JOB, obj, m, ocl):
    """Tessellate model shapes or copy existing meshes into ocl.STLSurf
    objects"""
    if self.modelSTLs[m] is True:
        model = JOB.Model.Group[m]
        self.modelSTLs[m] = _makeSTL(model, obj, ocl, self.modelTypes[m])


def _makeSafeSTL(self, JOB, obj, mdlIdx, faceShapes, voidShapes, ocl):
    """_makeSafeSTL(JOB, obj, mdlIdx, faceShapes, voidShapes)...
    Creates and OCL.stl object with combined data with waste stock,
    model, and avoided faces.  Travel lines can be checked against this
    STL object to determine minimum travel height to clear stock and model."""
    Path.Log.debug("_makeSafeSTL()")

    fuseShapes = []
    Mdl = JOB.Model.Group[mdlIdx]
    mBB = Mdl.Shape.BoundBox
    sBB = JOB.Stock.Shape.BoundBox

    # add Model shape to safeSTL shape
    fuseShapes.append(Mdl.Shape)

    if obj.BoundBox == "BaseBoundBox":
        cont = False
        extFwd = sBB.ZLength
        zmin = mBB.ZMin
        zmax = mBB.ZMin + extFwd
        stpDwn = (zmax - zmin) / 4.0
        dep_par = PathUtils.depth_params(
            zmax + 5.0, zmax + 3.0, zmax, stpDwn, 0.0, zmin
        )

        try:
            envBB = PathUtils.getEnvelope(
                partshape=Mdl.Shape, depthparams=dep_par
            )  # Produces .Shape
            cont = True
        except Exception as ee:
            Path.Log.error(str(ee))
            shell = Mdl.Shape.Shells[0]
            solid = Part.makeSolid(shell)
            try:
                envBB = PathUtils.getEnvelope(
                    partshape=solid, depthparams=dep_par
                )  # Produces .Shape
                cont = True
            except Exception as eee:
                Path.Log.error(str(eee))

        if cont:
            stckWst = JOB.Stock.Shape.cut(envBB)
            if obj.BoundaryAdjustment > 0.0:
                cmpndFS = Part.makeCompound(faceShapes)
                baBB = PathUtils.getEnvelope(
                    partshape=cmpndFS, depthparams=self.depthParams
                )  # Produces .Shape
                adjStckWst = stckWst.cut(baBB)
            else:
                adjStckWst = stckWst
            fuseShapes.append(adjStckWst)
        else:
            msg = "Path transitions might not avoid the model. Verify paths.\n"
            FreeCAD.Console.PrintWarning(msg)
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

    if voidShapes:
        voidComp = Part.makeCompound(voidShapes)
        voidEnv = PathUtils.getEnvelope(
            partshape=voidComp, depthparams=self.depthParams
        )  # Produces .Shape
        fuseShapes.append(voidEnv)

    fused = Part.makeCompound(fuseShapes)

    if self.showDebugObjects:
        T = FreeCAD.ActiveDocument.addObject("Part::Feature", "safeSTLShape")
        T.Shape = fused
        T.purgeTouched()
        self.tempGroup.addObject(T)

    self.safeSTLs[mdlIdx] = _makeSTL(fused, obj, ocl)


def _makeSTL(model, obj, ocl, model_type=None):
    """Convert a mesh or shape into an OCL STL, using the tessellation
    tolerance specified in obj.LinearDeflection.
    Returns an ocl.STLSurf()."""
    if model_type == "M":
        facets = model.Mesh.Facets.Points
    else:
        if hasattr(model, "Shape"):
            shape = model.Shape
        else:
            shape = model
        vertices, facet_indices = shape.tessellate(obj.LinearDeflection.Value)
        facets = (
            (vertices[f[0]], vertices[f[1]], vertices[f[2]]) for f in facet_indices
        )
    stl = ocl.STLSurf()
    for tri in facets:
        v1, v2, v3 = tri
        t = ocl.Triangle(
            ocl.Point(v1[0], v1[1], v1[2]),
            ocl.Point(v2[0], v2[1], v2[2]),
            ocl.Point(v3[0], v3[1], v3[2]),
        )
        stl.addTriangle(t)
    return stl


# Functions to convert path geometry into line/arc segments for OCL input or directly to g-code
def pathGeomToLinesPointSet(self, obj, compGeoShp):
    """pathGeomToLinesPointSet(self, obj, compGeoShp)...
    Convert a compound set of sequential line segments to directionally-oriented collinear groupings."""
    Path.Log.debug("pathGeomToLinesPointSet()")
    # Extract intersection line segments for return value as []
    LINES = []
    inLine = []
    chkGap = False
    lnCnt = 0
    ec = len(compGeoShp.Edges)
    cpa = obj.CutPatternAngle

    edg0 = compGeoShp.Edges[0]
    p1 = (edg0.Vertexes[0].X, edg0.Vertexes[0].Y)
    p2 = (edg0.Vertexes[1].X, edg0.Vertexes[1].Y)
    if self.CutClimb is True:
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
            inLine.append("BRK")
            chkGap = True
        else:
            if self.CutClimb is True:
                inLine.reverse()
            LINES.append(inLine)  # Save inLine segments
            lnCnt += 1
            inLine = []  # reset collinear container
            if self.CutClimb is True:
                sp = cp  # FreeCAD.Vector(v1[0], v1[1], 0.0)
            else:
                sp = ep

        if self.CutClimb is True:
            tup = (v2, v1)
            if chkGap:
                gap = abs(self.toolDiam - lst.sub(ep).Length)
            lst = cp
        else:
            tup = (v1, v2)
            if chkGap:
                gap = abs(self.toolDiam - lst.sub(cp).Length)
            lst = ep

        if chkGap:
            if gap < obj.GapThreshold.Value:
                inLine.pop()  # pop off 'BRK' marker
                (
                    vA,
                    vB,
                ) = (
                    inLine.pop()
                )  # pop off previous line segment for combining with current
                tup = (vA, tup[1])
                self.closedGap = True
            else:
                gap = round(gap, 6)
                if gap < self.gaps[0]:
                    self.gaps.insert(0, gap)
                    self.gaps.pop()
        inLine.append(tup)

    # Efor
    lnCnt += 1
    if self.CutClimb is True:
        inLine.reverse()
    LINES.append(inLine)  # Save inLine segments

    # Handle last inLine set, reversing it.
    if obj.CutPatternReversed is True:
        if cpa != 0.0 and cpa % 90.0 == 0.0:
            F = LINES.pop(0)
            rev = []
            for iL in F:
                if iL == "BRK":
                    rev.append(iL)
                else:
                    (p1, p2) = iL
                    rev.append((p2, p1))
            rev.reverse()
            LINES.insert(0, rev)

    isEven = lnCnt % 2
    if isEven == 0:
        Path.Log.debug("Line count is ODD: {}.".format(lnCnt))
    else:
        Path.Log.debug("Line count is even: {}.".format(lnCnt))

    return LINES


def pathGeomToZigzagPointSet(self, obj, compGeoShp):
    """_pathGeomToZigzagPointSet(self, obj, compGeoShp)...
    Convert a compound set of sequential line segments to directionally-oriented collinear groupings
    with a ZigZag directional indicator included for each collinear group."""
    Path.Log.debug("_pathGeomToZigzagPointSet()")
    # Extract intersection line segments for return value as []
    LINES = []
    inLine = []
    lnCnt = 0
    chkGap = False
    ec = len(compGeoShp.Edges)
    dirFlg = 1

    if self.CutClimb:
        dirFlg = -1

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
        iC = cp.isOnLineSegment(sp, ep)
        if iC:
            inLine.append("BRK")
            chkGap = True
            gap = abs(self.toolDiam - lst.sub(cp).Length)
        else:
            chkGap = False
            if dirFlg == -1:
                inLine.reverse()
            LINES.append(inLine)
            lnCnt += 1
            dirFlg = -1 * dirFlg  # Change zig to zag
            inLine = []  # reset collinear container
            sp = cp  # FreeCAD.Vector(v1[0], v1[1], 0.0)

        lst = ep
        if dirFlg == 1:
            tup = (v1, v2)
        else:
            tup = (v2, v1)

        if chkGap:
            if gap < obj.GapThreshold.Value:
                inLine.pop()  # pop off 'BRK' marker
                (
                    vA,
                    vB,
                ) = (
                    inLine.pop()
                )  # pop off previous line segment for combining with current
                if dirFlg == 1:
                    tup = (vA, tup[1])
                else:
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
        Path.Log.debug("Line count is even: {}.".format(lnCnt))
    else:
        Path.Log.debug("Line count is ODD: {}.".format(lnCnt))
        dirFlg = -1 * dirFlg
        if not obj.CutPatternReversed:
            if self.CutClimb:
                dirFlg = -1 * dirFlg

    if obj.CutPatternReversed:
        dirFlg = -1 * dirFlg

    # Handle last inLine list
    if dirFlg == 1:
        rev = []
        for iL in inLine:
            if iL == "BRK":
                rev.append(iL)
            else:
                (p1, p2) = iL
                rev.append((p2, p1))

        if not obj.CutPatternReversed:
            rev.reverse()
        else:
            rev2 = []
            for iL in rev:
                if iL == "BRK":
                    rev2.append(iL)
                else:
                    (p1, p2) = iL
                    rev2.append((p2, p1))
            rev2.reverse()
            rev = rev2
        LINES.append(rev)
    else:
        LINES.append(inLine)

    return LINES


def pathGeomToCircularPointSet(self, obj, compGeoShp):
    """pathGeomToCircularPointSet(self, obj, compGeoShp)...
    Convert a compound set of arcs/circles to a set of directionally-oriented arc end points
    and the corresponding center point."""
    # Extract intersection line segments for return value as []
    Path.Log.debug("pathGeomToCircularPointSet()")
    ARCS = []
    stpOvrEI = []
    segEI = []
    isSame = False
    sameRad = None
    ec = len(compGeoShp.Edges)

    def gapDist(sp, ep):
        X = (ep[0] - sp[0]) ** 2
        Y = (ep[1] - sp[1]) ** 2
        return math.sqrt(X + Y)  # the 'z' value is zero in both points

    def dist_to_cent(item):
        # Sort incoming arcs by distance to center
        # item: edge type, direction flag, parts tuple
        # parts: start tuple, end tuple, center tuple
        s = item[2][0][0]
        p1 = FreeCAD.Vector(s[0], s[1], 0.0)
        e = item[2][0][2]
        p2 = FreeCAD.Vector(e[0], e[1], 0.0)
        return p1.sub(p2).Length

    if obj.CutPatternReversed:
        if self.CutClimb:
            self.CutClimb = False
        else:
            self.CutClimb = True

    # Separate arc data into Loops and Arcs
    for ei in range(0, ec):
        edg = compGeoShp.Edges[ei]
        if edg.Closed is True:
            stpOvrEI.append(("L", ei, False))
        else:
            if isSame is False:
                segEI.append(ei)
                isSame = True
                pnt = FreeCAD.Vector(edg.Vertexes[0].X, edg.Vertexes[0].Y, 0.0)
                sameRad = pnt.sub(self.tmpCOM).Length
            else:
                # Check if arc is co-radial to current SEGS
                pnt = FreeCAD.Vector(edg.Vertexes[0].X, edg.Vertexes[0].Y, 0.0)
                if abs(sameRad - pnt.sub(self.tmpCOM).Length) > 0.00001:
                    isSame = False

                if isSame is True:
                    segEI.append(ei)
                else:
                    # Move co-radial arc segments
                    stpOvrEI.append(["A", segEI, False])
                    # Start new list of arc segments
                    segEI = [ei]
                    isSame = True
                    pnt = FreeCAD.Vector(edg.Vertexes[0].X, edg.Vertexes[0].Y, 0.0)
                    sameRad = pnt.sub(self.tmpCOM).Length
    # Process trailing `segEI` data, if available
    if isSame is True:
        stpOvrEI.append(["A", segEI, False])

    # Identify adjacent arcs with y=0 start/end points that connect
    for so in range(0, len(stpOvrEI)):
        SO = stpOvrEI[so]
        if SO[0] == "A":
            startOnAxis = []
            endOnAxis = []
            EI = SO[1]  # list of corresponding compGeoShp.Edges indexes

            # Identify startOnAxis and endOnAxis arcs
            for i in range(0, len(EI)):
                ei = EI[i]  # edge index
                E = compGeoShp.Edges[ei]  # edge object
                if abs(self.tmpCOM.y - E.Vertexes[0].Y) < 0.00001:
                    startOnAxis.append((i, ei, E.Vertexes[0]))
                elif abs(self.tmpCOM.y - E.Vertexes[1].Y) < 0.00001:
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
    if not self.CutClimb:  # True yields Climb when set to Conventional
        dirFlg = -1

    # Declare center point of circle pattern
    cp = (self.tmpCOM.x, self.tmpCOM.y, 0.0)

    # Cycle through stepOver data
    for so in range(0, len(stpOvrEI)):
        SO = stpOvrEI[so]
        if SO[0] == "L":  # L = Loop/Ring/Circle
            # Path.Log.debug("SO[0] == 'Loop'")
            lei = SO[1]  # loop Edges index
            v1 = compGeoShp.Edges[lei].Vertexes[0]

            # space = obj.SampleInterval.Value / 10.0
            # space = 0.000001
            space = (
                self.toolDiam * 0.005
            )  # If too small, OCL will fail to scan the loop

            # p1 = FreeCAD.Vector(v1.X, v1.Y, v1.Z)
            p1 = FreeCAD.Vector(
                v1.X, v1.Y, 0.0
            )  # z=0.0 for waterline; z=v1.Z for 3D Surface
            rad = p1.sub(self.tmpCOM).Length
            spcRadRatio = space / rad
            if spcRadRatio < 1.0:
                tolrncAng = math.asin(spcRadRatio)
            else:
                tolrncAng = 0.99999998 * math.pi
            EX = self.tmpCOM.x + (rad * math.cos(tolrncAng))
            EY = v1.Y - space  # rad * math.sin(tolrncAng)

            sp = (v1.X, v1.Y, 0.0)
            ep = (EX, EY, 0.0)
            if dirFlg == 1:
                arc = (sp, ep, cp)
            else:
                arc = (
                    ep,
                    sp,
                    cp,
                )  # OCL.Arc(firstPnt, lastPnt, centerPnt, dir=True(CCW direction))
            ARCS.append(("L", dirFlg, [arc]))
        elif SO[0] == "A":  # A = Arc
            # Path.Log.debug("SO[0] == 'Arc'")
            PRTS = []
            EI = SO[1]  # list of corresponding Edges indexes
            CONN = SO[2]  # list of corresponding connected edges tuples (iE, iS)
            chkGap = False
            lst = None

            if CONN:  # Connected edges(arcs)
                (iE, iS) = CONN
                v1 = compGeoShp.Edges[iE].Vertexes[0]
                v2 = compGeoShp.Edges[iS].Vertexes[1]
                sp = (v1.X, v1.Y, 0.0)
                ep = (v2.X, v2.Y, 0.0)
                if dirFlg == 1:
                    arc = (sp, ep, cp)
                    lst = ep
                else:
                    arc = (
                        ep,
                        sp,
                        cp,
                    )  # OCL.Arc(firstPnt, lastPnt, centerPnt, dir=True(CCW direction))
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
                    PRTS.append("BRK")
                    chkGap = True
            cnt = 0
            for ei in EI:
                if cnt > 0:
                    PRTS.append("BRK")
                    chkGap = True
                v1 = compGeoShp.Edges[ei].Vertexes[0]
                v2 = compGeoShp.Edges[ei].Vertexes[1]
                sp = (v1.X, v1.Y, 0.0)
                ep = (v2.X, v2.Y, 0.0)
                if dirFlg == 1:
                    arc = (sp, ep, cp)
                    if chkGap:
                        gap = abs(
                            self.toolDiam - gapDist(lst, sp)
                        )  # abs(self.toolDiam - lst.sub(sp).Length)
                    lst = ep
                else:
                    arc = (
                        ep,
                        sp,
                        cp,
                    )  # OCL.Arc(firstPnt, lastPnt, centerPnt, dir=True(CCW direction))
                    if chkGap:
                        gap = abs(
                            self.toolDiam - gapDist(lst, ep)
                        )  # abs(self.toolDiam - lst.sub(ep).Length)
                    lst = sp
                if chkGap:
                    if gap < obj.GapThreshold.Value:
                        PRTS.pop()  # pop off 'BRK' marker
                        (
                            vA,
                            vB,
                            vC,
                        ) = (
                            PRTS.pop()
                        )  # pop off previous arc segment for combining with current
                        arc = (vA, arc[1], vC)
                        self.closedGap = True
                    else:
                        gap = round(gap, 6)
                        if gap < self.gaps[0]:
                            self.gaps.insert(0, gap)
                            self.gaps.pop()
                PRTS.append(arc)
                cnt += 1

            if dirFlg == -1:
                PRTS.reverse()

            ARCS.append(("A", dirFlg, PRTS))
        # Eif
        if obj.CutPattern == "CircularZigZag":
            dirFlg = -1 * dirFlg
    # Efor

    ARCS.sort(key=dist_to_cent, reverse=obj.CutPatternReversed)

    return ARCS


def pathGeomToSpiralPointSet(obj, compGeoShp):
    """_pathGeomToSpiralPointSet(obj, compGeoShp)...
    Convert a compound set of sequential line segments to directional, connected groupings."""
    Path.Log.debug("_pathGeomToSpiralPointSet()")
    # Extract intersection line segments for return value as []
    LINES = []
    inLine = []
    lnCnt = 0
    ec = len(compGeoShp.Edges)
    start = 2

    if obj.CutPatternReversed:
        edg1 = compGeoShp.Edges[
            0
        ]  # Skip first edge, as it is the closing edge: center to outer tail
        ec -= 1
        start = 1
    else:
        edg1 = compGeoShp.Edges[
            1
        ]  # Skip first edge, as it is the closing edge: center to outer tail
    p1 = FreeCAD.Vector(edg1.Vertexes[0].X, edg1.Vertexes[0].Y, 0.0)
    p2 = FreeCAD.Vector(edg1.Vertexes[1].X, edg1.Vertexes[1].Y, 0.0)
    tup = ((p1.x, p1.y), (p2.x, p2.y))
    inLine.append(tup)

    for ei in range(
        start, ec
    ):  # Skipped first edge, started with second edge above as edg1
        edg = compGeoShp.Edges[ei]  # Get edge for vertexes
        sp = FreeCAD.Vector(
            edg.Vertexes[0].X, edg.Vertexes[0].Y, 0.0
        )  # check point (first / middle point)
        ep = FreeCAD.Vector(edg.Vertexes[1].X, edg.Vertexes[1].Y, 0.0)  # end point
        tup = ((sp.x, sp.y), (ep.x, ep.y))

        if sp.sub(p2).Length < 0.000001:
            inLine.append(tup)
        else:
            LINES.append(inLine)  # Save inLine segments
            lnCnt += 1
            inLine = []  # reset container
            inLine.append(tup)
        # p1 = sp
        p2 = ep
    # Efor

    lnCnt += 1
    LINES.append(inLine)  # Save inLine segments

    return LINES


def pathGeomToOffsetPointSet(obj, compGeoShp):
    """pathGeomToOffsetPointSet(obj, compGeoShp)...
    Convert a compound set of 3D profile segmented wires to 2D segments, applying linear optimization."""
    Path.Log.debug("pathGeomToOffsetPointSet()")

    LINES = []
    optimize = obj.OptimizeLinearPaths
    ofstCnt = len(compGeoShp)

    # Cycle through offset loops
    iPOL = False
    for ei in range(0, ofstCnt):
        OS = compGeoShp[ei]
        lenOS = len(OS)

        if ei > 0:
            LINES.append("BRK")

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


class FindUnifiedRegions:
    """FindUnifiedRegions() This class requires a list of face shapes.
    It finds the unified horizontal unified regions, if they exist."""

    def __init__(self, facesList, geomToler):
        self.FACES = facesList  # format is tuple (faceShape, faceIndex_on_base)
        self.geomToler = geomToler
        self.tempGroup = None
        self.topFaces = []
        self.edgeData = []
        self.circleData = []
        self.noSharedEdges = True
        self.topWires = []
        self.REGIONS = []
        self.INTERNALS = []
        self.idGroups = []
        self.sharedEdgeIdxs = []
        self.fusedFaces = None
        self.internalsReady = False

        if self.geomToler == 0.0:
            self.geomToler = 0.00001

    # Internal processing methods
    def _showShape(self, shape, name):
        if self.tempGroup:
            S = FreeCAD.ActiveDocument.addObject("Part::Feature", "tmp" + name)
            S.Shape = shape
            S.purgeTouched()
            self.tempGroup.addObject(S)

    def _extractTopFaces(self):
        for (F, fcIdx) in self.FACES:  # format is tuple (faceShape, faceIndex_on_base)
            cont = True
            fNum = fcIdx + 1
            # Extrude face
            fBB = F.BoundBox
            extFwd = math.floor(2.0 * fBB.ZLength) + 10.0
            ef = F.extrude(FreeCAD.Vector(0.0, 0.0, extFwd))
            ef = Part.makeSolid(ef)

            # Cut top off of extrusion with Part.box
            efBB = ef.BoundBox
            ZLen = efBB.ZLength / 2.0
            cutBox = Part.makeBox(efBB.XLength + 2.0, efBB.YLength + 2.0, ZLen)
            zHght = efBB.ZMin + ZLen
            cutBox.translate(FreeCAD.Vector(efBB.XMin - 1.0, efBB.YMin - 1.0, zHght))
            base = ef.cut(cutBox)

            if base.Volume == 0:
                Path.Log.debug(
                    "Ignoring Face{}.  It is likely vertical with no horizontal exposure.".format(
                        fcIdx
                    )
                )
                cont = False

            if cont:
                # Identify top face of base
                fIdx = 0
                zMin = base.Faces[fIdx].BoundBox.ZMin
                for bfi in range(0, len(base.Faces)):
                    fzmin = base.Faces[bfi].BoundBox.ZMin
                    if fzmin > zMin:
                        fIdx = bfi
                        zMin = fzmin

                # Translate top face to Z=0.0 and save to topFaces list
                topFace = base.Faces[fIdx]
                # self._showShape(topFace, 'topFace_{}'.format(fNum))
                tfBB = topFace.BoundBox
                tfBB_Area = tfBB.XLength * tfBB.YLength
                fBB_Area = fBB.XLength * fBB.YLength
                if tfBB_Area < (fBB_Area * 0.9):
                    # attempt alternate methods
                    topFace = self._getCompleteCrossSection(ef)
                    tfBB = topFace.BoundBox
                    tfBB_Area = tfBB.XLength * tfBB.YLength
                    # self._showShape(topFace, 'topFaceAlt_1_{}'.format(fNum))
                    if tfBB_Area < (fBB_Area * 0.9):
                        topFace = getShapeSlice(ef)
                        tfBB = topFace.BoundBox
                        tfBB_Area = tfBB.XLength * tfBB.YLength
                        # self._showShape(topFace, 'topFaceAlt_2_{}'.format(fNum))
                        if tfBB_Area < (fBB_Area * 0.9):
                            msg = "Failed to extract processing region for Face {}\n".format(
                                fNum
                            )
                            FreeCAD.Console.PrintError(msg)
                            cont = False
            # Eif

            if cont:
                topFace.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - zMin))
                self.topFaces.append((topFace, fcIdx))

    def _fuseTopFaces(self):
        (one, baseFcIdx) = self.topFaces.pop(0)
        base = one
        for (face, fcIdx) in self.topFaces:
            base = base.fuse(face)
        self.topFaces.insert(0, (one, baseFcIdx))
        self.fusedFaces = base

    def _getEdgesData(self):
        topFaces = self.fusedFaces.Faces
        tfLen = len(topFaces)
        count = [0, 0]

        # Get length and center of mass for each edge in all top faces
        for fi in range(0, tfLen):
            F = topFaces[fi]
            edgCnt = len(F.Edges)
            for ei in range(0, edgCnt):
                E = F.Edges[ei]
                tup = (E.Length, E.CenterOfMass, E, fi)
                if len(E.Vertexes) == 1:
                    self.circleData.append(tup)
                    count[0] += 1
                else:
                    self.edgeData.append(tup)
                    count[1] += 1

    def _groupEdgesByLength(self):
        Path.Log.debug("_groupEdgesByLength()")
        threshold = self.geomToler
        grp = []
        processLast = False

        def keyFirst(tup):
            return tup[0]

        # Sort edgeData data and prepare proxy indexes
        self.edgeData.sort(key=keyFirst)
        DATA = self.edgeData
        lenDATA = len(DATA)
        indexes = [i for i in range(0, lenDATA)]
        idxCnt = len(indexes)

        while idxCnt > 0:
            processLast = True
            # Pop off index for first edge
            actvIdx = indexes.pop(0)
            actvItem = DATA[actvIdx][0]  # 0 index is length
            grp.append(actvIdx)
            idxCnt -= 1

            while idxCnt > 0:
                tstIdx = indexes[0]
                tstItem = DATA[tstIdx][0]

                # test case(s) goes here
                absLenDiff = abs(tstItem - actvItem)
                if absLenDiff < threshold:
                    # Remove test index from indexes
                    indexes.pop(0)
                    idxCnt -= 1
                    grp.append(tstIdx)
                else:
                    if len(grp) > 1:
                        # grp.sort()
                        self.idGroups.append(grp)
                    grp = []
                    break
            # Ewhile
        # Ewhile
        if processLast:
            if len(grp) > 1:
                # grp.sort()
                self.idGroups.append(grp)

    def _identifySharedEdgesByLength(self, grp):
        Path.Log.debug("_identifySharedEdgesByLength()")
        holds = []
        specialIndexes = []
        threshold = self.geomToler

        def keyFirst(tup):
            return tup[0]

        # Sort edgeData data
        self.edgeData.sort(key=keyFirst)
        DATA = self.edgeData
        lenGrp = len(grp)

        while lenGrp > 0:
            # Pop off index for first edge
            actvIdx = grp.pop(0)
            actvItem = DATA[actvIdx][0]  # 0 index is length
            lenGrp -= 1
            while lenGrp > 0:
                isTrue = False
                # Pop off index for test edge
                tstIdx = grp.pop(0)
                tstItem = DATA[tstIdx][0]
                lenGrp -= 1

                # test case(s) goes here
                lenDiff = tstItem - actvItem
                absLenDiff = abs(lenDiff)
                if lenDiff > threshold:
                    break
                if absLenDiff < threshold:
                    com1 = DATA[actvIdx][1]
                    com2 = DATA[tstIdx][1]
                    comDiff = com2.sub(com1).Length
                    if comDiff < threshold:
                        isTrue = True

                # Action if test is true (finds special case)
                if isTrue:
                    specialIndexes.append(actvIdx)
                    specialIndexes.append(tstIdx)
                    break
                else:
                    holds.append(tstIdx)

            # Put hold indexes back in search group
            holds.extend(grp)
            grp = holds
            lenGrp = len(grp)
            holds = []

        if len(specialIndexes) > 0:
            # Remove shared edges from EDGES data
            uniqueShared = list(set(specialIndexes))
            self.sharedEdgeIdxs.extend(uniqueShared)
            self.noSharedEdges = False

    def _extractWiresFromEdges(self):
        Path.Log.debug("_extractWiresFromEdges()")
        DATA = self.edgeData
        holds = []
        firstEdge = None
        cont = True
        connectedEdges = []
        connectedIndexes = []
        connectedCnt = 0
        LOOPS = []

        def faceIndex(tup):
            return tup[3]

        def faceArea(face):
            return face.Area

        # Sort by face index on original model base
        DATA.sort(key=faceIndex)
        lenDATA = len(DATA)
        indexes = [i for i in range(0, lenDATA)]
        idxCnt = len(indexes)

        # Add circle edges into REGIONS list
        if len(self.circleData) > 0:
            for C in self.circleData:
                face = Part.Face(Part.Wire(C[2]))
                self.REGIONS.append(face)

        actvIdx = indexes.pop(0)
        actvEdge = DATA[actvIdx][2]
        firstEdge = actvEdge  # DATA[connectedIndexes[0]][2]
        idxCnt -= 1
        connectedIndexes.append(actvIdx)
        connectedEdges.append(actvEdge)
        connectedCnt = 1

        safety = 750
        while cont:  # safety > 0
            safety -= 1
            notConnected = True
            while idxCnt > 0:
                isTrue = False
                # Pop off index for test edge
                tstIdx = indexes.pop(0)
                tstEdge = DATA[tstIdx][2]
                idxCnt -= 1
                if self._edgesAreConnected(actvEdge, tstEdge):
                    isTrue = True

                if isTrue:
                    notConnected = False
                    connectedIndexes.append(tstIdx)
                    connectedEdges.append(tstEdge)
                    connectedCnt += 1
                    actvIdx = tstIdx
                    actvEdge = tstEdge
                    break
                else:
                    holds.append(tstIdx)
            # Ewhile

            if connectedCnt > 2:
                if self._edgesAreConnected(actvEdge, firstEdge):
                    notConnected = False
                    # Save loop components
                    LOOPS.append(connectedEdges)
                    # reset connected variables and re-assess
                    connectedEdges = []
                    connectedIndexes = []
                    connectedCnt = 0
                    indexes.sort()
                    idxCnt = len(indexes)
                    if idxCnt > 0:
                        # Pop off index for first edge
                        actvIdx = indexes.pop(0)
                        actvEdge = DATA[actvIdx][2]
                        idxCnt -= 1
                        firstEdge = actvEdge
                        connectedIndexes.append(actvIdx)
                        connectedEdges.append(actvEdge)
                        connectedCnt = 1
            # Eif

            # Put holds indexes back in search stack
            if notConnected:
                holds.append(actvIdx)
            holds.extend(indexes)
            indexes = holds
            idxCnt = len(indexes)
            holds = []
            if idxCnt == 0:
                cont = False
            if safety == 0:
                cont = False
        # Ewhile

        numLoops = len(LOOPS)
        Path.Log.debug(" -numLoops: {}.".format(numLoops))
        if numLoops > 0:
            for li in range(0, numLoops):
                Edges = LOOPS[li]
                # for e in Edges:
                #    self._showShape(e, 'Loop_{}_Edge'.format(li))
                wire = Part.Wire(Part.__sortEdges__(Edges))
                if wire.isClosed():
                    # This simple Part.Face() method fails to catch
                    # wires with tangent closed wires, or an external
                    # wire with one or more internal tangent wires.
                    # face = Part.Face(wire)

                    # This method works with the complex tangent
                    # closed wires mentioned above.
                    extWire = wire.extrude(FreeCAD.Vector(0.0, 0.0, 2.0))
                    wireSolid = Part.makeSolid(extWire)
                    extdBBFace1 = makeExtendedBoundBox(
                        wireSolid.BoundBox, 5.0, wireSolid.BoundBox.ZMin + 1.0
                    )
                    extdBBFace2 = makeExtendedBoundBox(
                        wireSolid.BoundBox, 5.0, wireSolid.BoundBox.ZMin + 1.0
                    )
                    inverse = extdBBFace1.cut(wireSolid)
                    face = extdBBFace2.cut(inverse)
                    self.REGIONS.append(face)
            self.REGIONS.sort(key=faceArea, reverse=True)

    def _identifyInternalFeatures(self):
        Path.Log.debug("_identifyInternalFeatures()")
        remList = []

        for (top, fcIdx) in self.topFaces:
            big = Part.Face(top.OuterWire)
            for s in range(0, len(self.REGIONS)):
                if s not in remList:
                    small = self.REGIONS[s]
                    if self._isInBoundBox(big, small):
                        cmn = big.common(small)
                        if cmn.Area > 0.0:
                            self.INTERNALS.append(small)
                            remList.append(s)
                            break
                        else:
                            Path.Log.debug(" - No common area.\n")

        remList.sort(reverse=True)
        for ri in remList:
            self.REGIONS.pop(ri)

    def _processNestedRegions(self):
        Path.Log.debug("_processNestedRegions()")
        cont = True
        hold = []
        Ids = []
        remList = []
        for i in range(0, len(self.REGIONS)):
            Ids.append(i)
        idsCnt = len(Ids)

        while cont:
            while idsCnt > 0:
                hi = Ids.pop(0)
                high = self.REGIONS[hi]
                idsCnt -= 1
                while idsCnt > 0:
                    isTrue = False
                    li = Ids.pop(0)
                    idsCnt -= 1
                    low = self.REGIONS[li]
                    # Test case here
                    if self._isInBoundBox(high, low):
                        cmn = high.common(low)
                        if cmn.Area > 0.0:
                            isTrue = True
                    # if True action here
                    if isTrue:
                        self.REGIONS[hi] = high.cut(low)
                        remList.append(li)
                    else:
                        hold.append(hi)
                # Ewhile
                hold.extend(Ids)
                Ids = hold
                hold = []
                idsCnt = len(Ids)
                if len(Ids) == 0:
                    cont = False
            # Ewhile
        # Ewhile
        remList.sort(reverse=True)
        for ri in remList:
            self.REGIONS.pop(ri)

    # Accessory methods
    def _getCompleteCrossSection(self, shape):
        Path.Log.debug("_getCompleteCrossSection()")
        wires = []
        bb = shape.BoundBox
        mid = (bb.ZMin + bb.ZMax) / 2.0

        for i in shape.slice(FreeCAD.Vector(0, 0, 1), mid):
            wires.append(i)

        if len(wires) > 0:
            comp = Part.Compound(wires)  # produces correct cross-section wire !
            CS = Part.Face(comp.Wires[0])
            CS.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - CS.BoundBox.ZMin))
            return CS

        Path.Log.debug(" -No wires from .slice() method")
        return False

    def _edgesAreConnected(self, e1, e2):
        # Assumes edges are flat and are at Z=0.0

        def isSameVertex(v1, v2):
            # Assumes vertexes at Z=0.0
            if abs(v1.X - v2.X) < 0.000001:
                if abs(v1.Y - v2.Y) < 0.000001:
                    return True
            return False

        if isSameVertex(e1.Vertexes[0], e2.Vertexes[0]):
            return True
        if isSameVertex(e1.Vertexes[0], e2.Vertexes[1]):
            return True
        if isSameVertex(e1.Vertexes[1], e2.Vertexes[0]):
            return True
        if isSameVertex(e1.Vertexes[1], e2.Vertexes[1]):
            return True

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

    # Public methods
    def setTempGroup(self, grpObj):
        """setTempGroup(grpObj)... For debugging, pass temporary object group."""
        self.tempGroup = grpObj

    def getUnifiedRegions(self):
        """getUnifiedRegions()... Returns a list of unified regions from list
        of tuples (faceShape, faceIndex) received at instantiation of the class object."""
        Path.Log.debug("getUnifiedRegions()")
        if len(self.FACES) == 0:
            msg = "No FACE data tuples received at instantiation of class.\n"
            FreeCAD.Console.PrintError(msg)
            return []

        self._extractTopFaces()
        lenFaces = len(self.topFaces)
        if lenFaces == 0:
            return []

        # if single topFace, return it
        if lenFaces == 1:
            topFace = self.topFaces[0][0]
            self._showShape(topFace, "TopFace")
            # prepare inner wires as faces for internal features
            lenWrs = len(topFace.Wires)
            if lenWrs > 1:
                for w in range(1, lenWrs):
                    wr = topFace.Wires[w]
                    self.INTERNALS.append(Part.Face(wr))
            self.internalsReady = True
            # Flatten face and extract outer wire, then convert to face
            extWire = getExtrudedShape(topFace)
            wCS = getCrossSection(extWire)
            if wCS:
                face = Part.Face(wCS)
                return [face]
            else:
                (faceShp, fcIdx) = self.FACES[0]
                msg = translate(
                    "PathSurfaceSupport",
                    "Failed to identify a horizontal cross-section for Face",
                )
                msg += "{}.\n".format(fcIdx + 1)
                FreeCAD.Console.PrintWarning(msg)
                return []

        # process multiple top faces, unifying if possible
        self._fuseTopFaces()
        for F in self.fusedFaces.Faces:
            self._showShape(F, "TopFaceFused")

        self._getEdgesData()
        self._groupEdgesByLength()
        for grp in self.idGroups:
            self._identifySharedEdgesByLength(grp)

        if self.noSharedEdges:
            Path.Log.debug("No shared edges by length detected.")
            allTopFaces = []
            for (topFace, fcIdx) in self.topFaces:
                allTopFaces.append(topFace)
                # Identify internal features
                lenWrs = len(topFace.Wires)
                if lenWrs > 1:
                    for w in range(1, lenWrs):
                        wr = topFace.Wires[w]
                        self.INTERNALS.append(Part.Face(wr))
            self.internalsReady = True
            return allTopFaces
        else:
            # Delete shared edges from edgeData list
            self.sharedEdgeIdxs.sort(reverse=True)
            for se in self.sharedEdgeIdxs:
                self.edgeData.pop(se)

        self._extractWiresFromEdges()
        self._identifyInternalFeatures()
        self._processNestedRegions()
        # for ri in range(0, len(self.REGIONS)):
        #    self._showShape(self.REGIONS[ri], 'UnifiedRegion_{}'.format(ri))

        self.internalsReady = True
        return self.REGIONS

    def getInternalFeatures(self):
        """getInternalFeatures()... Returns internal features identified
        after calling getUnifiedRegions()."""
        if self.internalsReady:
            if len(self.INTERNALS) > 0:
                return self.INTERNALS
            else:
                return False

        msg = "getUnifiedRegions() must be called before getInternalFeatures().\n"
        FreeCAD.Console.PrintError(msg)
        return False


class OCL_Tool:
    """The OCL_Tool class is designed to translate a FreeCAD standard ToolBit shape
    in the active Tool Controller, into an OCL tool type."""

    def __init__(self, ocl, obj, safe=False):
        self.ocl = ocl
        self.obj = obj
        self.tool = None
        self.tiltCutter = False
        self.safe = safe
        self.oclTool = None
        self.toolType = None
        self.toolMode = None
        self.toolMethod = None

        self.diameter = -1.0
        self.cornerRadius = -1.0
        self.flatRadius = -1.0
        self.cutEdgeHeight = -1.0
        self.cutEdgeAngle = -1.0
        # Default to zero. ToolBit likely is without.
        self.lengthOffset = 0.0

        if hasattr(obj, "ToolController"):
            if hasattr(obj.ToolController, "Tool"):
                self.tool = obj.ToolController.Tool
                if hasattr(self.tool, "ShapeName"):
                    self.toolType = self.tool.ShapeName  # Indicates ToolBit tool
                    self.toolMode = "ToolBit"
        if self.toolType:
            Path.Log.debug(
                "OCL_Tool tool mode, type: {}, {}".format(self.toolMode, self.toolType)
            )

    """
        #### FreeCAD Legacy tool shape properties per tool type
        shape = EndMill
        Diameter
        CuttingEdgeHeight
        LengthOffset

        shape = Drill
        Diameter
        CuttingEdgeAngle  # TipAngle from above, center shaft. 180 = flat tip (endmill)
        CuttingEdgeHeight
        LengthOffset

        shape = CenterDrill
        Diameter
        FlatRadius
        CornerRadius
        CuttingEdgeAngle  # TipAngle from above, center shaft. 180 = flat tip (endmill)
        CuttingEdgeHeight
        LengthOffset

        shape = CounterSink
        Diameter
        FlatRadius
        CornerRadius
        CuttingEdgeAngle  # TipAngle from above, center shaft. 180 = flat tip (endmill)
        CuttingEdgeHeight
        LengthOffset

        shape = CounterBore
        Diameter
        FlatRadius
        CornerRadius
        CuttingEdgeAngle  # TipAngle from above, center shaft. 180 = flat tip (endmill)
        CuttingEdgeHeight
        LengthOffset

        shape = FlyCutter
        Diameter
        FlatRadius
        CornerRadius
        CuttingEdgeAngle  # TipAngle from above, center shaft. 180 = flat tip (endmill)
        CuttingEdgeHeight
        LengthOffset

        shape = Reamer
        Diameter
        FlatRadius
        CornerRadius
        CuttingEdgeAngle  # TipAngle from above, center shaft. 180 = flat tip (endmill)
        CuttingEdgeHeight
        LengthOffset

        shape = Tap
        Diameter
        FlatRadius
        CornerRadius
        CuttingEdgeAngle  # TipAngle from above, center shaft. 180 = flat tip (endmill)
        CuttingEdgeHeight
        LengthOffset

        shape = SlotCutter
        Diameter
        FlatRadius
        CornerRadius
        CuttingEdgeAngle  # TipAngle from above, center shaft. 180 = flat tip (endmill)
        CuttingEdgeHeight
        LengthOffset

        shape = BallEndMill
        Diameter
        FlatRadius
        CornerRadius
        CuttingEdgeAngle  # TipAngle from above, center shaft. 180 = flat tip (endmill)
        CuttingEdgeHeight
        LengthOffset

        shape = ChamferMill
        Diameter
        FlatRadius
        CornerRadius
        CuttingEdgeAngle  # TipAngle from above, center shaft. 180 = flat tip (endmill)
        CuttingEdgeHeight
        LengthOffset

        shape = CornerRound
        Diameter
        FlatRadius
        CornerRadius
        CuttingEdgeAngle  # TipAngle from above, center shaft. 180 = flat tip (endmill)
        CuttingEdgeHeight
        LengthOffset

        shape = Engraver
        Diameter
        CuttingEdgeAngle  # TipAngle from above, center shaft. 180 = flat tip (endmill)
        CuttingEdgeHeight
        LengthOffset


        #### FreeCAD packaged ToolBit named constraints per shape files
        shape = endmill
        Diameter; Endmill diameter
        Length; Overall length of the endmill
        ShankDiameter; diameter of the shank
        CuttingEdgeHeight

        shape = ballend
        Diameter; Endmill diameter
        Length; Overall length of the endmill
        ShankDiameter; diameter of the shank
        CuttingEdgeHeight

        shape = bullnose
        Diameter; Endmill diameter
        Length; Overall length of the endmill
        ShankDiameter; diameter of the shank
        FlatRadius;Radius of the bottom flat part.
        CuttingEdgeHeight

        shape = drill
        TipAngle; Full angle of the drill tip
        Diameter; Drill bit diameter
        Length; Overall length of the drillbit

        shape = v-bit
        Diameter; Overall diameter of the V-bit
        CuttingEdgeAngle;Full angle of the v-bit
        Length; Overall  bit length
        ShankDiameter
        FlatHeight;Height of the flat extension of the v-bit
        FlatRadius; Diameter of the flat end of the tip
    """

    # Private methods
    def _setDimensions(self):
        """_setDimensions() ... Set values for possible dimensions."""
        if hasattr(self.tool, "Diameter"):
            self.diameter = float(self.tool.Diameter)
        else:
            msg = translate(
                "PathSurfaceSupport", "Diameter dimension missing from ToolBit shape."
            )
            FreeCAD.Console.PrintError(msg + "\n")
            return False
        if hasattr(self.tool, "LengthOffset"):
            self.lengthOffset = float(self.tool.LengthOffset)
        if hasattr(self.tool, "FlatRadius"):
            self.flatRadius = float(self.tool.FlatRadius)
        if hasattr(self.tool, "CuttingEdgeHeight"):
            self.cutEdgeHeight = float(self.tool.CuttingEdgeHeight)
        if hasattr(self.tool, "CuttingEdgeAngle"):
            self.cutEdgeAngle = float(self.tool.CuttingEdgeAngle)
        return True

    def _makeSafeCutter(self):
        # Make safeCutter with 25% buffer around physical cutter
        if self.safe:
            self.diameter = self.diameter * 1.25
            if self.flatRadius == 0.0:
                self.flatRadius = self.diameter * 0.25
            elif self.flatRadius > 0.0:
                self.flatRadius = self.flatRadius * 1.25

    def _oclCylCutter(self):
        # Standard End Mill, Slot cutter, or Fly cutter
        # OCL -> CylCutter::CylCutter(diameter, length)
        if self.diameter == -1.0 or self.cutEdgeHeight == -1.0:
            return
        self.oclTool = self.ocl.CylCutter(
            self.diameter, self.cutEdgeHeight + self.lengthOffset
        )

    def _oclBallCutter(self):
        # Standard Ball End Mill
        # OCL -> BallCutter::BallCutter(diameter, length)
        if self.diameter == -1.0 or self.cutEdgeHeight == -1.0:
            return
        self.tiltCutter = True
        if self.cutEdgeHeight == 0:
            self.cutEdgeHeight = self.diameter / 2
        self.oclTool = self.ocl.BallCutter(
            self.diameter, self.cutEdgeHeight + self.lengthOffset
        )

    def _oclBullCutter(self):
        # Standard Bull Nose cutter
        # Reference: https://www.fine-tools.com/halbstabfraeser.html
        # OCL -> BullCutter::BullCutter(diameter, minor radius, length)
        if (
            self.diameter == -1.0
            or self.flatRadius == -1.0
            or self.cutEdgeHeight == -1.0
        ):
            return
        self.oclTool = self.ocl.BullCutter(
            self.diameter,
            self.diameter - self.flatRadius,
            self.cutEdgeHeight + self.lengthOffset,
        )

    def _oclConeCutter(self):
        # Engraver or V-bit cutter
        # OCL -> ConeCutter::ConeCutter(diameter, angle, length)
        if (
            self.diameter == -1.0
            or self.cutEdgeAngle == -1.0
            or self.cutEdgeHeight == -1.0
        ):
            return
        self.oclTool = self.ocl.ConeCutter(
            self.diameter, self.cutEdgeAngle / 2, self.lengthOffset
        )

    def _setToolMethod(self):
        toolMap = dict()

        if self.toolMode == "ToolBit":
            toolMap = {
                "endmill": "CylCutter",
                "ballend": "BallCutter",
                "bullnose": "BullCutter",
                "drill": "ConeCutter",
                "engraver": "ConeCutter",
                "v-bit": "ConeCutter",
                "chamfer": "None",
            }
        self.toolMethod = "None"
        if self.toolType in toolMap:
            self.toolMethod = toolMap[self.toolType]

    # Public methods
    def getOclTool(self):
        """getOclTool()... Call this method after class instantiation
        to return OCL tool object."""
        # Check for tool controller and tool object
        if not self.tool or not self.toolMode:
            msg = translate("PathSurface", "Failed to identify tool for operation.")
            FreeCAD.Console.PrintError(msg + "\n")
            return False

        if not self._setDimensions():
            return False

        self._setToolMethod()

        if self.toolMethod == "None":
            err = translate(
                "PathSurface", "Failed to map selected tool to an OCL tool type."
            )
            FreeCAD.Console.PrintError(err + "\n")
            return False
        else:
            Path.Log.debug("OCL_Tool tool method: {}".format(self.toolMethod))
            oclToolMethod = getattr(self, "_ocl" + self.toolMethod)
            oclToolMethod()

        if self.oclTool:
            return self.oclTool

        # Set error messages
        err = translate(
            "PathSurface", "Failed to translate active tool to OCL tool type."
        )
        FreeCAD.Console.PrintError(err + "\n")
        return False

    def useTiltCutter(self):
        """useTiltCutter()... Call this method after getOclTool() method
        to return status of cutter tilt availability - generally this
        is for a ball end mill."""
        if not self.tool or not self.oclTool:
            err = translate(
                "PathSurface",
                "OCL tool not available. Cannot determine is cutter has tilt available.",
            )
            FreeCAD.Console.PrintError(err + "\n")
            return False
        return self.tiltCutter


# Eclass

# Support functions
def makeExtendedBoundBox(wBB, bbBfr, zDep):
    Path.Log.debug("makeExtendedBoundBox()")
    p1 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMin - bbBfr, zDep)
    p2 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMin - bbBfr, zDep)
    p3 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMax + bbBfr, zDep)
    p4 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMax + bbBfr, zDep)

    L1 = Part.makeLine(p1, p2)
    L2 = Part.makeLine(p2, p3)
    L3 = Part.makeLine(p3, p4)
    L4 = Part.makeLine(p4, p1)

    return Part.Face(Part.Wire([L1, L2, L3, L4]))
