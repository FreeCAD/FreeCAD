# -*- coding: utf-8 -*-
# ***************************************************************************
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

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Path
import Path.Base.Util as PathUtil
import Path.Dressup.Utils as PathDressup
import Path.Main.Stock as PathStock
import PathScripts.PathUtils as PathUtils

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


def _vstr(v):
    if v:
        return "(%.2f, %.2f, %.2f)" % (v.x, v.y, v.z)
    return "-"


class DressupPathBoundary(object):
    def __init__(self, obj, base, job):
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "The base path to modify"),
        )
        obj.Base = base
        obj.addProperty(
            "App::PropertyLink",
            "Stock",
            "Boundary",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Solid object to be used to limit the generated Path.",
            ),
        )
        obj.Stock = PathStock.CreateFromBase(job)
        obj.addProperty(
            "App::PropertyBool",
            "Inside",
            "Boundary",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Determines if Boundary describes an inclusion or exclusion mask.",
            ),
        )
        obj.Inside = True

        self.obj = obj
        self.safeHeight = None
        self.clearanceHeight = None

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onDocumentRestored(self, obj):
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

    def execute(self, obj):
        pb = PathBoundary(obj.Base, obj.Stock.Shape, obj.Inside)
        obj.Path = pb.execute()


# Eclass


