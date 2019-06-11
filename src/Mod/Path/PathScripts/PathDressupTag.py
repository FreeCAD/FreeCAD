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
import FreeCAD
import DraftGeomUtils
import Part
import PathScripts.PathDressup as PathDressup
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
import math
import sys

from PathScripts.PathDressupTagPreferences import HoldingTagPreferences
from PySide import QtCore

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule()


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class TagSolid:
    def __init__(self, proxy, z, R):
        self.proxy = proxy
        self.z = z
        self.toolRadius = R
        self.angle = math.fabs(proxy.obj.Angle)
        self.width = math.fabs(proxy.obj.Width)
        self.height = math.fabs(proxy.obj.Height)
        self.radius = math.fabs(proxy.obj.Radius)
        self.actualHeight = self.height
        self.fullWidth = 2 * self.toolRadius + self.width

        r1 = self.fullWidth / 2
        self.r1 = r1
        self.r2 = r1
        height = self.actualHeight * 1.01
        radius = 0
        if self.angle == 90 and height > 0:
            # cylinder
            self.solid = Part.makeCylinder(r1, height)
            radius = min(min(self.radius, r1), self.height)
            PathLog.debug("Part.makeCylinder(%f, %f)" % (r1, height))
        elif self.angle > 0.0 and height > 0.0:
            # cone
            rad = math.radians(self.angle)
            tangens = math.tan(rad)
            dr = height / tangens
            if dr < r1:
                # with top
                r2 = r1 - dr
                s = height / math.sin(rad)
                radius = min(r2, s) * math.tan((math.pi - rad)/2) * 0.95
            else:
                # triangular
                r2 = 0
                height = r1 * tangens * 1.01
                self.actualHeight = height
            self.r2 = r2
            PathLog.debug("Part.makeCone(r1=%.2f, r2=%.2f, h=%.2f)" % (r1, r2, height))
            self.solid = Part.makeCone(r1, r2, height)
        else:
            # degenerated case - no tag
            PathLog.debug("Part.makeSphere(%.2f)" % (r1 / 10000))
            self.solid = Part.makeSphere(r1 / 10000)

        radius = min(self.radius, radius)
        self.realRadius = radius
        if radius != 0:
            PathLog.debug("makeFillet(%.4f)" % radius)
            self.solid = self.solid.makeFillet(radius, [self.solid.Edges[0]])

        # lastly determine the center of the model, we want to make sure the seam of
        # the tag solid points away (in the hopes it doesn't coincide with a path)
        self.baseCenter = FreeCAD.Vector((proxy.ptMin.x+proxy.ptMax.x)/2, (proxy.ptMin.y+proxy.ptMax.y)/2, 0)

    def cloneAt(self, pos):
        clone = self.solid.copy()
        pos.z = 0
        angle = -PathGeom.getAngle(pos - self.baseCenter) * 180 / math.pi
        clone.rotate(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1), angle)
        pos.z = self.z - self.actualHeight * 0.01
        clone.translate(pos)
        return clone


