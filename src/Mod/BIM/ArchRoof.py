# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2012 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD Roof"
__author__ = "Yorik van Havre", "Jonathan Wiedemann"
__url__ = "https://www.freecad.org"

## @package ArchRoof
#  \ingroup ARCH
#  \brief The Roof object and tools
#
#  This module provides tools to build Roof objects.
#  Roofs are built from a closed contour and a series of
#  slopes.

import math

import FreeCAD
import ArchComponent
import DraftGeomUtils
import DraftVecUtils
import Part

from FreeCAD import Units
from FreeCAD import Vector

if FreeCAD.GuiUp:
    from PySide import QtCore, QtGui, QtWidgets
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    from draftutils.translate import translate
else:
    # \cond
    def translate(ctxt, txt):
        return txt

    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt

    # \endcond


def adjust_list_len(lst, newLn, val):
    """Returns a clone of lst with length newLn, val is appended if required"""
    ln = len(lst)
    if ln > newLn:
        return lst[0:newLn]
    else:
        return lst[:] + ([val] * (newLn - ln))


def find_inters(edge1, edge2, infinite1=True, infinite2=True):
    """Future wrapper for DraftGeomUtils.findIntersection. The function now
    contains a modified copy of getLineIntersections from that function.
    """

    def getLineIntersections(pt1, pt2, pt3, pt4, infinite1, infinite2):
        # if pt1:
        ## first check if we don't already have coincident endpoints ######## we do not want that here ########
        # if pt1 in [pt3, pt4]:
        # return [pt1]
        # elif (pt2 in [pt3, pt4]):
        # return [pt2]
        norm1 = pt2.sub(pt1).cross(pt3.sub(pt1))
        norm2 = pt2.sub(pt4).cross(pt3.sub(pt4))

        if not DraftVecUtils.isNull(norm1):
            try:
                norm1.normalize()
            except Part.OCCError:
                return []

        if not DraftVecUtils.isNull(norm2):
            try:
                norm2.normalize()
            except Part.OCCError:
                return []

        if DraftVecUtils.isNull(norm1.cross(norm2)):
            vec1 = pt2.sub(pt1)
            vec2 = pt4.sub(pt3)
            if DraftVecUtils.isNull(vec1) or DraftVecUtils.isNull(vec2):
                return []  # One of the lines has zero-length
            try:
                vec1.normalize()
                vec2.normalize()
            except Part.OCCError:
                return []
            norm3 = vec1.cross(vec2)
            denom = norm3.x + norm3.y + norm3.z
            if not DraftVecUtils.isNull(norm3) and denom != 0:
                k = (
                    (pt3.z - pt1.z) * (vec2.x - vec2.y)
                    + (pt3.y - pt1.y) * (vec2.z - vec2.x)
                    + (pt3.x - pt1.x) * (vec2.y - vec2.z)
                ) / denom
                vec1.scale(k, k, k)
                intp = pt1.add(vec1)

                if infinite1 is False and not isPtOnEdge(intp, edge1):
                    return []

                if infinite2 is False and not isPtOnEdge(intp, edge2):
                    return []

                return [intp]
            else:
                return []  # Lines have same direction
        else:
            return []  # Lines aren't on same plane

    pt1, pt2, pt3, pt4 = [
        edge1.Vertexes[0].Point,
        edge1.Vertexes[1].Point,
        edge2.Vertexes[0].Point,
        edge2.Vertexes[1].Point,
    ]

    return getLineIntersections(pt1, pt2, pt3, pt4, infinite1, infinite2)


def face_from_points(ptLst):
    ptLst.append(ptLst[0])
    # Use DraftVecUtils.removeDouble after append as it does not compare the first and last vector:
    ptLst = DraftVecUtils.removeDoubles(ptLst)
    ln = len(ptLst)
    if ln < 4:  # at least 4 points are required for 3 edges
        return None
    edgeLst = []
    for i in range(ln - 1):
        edge = Part.makeLine(ptLst[i], ptLst[i + 1])
        edgeLst.append(edge)
    wire = Part.Wire(edgeLst)
    return Part.Face(wire)


