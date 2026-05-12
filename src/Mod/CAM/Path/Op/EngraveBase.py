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
import Path.Op.Base as PathOp
import Path.Op.Util as PathOpUtil
import PathScripts.PathUtils as PathUtils

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

        # decompose wires to get edges in right order
        wires = [w for wire in wires for w in PathOpUtil.makeWires(wire.Edges)]

        # sort wires, adapted from Area.py
        if len(wires) > 1:
            locations = []
            for w in wires:
                locations.append({"x": w.BoundBox.Center.x, "y": w.BoundBox.Center.y, "wire": w})

            locations = PathUtils.sort_locations(locations, ["x", "y"])
            wires = [j["wire"] for j in locations]

        for wire in wires:
            wire = PathOpUtil.orientWire(wire, forward=forward)
            reverseDir = False
            edges = wire.Edges
            startIdx = min(start_idx, len(wire.Edges) - 1)
            if wire.isClosed() and startIdx:
                edges = edges[startIdx:] + edges[:startIdx]

            startPoint = edges[0].valueAt(edges[0].FirstParameter)

            for indexZ, z in enumerate(zValues):
                Path.Log.debug(z)
                if indexZ and (wire.isClosed() or biDir):
                    # Skip retract and add step down to next Z for closed profile
                    self.appendCommand(
                        Path.Command("G1", {"Z": startPoint.z}),
                        z,
                        relZ,
                        self.vertFeed,
                    )

                edgesDir = reversed(edges) if reverseDir and indexZ else edges

                for indexE, edge in enumerate(edgesDir):
                    if indexE == 0 and (indexZ == 0 or (not wire.isClosed() and not biDir)):
                        Path.Log.debug("processing first edge entry")
                        # Add moves to first point of wire
                        self.commandlist.append(
                            Path.Command("G0", {"Z": obj.ClearanceHeight.Value})
                        )
                        self.commandlist.append(
                            Path.Command("G0", {"X": startPoint.x, "Y": startPoint.y})
                        )
                        self.commandlist.append(Path.Command("G0", {"Z": obj.SafeHeight.Value}))
                        self.appendCommand(
                            Path.Command("G1", {"Z": startPoint.z}),
                            z,
                            relZ,
                            self.vertFeed,
                        )
                        lastPoint = startPoint

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
                obj.OpFinalDepth = obj.Base[0][0].Shape.BoundBox.ZMax
            elif obj.BaseShapes:
                obj.OpFinalDepth = obj.BaseShapes[0].Shape.BoundBox.ZMax

            obj.OpStartDepth = max(obj.OpStartDepth, obj.OpFinalDepth)
