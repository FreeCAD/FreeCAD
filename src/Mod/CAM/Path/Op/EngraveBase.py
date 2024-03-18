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

        decomposewires = []
        for wire in wires:
            decomposewires.extend(PathOpUtil.makeWires(wire.Edges))

        wires = decomposewires
        for wire in wires:
            # offset = wire

            # reorder the wire
            if hasattr(obj, "StartVertex"):
                start_idx = obj.StartVertex
            edges = wire.Edges

            # edges = copy.copy(PathOpUtil.orientWire(offset, forward).Edges)
            # Path.Log.track("wire: {} offset: {}".format(len(wire.Edges), len(edges)))
            # edges = Part.sortEdges(edges)[0]
            # Path.Log.track("edges: {}".format(len(edges)))

            last = None

            for z in zValues:
                Path.Log.debug(z)
                if last:
                    self.appendCommand(
                        Path.Command("G1", {"X": last.x, "Y": last.y, "Z": last.z}),
                        z,
                        relZ,
                        self.vertFeed,
                    )

                first = True
                if start_idx > len(edges) - 1:
                    start_idx = len(edges) - 1

                edges = edges[start_idx:] + edges[:start_idx]
                for edge in edges:
                    Path.Log.debug(
                        "points: {} -> {}".format(
                            edge.Vertexes[0].Point, edge.Vertexes[-1].Point
                        )
                    )
                    Path.Log.debug(
                        "valueat {} -> {}".format(
                            edge.valueAt(edge.FirstParameter),
                            edge.valueAt(edge.LastParameter),
                        )
                    )
                    if first and (not last or not wire.isClosed()):
                        Path.Log.debug("processing first edge entry")
                        # we set the first move to our first point
                        last = edge.Vertexes[0].Point

                        self.commandlist.append(
                            Path.Command(
                                "G0",
                                {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid},
                            )
                        )
                        self.commandlist.append(
                            Path.Command(
                                "G0", {"X": last.x, "Y": last.y, "F": self.horizRapid}
                            )
                        )
                        self.commandlist.append(
                            Path.Command(
                                "G0", {"Z": obj.SafeHeight.Value, "F": self.vertRapid}
                            )
                        )
                        self.appendCommand(
                            Path.Command("G1", {"X": last.x, "Y": last.y, "Z": last.z}),
                            z,
                            relZ,
                            self.vertFeed,
                        )
                    first = False

                    if Path.Geom.pointsCoincide(
                        last, edge.valueAt(edge.FirstParameter)
                    ):
                        # if Path.Geom.pointsCoincide(last, edge.Vertexes[0].Point):
                        for cmd in Path.Geom.cmdsForEdge(edge):
                            self.appendCommand(cmd, z, relZ, self.horizFeed)
                        last = edge.Vertexes[-1].Point
                    else:
                        for cmd in Path.Geom.cmdsForEdge(edge, True):
                            self.appendCommand(cmd, z, relZ, self.horizFeed)
                        last = edge.Vertexes[0].Point
            self.commandlist.append(
                Path.Command(
                    "G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid}
                )
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
