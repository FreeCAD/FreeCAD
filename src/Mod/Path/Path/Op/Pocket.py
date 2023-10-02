# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Part
import Path
import Path.Op.Base as PathOp
import Path.Op.PocketBase as PathPocketBase
import PathScripts.PathUtils as PathUtils

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

__title__ = "Path 3D Pocket Operation"
__author__ = "Yorik van Havre <yorik@uncreated.net>"
__url__ = "https://www.freecad.org"
__doc__ = "Class and implementation of the 3D Pocket operation."
__created__ = "2014"

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


class ObjectPocket(PathPocketBase.ObjectPocket):
    """Proxy object for Pocket operation."""

    def pocketOpFeatures(self, obj):
        return PathOp.FeatureNoFinalDepth

    def initPocketOp(self, obj):
        """initPocketOp(obj) ... setup receiver"""
        if not hasattr(obj, "HandleMultipleFeatures"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "HandleMultipleFeatures",
                "Pocket",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Choose how to process multiple Base Geometry features.",
                ),
            )

        if not hasattr(obj, "AdaptivePocketStart"):
            obj.addProperty(
                "App::PropertyBool",
                "AdaptivePocketStart",
                "Pocket",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Use adaptive algorithm to eliminate excessive air milling above planar pocket top.",
                ),
            )
        if not hasattr(obj, "AdaptivePocketFinish"):
            obj.addProperty(
                "App::PropertyBool",
                "AdaptivePocketFinish",
                "Pocket",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Use adaptive algorithm to eliminate excessive air milling below planar pocket bottom.",
                ),
            )
        if not hasattr(obj, "ProcessStockArea"):
            obj.addProperty(
                "App::PropertyBool",
                "ProcessStockArea",
                "Pocket",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Process the model and stock in an operation with no Base Geometry selected.",
                ),
            )

        # populate the property enumerations
        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

    @classmethod
    def propertyEnumerations(self, dataType="data"):
        """propertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        enums = {
            "HandleMultipleFeatures": [
                (translate("Path_Pocket", "Collectively"), "Collectively"),
                (translate("Path_Pocket", "Individually"), "Individually"),
            ],
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def opOnDocumentRestored(self, obj):
        """opOnDocumentRestored(obj) ... adds the properties if they doesn't exist."""
        super().opOnDocumentRestored(obj)
        self.initPocketOp(obj)

    def pocketInvertExtraOffset(self):
        return False

    def opUpdateDepths(self, obj):
        """opUpdateDepths(obj) ... Implement special depths calculation."""
        # Set Final Depth to bottom of model if whole model is used
        if not obj.Base or len(obj.Base) == 0:
            if len(self.job.Model.Group) == 1:
                finDep = self.job.Model.Group[0].Shape.BoundBox.ZMin
            else:
                finDep = min([m.Shape.BoundBox.ZMin for m in self.job.Model.Group])
            obj.setExpression("OpFinalDepth", "{} mm".format(finDep))

    def areaOpShapes(self, obj):
        """areaOpShapes(obj) ... return shapes representing the solids to be removed."""
        Path.Log.track()

        subObjTups = []
        removalshapes = []

        if obj.Base:
            Path.Log.debug("base items exist.  Processing... ")
            for base in obj.Base:
                Path.Log.debug("obj.Base item: {}".format(base))

                # Check if all subs are faces
                allSubsFaceType = True
                Faces = []
                for sub in base[1]:
                    if "Face" in sub:
                        face = getattr(base[0].Shape, sub)
                        Faces.append(face)
                        subObjTups.append((sub, face))
                    else:
                        allSubsFaceType = False
                        break

                if len(Faces) == 0:
                    allSubsFaceType = False

                if (
                    allSubsFaceType is True
                    and obj.HandleMultipleFeatures == "Collectively"
                ):
                    (fzmin, fzmax) = self.getMinMaxOfFaces(Faces)
                    if obj.FinalDepth.Value < fzmin:
                        Path.Log.warning(
                            translate(
                                "PathPocket",
                                "Final depth set below ZMin of face(s) selected.",
                            )
                        )

                    if (
                        obj.AdaptivePocketStart is True
                        or obj.AdaptivePocketFinish is True
                    ):
                        pocketTup = self.calculateAdaptivePocket(obj, base, subObjTups)
                        if pocketTup is not False:
                            obj.removalshape = pocketTup[0]
                            removalshapes.append(pocketTup)  # (shape, isHole, detail)
                    else:
                        shape = Part.makeCompound(Faces)
                        env = PathUtils.getEnvelope(
                            base[0].Shape, subshape=shape, depthparams=self.depthparams
                        )
                        rawRemovalShape = env.cut(base[0].Shape)
                        faceExtrusions = [
                            f.extrude(FreeCAD.Vector(0.0, 0.0, 1.0)) for f in Faces
                        ]
                        obj.removalshape = _identifyRemovalSolids(
                            rawRemovalShape, faceExtrusions
                        )
                        removalshapes.append(
                            (obj.removalshape, False, "3DPocket")
                        )  # (shape, isHole, detail)
                else:
                    for sub in base[1]:
                        if "Face" in sub:
                            shape = Part.makeCompound([getattr(base[0].Shape, sub)])
                        else:
                            edges = [getattr(base[0].Shape, sub) for sub in base[1]]
                            shape = Part.makeFace(edges, "Part::FaceMakerSimple")

                        env = PathUtils.getEnvelope(
                            base[0].Shape, subshape=shape, depthparams=self.depthparams
                        )
                        rawRemovalShape = env.cut(base[0].Shape)
                        faceExtrusions = [shape.extrude(FreeCAD.Vector(0.0, 0.0, 1.0))]
                        obj.removalshape = _identifyRemovalSolids(
                            rawRemovalShape, faceExtrusions
                        )
                        removalshapes.append((obj.removalshape, False, "3DPocket"))

        else:  # process the job base object as a whole
            Path.Log.debug("processing the whole job base object")
            for base in self.model:
                if obj.ProcessStockArea is True:
                    job = PathUtils.findParentJob(obj)

                    stockEnvShape = PathUtils.getEnvelope(
                        job.Stock.Shape, subshape=None, depthparams=self.depthparams
                    )

                    rawRemovalShape = stockEnvShape.cut(base.Shape)
                else:
                    env = PathUtils.getEnvelope(
                        base.Shape, subshape=None, depthparams=self.depthparams
                    )
                    rawRemovalShape = env.cut(base.Shape)

                # Identify target removal shapes after cutting envelope with base shape
                removalSolids = [
                    s
                    for s in rawRemovalShape.Solids
                    if Path.Geom.isRoughly(
                        s.BoundBox.ZMax, rawRemovalShape.BoundBox.ZMax
                    )
                ]

                # Fuse multiple solids
                if len(removalSolids) > 1:
                    seed = removalSolids[0]
                    for tt in removalSolids[1:]:
                        fusion = seed.fuse(tt)
                        seed = fusion
                    removalShape = seed
                else:
                    removalShape = removalSolids[0]

                obj.removalshape = removalShape
                removalshapes.append((obj.removalshape, False, "3DPocket"))

        return removalshapes

    def areaOpSetDefaultValues(self, obj, job):
        """areaOpSetDefaultValues(obj, job) ... set default values"""
        obj.StepOver = 100
        obj.ZigZagAngle = 45
        obj.HandleMultipleFeatures = "Collectively"
        obj.AdaptivePocketStart = False
        obj.AdaptivePocketFinish = False
        obj.ProcessStockArea = False

    # methods for eliminating air milling with some pockets: adaptive start and finish
    def calculateAdaptivePocket(self, obj, base, subObjTups):
        """calculateAdaptivePocket(obj, base, subObjTups)
        Orient multiple faces around common facial center of mass.
        Identify edges that are connections for adjacent faces.
        Attempt to separate unconnected edges into top and bottom loops of the pocket.
        Trim the top and bottom of the pocket if available and requested.
        return: tuple with pocket shape information"""
        low = []
        high = []
        removeList = []
        Faces = []
        allEdges = []
        makeHighFace = 0
        tryNonPlanar = False
        isHighFacePlanar = True
        isLowFacePlanar = True

        for (sub, face) in subObjTups:
            Faces.append(face)

        # identify max and min face heights for top loop
        (zmin, zmax) = self.getMinMaxOfFaces(Faces)

        # Order faces around common center of mass
        subObjTups = self.orderFacesAroundCenterOfMass(subObjTups)
        # find connected edges and map to edge names of base
        (connectedEdges, touching) = self.findSharedEdges(subObjTups)
        (low, high) = self.identifyUnconnectedEdges(subObjTups, touching)

        if len(high) > 0 and obj.AdaptivePocketStart is True:
            # attempt planar face with top edges of pocket
            allEdges = []
            makeHighFace = 0
            tryNonPlanar = False
            for (sub, face, ei) in high:
                allEdges.append(face.Edges[ei])

            (hzmin, hzmax) = self.getMinMaxOfFaces(allEdges)

            try:
                highFaceShape = Part.Face(Part.Wire(Part.__sortEdges__(allEdges)))
            except Exception as ee:
                Path.Log.warning(ee)
                Path.Log.error(
                    translate(
                        "Path",
                        "A planar adaptive start is unavailable. The non-planar will be attempted.",
                    )
                )
                tryNonPlanar = True
            else:
                makeHighFace = 1

            if tryNonPlanar is True:
                try:
                    highFaceShape = Part.makeFilledFace(
                        Part.__sortEdges__(allEdges)
                    )  # NON-planar face method
                except Exception as eee:
                    Path.Log.warning(eee)
                    Path.Log.error(
                        translate(
                            "Path", "The non-planar adaptive start is also unavailable."
                        )
                        + "(1)"
                    )
                    isHighFacePlanar = False
                else:
                    makeHighFace = 2

            if makeHighFace > 0:
                FreeCAD.ActiveDocument.addObject("Part::Feature", "topEdgeFace")
                highFace = FreeCAD.ActiveDocument.ActiveObject
                highFace.Shape = highFaceShape
                removeList.append(highFace.Name)

            # verify non-planar face is within high edge loop Z-boundaries
            if makeHighFace == 2:
                mx = hzmax + obj.StepDown.Value
                mn = hzmin - obj.StepDown.Value
                if (
                    highFace.Shape.BoundBox.ZMax > mx
                    or highFace.Shape.BoundBox.ZMin < mn
                ):
                    Path.Log.warning(
                        "ZMaxDiff: {};  ZMinDiff: {}".format(
                            highFace.Shape.BoundBox.ZMax - mx,
                            highFace.Shape.BoundBox.ZMin - mn,
                        )
                    )
                    Path.Log.error(
                        translate(
                            "Path", "The non-planar adaptive start is also unavailable."
                        )
                        + "(2)"
                    )
                    isHighFacePlanar = False
                    makeHighFace = 0
        else:
            isHighFacePlanar = False

        if len(low) > 0 and obj.AdaptivePocketFinish is True:
            # attempt planar face with bottom edges of pocket
            allEdges = []
            for (sub, face, ei) in low:
                allEdges.append(face.Edges[ei])

            # (lzmin, lzmax) = self.getMinMaxOfFaces(allEdges)

            try:
                lowFaceShape = Part.Face(Part.Wire(Part.__sortEdges__(allEdges)))
                # lowFaceShape = Part.makeFilledFace(Part.__sortEdges__(allEdges))  # NON-planar face method
            except Exception as ee:
                Path.Log.error(ee)
                Path.Log.error("An adaptive finish is unavailable.")
                isLowFacePlanar = False
            else:
                FreeCAD.ActiveDocument.addObject("Part::Feature", "bottomEdgeFace")
                lowFace = FreeCAD.ActiveDocument.ActiveObject
                lowFace.Shape = lowFaceShape
                removeList.append(lowFace.Name)
        else:
            isLowFacePlanar = False

        # Start with a regular pocket envelope
        strDep = obj.StartDepth.Value
        finDep = obj.FinalDepth.Value
        cuts = []
        starts = []
        finals = []
        starts.append(obj.StartDepth.Value)
        finals.append(zmin)
        if obj.AdaptivePocketStart is True or len(subObjTups) == 1:
            strDep = zmax + obj.StepDown.Value
            starts.append(zmax + obj.StepDown.Value)

        finish_step = obj.FinishDepth.Value if hasattr(obj, "FinishDepth") else 0.0
        depthparams = PathUtils.depth_params(
            clearance_height=obj.ClearanceHeight.Value,
            safe_height=obj.SafeHeight.Value,
            start_depth=strDep,
            step_down=obj.StepDown.Value,
            z_finish_step=finish_step,
            final_depth=finDep,
            user_depths=None,
        )
        shape = Part.makeCompound(Faces)
        env = PathUtils.getEnvelope(
            base[0].Shape, subshape=shape, depthparams=depthparams
        )
        cuts.append(env.cut(base[0].Shape))

        # Might need to change to .cut(job.Stock.Shape) if pocket has no bottom
        # job = PathUtils.findParentJob(obj)
        # envBody = env.cut(job.Stock.Shape)

        if isHighFacePlanar is True and len(subObjTups) > 1:
            starts.append(hzmax + obj.StepDown.Value)
            # make shape to trim top of reg pocket
            strDep1 = obj.StartDepth.Value + (hzmax - hzmin)
            if makeHighFace == 1:
                # Planar face
                finDep1 = highFace.Shape.BoundBox.ZMin + obj.StepDown.Value
            else:
                # Non-Planar face
                finDep1 = hzmin + obj.StepDown.Value
            depthparams1 = PathUtils.depth_params(
                clearance_height=obj.ClearanceHeight.Value,
                safe_height=obj.SafeHeight.Value,
                start_depth=strDep1,
                step_down=obj.StepDown.Value,
                z_finish_step=finish_step,
                final_depth=finDep1,
                user_depths=None,
            )
            envTop = PathUtils.getEnvelope(
                base[0].Shape, subshape=highFace.Shape, depthparams=depthparams1
            )
            cbi = len(cuts) - 1
            cuts.append(cuts[cbi].cut(envTop))

        if isLowFacePlanar is True and len(subObjTups) > 1:
            # make shape to trim top of pocket
            if makeHighFace == 1:
                # Planar face
                strDep2 = lowFace.Shape.BoundBox.ZMax
            else:
                # Non-Planar face
                strDep2 = hzmax
            finDep2 = obj.FinalDepth.Value
            depthparams2 = PathUtils.depth_params(
                clearance_height=obj.ClearanceHeight.Value,
                safe_height=obj.SafeHeight.Value,
                start_depth=strDep2,
                step_down=obj.StepDown.Value,
                z_finish_step=finish_step,
                final_depth=finDep2,
                user_depths=None,
            )
            envBottom = PathUtils.getEnvelope(
                base[0].Shape, subshape=lowFace.Shape, depthparams=depthparams2
            )
            cbi = len(cuts) - 1
            cuts.append(cuts[cbi].cut(envBottom))

        # package pocket details into tuple
        cbi = len(cuts) - 1
        pocket = (cuts[cbi], False, "3DPocket")
        if FreeCAD.GuiUp:
            import FreeCADGui

            for rn in removeList:
                FreeCADGui.ActiveDocument.getObject(rn).Visibility = False

        for rn in removeList:
            FreeCAD.ActiveDocument.getObject(rn).purgeTouched()
            self.tempObjectNames.append(rn)
        return pocket

    def orderFacesAroundCenterOfMass(self, subObjTups):
        """orderFacesAroundCenterOfMass(subObjTups)
        Order list of faces by center of mass in angular order around
        average center of mass for all faces. Positive X-axis is zero degrees.
        return: subObjTups [ordered/sorted]"""
        import math

        newList = []
        vectList = []
        comList = []
        sortList = []
        subCnt = 0
        sumCom = FreeCAD.Vector(0.0, 0.0, 0.0)
        avgCom = FreeCAD.Vector(0.0, 0.0, 0.0)

        def getDrctn(vectItem):
            return vectItem[3]

        def getFaceIdx(sub):
            return int(sub.replace("Face", "")) - 1

        # get CenterOfMass for each face and add to sumCenterOfMass for average calculation
        for (sub, face) in subObjTups:
            # for (bsNm, fIdx, eIdx, vIdx) in bfevList:
            # face = FreeCAD.ActiveDocument.getObject(bsNm).Shape.Faces[fIdx]
            subCnt += 1
            com = face.CenterOfMass
            comList.append((sub, face, com))
            sumCom = sumCom.add(com)  # add sub COM to sum

        # Calculate average CenterOfMass for all faces combined
        avgCom.x = sumCom.x / subCnt
        avgCom.y = sumCom.y / subCnt
        avgCom.z = sumCom.z / subCnt

        # calculate vector (mag, direct) for each face from avgCom
        for (sub, face, com) in comList:
            adjCom = com.sub(
                avgCom
            )  # effectively treats avgCom as origin for each face.
            mag = math.sqrt(
                adjCom.x**2 + adjCom.y**2
            )  # adjCom.Length without Z values
            drctn = 0.0
            # Determine direction of vector
            if adjCom.x > 0.0:
                if adjCom.y > 0.0:  # Q1
                    drctn = math.degrees(math.atan(adjCom.y / adjCom.x))
                elif adjCom.y < 0.0:
                    drctn = -math.degrees(math.atan(adjCom.x / adjCom.y)) + 270.0
                elif adjCom.y == 0.0:
                    drctn = 0.0
            elif adjCom.x < 0.0:
                if adjCom.y < 0.0:
                    drctn = math.degrees(math.atan(adjCom.y / adjCom.x)) + 180.0
                elif adjCom.y > 0.0:
                    drctn = -math.degrees(math.atan(adjCom.x / adjCom.y)) + 90.0
                elif adjCom.y == 0.0:
                    drctn = 180.0
            elif adjCom.x == 0.0:
                if adjCom.y < 0.0:
                    drctn = 270.0
                elif adjCom.y > 0.0:
                    drctn = 90.0
            vectList.append((sub, face, mag, drctn))

        # Sort faces by directional component of vector
        sortList = sorted(vectList, key=getDrctn)

        # remove magnitute and direction values
        for (sub, face, mag, drctn) in sortList:
            newList.append((sub, face))

        # Rotate list items so highest face is first
        zmax = newList[0][1].BoundBox.ZMax
        idx = 0
        for i in range(0, len(newList)):
            (sub, face) = newList[i]
            fIdx = getFaceIdx(sub)
            # face = FreeCAD.ActiveDocument.getObject(bsNm).Shape.Faces[fIdx]
            if face.BoundBox.ZMax > zmax:
                zmax = face.BoundBox.ZMax
                idx = i
            if face.BoundBox.ZMax == zmax:
                if fIdx < getFaceIdx(newList[idx][0]):
                    idx = i
        if idx > 0:
            for z in range(0, idx):
                newList.append(newList.pop(0))

        return newList

    def findSharedEdges(self, subObjTups):
        """findSharedEdges(self, subObjTups)
        Find connected edges given a group of faces"""
        checkoutList = []
        searchedList = []
        shared = []
        touching = {}
        touchingCleaned = {}

        # Prepare dictionary for edges in shared
        for (sub, face) in subObjTups:
            touching[sub] = []

        # prepare list of indexes as proxies for subObjTups items
        numFaces = len(subObjTups)
        for nf in range(0, numFaces):
            checkoutList.append(nf)

        for co in range(0, len(checkoutList)):
            if len(checkoutList) < 2:
                break

            # Checkout  first sub for analysis
            checkedOut1 = checkoutList.pop()
            searchedList.append(checkedOut1)
            (sub1, face1) = subObjTups[checkedOut1]

            # Compare checked out sub to others for shared
            for co in range(0, len(checkoutList)):
                # Checkout  second sub for analysis
                (sub2, face2) = subObjTups[co]

                # analyze two subs for common faces
                for ei1 in range(0, len(face1.Edges)):
                    edg1 = face1.Edges[ei1]
                    for ei2 in range(0, len(face2.Edges)):
                        edg2 = face2.Edges[ei2]
                        if edg1.isSame(edg2) is True:
                            Path.Log.debug(
                                "{}.Edges[{}] connects at {}.Edges[{}]".format(
                                    sub1, ei1, sub2, ei2
                                )
                            )
                            shared.append((sub1, face1, ei1))
                            touching[sub1].append(ei1)
                            touching[sub2].append(ei2)
        # Efor
        # Remove duplicates from edge lists
        for sub in touching:
            touchingCleaned[sub] = []
            for s in touching[sub]:
                if s not in touchingCleaned[sub]:
                    touchingCleaned[sub].append(s)

        return (shared, touchingCleaned)

    def identifyUnconnectedEdges(self, subObjTups, touching):
        """identifyUnconnectedEdges(subObjTups, touching)
        Categorize unconnected edges into two groups, if possible: low and high"""
        # Identify unconnected edges
        # (should be top edge loop if all faces form loop with bottom face(s) included)
        high = []
        low = []
        holding = []

        for (sub, face) in subObjTups:
            holding = []
            for ei in range(0, len(face.Edges)):
                if ei not in touching[sub]:
                    holding.append((sub, face, ei))
            # Assign unconnected edges based upon category: high or low
            if len(holding) == 1:
                high.append(holding.pop())
            elif len(holding) == 2:
                edg0 = holding[0][1].Edges[holding[0][2]]
                edg1 = holding[1][1].Edges[holding[1][2]]
                if self.hasCommonVertex(edg0, edg1, show=False) < 0:
                    # Edges not connected - probably top and bottom if faces in loop
                    if edg0.CenterOfMass.z > edg1.CenterOfMass.z:
                        high.append(holding[0])
                        low.append(holding[1])
                    else:
                        high.append(holding[1])
                        low.append(holding[0])
                else:
                    # Edges are connected - all top, or all bottom edges
                    com = FreeCAD.Vector(0, 0, 0)
                    com.add(edg0.CenterOfMass)
                    com.add(edg1.CenterOfMass)
                    avgCom = FreeCAD.Vector(com.x / 2.0, com.y / 2.0, com.z / 2.0)
                    if avgCom.z > face.CenterOfMass.z:
                        high.extend(holding)
                    else:
                        low.extend(holding)
            elif len(holding) > 2:
                # attempt to break edges into two groups of connected edges.
                # determine which group has higher center of mass, and assign as high, the other as low
                (lw, hgh) = self.groupConnectedEdges(holding)
                low.extend(lw)
                high.extend(hgh)
            # Eif
        # Efor
        return (low, high)

    def hasCommonVertex(self, edge1, edge2, show=False):
        """findCommonVertexIndexes(edge1, edge2, show=False)
        Compare vertexes of two edges to identify a common vertex.
        Returns the vertex index of edge1 to which edge2 is connected"""
        if show is True:
            Path.Log.info("New findCommonVertex()... ")

        oIdx = 0
        listOne = edge1.Vertexes
        listTwo = edge2.Vertexes

        # Find common vertexes
        for o in listOne:
            if show is True:
                Path.Log.info("   one ({}, {}, {})".format(o.X, o.Y, o.Z))
            for t in listTwo:
                if show is True:
                    Path.Log.error("two ({}, {}, {})".format(t.X, t.Y, t.Z))
                if o.X == t.X:
                    if o.Y == t.Y:
                        if o.Z == t.Z:
                            if show is True:
                                Path.Log.info("found")
                            return oIdx
            oIdx += 1
        return -1

    def groupConnectedEdges(self, holding):
        """groupConnectedEdges(self, holding)
        Take edges and determine which are connected.
        Group connected chains/loops into: low and high"""
        holds = []
        grps = []
        searched = []
        stop = False
        attachments = []
        loops = 1

        def updateAttachments(grps):
            atchmnts = []
            lenGrps = len(grps)
            if lenGrps > 0:
                lenG0 = len(grps[0])
                if lenG0 < 2:
                    atchmnts.append((0, 0))
                else:
                    atchmnts.append((0, 0))
                    atchmnts.append((0, lenG0 - 1))
            if lenGrps == 2:
                lenG1 = len(grps[1])
                if lenG1 < 2:
                    atchmnts.append((1, 0))
                else:
                    atchmnts.append((1, 0))
                    atchmnts.append((1, lenG1 - 1))
            return atchmnts

        def isSameVertex(o, t):
            if o.X == t.X:
                if o.Y == t.Y:
                    if o.Z == t.Z:
                        return True
            return False

        for hi in range(0, len(holding)):
            holds.append(hi)

        # Place initial edge in first group and update attachments
        h0 = holds.pop()
        grps.append([h0])
        attachments = updateAttachments(grps)

        while len(holds) > 0:
            if loops > 500:
                Path.Log.error("BREAK --- LOOPS LIMIT of 500 ---")
                break
            save = False

            h2 = holds.pop()
            (sub2, face2, ei2) = holding[h2]

            # Cycle through attachments for connection to existing
            for (g, t) in attachments:
                h1 = grps[g][t]
                (sub1, face1, ei1) = holding[h1]

                edg1 = face1.Edges[ei1]
                edg2 = face2.Edges[ei2]

                # CV = self.hasCommonVertex(edg1, edg2, show=False)

                # Check attachment based on attachments order
                if t == 0:
                    # is last vertex of h2 == first vertex of h1
                    e2lv = len(edg2.Vertexes) - 1
                    one = edg2.Vertexes[e2lv]
                    two = edg1.Vertexes[0]
                    if isSameVertex(one, two) is True:
                        # Connected, insert h1 in front of h2
                        grps[g].insert(0, h2)
                        stop = True
                else:
                    # is last vertex of h1 == first vertex of h2
                    e1lv = len(edg1.Vertexes) - 1
                    one = edg1.Vertexes[e1lv]
                    two = edg2.Vertexes[0]
                    if isSameVertex(one, two) is True:
                        # Connected, append h1 after h2
                        grps[g].append(h2)
                        stop = True

                if stop is True:
                    # attachment was found
                    attachments = updateAttachments(grps)
                    holds.extend(searched)
                    stop = False
                    break
                else:
                    # no attachment found
                    save = True
            # Efor
            if save is True:
                searched.append(h2)
                if len(holds) == 0:
                    if len(grps) == 1:
                        h0 = searched.pop(0)
                        grps.append([h0])
                        attachments = updateAttachments(grps)
                        holds.extend(searched)
            # Eif
            loops += 1
        # Ewhile

        low = []
        high = []
        if len(grps) == 1:
            grps.append([])
        grp0 = []
        grp1 = []
        com0 = FreeCAD.Vector(0, 0, 0)
        com1 = FreeCAD.Vector(0, 0, 0)
        if len(grps[0]) > 0:
            for g in grps[0]:
                grp0.append(holding[g])
                (sub, face, ei) = holding[g]
                com0 = com0.add(face.Edges[ei].CenterOfMass)
            com0z = com0.z / len(grps[0])
        if len(grps[1]) > 0:
            for g in grps[1]:
                grp1.append(holding[g])
                (sub, face, ei) = holding[g]
                com1 = com1.add(face.Edges[ei].CenterOfMass)
            com1z = com1.z / len(grps[1])

        if len(grps[1]) > 0:
            if com0z > com1z:
                low = grp1
                high = grp0
            else:
                low = grp0
                high = grp1
        else:
            low = grp0
            high = grp0

        return (low, high)

    def getMinMaxOfFaces(self, Faces):
        """getMinMaxOfFaces(Faces)
        return the zmin and zmax values for given set of faces or edges."""
        zmin = Faces[0].BoundBox.ZMax
        zmax = Faces[0].BoundBox.ZMin
        for f in Faces:
            if f.BoundBox.ZMin < zmin:
                zmin = f.BoundBox.ZMin
            if f.BoundBox.ZMax > zmax:
                zmax = f.BoundBox.ZMax
        return (zmin, zmax)