class _Roof(ArchComponent.Component):
    """The Roof object"""

    def __init__(self, obj):
        ArchComponent.Component.__init__(self, obj)
        self.Type = "Roof"
        self.setProperties(obj)
        obj.IfcType = "Roof"

    def setProperties(self, obj):
        pl = obj.PropertiesList
        if not "Angles" in pl:
            obj.addProperty(
                "App::PropertyFloatList",
                "Angles",
                "Roof",
                QT_TRANSLATE_NOOP("App::Property", "The list of angles of the roof segments"),
                locked=True,
            )
        if not "Runs" in pl:
            obj.addProperty(
                "App::PropertyFloatList",
                "Runs",
                "Roof",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The list of horizontal length projections of the roof segments",
                ),
                locked=True,
            )
        if not "IdRel" in pl:
            obj.addProperty(
                "App::PropertyIntegerList",
                "IdRel",
                "Roof",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The list of IDs of the relative profiles of the roof segments"
                ),
                locked=True,
            )
        if not "Thickness" in pl:
            obj.addProperty(
                "App::PropertyFloatList",
                "Thickness",
                "Roof",
                QT_TRANSLATE_NOOP("App::Property", "The list of thicknesses of the roof segments"),
                locked=True,
            )
        if not "Overhang" in pl:
            obj.addProperty(
                "App::PropertyFloatList",
                "Overhang",
                "Roof",
                QT_TRANSLATE_NOOP("App::Property", "The list of overhangs of the roof segments"),
                locked=True,
            )
        if not "Heights" in pl:
            obj.addProperty(
                "App::PropertyFloatList",
                "Heights",
                "Roof",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The list of calculated heights of the roof segments"
                ),
                locked=True,
            )
        if not "Face" in pl:
            obj.addProperty(
                "App::PropertyInteger",
                "Face",
                "Roof",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The face number of the base object used to build the roof"
                ),
                locked=True,
            )
        if not "RidgeLength" in pl:
            obj.addProperty(
                "App::PropertyLength",
                "RidgeLength",
                "Roof",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The total length of the ridges and hips of the roof"
                ),
                locked=True,
            )
            obj.setEditorMode("RidgeLength", 1)
        if not "BorderLength" in pl:
            obj.addProperty(
                "App::PropertyLength",
                "BorderLength",
                "Roof",
                QT_TRANSLATE_NOOP("App::Property", "The total length of the borders of the roof"),
                locked=True,
            )
            obj.setEditorMode("BorderLength", 1)
        if not "Flip" in pl:
            obj.addProperty(
                "App::PropertyBool",
                "Flip",
                "Roof",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Specifies if the direction of the roof should be flipped"
                ),
                locked=True,
            )
        if not "Subvolume" in pl:
            obj.addProperty(
                "App::PropertyLink",
                "Subvolume",
                "Roof",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "An optional object that defines a volume to be subtracted from walls. If field is set - it has a priority over auto-generated subvolume",
                ),
                locked=True,
            )

    def onDocumentRestored(self, obj):
        ArchComponent.Component.onDocumentRestored(self, obj)
        self.setProperties(obj)

    def loads(self, state):
        self.Type = "Roof"

    def flipEdges(self, edges):
        edges.reverse()
        newEdges = []
        for edge in edges:
            NewEdge = DraftGeomUtils.edg(edge.Vertexes[1].Point, edge.Vertexes[0].Point)
            newEdges.append(NewEdge)
        return newEdges

    def calcHeight(self, id):
        """Get the height from run and angle of the given roof profile"""
        htRel = self.profilsDico[id]["run"] * (
            math.tan(math.radians(self.profilsDico[id]["angle"]))
        )
        return htRel

    def calcRun(self, id):
        """Get the run from height and angle of the given roof profile"""
        runRel = self.profilsDico[id]["height"] / (
            math.tan(math.radians(self.profilsDico[id]["angle"]))
        )
        return runRel

    def calcAngle(self, id):
        """Get the angle from height and run of the given roof profile"""
        ang = math.degrees(math.atan(self.profilsDico[id]["height"] / self.profilsDico[id]["run"]))
        return ang

    def getPerpendicular(self, vec, rotEdge, l):
        """Get the perpendicular vec of given edge on xy plane"""
        norm = Vector(0.0, 0.0, 1.0)
        if hasattr(self, "normal"):
            if self.normal:
                norm = self.normal
        per = vec.cross(norm)
        if -180.0 <= rotEdge < -90.0:
            per[0] = -abs(per[0])
            per[1] = -abs(per[1])
        elif -90.0 <= rotEdge <= 0.0:
            per[0] = -abs(per[0])
            per[1] = abs(per[1])
        elif 0.0 < rotEdge <= 90.0:
            per[0] = abs(per[0])
            per[1] = abs(per[1])
        elif 90.0 < rotEdge <= 180.0:
            per[0] = abs(per[0])
            per[1] = -abs(per[1])
        else:
            print("Unknown Angle")
        per[2] = abs(per[2])
        per.normalize()
        per = per.multiply(l)
        return per

    def makeRoofProfilsDic(self, id, angle, run, idrel, overhang, thickness):
        profilDico = {}
        profilDico["id"] = id
        if angle == 90.0:
            profilDico["name"] = "Gable" + str(id)
            profilDico["run"] = 0.0
        else:
            profilDico["name"] = "Sloped" + str(id)
            profilDico["run"] = run
        profilDico["angle"] = angle
        profilDico["idrel"] = idrel
        profilDico["overhang"] = overhang
        profilDico["thickness"] = thickness
        profilDico["height"] = None
        profilDico["points"] = []
        self.profilsDico.append(profilDico)

    def calcEdgeGeometry(self, i, edge):
        profilCurr = self.profilsDico[i]
        profilCurr["edge"] = edge
        vec = edge.Vertexes[1].Point.sub(edge.Vertexes[0].Point)
        profilCurr["vec"] = vec
        rot = math.degrees(DraftVecUtils.angle(vec))
        profilCurr["rot"] = rot

    def helperCalcApex(self, profilCurr, profilOpposite):
        ptCurr = profilCurr["edge"].Vertexes[0].Point
        ptOpposite = profilOpposite["edge"].Vertexes[0].Point
        dis = ptCurr.distanceToLine(ptOpposite, profilOpposite["vec"])
        if dis < profilCurr["run"] + profilOpposite["run"]:  # sum of runs is larger than dis
            angCurr = profilCurr["angle"]
            angOpposite = profilOpposite["angle"]
            return dis / (
                math.tan(math.radians(angCurr)) / math.tan(math.radians(angOpposite)) + 1.0
            )
        return profilCurr["run"]

    def calcApex(self, i, numEdges):
        """Recalculate the run and height if there is an opposite roof segment
        with a parallel edge, and if the sum of the runs of the segments is
        larger than the distance between the edges of the segments.
        """
        profilCurr = self.findProfil(i)
        if 0 <= profilCurr["idrel"] < numEdges:  # no apex calculation if idrel is used
            return
        if not 0.0 < profilCurr["angle"] < 90.0:
            return
        profilNext2 = self.findProfil(i + 2)
        profilBack2 = self.findProfil(i - 2)
        vecCurr = profilCurr["vec"]
        vecNext2 = profilNext2["vec"]
        vecBack2 = profilBack2["vec"]
        runs = []
        if (
            (not 0 <= profilNext2["idrel"] < numEdges)
            and 0.0 < profilNext2["angle"] < 90.0
            and math.isclose(vecCurr.getAngle(vecNext2), math.pi, abs_tol=1e-7)
        ):
            runs.append((self.helperCalcApex(profilCurr, profilNext2)))
        if (
            (not 0 <= profilBack2["idrel"] < numEdges)
            and 0.0 < profilBack2["angle"] < 90.0
            and math.isclose(vecCurr.getAngle(vecBack2), math.pi, abs_tol=1e-7)
        ):
            runs.append((self.helperCalcApex(profilCurr, profilBack2)))
        runs.sort()
        if len(runs) != 0 and runs[0] != profilCurr["run"]:
            profilCurr["run"] = runs[0]
            hgt = self.calcHeight(i)
            profilCurr["height"] = hgt

    def calcMissingData(self, i, numEdges):
        profilCurr = self.profilsDico[i]
        ang = profilCurr["angle"]
        run = profilCurr["run"]
        rel = profilCurr["idrel"]
        if i != rel and 0 <= rel < numEdges:
            profilRel = self.profilsDico[rel]
            # do not use data from the relative profile if it in turn references a relative profile:
            if (
                0 <= profilRel["idrel"] < numEdges  # idrel of profilRel points to a profile
                and rel != profilRel["idrel"]  # profilRel does not reference itself
                and (profilRel["angle"] == 0.0 or profilRel["run"] == 0.0)
            ):  # run or angle of profilRel is zero
                hgt = self.calcHeight(i)
                profilCurr["height"] = hgt
            elif ang == 0.0 and run == 0.0:
                profilCurr["run"] = profilRel["run"]
                profilCurr["angle"] = profilRel["angle"]
                profilCurr["height"] = self.calcHeight(i)
            elif run == 0.0:
                if ang == 90.0:
                    htRel = self.calcHeight(rel)
                    profilCurr["height"] = htRel
                else:
                    htRel = self.calcHeight(rel)
                    profilCurr["height"] = htRel
                    run = self.calcRun(i)
                    profilCurr["run"] = run
            elif ang == 0.0:
                htRel = self.calcHeight(rel)
                profilCurr["height"] = htRel
                ang = self.calcAngle(i)
                profilCurr["angle"] = ang
            else:
                hgt = self.calcHeight(i)
                profilCurr["height"] = hgt
        else:
            hgt = self.calcHeight(i)
            profilCurr["height"] = hgt

    def calcDraftEdges(self, i):
        profilCurr = self.profilsDico[i]
        edge = profilCurr["edge"]
        vec = profilCurr["vec"]
        rot = profilCurr["rot"]
        ang = profilCurr["angle"]
        run = profilCurr["run"]
        if ang != 90 and run == 0.0:
            overhang = 0.0
        else:
            overhang = profilCurr["overhang"]
        per = self.getPerpendicular(vec, rot, overhang).negative()
        eaveDraft = DraftGeomUtils.offset(edge, per)
        profilCurr["eaveDraft"] = eaveDraft
        per = self.getPerpendicular(vec, rot, run)
        ridge = DraftGeomUtils.offset(edge, per)
        profilCurr["ridge"] = ridge

    def calcEave(self, i):
        profilCurr = self.findProfil(i)
        ptInterEaves1Lst = find_inters(profilCurr["eaveDraft"], self.findProfil(i - 1)["eaveDraft"])
        if ptInterEaves1Lst:
            ptInterEaves1 = ptInterEaves1Lst[0]
        else:
            ptInterEaves1 = profilCurr["eaveDraft"].Vertexes[0].Point
        ptInterEaves2Lst = find_inters(profilCurr["eaveDraft"], self.findProfil(i + 1)["eaveDraft"])
        if ptInterEaves2Lst:
            ptInterEaves2 = ptInterEaves2Lst[0]
        else:
            ptInterEaves2 = profilCurr["eaveDraft"].Vertexes[1].Point
        profilCurr["eavePtLst"] = [
            ptInterEaves1,
            ptInterEaves2,
        ]  # list of points instead of edge as points can be identical

    def findProfil(self, i):
        if 0 <= i < len(self.profilsDico):
            profil = self.profilsDico[i]
        else:
            i = abs(abs(i) - len(self.profilsDico))
            profil = self.profilsDico[i]
        return profil

    def helperGable(self, profilCurr, profilOther, isBack):
        if isBack:
            i = 0
        else:
            i = 1
        ptIntLst = find_inters(profilCurr["ridge"], profilOther["eaveDraft"])
        if ptIntLst:  # the edges of the roof segments are not parallel
            ptProjLst = [ptIntLst[0]]
        else:  # the edges of the roof segments are parallel
            ptProjLst = [profilCurr["ridge"].Vertexes[i].Point]
        ptProjLst = ptProjLst + [profilCurr["eavePtLst"][i]]
        if not isBack:
            ptProjLst.reverse()
        for ptProj in ptProjLst:
            self.ptsPaneProject.append(ptProj)

    def backGable(self, i):
        profilCurr = self.findProfil(i)
        profilBack = self.findProfil(i - 1)
        self.helperGable(profilCurr, profilBack, isBack=True)

    def nextGable(self, i):
        profilCurr = self.findProfil(i)
        profilNext = self.findProfil(i + 1)
        self.helperGable(profilCurr, profilNext, isBack=False)

    def helperSloped(
        self, profilCurr, profilOther, ridgeCurr, ridgeOther, isBack, otherIsLower=False
    ):
        if isBack:
            i = 0
        else:
            i = 1
        ptIntLst = find_inters(ridgeCurr, ridgeOther)
        if ptIntLst:  # the edges of the roof segments are not parallel
            ptInt = ptIntLst[0]
            if otherIsLower:
                ptRidgeLst = find_inters(profilCurr["ridge"], profilOther["ridge"])
                ptProjLst = [ptRidgeLst[0], ptInt]
            else:
                ptProjLst = [ptInt]
            hip = DraftGeomUtils.edg(ptInt, profilCurr["edge"].Vertexes[i].Point)
            ptEaveCurrLst = find_inters(hip, profilCurr["eaveDraft"])
            ptEaveOtherLst = find_inters(hip, profilOther["eaveDraft"])
            if ptEaveCurrLst and ptEaveOtherLst:  # both roof segments are sloped
                lenToEaveCurr = ptEaveCurrLst[0].sub(ptInt).Length
                lenToEaveOther = ptEaveOtherLst[0].sub(ptInt).Length
                if lenToEaveCurr < lenToEaveOther:
                    ptProjLst = ptProjLst + [ptEaveCurrLst[0]]
                else:
                    ptProjLst = ptProjLst + [ptEaveOtherLst[0], profilCurr["eavePtLst"][i]]
            elif ptEaveCurrLst:  # current angle is 0
                ptProjLst = ptProjLst + [ptEaveCurrLst[0]]
            elif ptEaveOtherLst:  # other angle is 0
                ptProjLst = ptProjLst + [ptEaveOtherLst[0], profilCurr["eavePtLst"][i]]
            else:
                print("Error determining outline")
        else:  # the edges of the roof segments are parallel
            ptProjLst = [profilCurr["ridge"].Vertexes[i].Point, profilCurr["eavePtLst"][i]]
        if not isBack:
            ptProjLst.reverse()
        for ptProj in ptProjLst:
            self.ptsPaneProject.append(ptProj)

    def backSameHeight(self, i):
        profilCurr = self.findProfil(i)
        profilBack = self.findProfil(i - 1)
        self.helperSloped(
            profilCurr, profilBack, profilCurr["ridge"], profilBack["ridge"], isBack=True
        )

    def nextSameHeight(self, i):
        profilCurr = self.findProfil(i)
        profilNext = self.findProfil(i + 1)
        self.helperSloped(
            profilCurr, profilNext, profilCurr["ridge"], profilNext["ridge"], isBack=False
        )

    def backHigher(self, i):
        profilCurr = self.findProfil(i)
        profilBack = self.findProfil(i - 1)
        dec = profilCurr["height"] / math.tan(math.radians(profilBack["angle"]))
        per = self.getPerpendicular(profilBack["vec"], profilBack["rot"], dec)
        edgeRidgeOnPane = DraftGeomUtils.offset(profilBack["edge"], per)
        self.helperSloped(profilCurr, profilBack, profilCurr["ridge"], edgeRidgeOnPane, isBack=True)

    def nextHigher(self, i):
        profilCurr = self.findProfil(i)
        profilNext = self.findProfil(i + 1)
        dec = profilCurr["height"] / math.tan(math.radians(profilNext["angle"]))
        per = self.getPerpendicular(profilNext["vec"], profilNext["rot"], dec)
        edgeRidgeOnPane = DraftGeomUtils.offset(profilNext["edge"], per)
        self.helperSloped(
            profilCurr, profilNext, profilCurr["ridge"], edgeRidgeOnPane, isBack=False
        )

    def backLower(self, i):
        profilCurr = self.findProfil(i)
        profilBack = self.findProfil(i - 1)
        dec = profilBack["height"] / math.tan(math.radians(profilCurr["angle"]))
        per = self.getPerpendicular(profilCurr["vec"], profilCurr["rot"], dec)
        edgeRidgeOnPane = DraftGeomUtils.offset(profilCurr["edge"], per)
        self.helperSloped(
            profilCurr,
            profilBack,
            edgeRidgeOnPane,
            profilBack["ridge"],
            isBack=True,
            otherIsLower=True,
        )

    def nextLower(self, i):
        profilCurr = self.findProfil(i)
        profilNext = self.findProfil(i + 1)
        dec = profilNext["height"] / math.tan(math.radians(profilCurr["angle"]))
        per = self.getPerpendicular(profilCurr["vec"], profilCurr["rot"], dec)
        edgeRidgeOnPane = DraftGeomUtils.offset(profilCurr["edge"], per)
        self.helperSloped(
            profilCurr,
            profilNext,
            edgeRidgeOnPane,
            profilNext["ridge"],
            isBack=False,
            otherIsLower=True,
        )

    def getRoofPaneProject(self, i):
        self.ptsPaneProject = []
        profilCurr = self.findProfil(i)
        profilBack = self.findProfil(i - 1)
        profilNext = self.findProfil(i + 1)
        if profilCurr["angle"] == 90.0 or profilCurr["run"] == 0.0:
            self.ptsPaneProject = []
        else:
            if profilBack["angle"] == 90.0 or profilBack["run"] == 0.0:
                self.backGable(i)
            elif profilBack["height"] == profilCurr["height"]:
                self.backSameHeight(i)
            elif profilBack["height"] < profilCurr["height"]:
                self.backLower(i)
            elif profilBack["height"] > profilCurr["height"]:
                self.backHigher(i)
            else:
                print("Arch Roof: Case not implemented")

            if profilNext["angle"] == 90.0 or profilNext["run"] == 0.0:
                self.nextGable(i)
            elif profilNext["height"] == profilCurr["height"]:
                self.nextSameHeight(i)
            elif profilNext["height"] < profilCurr["height"]:
                self.nextLower(i)
            elif profilNext["height"] > profilCurr["height"]:
                self.nextHigher(i)
            else:
                print("Arch Roof: Case not implemented")

        profilCurr["points"] = self.ptsPaneProject

    def createProfilShape(self, points, midpoint, rot, vec, run, diag, sol):
        lp = len(points)
        points.append(points[0])
        edgesWire = []
        for i in range(lp):
            edge = Part.makeLine(points[i], points[i + 1])
            edgesWire.append(edge)
        profil = Part.Wire(edgesWire)
        profil.translate(midpoint)
        profil.rotate(midpoint, Vector(0.0, 0.0, 1.0), 90.0 - rot)
        per = self.getPerpendicular(vec, rot, run)
        profil.rotate(midpoint, per, 90.0)
        vecT = vec.normalize()
        vecT.multiply(diag)
        profil.translate(vecT)
        vecE = vecT.multiply(-2.0)
        profilFace = Part.Face(profil)
        profilShp = profilFace.extrude(vecE)
        profilShp = sol.common(profilShp)
        # shapesList.append(profilShp)
        return profilShp

    def execute(self, obj):

        if self.clone(obj):
            return
        if not self.ensureBase(obj):
            return

        pl = obj.Placement
        # self.baseface = None
        self.flip = False
        if hasattr(obj, "Flip"):
            if obj.Flip:
                self.flip = True
        base = None
        baseWire = None
        if obj.Base:
            if hasattr(obj.Base, "Shape"):
                if obj.Base.Shape.Solids:
                    base = obj.Base.Shape
                    # pl = obj.Base.Placement
                else:
                    if obj.Base.Shape.Faces and obj.Face:
                        baseWire = obj.Base.Shape.Faces[obj.Face - 1].Wires[0]
                    elif obj.Base.Shape.Wires:
                        baseWire = obj.Base.Shape.Wires[0]
        if baseWire:
            if baseWire.isClosed():
                self.profilsDico = []
                self.shps = []
                self.subVolShps = []
                heights = []
                edges = Part.__sortEdges__(baseWire.Edges)
                if self.flip:
                    edges = self.flipEdges(edges)

                ln = len(edges)

                obj.Angles = adjust_list_len(obj.Angles, ln, obj.Angles[0])
                obj.Runs = adjust_list_len(obj.Runs, ln, obj.Runs[0])
                obj.IdRel = adjust_list_len(obj.IdRel, ln, obj.IdRel[0])
                obj.Thickness = adjust_list_len(obj.Thickness, ln, obj.Thickness[0])
                obj.Overhang = adjust_list_len(obj.Overhang, ln, obj.Overhang[0])

                for i in range(ln):
                    self.makeRoofProfilsDic(
                        i,
                        obj.Angles[i],
                        obj.Runs[i],
                        obj.IdRel[i],
                        obj.Overhang[i],
                        obj.Thickness[i],
                    )
                for i in range(ln):
                    self.calcEdgeGeometry(i, edges[i])
                for i in range(ln):
                    self.calcApex(i, ln)  # after calcEdgeGeometry as it uses vec data
                for i in range(ln):
                    self.calcMissingData(i, ln)  # after calcApex so it can use recalculated heights
                for i in range(ln):
                    self.calcDraftEdges(i)
                for i in range(ln):
                    self.calcEave(i)
                for profil in self.profilsDico:
                    heights.append(profil["height"])
                obj.Heights = heights
                for i in range(ln):
                    self.getRoofPaneProject(i)
                    profilCurr = self.profilsDico[i]
                    ptsPaneProject = profilCurr["points"]
                    if len(ptsPaneProject) == 0:
                        continue
                    face = face_from_points(ptsPaneProject)
                    if face:
                        diag = face.BoundBox.DiagonalLength
                        midpoint = DraftGeomUtils.findMidpoint(profilCurr["edge"])
                        thicknessV = profilCurr["thickness"] / (
                            math.cos(math.radians(profilCurr["angle"]))
                        )
                        overhangV = profilCurr["overhang"] * math.tan(
                            math.radians(profilCurr["angle"])
                        )
                        sol = face.extrude(Vector(0.0, 0.0, profilCurr["height"] + 1000000.0))
                        sol.translate(Vector(0.0, 0.0, -2.0 * overhangV))

                        ## baseVolume shape
                        ptsPaneProfil = [
                            Vector(-profilCurr["overhang"], -overhangV, 0.0),
                            Vector(profilCurr["run"], profilCurr["height"], 0.0),
                            Vector(profilCurr["run"], profilCurr["height"] + thicknessV, 0.0),
                            Vector(-profilCurr["overhang"], -overhangV + thicknessV, 0.0),
                        ]
                        self.shps.append(
                            self.createProfilShape(
                                ptsPaneProfil,
                                midpoint,
                                profilCurr["rot"],
                                profilCurr["vec"],
                                profilCurr["run"],
                                diag,
                                sol,
                            )
                        )

                        ## subVolume shape
                        ptsSubVolProfil = [
                            Vector(-profilCurr["overhang"], -overhangV, 0.0),
                            Vector(profilCurr["run"], profilCurr["height"], 0.0),
                            Vector(profilCurr["run"], profilCurr["height"] + 900000.0, 0.0),
                            Vector(-profilCurr["overhang"], profilCurr["height"] + 900000.0, 0.0),
                        ]
                        self.subVolShps.append(
                            self.createProfilShape(
                                ptsSubVolProfil,
                                midpoint,
                                profilCurr["rot"],
                                profilCurr["vec"],
                                profilCurr["run"],
                                diag,
                                sol,
                            )
                        )

                if len(self.shps) == 0:  # occurs if all segments have angle=90 or run=0.
                    # create a flat roof using the eavePtLst outline:
                    ptsPaneProject = []
                    for i in range(ln):
                        ptsPaneProject.append(self.profilsDico[i]["eavePtLst"][0])
                    face = face_from_points(ptsPaneProject)
                    if face:
                        thk = max(
                            1.0, self.profilsDico[0]["thickness"]
                        )  # FreeCAD will crash when extruding with a null vector here
                        self.shps = [face.extrude(Vector(0.0, 0.0, thk))]
                        self.subVolShps = [face.extrude(Vector(0.0, 0.0, 1000000.0))]

                ## baseVolume
                base = self.shps.pop()
                for s in self.shps:
                    base = base.fuse(s)
                base = self.processSubShapes(obj, base, pl)
                self.applyShape(obj, base, pl, allownosolid=True)

                ## subVolume
                self.sub = self.subVolShps.pop()
                for s in self.subVolShps:
                    self.sub = self.sub.fuse(s)
                self.sub = self.sub.removeSplitter()
                if not self.sub.isNull():
                    if not DraftGeomUtils.isNull(pl):
                        self.sub.Placement = pl

        elif base:
            base = self.processSubShapes(obj, base, pl)
            self.applyShape(obj, base, pl, allownosolid=True)
        else:
            FreeCAD.Console.PrintMessage(translate("Arch", "Unable to create a roof"))

    def getSubVolume(self, obj):
        """returns a volume to be subtracted"""
        custom_subvolume = getattr(obj, "Subvolume", None)
        if custom_subvolume:
            return custom_subvolume.Shape

        if not obj.Base:
            return None

        if not hasattr(obj.Base, "Shape"):
            return None

        if obj.Base.Shape.Solids:
            # For roof created from Base object as solids:
            # Not only the solid of the base object itself be subtracted from
            # a Wall, but all portion of the wall above the roof solid would be
            # subtracted as well.
            #

            # FC forum discussion, 2024.1.15 :
            # Sketch based Arch_Roof and wall substraction
            # - https://forum.freecad.org/viewtopic.php?t=84389
            #
            # Github issue #21633, 2025.5.29 :
            # BIM: Holes in roof are causing troubles
            # - https://github.com/FreeCAD/FreeCAD/issues/21633#issuecomment-2969640142

            faces = []
            solids = []
            for f in obj.Base.Shape.Faces:  # obj.Base.Shape.Solids.Faces
                p = f.findPlane()  # Curve face (surface) seems return no Plane
                if p:
                    # See github issue #21633, all planes are added for safety
                    # if p.Axis[2] < -1e-7:  # i.e. normal pointing below horizon
                    faces.append(f)
                else:
                    # TODO 2025.6.15: See github issue #21633: Find better way
                    #      to test and maybe to split surface point up and down
                    #      and extrude separately

                    # Not sure if it is pointing towards and/or above horizon
                    # (upward or downward), or it is curve surface, just add.
                    faces.append(f)

                    # Attempt to find normal at non-planar face to verify if
                    # it is pointing downward, but cannot conclude even all test
                    # points happens to be pointing upward. So add in any rate.

            for f in faces:
                solid = f.extrude(Vector(0.0, 0.0, 1000000.0))
                if not solid.isNull() and solid.isValid() and solid.Volume > 1e-3:
                    solids.append(solid)

            # See github issue #21633: Solids are added for safety
            for s in obj.Base.Shape.Solids:
                solids.append(s)

            compound = Part.Compound(solids)
            compound.Placement = obj.Placement
            return compound

        sub_field = getattr(self, "sub", None)
        if not sub_field:
            self.execute(obj)
        return self.sub

    def computeAreas(self, obj):
        """computes border and ridge roof edges length"""
        if hasattr(obj, "RidgeLength") and hasattr(obj, "BorderLength"):
            rl = 0
            bl = 0
            rn = 0
            bn = 0
            if obj.Shape:
                if obj.Shape.Faces:
                    faceLst = []
                    for face in obj.Shape.Faces:
                        if face.normalAt(0, 0).getAngle(Vector(0.0, 0.0, 1.0)) < math.pi / 2.0:
                            faceLst.append(face)
                    if faceLst:
                        try:
                            shell = Part.Shell(faceLst)
                        except Exception:
                            pass
                        else:
                            lut = {}
                            if shell.Faces:
                                for face in shell.Faces:
                                    for edge in face.Edges:
                                        hc = edge.hashCode()
                                        if hc in lut:
                                            lut[hc] = lut[hc] + 1
                                        else:
                                            lut[hc] = 1
                                for edge in shell.Edges:
                                    if lut[edge.hashCode()] == 1:
                                        bl += edge.Length
                                        bn += 1
                                    elif lut[edge.hashCode()] == 2:
                                        rl += edge.Length
                                        rn += 1
            if obj.RidgeLength.Value != rl:
                obj.RidgeLength = rl
                # print(str(rn)+" ridge edges in roof "+obj.Name)
            if obj.BorderLength.Value != bl:
                obj.BorderLength = bl
                # print(str(bn)+" border edges in roof "+obj.Name)
        ArchComponent.Component.computeAreas(self, obj)


