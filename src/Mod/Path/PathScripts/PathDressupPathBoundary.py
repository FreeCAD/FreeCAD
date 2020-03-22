# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathDressup as PathDressup
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathStock as PathStock
import PathScripts.PathUtil as PathUtil
import PathScripts.PathUtils as PathUtils

from PySide import QtCore

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
#PathLog.trackModule(PathLog.thisModule())

def _vstr(v):
    if v:
        return "(%.2f, %.2f, %.2f)" % (v.x, v.y, v.z)
    return '-'

class DressupPathBoundary(object):

    def __init__(self, obj, base, job):
        obj.addProperty("App::PropertyLink", "Base", "Base", QtCore.QT_TRANSLATE_NOOP("Path_DressupPathBoundary", "The base path to modify"))
        obj.Base = base
        obj.addProperty("App::PropertyLink", "Stock", "Boundary", QtCore.QT_TRANSLATE_NOOP("Path_DressupPathBoundary", "Solid object to be used to limit the generated Path."))
        obj.Stock = PathStock.CreateFromBase(job)
        obj.addProperty("App::PropertyBool", "Inside", "Boundary", QtCore.QT_TRANSLATE_NOOP("Path_DressupPathBoundary", "Determines if Boundary describes an inclusion or exclusion mask."))
        obj.Inside = True

        self.obj = obj
        self.safeHeight = None
        self.clearanceHeight = None

    def __getstate__(self):
        return None
    def __setstate__(self, state):
        return None

    def onDcoumentRestored(self, obj):
        self.obj = obj

    def onDelete(self, obj, args):
        if obj.Base:
            job = PathUtils.findParentJob(obj)
            if job:
                job.Proxy.addOperation(obj.Base, obj)
            if obj.Base.ViewObject:
                obj.Base.ViewObject.Visibility = True
            obj.Base = None
        if obj.Stock:
            obj.Document.removeObject(obj.Stock.Name)
            obj.Stock = None
        return True

    def boundaryCommands(self, obj, begin, end, verticalFeed):
        PathLog.track(_vstr(begin), _vstr(end))
        if end and PathGeom.pointsCoincide(begin, end):
            return []
        cmds = []
        if begin.z < self.safeHeight:
            cmds.append(Path.Command('G1', {'Z': self.safeHeight, 'F': verticalFeed}))
        if begin.z < self.clearanceHeight:
            cmds.append(Path.Command('G0', {'Z': self.clearanceHeight}))
        if end:
            cmds.append(Path.Command('G0', {'X': end.x, 'Y': end.y}))
            if end.z < self.clearanceHeight:
                cmds.append(Path.Command('G0', {'Z': max(self.safeHeight, end.z)}))
            if end.z < self.safeHeight:
                cmds.append(Path.Command('G1', {'Z': end.z, 'F': verticalFeed}))
        return cmds

    def execute(self, obj):
        if not obj.Base or not obj.Base.isDerivedFrom('Path::Feature') or not obj.Base.Path:
            return

        tc = PathDressup.toolController(obj.Base)

        if len(obj.Base.Path.Commands) > 0:
            self.safeHeight = float(PathUtil.opProperty(obj.Base, 'SafeHeight'))
            self.clearanceHeight = float(PathUtil.opProperty(obj.Base, 'ClearanceHeight'))

            boundary = obj.Stock.Shape
            cmd = obj.Base.Path.Commands[0]
            pos = cmd.Placement.Base
            commands = [cmd]
            lastExit = None
            for cmd in obj.Base.Path.Commands[1:]:
                if cmd.Name in PathGeom.CmdMoveAll:
                    edge = PathGeom.edgeForCmd(cmd, pos)
                    inside  = edge.common(boundary).Edges
                    outside = edge.cut(boundary).Edges
                    if not obj.Inside:
                        t = inside
                        inside = outside
                        outside = t
                    # it's really a shame that one cannot trust the sequence and/or
                    # orientation of edges
                    if 1 == len(inside) and 0 == len(outside):
                        PathLog.track(_vstr(pos), _vstr(lastExit), ' + ', cmd)
                        # cmd fully included by boundary
                        if lastExit:
                            commands.extend(self.boundaryCommands(obj, lastExit, pos, tc.VertFeed.Value))
                            lastExit = None
                        commands.append(cmd)
                        pos = PathGeom.commandEndPoint(cmd, pos)
                    elif 0 == len(inside) and 1 == len(outside):
                        PathLog.track(_vstr(pos), _vstr(lastExit), ' - ', cmd)
                        # cmd fully excluded by boundary
                        if not lastExit:
                            lastExit = pos
                        pos = PathGeom.commandEndPoint(cmd, pos)
                    else:
                        PathLog.track(_vstr(pos), _vstr(lastExit), len(inside), len(outside), cmd)
                        # cmd pierces boundary
                        while inside or outside:
                            ie = [e for e in inside if PathGeom.edgeConnectsTo(e, pos)]
                            PathLog.track(ie)
                            if ie:
                                e = ie[0]
                                ptL = e.valueAt(e.LastParameter)
                                flip = PathGeom.pointsCoincide(pos, ptL)
                                newPos = e.valueAt(e.FirstParameter) if flip else ptL
                                # inside edges are taken at this point (see swap of inside/outside
                                # above - so we can just connect the dots ...
                                if lastExit:
                                    commands.extend(self.boundaryCommands(obj, lastExit, pos, tc.VertFeed.Value))
                                    lastExit = None
                                PathLog.track(e, flip)
                                commands.extend(PathGeom.cmdsForEdge(e, flip, False,50,tc.HorizFeed.Value,tc.VertFeed.Value)) # add missing HorizFeed to G2 paths
                                inside.remove(e)
                                pos = newPos
                                lastExit = newPos
                            else:
                                oe = [e for e in outside if PathGeom.edgeConnectsTo(e, pos)]
                                PathLog.track(oe)
                                if oe:
                                    e = oe[0]
                                    ptL = e.valueAt(e.LastParameter)
                                    flip = PathGeom.pointsCoincide(pos, ptL)
                                    newPos = e.valueAt(e.FirstParameter) if flip else ptL
                                    # outside edges are never taken at this point (see swap of
                                    # inside/outside above) - so just move along ...
                                    outside.remove(e)
                                    pos = newPos
                                else:
                                    PathLog.error('huh?')
                                    import Part
                                    Part.show(Part.Vertex(pos), 'pos')
                                    for e in inside:
                                        Part.show(e, 'ei')
                                    for e in outside:
                                        Part.show(e, 'eo')
                                    raise Exception('This is not supposed to happen')
                    #pos = PathGeom.commandEndPoint(cmd, pos)
                else:
                    PathLog.track('no-move', cmd)
                    commands.append(cmd)
            if lastExit:
                commands.extend(self.boundaryCommands(obj, lastExit, None, tc.VertFeed.Value))
                lastExit = None
        else:
            PathLog.warning("No Path Commands for %s" % obj.Base.Label)
            commands = []
        PathLog.track(commands)
        obj.Path = Path.Path(commands)


def Create(base, name='DressupPathBoundary'):
    '''Create(base, name='DressupPathBoundary') ... creates a dressup limiting base's Path to a boundary.'''

    if not base.isDerivedFrom('Path::Feature'):
        PathLog.error(translate('Path_DressupPathBoundary', 'The selected object is not a path')+'\n')
        return None

    obj = FreeCAD.ActiveDocument.addObject('Path::FeaturePython', name)
    job = PathUtils.findParentJob(base)
    obj.Proxy = DressupPathBoundary(obj, base, job)
    job.Proxy.addOperation(obj, base, True)
    return obj