def _identifyRemovalSolids(sourceShape, commonShapes):
    """_identifyRemovalSolids(sourceShape, commonShapes)
    Loops through solids in sourceShape to identify commonality with solids in commonShapes.
    The sourceShape solids with commonality are returned as Part.Compound shape."""
    common = Part.makeCompound(commonShapes)
    removalSolids = [s for s in sourceShape.Solids if s.common(common).Volume > 0.0]
    return Part.makeCompound(removalSolids)


def _extrudeBaseDown(base):
    """_extrudeBaseDown(base)
    Extrudes and fuses all non-vertical faces downward to a level 1.0 mm below base ZMin."""
    allExtrusions = list()
    zMin = base.Shape.BoundBox.ZMin
    bbFace = Path.Geom.makeBoundBoxFace(base.Shape.BoundBox, offset=5.0)
    bbFace.translate(
        FreeCAD.Vector(0.0, 0.0, float(int(base.Shape.BoundBox.ZMin - 5.0)))
    )
    direction = FreeCAD.Vector(0.0, 0.0, -1.0)

    # Make projections of each non-vertical face and extrude it
    for f in base.Shape.Faces:
        fbb = f.BoundBox
        if not Path.Geom.isRoughly(f.normalAt(0, 0).z, 0.0):
            pp = bbFace.makeParallelProjection(f.Wires[0], direction)
            face = Part.Face(Part.Wire(pp.Edges))
            face.translate(FreeCAD.Vector(0.0, 0.0, fbb.ZMin))
            ext = face.extrude(FreeCAD.Vector(0.0, 0.0, zMin - fbb.ZMin - 1.0))
            allExtrusions.append(ext)

    # Fuse all extrusions together
    seed = allExtrusions.pop()
    fusion = seed.fuse(allExtrusions)
    fusion.translate(FreeCAD.Vector(0.0, 0.0, zMin - fusion.BoundBox.ZMin - 1.0))

    return fusion.cut(base.Shape)


def SetupProperties():
    return PathPocketBase.SetupProperties() + ["HandleMultipleFeatures"]


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Pocket operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectPocket(obj, name, parentJob)
    return obj