class _ViewProviderRoof(ArchComponent.ViewProviderComponent):
    """A View Provider for the Roof object"""

    def __init__(self, vobj):
        ArchComponent.ViewProviderComponent.__init__(self, vobj)

    def getIcon(self):
        return ":/icons/Arch_Roof_Tree.svg"

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def setEdit(self, vobj, mode=0):
        if mode != 0:
            return None

        if vobj.Object.Base.Shape.Solids:
            taskd = ArchComponent.ComponentTaskPanel()
            taskd.obj = self.Object
            taskd.update()
            FreeCADGui.Control.showDialog(taskd)
        else:
            taskd = _RoofTaskPanel()
            taskd.obj = self.Object
            taskd.update()
            FreeCADGui.Control.showDialog(taskd)
        return True


class _RoofTaskPanel:
    """The editmode TaskPanel for Roof objects"""

    def __init__(self):
        self.updating = False
        self.obj = None
        self.form = QtGui.QWidget()
        self.form.setObjectName("TaskPanel")
        self.grid = QtGui.QGridLayout(self.form)
        self.grid.setObjectName("grid")
        self.title = QtGui.QLabel(self.form)
        self.grid.addWidget(self.title, 0, 0, 1, 1)

        # tree
        self.tree = QtGui.QTreeWidget(self.form)
        self.grid.addWidget(self.tree, 1, 0, 1, 1)
        self.tree.setItemDelegate(_RoofTaskPanel_Delegate())
        self.tree.setRootIsDecorated(False)  # remove 1st column's extra left margin
        self.tree.setColumnCount(7)
        self.tree.header().resizeSection(0, 37)  # 37px seems to be the minimum size
        self.tree.header().resizeSection(1, 60)
        self.tree.header().resizeSection(2, 90)
        self.tree.header().resizeSection(3, 37)
        self.tree.header().resizeSection(4, 90)
        self.tree.header().resizeSection(5, 90)
        self.tree.header().resizeSection(6, 90)

        QtCore.QObject.connect(
            self.tree, QtCore.SIGNAL("itemChanged(QTreeWidgetItem *, int)"), self.edit
        )
        self.update()

    def isAllowedAlterSelection(self):
        return False

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return QtGui.QDialogButtonBox.Close

    def update(self):
        """fills the treewidget"""
        self.updating = True
        if self.obj:
            root = self.tree.invisibleRootItem()
            if root.childCount() == 0:
                for i in range(len(self.obj.Angles)):
                    QtGui.QTreeWidgetItem(self.tree)
            for i in range(len(self.obj.Angles)):
                item = root.child(i)
                item.setText(0, str(i))
                item.setText(1, Units.Quantity(self.obj.Angles[i], Units.Angle).UserString)
                item.setText(2, Units.Quantity(self.obj.Runs[i], Units.Length).UserString)
                item.setText(3, str(self.obj.IdRel[i]))
                item.setText(4, Units.Quantity(self.obj.Thickness[i], Units.Length).UserString)
                item.setText(5, Units.Quantity(self.obj.Overhang[i], Units.Length).UserString)
                item.setText(6, Units.Quantity(self.obj.Heights[i], Units.Length).UserString)
                item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
            # treeHgt = 1 + 23 + (len(self.obj.Angles) * 17) + 1 # 1px borders, 23px header, 17px rows
            # self.tree.setMinimumSize(QtCore.QSize(445, treeHgt))
        self.retranslateUi(self.form)
        self.updating = False

    def _update_value(self, row, prop, str_val):
        # Workaround for Building US unit system bug (Version 1.1, 2025):
        str_val = str_val.replace("+", "--")
        val_list = getattr(self.obj, prop)
        val_list[row] = Units.Quantity(str_val).Value
        setattr(self.obj, prop, val_list)

    def edit(self, item, column):
        """transfers an edited value from the widget to the object"""
        if self.updating:
            return
        row = int(item.text(0))
        match column:
            case 1:
                self._update_value(row, "Angles", item.text(1))
            case 2:
                self._update_value(row, "Runs", item.text(2))
            case 3:
                val_list = self.obj.IdRel
                val_list[row] = int(item.text(3))
                self.obj.IdRel = val_list
            case 4:
                self._update_value(row, "Thickness", item.text(4))
            case 5:
                self._update_value(row, "Overhang", item.text(5))
            case _:
                return
        self.obj.touch()
        FreeCAD.ActiveDocument.recompute()
        self.update()

    def reject(self):
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Roof", None))
        self.title.setText(
            QtGui.QApplication.translate(
                "Arch",
                "Parameters of the roof profiles:\n* Angle: slope in degrees relative to the horizontal.\n* Run: horizontal distance between the wall and the ridge.\n* IdRel: Id of the relative profile used for automatic calculations.\n* Thickness: thickness of the roof.\n* Overhang: horizontal distance between the eave and the wall.\n* Height: height of the ridge above the base (calculated automatically).\n---\nIf Angle = 0 and Run = 0 then the profile is identical to the relative profile.\nIf Angle = 0 then the angle is calculated so that the height is the same as the relative profile.\nIf Run = 0 then the run is calculated so that the height is the same as the relative profile.",
                None,
            )
        )
        self.tree.setHeaderLabels(
            [
                QtGui.QApplication.translate("Arch", "Id", None),
                QtGui.QApplication.translate("Arch", "Angle", None),
                QtGui.QApplication.translate("Arch", "Run", None),
                QtGui.QApplication.translate("Arch", "IdRel", None),
                QtGui.QApplication.translate("Arch", "Thickness", None),
                QtGui.QApplication.translate("Arch", "Overhang", None),
                QtGui.QApplication.translate("Arch", "Height", None),
            ]
        )


if FreeCAD.GuiUp:

    class _RoofTaskPanel_Delegate(QtWidgets.QStyledItemDelegate):
        """Model delegate"""

        def createEditor(self, parent, option, index):
            if index.column() in (0, 6):
                # Make these columns read-only.
                return None
            editor = QtWidgets.QLineEdit(parent)
            if index.column() != 3:
                editor.installEventFilter(self)
            return editor

        def setEditorData(self, editor, index):
            editor.setText(index.data())

        def setModelData(self, editor, model, index):
            model.setData(index, editor.text())

        def eventFilter(self, widget, event):
            if event.type() == QtCore.QEvent.FocusIn:
                widget.setSelection(0, FreeCADGui.draftToolBar.number_length(widget.text()))
            return super().eventFilter(widget, event)