class PathBoundary:
    """class PathBoundary...
    This class requires a base operation, boundary shape, and optional inside boolean (default is True).
    The `execute()` method returns a Path object with path commands limited to cut paths inside or outside
    the provided boundary shape.
    """

    def __init__(self, baseOp, boundaryShape, inside=True):
        self.baseOp = baseOp
        self.boundary = boundaryShape
        self.inside = inside
        self.safeHeight = None
        self.clearanceHeight = None
        self.strG0ZsafeHeight = None
        self.strG0ZclearanceHeight = None

    def boundaryCommands(self, begin, end, verticalFeed):
        Path.Log.track(_vstr(begin), _vstr(end))
        if end and Path.Geom.pointsCoincide(begin, end):
            return []
        cmds = []
        if begin.z < self.safeHeight:
            cmds.append(self.strG0ZsafeHeight)
        if begin.z < self.clearanceHeight:
            cmds.append(self.strG0ZclearanceHeight)
        if end:
            cmds.append(Path.Command("G0", {"X": end.x, "Y": end.y}))
            if end.z < self.clearanceHeight:
                cmds.append(Path.Command("G0", {"Z": max(self.safeHeight, end.z)}))
            if end.z < self.safeHeight:
                cmds.append(Path.Command("G1", {"Z": end.z, "F": verticalFeed}))
        return cmds

    def execute(self):
        if (
            not self.baseOp
            or not self.baseOp.isDerivedFrom("Path::Feature")
            or not self.baseOp.Path
        ):
            return None

        path = PathUtils.getPathWithPlacement(self.baseOp)
        if len(path.Commands) == 0:
            Path.Log.warning("No Path Commands for %s" % self.baseOp.Label)
            return []

        tc = PathDressup.toolController(self.baseOp)

        self.safeHeight = float(PathUtil.opProperty(self.baseOp, "SafeHeight"))
        self.clearanceHeight = float(
            PathUtil.opProperty(self.baseOp, "ClearanceHeight")
        )
        self.strG0ZsafeHeight = Path.Command(  # was a Feed rate with G1
            "G0", {"Z": self.safeHeight, "F": tc.VertRapid.Value}
        )
        self.strG0ZclearanceHeight = Path.Command("G0", {"Z": self.clearanceHeight})

        cmd = path.Commands[0]
        pos = cmd.Placement.Base  # bogus m/c position to create first edge
        bogusX = True
        bogusY = True
        commands = [cmd]
        lastExit = None
        for cmd in path.Commands[1:]:
            if cmd.Name in Path.Geom.CmdMoveAll:
                if bogusX:
                    bogusX = "X" not in cmd.Parameters
                if bogusY:
                    bogusY = "Y" not in cmd.Parameters
                edge = Path.Geom.edgeForCmd(cmd, pos)
                if edge:
                    inside = edge.common(self.boundary).Edges
                    outside = edge.cut(self.boundary).Edges
                    if not self.inside:  # UI "inside boundary" param
                        tmp = inside
                        inside = outside
                        outside = tmp
                    # it's really a shame that one cannot trust the sequence and/or
                    # orientation of edges
                    if 1 == len(inside) and 0 == len(outside):
                        Path.Log.track(_vstr(pos), _vstr(lastExit), " + ", cmd)
                        # cmd fully included by boundary
                        if lastExit:
                            if not (
                                bogusX or bogusY
                            ):  # don't insert false paths based on bogus m/c position
                                commands.extend(
                                    self.boundaryCommands(
                                        lastExit, pos, tc.VertFeed.Value
                                    )
                                )
                            lastExit = None
                        commands.append(cmd)
                        pos = Path.Geom.commandEndPoint(cmd, pos)
                    elif 0 == len(inside) and 1 == len(outside):
                        Path.Log.track(_vstr(pos), _vstr(lastExit), " - ", cmd)
                        # cmd fully excluded by boundary
                        if not lastExit:
                            lastExit = pos
                        pos = Path.Geom.commandEndPoint(cmd, pos)
                    else:
                        Path.Log.track(
                            _vstr(pos), _vstr(lastExit), len(inside), len(outside), cmd
                        )
                        # cmd pierces boundary
                        while inside or outside:
                            ie = [e for e in inside if Path.Geom.edgeConnectsTo(e, pos)]
                            Path.Log.track(ie)
                            if ie:
                                e = ie[0]
                                LastPt = e.valueAt(e.LastParameter)
                                flip = Path.Geom.pointsCoincide(pos, LastPt)
                                newPos = e.valueAt(e.FirstParameter) if flip else LastPt
                                # inside edges are taken at this point (see swap of inside/outside
                                # above - so we can just connect the dots ...
                                if lastExit:
                                    if not (bogusX or bogusY):
                                        commands.extend(
                                            self.boundaryCommands(
                                                lastExit, pos, tc.VertFeed.Value
                                            )
                                        )
                                    lastExit = None
                                Path.Log.track(e, flip)
                                if not (
                                    bogusX or bogusY
                                ):  # don't insert false paths based on bogus m/c position
                                    commands.extend(
                                        Path.Geom.cmdsForEdge(
                                            e,
                                            flip,
                                            False,
                                            50,
                                            tc.HorizFeed.Value,
                                            tc.VertFeed.Value,
                                        )
                                    )
                                inside.remove(e)
                                pos = newPos
                                lastExit = newPos
                            else:
                                oe = [
                                    e
                                    for e in outside
                                    if Path.Geom.edgeConnectsTo(e, pos)
                                ]
                                Path.Log.track(oe)
                                if oe:
                                    e = oe[0]
                                    ptL = e.valueAt(e.LastParameter)
                                    flip = Path.Geom.pointsCoincide(pos, ptL)
                                    newPos = (
                                        e.valueAt(e.FirstParameter) if flip else ptL
                                    )
                                    # outside edges are never taken at this point (see swap of
                                    # inside/outside above) - so just move along ...
                                    outside.remove(e)
                                    pos = newPos
                                else:
                                    Path.Log.error("huh?")
                                    import Part

                                    Part.show(Part.Vertex(pos), "pos")
                                    for e in inside:
                                        Part.show(e, "ei")
                                    for e in outside:
                                        Part.show(e, "eo")
                                    raise Exception("This is not supposed to happen")
                                # Eif
                            # Eif
                        # Ewhile
                    # Eif
                    # pos = Path.Geom.commandEndPoint(cmd, pos)
                # Eif
            else:
                Path.Log.track("no-move", cmd)
                commands.append(cmd)
        if lastExit:
            commands.extend(self.boundaryCommands(lastExit, None, tc.VertFeed.Value))
            lastExit = None

        Path.Log.track(commands)
        return Path.Path(commands)


# Eclass


def Create(base, name="DressupPathBoundary"):
    """Create(base, name='DressupPathBoundary') ... creates a dressup limiting base's Path to a boundary."""

    if not base.isDerivedFrom("Path::Feature"):
        Path.Log.error(
            translate("Path_DressupPathBoundary", "The selected object is not a path")
            + "\n"
        )
        return None

    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    job = PathUtils.findParentJob(base)
    obj.Proxy = DressupPathBoundary(obj, base, job)
    job.Proxy.addOperation(obj, base, True)
    return obj
