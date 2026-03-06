# SPDX-License-Identifier: LGPL-2.1-or-later

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
    def promoteStockToBoundary(self, stock):
        """Ensure stock object has boundary properties set."""
        if stock:
            if not hasattr(stock, "IsBoundary"):
                stock.addProperty("App::PropertyBool", "IsBoundary", "Base")
            stock.IsBoundary = True
            if hasattr(stock, "setEditorMode"):
                stock.setEditorMode("IsBoundary", 3)
            stock.Label = "Boundary"

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
        self.promoteStockToBoundary(obj.Stock)
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
        obj.addProperty(
            "App::PropertyLength",
            "RetractThreshold",
            "Boundary",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Set distance which will attempts to avoid unnecessary retractions.",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "ApplyToRestMachining",
            "Boundary",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Apply boundary to Rest Machining.",
            ),
        )

        self.obj = obj
        self.safeHeight = None
        self.clearanceHeight = None

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, obj, prop):
        if prop == "Path" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

        # If Stock is changed, ensure boundary stock properties are set
        if prop == "Stock" and obj.Stock:
            self.promoteStockToBoundary(obj.Stock)

    def onDocumentRestored(self, obj):
        self.obj = obj
        # Ensure Stock property exists and is flagged as boundary stock
        self.promoteStockToBoundary(obj.Stock)
        if not hasattr(obj, "RetractThreshold"):
            obj.addProperty(
                "App::PropertyLength",
                "RetractThreshold",
                "Boundary",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set distance which will attempts to avoid unnecessary retractions.",
                ),
            )
        if hasattr(obj, "KeepToolDown"):
            if obj.KeepToolDown:
                baseOp = PathDressup.baseOp(obj.Base)
                expr = f"{baseOp.Name}.ToolController.Tool.Diameter.Value"
                obj.setExpression("RetractThreshold", expr)
            obj.removeProperty("KeepToolDown")

        if not hasattr(obj, "ApplyToRestMachining"):
            obj.addProperty(
                "App::PropertyBool",
                "ApplyToRestMachining",
                "Boundary",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Apply boundary to Rest Machining.",
                ),
            )

    def onDelete(self, obj, args):
        if obj.Base:
            job = PathUtils.findParentJob(obj)
            if job:
                job.Proxy.addOperation(obj.Base, obj)
            if obj.Base.ViewObject:
                obj.Base.ViewObject.Visibility = True
            obj.Base = None
        if hasattr(obj, "Stock") and obj.Stock:
            obj.Document.removeObject(obj.Stock.Name)
            obj.Stock = None
        return True

    def execute(self, obj):
        if not hasattr(obj, "Stock") or obj.Stock is None:
            Path.Log.error("BoundaryStock (Stock) missing; cannot execute dressup.")
            obj.Path = Path.Path([])
            return
        if not hasattr(obj.Stock, "Shape") or obj.Stock.Shape is None:
            Path.Log.error("Boundary stock has no Shape; cannot execute dressup.")
            obj.Path = Path.Path([])
            return
        pb = PathBoundary(obj.Base, obj.Stock.Shape, obj.Inside, obj.RetractThreshold)
        obj.Path = pb.execute()


