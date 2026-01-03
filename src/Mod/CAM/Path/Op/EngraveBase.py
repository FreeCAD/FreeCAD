# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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

from lazy_loader.lazy_loader import LazyLoader
import Path
import Path.Op.Base as PathOp
import Path.Op.Util as PathOpUtil
import PathScripts.PathUtils as PathUtils
import PathScripts.tsp as tsp

import copy
import time
import tsp_solver


__doc__ = "Base class for all ops in the engrave family."

# lazily loaded modules
DraftGeomUtils = LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")
Part = LazyLoader("Part", globals(), "Part")

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectOp(PathOp.ObjectOp):
    """Proxy base class for engrave operations."""

    def getZValues(self, obj):
        zValues = []
        if obj.StepDown.Value != 0:
            z = obj.StartDepth.Value - obj.StepDown.Value
            stepdown = obj.StepDown.Value
            if stepdown < 0:
                stepdown = -stepdown

            while z > obj.FinalDepth.Value:
                zValues.append(z)
                z -= stepdown

        zValues.append(obj.FinalDepth.Value)
        return zValues

    def buildpathocc(self, obj, wires, zValues, relZ=False, forward=True, start_idx=0):
        """buildpathocc(obj, wires, zValues, relZ=False) ... internal helper function to generate engraving commands."""
        Path.Log.track(obj.Label, len(wires), zValues)

        tol = self.job.GeometryTolerance.Value if getattr(self, "job", None) else 0.01
        biDir = getattr(obj, "Pattern", None) == "Bidirectional"

        # Sorting wires
        sortingMode = getattr(obj, "SortingMode", None)
        if len(wires) > 1 and sortingMode and "Automatic" in sortingMode:
            locations = []
            if sortingMode == "Automatic":
                # old sorting method
                for w in wires:
                    locations.append(
                        {"x": w.BoundBox.Center.x, "y": w.BoundBox.Center.y, "wire": w}
                    )
                locations = PathUtils.sort_locations(locations, ["x", "y"])
                wires = [loc["wire"] for loc in locations]

            elif sortingMode == "Automatic2" or sortingMode == "Automatic3":
                """it uses nearest neighbour algorithm,
                further improved with relocation and (if allowFlipping=True) 2-opt + flipping

                'allowFlipping' defines whether entry/exit point of each tunnel can be flipped

                'routeStartPoint' and 'routeEndPoint' are optional
                and specifiy where ROUGHLY the route should start and end

                https://forum.freecad.org/viewtopic.php?p=852141#p852141
                """
                route = []
                for i, wire in enumerate(wires):
                    route.append(
                        {
                            "index": i,
                            "startX": wire.OrderedEdges[0].firstVertex().X,
                            "startY": wire.OrderedEdges[0].firstVertex().Y,
                            "endX": wire.OrderedEdges[-1].lastVertex().X,
                            "endY": wire.OrderedEdges[-1].lastVertex().Y,
                            "isOpen": not wire.isClosed(),
                            "flipped": False,
                        }
                    )

                allowFlipping = bool(hasattr(obj, "Pattern") and obj.Pattern == "Bidirectional")
                endPoint = obj.EndPoint if obj.UseEndPoint else None
                timeStart = time.monotonic()
                if sortingMode == "Automatic2":
                    route = tsp_solver.solveTunnels(
                        route,
                        allowFlipping=allowFlipping,
                        routeStartPoint=obj.StartPoint,
                        routeEndPoint=endPoint,
                    )
                if sortingMode == "Automatic3":
                    route = tsp.tsp_solver_tunnels(
                        route,
                        allowFlipping=allowFlipping,
                        routeStartPoint=obj.StartPoint,
                        routeEndPoint=endPoint,
                    )
                print()
                print()
                print(round(time.monotonic() - timeStart, 5))
                orderedWires = []
                for wire in route:
                    if wire["flipped"]:
                        orderedWires.append(Path.Geom.flipWire(wires[wire["index"]]))
                    else:
                        orderedWires.append(wires[wire["index"]])
                wires = orderedWires

        decomposewires = []
        for wire in wires:
            decomposewires.extend(PathOpUtil.makeWires(wire.Edges))
        wires = decomposewires

        def _vstr(v):
            if v:
                return "(%.2f, %.2f, %.2f)" % (v.x, v.y, v.z)
            return "-"

        for wire in wires:
            print([_vstr(v.Point) for v in wire.OrderedVertexes])
            print([_vstr(v.Point) for edge in wire.Edges for v in edge.Vertexes])

            startIdx = min(start_idx, len(wire.Edges) - 1)

            if wire.isClosed():
                # forward and reversed closed wire
                edges = wire.Edges[startIdx:] + wire.Edges[:startIdx]
                startPoint = wire.OrderedVertexes[startIdx].Point
            elif forward:
                # forward open wire
                edges = wire.Edges[startIdx:]
                startPoint = wire.OrderedVertexes[startIdx].Point
            else:
                # reversed open wire
                edges = wire.Edges[: len(wire.Edges) - startIdx]
                startPoint = wire.OrderedVertexes[len(wire.Vertexes) - startIdx - 1].Point

            print("startPoint", _vstr(startPoint))
            lastPoint = None
            reverseDir = not forward

            for z in zValues:
                Path.Log.debug(z)
                if lastPoint and (wire.isClosed() or biDir):
                    # Add step down to next Z for closed profile
                    self.appendCommand(
                        Path.Command("G01", {"X": lastPoint.x, "Y": lastPoint.y, "Z": lastPoint.z}),
                        z,
                        relZ,
                        self.vertFeed,
                    )

                edgesDir = reversed(edges) if reverseDir else edges

                for i, edge in enumerate(edgesDir):
                    print("lastPoint", i, _vstr(lastPoint))
                    if not i and (not lastPoint or (not wire.isClosed() and not biDir)):
                        Path.Log.debug("processing first edge entry")
                        # Add moves to first point of wire
                        lastPoint = copy.copy(startPoint)

                        self.commandlist.append(
                            Path.Command(
                                "G0",
                                {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid},
                            )
                        )
                        self.commandlist.append(
                            Path.Command(
                                "G0", {"X": lastPoint.x, "Y": lastPoint.y, "F": self.horizRapid}
                            )
                        )
                        self.commandlist.append(
                            Path.Command("G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid})
                        )
                        self.appendCommand(
                            Path.Command(
                                "G1", {"X": lastPoint.x, "Y": lastPoint.y, "Z": lastPoint.z}
                            ),
                            z,
                            relZ,
                            self.vertFeed,
                        )

                    if Path.Geom.pointsCoincide(lastPoint, edge.Vertexes[0].Point):
                        flip = False
                        indexLastPoint = -1
                    elif Path.Geom.pointsCoincide(lastPoint, edge.Vertexes[-1].Point):
                        flip = True
                        indexLastPoint = 0
                    else:
                        print("Error: Can determine edge direction")
                        print(lastPoint)
                        print(edge.Vertexes[0].Point)
                        print(edge.Vertexes[-1].Point)
                        return

                    lastPoint = edge.Vertexes[indexLastPoint].Point
                    for cmd in Path.Geom.cmdsForEdge(edge, flip=flip, tol=tol):
                        # Add gcode for edge
                        self.appendCommand(cmd, z, relZ, self.horizFeed)

                if biDir:
                    reverseDir = not reverseDir

            self.commandlist.append(
                Path.Command("G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid})
            )

    def appendCommand(self, cmd, z, relZ, feed):
        params = cmd.Parameters
        if relZ:
            z = params["Z"] - z
        params.update({"Z": z, "F": feed})
        self.commandlist.append(Path.Command(cmd.Name, params))

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj) ... set depths for engraving"""
        if PathOp.FeatureDepths & self.opFeatures(obj):
            if job and len(job.Model.Group) > 0:
                bb = job.Proxy.modelBoundBox(job)
                obj.OpStartDepth = bb.ZMax
                obj.OpFinalDepth = bb.ZMax - max(obj.StepDown.Value, 0.1)
            else:
                obj.OpFinalDepth = -0.1
