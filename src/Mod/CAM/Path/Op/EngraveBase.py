# -*- coding: utf-8 -*-
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

import copy

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

        # sort wires, adapted from Area.py
        if len(wires) > 1:
            locations = []
            for w in wires:
                locations.append({"x": w.BoundBox.Center.x, "y": w.BoundBox.Center.y, "wire": w})

            locations = PathUtils.sort_locations(locations, ["x", "y"])
            wires = [j["wire"] for j in locations]

        decomposewires = []
        for wire in wires:
            decomposewires.extend(PathOpUtil.makeWires(wire.Edges))

        wires = decomposewires
        print()
        print("Direction", obj.Direction)
        print("start_idx", start_idx)
        print(f"wires {wires}")
        for wire in wires:
            print(f">>> wire {wire}")
            # offset = wire

            # reorder the wire
            if hasattr(obj, "StartVertex"):
                start_idx = obj.StartVertex
            edges = wire.Edges

            # edges = copy.copy(PathOpUtil.orientWire(offset, forward).Edges)
            # Path.Log.track("wire: {} offset: {}".format(len(wire.Edges), len(edges)))
            # edges = Part.sortEdges(edges)[0]
            # Path.Log.track("edges: {}".format(len(edges)))

            lastPoint = None
            reverseDir = False
            dualDir = False

            if hasattr(obj, "Direction"):
                if obj.Direction == "Reversed":
                    reverseDir = True
                if obj.Direction == "Dual":
                    dualDir = True

            for z in zValues:

                def rndv(vector):
                    if vector:
                        v = copy.copy(vector)
                        v.x = round(v.x, 1)
                        v.y = round(v.y, 1)
                        v.z = round(v.z, 1)
                        return (v.x, v.y, v.z)

                print()
                print(f"  z {z}")
                Path.Log.debug(z)
                if lastPoint and (wire.isClosed() or dualDir):
                    # Add step down to next Z for closed profile
                    self.appendCommand(
                        Path.Command("G01", {"X": lastPoint.x, "Y": lastPoint.y, "Z": lastPoint.z}),
                        z,
                        relZ,
                        self.vertFeed,
                    )

                first = True
                if start_idx > len(edges) - 1:
                    start_idx = len(edges) - 1

                edges = edges[start_idx:] + edges[:start_idx]

                if reverseDir:
                    edgesDir = reversed(edges)
                    sIndex, eIndex = -1, 0
                else:
                    edgesDir = edges
                    sIndex, eIndex = 0, -1

                print(f"    edges {edges}")
                for i, edge in enumerate(edgesDir):
                    print()
                    print(f"      lastPoint {rndv(lastPoint)}")
                    print(f"      edge {i}  {edge}")
                    print(
                        f"      s{rndv(edge.Vertexes[sIndex].Point)} e{rndv(edge.Vertexes[eIndex].Point)}"
                    )
                    print(f"      {round(edge.FirstParameter,3)}  {round(edge.LastParameter,3)}")

                    Path.Log.debug(
                        "points: {} -> {}".format(
                            edge.Vertexes[sIndex].Point, edge.Vertexes[eIndex].Point
                        )
                    )
                    Path.Log.debug(
                        "valueat {} -> {}".format(
                            edge.valueAt(edge.FirstParameter),
                            edge.valueAt(edge.LastParameter),
                        )
                    )
                    if first and (not lastPoint or (not wire.isClosed() and not dualDir)):
                        Path.Log.debug("processing first edge entry")
                        # Add moves to first point of wire
                        lastPoint = edge.Vertexes[sIndex].Point

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
                    first = False

                    # edgePar = edge.FirstParameter if not reverseDir else edge.LastParameter
                    # if Path.Geom.pointsCoincide(lastPoint, edge.valueAt(edgePar)):
                    if Path.Geom.pointsCoincide(lastPoint, edge.Vertexes[sIndex].Point):
                        print("      pointsCoincide True")
                        flip = False
                        lastPoint = edge.Vertexes[eIndex].Point
                    else:
                        print("      pointsCoincide False")
                        flip = True
                        lastPoint = edge.Vertexes[sIndex].Point

                    if reverseDir:
                        flip = not flip

                    print(f"      reverseDir {reverseDir}")
                    print(f"      flip {flip}")
                    for cmd in Path.Geom.cmdsForEdge(edge, flip):
                        # Add gcode for edge
                        self.appendCommand(cmd, z, relZ, self.horizFeed)

                if dualDir:
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