class ObjectDressup:

    def __init__(self, obj, base):

        obj.addProperty('App::PropertyLink', 'Base', 'Base', QtCore.QT_TRANSLATE_NOOP('Path_DressupTag', 'The base path to modify'))
        obj.addProperty('App::PropertyLength', 'Width', 'Tag', QtCore.QT_TRANSLATE_NOOP('Path_DressupTag', 'Width of tags.'))
        obj.addProperty('App::PropertyLength', 'Height', 'Tag', QtCore.QT_TRANSLATE_NOOP('Path_DressupTag', 'Height of tags.'))
        obj.addProperty('App::PropertyAngle', 'Angle', 'Tag', QtCore.QT_TRANSLATE_NOOP('Path_DressupTag', 'Angle of tag plunge and ascent.'))
        obj.addProperty('App::PropertyLength', 'Radius', 'Tag', QtCore.QT_TRANSLATE_NOOP('Path_DressupTag', 'Radius of the fillet for the tag.'))
        obj.addProperty('App::PropertyVectorList', 'Positions', 'Tag', QtCore.QT_TRANSLATE_NOOP('Path_DressupTag', 'Locations of inserted holding tags'))
        obj.addProperty('App::PropertyIntegerList', 'Disabled', 'Tag', QtCore.QT_TRANSLATE_NOOP('Path_DressupTag', 'IDs of disabled holding tags'))
        obj.addProperty('App::PropertyInteger', 'SegmentationFactor', 'Tag', QtCore.QT_TRANSLATE_NOOP('Path_DressupTag', 'Factor determining the # of segments used to approximate rounded tags.'))

        obj.Proxy = self
        obj.Base = base

        self.obj = obj
        self.solids = []

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def assignDefaultValues(self):
        self.obj.Width = HoldingTagPreferences.defaultWidth(self.toolRadius() * 2)
        self.obj.Height = HoldingTagPreferences.defaultHeight(self.toolRadius())
        self.obj.Angle = HoldingTagPreferences.defaultAngle()
        self.obj.Radius = HoldingTagPreferences.defaultRadius()

    def execute(self, obj):
        PathLog.track()
        if not obj.Base:
            PathLog.error(translate('Path_DressupTag', 'No Base object found.'))
            return
        if not obj.Base.isDerivedFrom('Path::Feature'):
            PathLog.error(translate('Path_DressupTag', 'Base is not a Path::Feature object.'))
            return
        if not obj.Base.Path:
            PathLog.error(translate('Path_DressupTag', 'Base doesn\'t have a Path to dress-up.'))
            return
        if not obj.Base.Path.Commands:
            PathLog.error(translate('Path_DressupTag', 'Base Path is empty.'))
            return

        self.obj = obj

        minZ = +sys.maxint
        minX = minZ
        minY = minZ

        maxZ = -sys.maxint
        maxX = maxZ
        maxY = maxZ

        # the assumption is that all helixes are in the xy-plane - in other words there is no
        # intermittent point of a command that has a lower/higher Z-position than the start and
        # and end positions of a command.
        lastPt = FreeCAD.Vector(0, 0, 0)
        for cmd in obj.Base.Path.Commands:
            pt = PathGeom.commandEndPoint(cmd, lastPt)
            if lastPt.x != pt.x:
                maxX = max(pt.x, maxX)
                minX = min(pt.x, minX)
            if lastPt.y != pt.y:
                maxY = max(pt.y, maxY)
                minY = min(pt.y, minY)
            if lastPt.z != pt.z:
                maxZ = max(pt.z, maxZ)
                minZ = min(pt.z, minZ)
            lastPt = pt
        PathLog.debug("bb = (%.2f, %.2f, %.2f) ... (%.2f, %.2f, %.2f)" % (minX, minY, minZ, maxX, maxY, maxZ))
        self.ptMin = FreeCAD.Vector(minX, minY, minZ)
        self.ptMax = FreeCAD.Vector(maxX, maxY, maxZ)
        self.masterSolid = TagSolid(self, minZ, self.toolRadius())
        self.solids = [self.masterSolid.cloneAt(pos) for pos in self.obj.Positions]
        self.tagSolid = Part.Compound(self.solids)

        self.wire, rapid = PathGeom.wireForPath(obj.Base.Path)
        self.edges = self.wire.Edges

        maxTagZ = minZ + obj.Height.Value

        # lastX = 0
        # lastY = 0
        lastZ = 0

        commands = []

        for cmd in obj.Base.Path.Commands:
            if cmd in PathGeom.CmdMove:
                if lastZ <= maxTagZ or cmd.Parameters.get('Z', lastZ) <= maxTagZ:
                    pass
                else:
                    commands.append(cmd)
            else:
                commands.append(cmd)

        obj.Path = obj.Base.Path

        PathLog.track()

    def toolRadius(self):
        return PathDressup.toolController(self.obj.Base).Tool.Diameter / 2.0

    def addTagsToDocuemnt(self):
        for i, solid in enumerate(self.solids):
            obj = FreeCAD.ActiveDocument.addObject('Part::Compound', "tag_%02d" % i)
            obj.Shape = solid

    def supportsTagGeneration(self, obj):
        return False

    def pointIsOnPath(self, obj, p):
        for e in self.edges:
            if DraftGeomUtils.isPtOnEdge(p, e):
                return True
        return False


def Create(baseObject, name='DressupTag'):
    '''
    Create(basePath, name = 'DressupTag') ... create tag dressup object for the given base path.
    '''
    if not baseObject.isDerivedFrom('Path::Feature'):
        PathLog.error(translate('Path_DressupTag', 'The selected object is not a path')+'\n')
        return None

    if baseObject.isDerivedFrom('Path::FeatureCompoundPython'):
        PathLog.error(translate('Path_DressupTag', 'Please select a Profile object'))
        return None

    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "TagDressup")
    dbo = ObjectDressup(obj, baseObject)
    job = PathUtils.findParentJob(baseObject)
    job.adddOperation(obj)
    dbo.assignDefaultValues()
    return obj

PathLog.notice('Loading Path_DressupTag... done\n')