class PathBoundary:
    """class PathBoundary...
    This class requires a base operation, boundary shape, and optional inside boolean (default is True).
    The `execute()` method returns a Path object with path commands limited to cut paths inside or outside
    the provided boundary shape.
    """

    def __init__(self, baseOp, boundaryShape, inside=True, retractThreshold=0):
        self.baseOp = baseOp
        self.boundary = boundaryShape
        self.inside = inside
        self.retractThreshold = retractThreshold
        self.firstBoundary = True

    def boundaryCommands(self, begin, end, vertFeed, horizFeed=None):
        Path.Log.track(_vstr(begin), _vstr(end))
        print("    boundary", _vstr(begin), _vstr(end))
        if Path.Geom.pointsCoincide(begin, end):
            return []
        cmds = []
        if self.firstBoundary or begin.distanceToPoint(end) > self.retractThreshold:
            # moves with retract
            if begin.z < self.clearanceHeight:
                cmds.append(self.strG0ZclearanceHeight)
            cmds.append(Path.Command("G0", {"X": end.x, "Y": end.y}))
            if end.z < self.clearanceHeight and end.z < self.safeHeight:
                cmds.append(self.strG0ZsafeHeight)
            cmds.append(Path.Command("G1", {"Z": end.z, "F": vertFeed}))
        else:
            # moves without retract
            if horizFeed and Path.Geom.isRoughly(begin.z, end.z, 0.001):
                speed = horizFeed
            else:
                speed = vertFeed
            cmds.append(Path.Command("G1", {"X": end.x, "Y": end.y, "Z": end.z, "F": speed}))

        self.firstBoundary = False

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
        self.clearanceHeight = float(PathUtil.opProperty(self.baseOp, "ClearanceHeight"))
        self.strG0ZsafeHeight = Path.Command("G0", {"Z": self.safeHeight})
        self.strG0ZclearanceHeight = Path.Command("G0", {"Z": self.clearanceHeight})

        cmd = path.Commands[0]
        pos = cmd.Placement.Base  # bogus m/c position to create first edge
        bogusX = True  # true for start moves, than position X not defined
        bogusY = True  # true for start moves, than position Y not defined
        commands = [cmd]
        lastExit = None
        for cmd in path.Commands[1:]:
            print()
            print(cmd)
            if cmd.Name not in Path.Geom.CmdMoveAll:
                Path.Log.track("no-move", cmd)
                commands.append(cmd)
                continue

            bogusX = True if bogusX and cmd.x is None else False
            bogusY = True if bogusY and cmd.y is None else False

            if cmd.z is not None and Path.Geom.isRoughly(cmd.z, self.clearanceHeight):
                # append move to clearance height
                print("  move to clearance height")
                commands.append(cmd)
                pos = Path.Geom.commandEndPoint(cmd, pos)
                lastExit = None
                continue

            # convert cmd to edge
            edge = Path.Geom.edgeForCmd(cmd, pos)
            if not edge:
                # skip cmd with zero move
                continue

            if cmd.Name in Path.Geom.CmdMoveDrill:
                # process cmd G73, G81, G82, G83, G85
                inside = edge.common(self.boundary).Edges
                outside = edge.cut(self.boundary).Edges
                if not self.inside:
                    inside, outside = outside, inside
                if inside and not outside:
                    commands.append(cmd)
                continue

            inside = edge.common(self.boundary).Edges
            outside = edge.cut(self.boundary).Edges
            if not self.inside:
                inside, outside = outside, inside

            print(f"  inside={len(inside)}  outside={len(outside)}")
            if inside:
                print("  inside", [_vstr(v.Point) for e in inside for v in e.Vertexes])
            if outside:
                print("  outside", [_vstr(v.Point) for e in outside for v in e.Vertexes])

            if inside and not outside:
                # cmd fully included by boundary
                print("  cmd fully included by boundary")
                Path.Log.track(_vstr(pos), _vstr(lastExit), " + ", cmd)
                if lastExit:
                    if not bogusX and not bogusY:
                        # don't insert false paths based on bogus m/c position
                        commands.extend(self.boundaryCommands(lastExit, pos, tc.VertFeed.Value))
                    lastExit = None
                commands.append(cmd)
                pos = Path.Geom.commandEndPoint(cmd, pos)
                continue

            if not inside and outside:
                # cmd fully excluded by boundary
                print("  cmd fully excluded by boundary")
                Path.Log.track(_vstr(pos), _vstr(lastExit), " - ", cmd)
                if not lastExit:
                    lastExit = pos
                pos = Path.Geom.commandEndPoint(cmd, pos)
                continue

            # cmd pierces boundary
            print("  cmd pierces boundary")
            Path.Log.track(_vstr(pos), _vstr(lastExit), len(inside), len(outside), cmd)
            while inside or outside:
                print("  lastExit", _vstr(lastExit))
                print("  pos", _vstr(pos))
                ie = [e for e in inside if Path.Geom.edgeConnectsTo(e, pos)]
                print("    ie", ie)
                Path.Log.track(ie)
                if ie:
                    e = ie[0]
                    LastPt = e.valueAt(e.LastParameter)
                    flip = Path.Geom.pointsCoincide(pos, LastPt)
                    # inside edges are taken at this point (see swap of inside/outside
                    # above - so we can just connect the dots ...
                    if lastExit:
                        if not bogusX and not bogusY:
                            commands.extend(
                                self.boundaryCommands(
                                    begin=lastExit,
                                    end=pos,
                                    vertFeed=tc.VertFeed.Value,
                                    horizFeed=tc.HorizFeed.Value,
                                )
                            )
                        lastExit = None
                    Path.Log.track(e, flip)
                    if not bogusX and not bogusY:
                        # don't insert false paths based on bogus m/c position
                        commands.extend(
                            Path.Geom.cmdsForEdge(
                                e,
                                flip=flip,
                                hSpeed=tc.HorizFeed.Value,
                                vSpeed=tc.VertFeed.Value,
                            )
                        )
                        # restore G0 movement
                        commands[-1].Name = cmd.Name
                    inside.remove(e)
                    newPos = e.valueAt(e.FirstParameter) if flip else LastPt
                    pos = newPos
                    lastExit = newPos
                else:
                    oe = [e for e in outside if Path.Geom.edgeConnectsTo(e, pos)]
                    print("    oe", oe)
                    Path.Log.track(oe)
                    if oe:
                        # define next point
                        e = oe[0]
                        ptL = e.valueAt(e.LastParameter)
                        flip = Path.Geom.pointsCoincide(pos, ptL)
                        pos = e.valueAt(e.FirstParameter) if flip else ptL
                        outside.remove(e)

        if lastExit:
            commands.append(self.strG0ZclearanceHeight)
            lastExit = None

        Path.Log.track(commands)
        return Path.Path(commands)


def Create(base, name="DressupPathBoundary"):
    """Create(base, name='DressupPathBoundary') ... creates a dressup limiting base's Path to a boundary."""

    if not base.isDerivedFrom("Path::Feature"):
        Path.Log.error(
            translate("CAM_DressupPathBoundary", "The selected object is not a path") + "\n"
        )
        return None

    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    job = PathUtils.findParentJob(base)
    obj.Proxy = DressupPathBoundary(obj, base, job)
    job.Proxy.addOperation(obj, base, True)
    return obj
