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
import FreeCAD
import Path
import Path.Base.Generator.linking as linking
import Path.Op.Base as PathOp
import Path.Op.Util as PathOpUtil
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

translate = FreeCAD.Qt.translate


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
        biDir = getattr(obj, "CutPattern", None) == "Bidirectional"

        # Prepare linking arguments for wire-to-wire collision-aware transitions
        solids = []
        if getattr(self, "job", None) and hasattr(self.job, "Model"):
            solids = [base.Shape for base in self.job.Model.Group if hasattr(base, "Shape")]
        linking_kwargs = {
            "start_position": None,
            "target_position": None,
            "local_clearance": obj.SafeHeight.Value,
            "global_clearance": obj.ClearanceHeight.Value,
            "solids": None,
            "tool_shape": None,
            "tool_diameter": None,
            "collision_clearance": obj.CollisionClearance.Value,
        }
        if obj.CollisionAvoidanceStrategy == "Clearance Height":
            linking_kwargs["local_clearance"] = obj.ClearanceHeight.Value
        elif obj.CollisionAvoidanceStrategy == "Retract Height":
            pass
        elif obj.CollisionAvoidanceStrategy == "Line of Sight":
            linking_kwargs["solids"] = solids
        elif obj.CollisionAvoidanceStrategy == "Tool Diameter":
            linking_kwargs["solids"] = solids
            linking_kwargs["tool_diameter"] = obj.ToolController.Tool.Diameter.Value
        elif obj.CollisionAvoidanceStrategy == "Tool Shape":
            linking_kwargs["solids"] = solids
            linking_kwargs["tool_shape"] = obj.ToolController.Tool.BitBody.Shape

        if getattr(obj, "Approximation", False):
            wires = [PathOpUtil.approximateWire(w, tolerance=tol) for w in wires]

        # decompose and orient wires to get edges in right order
        wires = [
            PathOpUtil.orientWire(Part.Wire(se), forward=forward)
            for wire in wires
            for se in Part.sortEdges(wire.Edges)
        ]

        # sorting wires
        if len(wires) > 1 and getattr(obj, "SortingMode", None) == "Automatic":
            endPoint = obj.EndPoint if obj.UseEndPoint else None
            if len(zValues) % 2 == 0 and biDir:  # sorting pairs points
                pairs = []
                for indexWire, wire in enumerate(wires):
                    indexAlt = -1 if not wire.isClosed() else 0
                    pairs.append(
                        {
                            "x": wire.Vertexes[0].X,
                            "y": wire.Vertexes[0].Y,
                            "xAlt": wire.Vertexes[indexAlt].X,
                            "yAlt": wire.Vertexes[indexAlt].Y,
                            "index": indexWire,
                        }
                    )

                sortedPairs = tsp_solver.solvePairs(
                    pairs, routeStartPoint=obj.StartPoint, routeEndPoint=endPoint
                )
                orderedWires = []
                for pair in sortedPairs:
                    x = wires[pair["index"]].Vertexes[0].X
                    y = wires[pair["index"]].Vertexes[0].Y
                    if pair["x"] != x or pair["y"] != y:
                        orderedWires.append(Path.Geom.flipWire(wires[pair["index"]]))
                    else:
                        orderedWires.append(wires[pair["index"]])

            else:  # sorting tunnels
                tunnels = []
                for wire in wires:
                    indexEnd = -1 if not wire.isClosed() else 0
                    tunnels.append(
                        {
                            "startX": wire.Vertexes[0].X,
                            "startY": wire.Vertexes[0].Y,
                            "endX": wire.Vertexes[indexEnd].X,
                            "endY": wire.Vertexes[indexEnd].Y,
                        }
                    )
                sortedTunnels = tsp_solver.solveTunnels(
                    tunnels,
                    allowFlipping=biDir,
                    routeStartPoint=obj.StartPoint,
                    routeEndPoint=endPoint,
                )
                orderedWires = []
                for tunnel in sortedTunnels:
                    if tunnel["flipped"]:
                        orderedWires.append(Path.Geom.flipWire(wires[tunnel["index"]]))
                    else:
                        orderedWires.append(wires[tunnel["index"]])

            wires = orderedWires

        tool_pos = None  # tracks tool position between entry moves

        for wire in wires:
            reverseDir = False
            edges = wire.Edges
            startIdx = min(start_idx, len(wire.Edges) - 1)
            if wire.isClosed() and startIdx:
                edges = edges[startIdx:] + edges[:startIdx]

            startPoint = edges[0].valueAt(edges[0].FirstParameter)

            for indexZ, z in enumerate(zValues):
                Path.Log.debug(z)

                if indexZ == 0 or (not wire.isClosed() and not biDir):
                    Path.Log.debug("processing first edge entry")
                    if tool_pos is None:
                        # Very first entry in the operation: retract to clearance height
                        self.commandlist.append(
                            Path.Command("G0", {"Z": obj.ClearanceHeight.Value})
                        )
                        self.commandlist.append(
                            Path.Command("G0", {"X": startPoint.x, "Y": startPoint.y})
                        )
                        self.commandlist.append(Path.Command("G0", {"Z": obj.SafeHeight.Value}))
                    else:
                        # Subsequent entries: use linking generator to find optimal retract height
                        target = FreeCAD.Vector(startPoint.x, startPoint.y, obj.SafeHeight.Value)
                        linking_kwargs["start_position"] = tool_pos
                        linking_kwargs["target_position"] = target
                        moves = linking.get_linking_moves(**linking_kwargs)
                        self.commandlist.extend(moves)
                    lastPoint = startPoint

                self.appendCommand(Path.Command("G1", {"Z": startPoint.z}), z, relZ, self.vertFeed)

                for edge in (reversed(edges) if reverseDir and indexZ else edges):
                    if len(edges) == 1:
                        # wire with one edge
                        flip = reverseDir
                    elif Path.Geom.pointsCoincide(lastPoint, edge.valueAt(edge.FirstParameter)):
                        flip = False
                        lastPoint = edge.valueAt(edge.LastParameter)
                    elif Path.Geom.pointsCoincide(lastPoint, edge.valueAt(edge.LastParameter)):
                        flip = True
                        lastPoint = edge.valueAt(edge.FirstParameter)
                    else:
                        Path.Log.warning("Error while checks points coincide")
                        return

                    for cmd in Path.Geom.cmdsForEdge(edge, flip=flip, tol=tol):
                        # Add gcode for edge
                        self.appendCommand(cmd, z, relZ, self.horizFeed)

                # Track tool position for the next entry move.
                # Use SafeHeight so the linking check only evaluates the horizontal
                # traverse, not the retract from cutting depth through the stock.
                tool_pos = FreeCAD.Vector(lastPoint.x, lastPoint.y, obj.SafeHeight.Value)

                if biDir and not wire.isClosed():
                    reverseDir = not reverseDir

    def appendCommand(self, cmd, z, relZ, feed):
        params = cmd.Parameters
        if relZ:
            z = params["Z"] - z
        params.update({"Z": z, "F": feed})
        self.commandlist.append(Path.Command(cmd.Name, params))

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj) ... set depths for engraving"""
        if PathOp.FeatureDepths & self.opFeatures(obj):
            if job and job.Stock:
                obj.OpStartDepth = job.Stock.Shape.BoundBox.ZMax
                obj.OpFinalDepth = job.Stock.Shape.BoundBox.ZMax

            if obj.Base:
                obj.OpFinalDepth = max(
                    sh.BoundBox.ZMax for base, sub in obj.Base for sh in base.getSubObject(sub)
                )
            elif obj.BaseShapes:
                obj.OpFinalDepth = max(base.Shape.BoundBox.ZMax for base in obj.BaseShapes)

            obj.OpStartDepth = max(obj.OpStartDepth, obj.OpFinalDepth)
