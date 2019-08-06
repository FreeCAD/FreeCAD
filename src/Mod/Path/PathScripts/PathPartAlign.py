# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Russell Johnson [russ4262] <russ4262@gmail.com>    *
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

from PySide import QtCore
# from __future__ import print_function

__title__ = "Part Alignment Tool"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Part Alignment Tool based on edges."
__contributors__ = ""
__createdDate__ = "2019"
__scriptVersion__ = "1c testing"
__lastModified__ = "2019-08-06 16:59 CST"

LOGLEVEL = False

if LOGLEVEL is True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectPartAlign(PathOp.ObjectOp):
    '''Proxy object for Surfacing operation.'''

    def baseObject(self):
        '''baseObject() ... returns super of receiver
        Used to call base implementation in overwritten functions.'''
        return super(self.__class__, self)

    def opFeatures(self, obj):
        '''opFeatures(obj) ... return all standard features and edges based geometries'''
        return PathOp.FeatureHeights | PathOp.FeatureTool | PathOp.FeatureBaseEdges

    def initOperation(self, obj):
        '''initPocketOp(obj) ... create facing specific properties'''
        obj.addProperty('App::PropertyEnumeration', 'AlignmentMode', 'Alignment', QtCore.QT_TRANSLATE_NOOP('PathPartAlign', ''))
        obj.addProperty('App::PropertyEnumeration', 'AlignmentType', 'Alignment', QtCore.QT_TRANSLATE_NOOP('PathPartAlign', ''))
        obj.addProperty("App::PropertyDistance", "ApproachDistance", "Alignment", QtCore.QT_TRANSLATE_NOOP('PathPartAlign', ''))
        obj.addProperty("App::PropertyDistance", "StopBelowVertex", "Alignment", QtCore.QT_TRANSLATE_NOOP('PathPartAlign', ''))
        obj.addProperty('App::PropertyInteger', 'FeedRatePercentForApproach', 'Alignment', QtCore.QT_TRANSLATE_NOOP('PathPartAlign', ''))
        obj.AlignmentMode = ['Auto', 'Manual']
        obj.AlignmentType = ['Static', 'Dynamic']

    def opSetDefaultValues(self, obj, job):
        '''opSetDefaultValues(obj, job) ... initialize defaults'''
        obj.AlignmentMode = 'Auto'
        obj.AlignmentType = 'Static'
        obj.ApproachDistance.Value = 15.0
        if hasattr(obj, 'OpToolDiameter'):
            obj.StopBelowVertex.Value = obj.OpToolDiameter.Value
        else:
            obj.StopBelowVertex.Value = 6.0
        obj.FeedRatePercentForApproach = 20

        if not hasattr(obj, 'DoNotSetDefaultValues'):
            self.setEditorProperties(obj)

    def setEditorProperties(self, obj):
        # Used to hide inputs in properties list
        if obj.AlignmentMode == 'Auto':
            obj.setEditorMode('AlignmentType', 1)
        else:
            obj.setEditorMode('AlignmentType', 0)

    def onChanged(self, obj, prop):
        if prop == 'AlignmentMode':
            self.setEditorProperties(obj)
        if prop == 'FeedRatePercentForApproach':
            if obj.FeedRatePercentForApproach < 5:
                obj.FeedRatePercentForApproach = 5
            elif obj.FeedRatePercentForApproach > 50:
                obj.FeedRatePercentForApproach = 50

    def opOnDocumentRestored(self, obj):
        self.setEditorProperties(obj)

    def opExecute(self, obj):
        '''opExecute(obj) ... process Part Align operation'''
        PathLog.track()
        import Draft
        import math
        import DraftGeomUtils
        import PathScripts.PathUtils as PathUtils

        apCnt = 0
        edgeTuples = []
        alignments = ['Static', 'Dynamic']
        missingAlignments = ['Static', 'Dynamic']
        Job = PathUtils.findParentJob(obj)

        # Check for previous Part_Align operations in Job.Operations group
        Ops = Job.Operations.Group
        for op in Ops:
            if 'PartAlign' in op.Name:
                apCnt += 1
                if obj.AlignmentMode == 'Auto':
                    try:
                        have = missingAlignments.index(op.AlignmentType)
                    except Exception:
                        pass
                    else:
                        missingAlignments.pop(have)

        if obj.Base:
            for b in obj.Base:
                edgelist = []
                for sub in b[1]:
                    edge = getattr(b[0].Shape, sub)
                    if len(edge.Vertexes) == 2:
                        edgeTuples.append((sub, edge))
                    else:
                        msg = translate("Path", "{} has more, or less, than two verteces. It cannot be used for alignment.".format(sub))
                        PathLog.error(msg)
        else:
            msg = translate("Path", "Insufficient edges provided.")
            PathLog.error(msg)
            return

        eCnt = len(edgeTuples)
        alignmentPoints = []
        if eCnt > 1:
            if eCnt % 2 == 0:
                pCnt = int(eCnt / 2)
            else:
                pCnt = int((eCnt - 1) / 2)

            # Limit points to two per Part_Align operation
            if pCnt > 2:
                pCnt = 2

            # Identify pairs of connected edges
            for i in range(0, pCnt):
                (sub0, edg0) = edgeTuples.pop(0)
                ev0 = edg0.Vertexes[0]
                ev1 = edg0.Vertexes[1]
                evi = 2  # vertex(common) index
                evt = 2  # terminal index
                pair = 2

                mtch = 0
                sub1 = None
                cv0 = None
                cv1 = None
                cvi = 2  # vertex(common) index
                cvt = 2  # terminal index
                for ei in range(0, len(edgeTuples)):
                    (sub1, edg1) = edgeTuples[ei]
                    mtch = ei
                    cv0 = edg1.Vertexes[0]
                    cv1 = edg1.Vertexes[1]
                    # common = DraftGeomUtils.findIntersection(edg0, edg1)
                    # if len(common) > 0:
                    #     PathLog.warning("{} and {} intersect at {}.".format(sub0, sub1, common[0]))
                    if ev0.isSame(cv0):
                        evi = 0
                        cvi = 0
                        evt = 1
                        cvt = 1
                        break
                    elif ev1.isSame(cv0):
                        evi = 1
                        cvi = 0
                        evt = 0
                        cvt = 1
                        break
                    elif ev0.isSame(cv1):
                        evi = 0
                        cvi = 1
                        evt = 1
                        cvt = 0
                        break
                    elif ev1.isSame(cv1):
                        evi = 1
                        cvi = 1
                        evt = 0
                        cvt = 0
                        break

                if cvi != 2:
                    rmvObjNms = []
                    edgeTuples.pop(mtch)
                    x = edg1.Vertexes[cvi].X
                    y = edg1.Vertexes[cvi].Y
                    z = edg1.Vertexes[cvi].Z
                    t0 = FreeCAD.Vector(edg0.Vertexes[evt].X - x, edg0.Vertexes[evt].Y - y, 0.0)  # also is directional vector
                    t1 = FreeCAD.Vector(edg1.Vertexes[cvt].X - x, edg1.Vertexes[cvt].Y - y, 0.0)

                    ls0vi = FreeCAD.Vector(edg0.Vertexes[evi].X, edg0.Vertexes[evi].Y, z)
                    ls0vt = FreeCAD.Vector(edg0.Vertexes[evt].X, edg0.Vertexes[evt].Y, z)
                    ls0 = Draft.makeLine(ls0vi, ls0vt)
                    ls0.recompute()

                    ls1vi = FreeCAD.Vector(edg1.Vertexes[cvi].X, edg1.Vertexes[cvi].Y, z)
                    ls1vt = FreeCAD.Vector(edg1.Vertexes[cvt].X, edg1.Vertexes[cvt].Y, z)
                    ls1 = Draft.makeLine(ls1vi, ls1vt)
                    ls1.recompute()

                    rmvObjNms.extend([ls0.Name, ls1.Name])
                    intersect = None
                    p = 4
                    for p in range(0, 4):
                        if p == 0:
                            tp0 = FreeCAD.Vector(-1 * (edg0.Vertexes[evt].Y - y), (edg0.Vertexes[evt].X - x), 0.0)  # also is directional vector
                            tp1 = FreeCAD.Vector(-1 * (edg1.Vertexes[cvt].Y - y), (edg1.Vertexes[cvt].X - x), 0.0)
                            ls0 = Draft.move([ls0], tp0.normalize().multiply(self.radius), copy=False)
                            ls1 = Draft.move([ls1], tp1.normalize().multiply(self.radius), copy=False)
                        elif p == 1:
                            tp1r = FreeCAD.Vector((edg1.Vertexes[cvt].Y - y), -1 * (edg1.Vertexes[cvt].X - x), 0.0)
                            ls1 = Draft.move([ls1], tp1r.normalize().multiply(2 * self.radius), copy=False)
                        elif p == 2:
                            tp0r = FreeCAD.Vector((edg0.Vertexes[evt].Y - y), -1 * (edg0.Vertexes[evt].X - x), 0.0)
                            ls0 = Draft.move([ls0], tp0r.normalize().multiply(2 * self.radius), copy=False)
                        elif p == 3:
                            tp1r = FreeCAD.Vector(-1 * (edg1.Vertexes[cvt].Y - y), (edg1.Vertexes[cvt].X - x), 0.0)
                            ls1 = Draft.move([ls1], tp1r.normalize().multiply(2 * self.radius), copy=False)
                        ls0.purgeTouched()
                        ls1.purgeTouched()

                        intersect = DraftGeomUtils.findIntersection(ls0.Shape.Edges[0], ls1.Shape.Edges[0])
                        if len(intersect) == 1:
                            PathLog.info("Alignment point for <{}> and <{}> is at ({:.3f}, {:.3f}).".format(sub0, sub1, intersect[0].x, intersect[0].y))
                            p = 4
                            break
                    # Efor
                    # Save alignment point details
                    if p == 4:
                        alignmentPoints.append((intersect[0], edg1.Vertexes[cvi]))
                    else:
                        PathLog.error("Tool diameter is too large to create alignment point between <{}> and <{}>.".format(sub0, sub1))
                    # Remove disposable line objects
                    for ro in rmvObjNms:
                        FreeCAD.ActiveDocument.removeObject(ro)
                # Eif
            # Efor
        # Eif

        if obj.AlignmentMode == 'Auto':
            if len(missingAlignments) == 0:
                for at in range(0, len(pCnt)):
                    missingAlignments.append(alignments[at])

            # Create Gcode commands for alignment proceedure
            for (ap, cmn) in alignmentPoints:
                apCnt += 1
                if len(missingAlignments) > 0:
                    obj.AlignmentType = missingAlignments.pop(0)
                    # Create visible circle objecct to represent cutter - if LogLevel = DEBUG
                    if PathLog.getLevel(PathLog.thisModule()) == 4:
                        plcmnt = FreeCAD.Placement(ap, FreeCAD.Rotation(FreeCAD.Vector(0, 0, 0), 0.0))
                        apCircle = Draft.makeCircle(self.radius, placement=plcmnt)
                        apCircle.recompute()
                        apCircle.purgeTouched()
                        PathLog.info("Visible circle(s) represent cutter placement for alignment. They are disposable.")
                    if obj.AlignmentType == 'Static':
                        self.commandlist.extend(self.makeStaticCommands(obj, ap, apCnt))
                    if obj.AlignmentType == 'Dynamic':
                        aprchDir = FreeCAD.Vector(ap.x - cmn.X, ap.y - cmn.Y, ap.z)
                        aprchDir.multiply(obj.ApproachDistance.Value / math.sqrt((ap.x - cmn.X)**2 + (ap.y - cmn.Y)**2))
                        aprchVect = FreeCAD.Vector(ap.x + aprchDir.x, ap.y + aprchDir.y, ap.z)
                        self.commandlist.extend(self.makeDynamicCommands(obj, ap, apCnt, aprchVect))
                else:
                    break
        elif obj.AlignmentMode == 'Manual':
            # Create Gcode commands for alignment proceedure
            for (ap, cmn) in alignmentPoints:
                apCnt += 1
                # Create visible circle objecct to represent cutter - if LogLevel = DEBUG
                if PathLog.getLevel(PathLog.thisModule()) == 4:
                    plcmnt = FreeCAD.Placement(ap, FreeCAD.Rotation(FreeCAD.Vector(0, 0, 0), 0.0))
                    apCircle = Draft.makeCircle(self.radius, placement=plcmnt)
                    apCircle.recompute()
                    apCircle.purgeTouched()
                    PathLog.info("Visible circle(s) represent cutter placement for alignment. They are disposable.")
                if obj.AlignmentType == 'Static':
                    self.commandlist.extend(self.makeStaticCommands(obj, ap, apCnt))
                if obj.AlignmentType == 'Dynamic':
                    aprchDir = FreeCAD.Vector(ap.x - cmn.X, ap.y - cmn.Y, ap.z)
                    aprchDir.multiply(obj.ApproachDistance.Value / math.sqrt((ap.x - cmn.X)**2 + (ap.y - cmn.Y)**2))
                    aprchVect = FreeCAD.Vector(ap.x + aprchDir.x, ap.y + aprchDir.y, ap.z)
                    self.commandlist.extend(self.makeDynamicCommands(obj, ap, apCnt, aprchVect))
                else:
                    break

    def makeStaticCommands(self, obj, ap, apCnt):
        cmds = []
        apTxt = "<{0:.3f}".format(ap.x) + ", {0:.3f}".format(ap.y)
        apTxt += ", {0:.3f}>".format(ap.z)
        cmds.append(Path.Command('N ({} alignment point {} at {} mm)'.format(obj.AlignmentType, apCnt, apTxt), {}))
        cmds.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        cmds.append(Path.Command('G0', {'X': ap.x, 'Y': ap.y, 'F': self.horizRapid}))
        cmds.append(Path.Command('G1', {'Z': ap.z, 'F': self.vertFeed}))
        cmds.append(Path.Command('G1', {'Z': ap.z - (obj.StopBelowVertex.Value), 'F': self.vertFeed}))
        cmds.append(Path.Command('M0', {}))  # Pause machine
        return cmds

    def makeDynamicCommands(self, obj, ap, apCnt, aprchVect):
        cmds = []
        horizApproachRate = self.horizFeed * obj.FeedRatePercentForApproach / 100.0
        vertApproachRate = self.vertFeed * obj.FeedRatePercentForApproach / 100.0
        apTxt = "<{0:.3f}".format(ap.x) + ", {0:.3f}".format(ap.y)
        apTxt += ", {0:.3f}>".format(ap.z)
        cmds.append(Path.Command('N ({} alignment point {} at {} mm)'.format(obj.AlignmentType, apCnt, apTxt), {}))
        cmds.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        cmds.append(Path.Command('G0', {'X': ap.x, 'Y': ap.y, 'F': self.horizRapid}))
        cmds.append(Path.Command('G1', {'Z': ap.z, 'F': (self.vertFeed / 2)}))
        cmds.append(Path.Command('M0', {}))  # Pause machine
        cmds.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        cmds.append(Path.Command('G0', {'X': aprchVect.x, 'Y': aprchVect.y, 'F': self.horizRapid}))
        cmds.append(Path.Command('G1', {'Z': (ap.z - obj.StopBelowVertex.Value), 'F': (vertApproachRate)}))
        cmds.append(Path.Command('G1', {'X': ap.x, 'Y': ap.y, 'F': (horizApproachRate)}))
        cmds.append(Path.Command('M0', {}))  # Pause machine
        cmds.append(Path.Command('G0', {'X': aprchVect.x, 'Y': aprchVect.y, 'F': self.horizRapid}))
        return cmds


def SetupProperties():
    setup = []
    setup.append("AlignmentMode")
    setup.append("AlignmentType")
    setup.append("ApproachDistance")
    setup.append("StopBelowVertex")
    setup.append("FeedRatePercentForApproach")
    return setup


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Surface operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectPartAlign(obj, name)
    return obj
