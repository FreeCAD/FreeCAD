# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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

import DraftGeomUtils
import FreeCAD
import Part
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils

from PathScripts.PathGeom import PathGeom
from PySide import QtCore

__doc__ = "Base class for all ops in the engrave family."

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ObjectOp(PathOp.ObjectOp):
    '''Proxy base class for engrave operations.'''

    def buildpathocc(self, obj, wires, zValues):
        '''buildpathocc(obj, wires, zValues) ... internal helper function to generate engraving commands.'''
        PathLog.track(obj.Label, len(wires), zValues)

        for wire in wires:
            offset = wire

            # reorder the wire
            offset = DraftGeomUtils.rebaseWire(offset, obj.StartVertex)

            last = None
            for z in zValues:
                if last:
                    self.commandlist.append(Path.Command('G1', {'X': last.x, 'Y': last.y, 'Z': z, 'F': self.vertFeed}))

                for edge in offset.Edges:
                    if not last:
                        # we set the first move to our first point
                        last = edge.Vertexes[0].Point
                        if len(offset.Edges) > 1:
                            e2 = offset.Edges[1]
                            if not PathGeom.pointsCoincide(edge.Vertexes[-1].Point, e2.Vertexes[0].Point) and not PathGeom.pointsCoincide(edge.Vertexes[-1].Point, e2.Vertexes[-1].Point):
                                PathLog.debug("flip first edge")
                                last = edge.Vertexes[-1].Point
                            else:
                                PathLog.debug("original first edge")
                        else:
                            PathLog.debug("not enough edges to flip")
                        self.commandlist.append(Path.Command('G0', {'X': last.x, 'Y': last.y, 'Z': obj.ClearanceHeight.Value, 'F': self.horizRapid}))
                        self.commandlist.append(Path.Command('G0', {'X': last.x, 'Y': last.y, 'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
                        self.commandlist.append(Path.Command('G0', {'X': last.x, 'Y': last.y, 'Z': z, 'F': self.vertFeed}))

                    if PathGeom.pointsCoincide(last, edge.Vertexes[0].Point):
                        for cmd in PathGeom.cmdsForEdge(edge):
                            params = cmd.Parameters
                            params.update({'Z': z, 'F': self.horizFeed})
                            self.commandlist.append(Path.Command(cmd.Name, params))
                        last = edge.Vertexes[-1].Point
                    else:
                        for cmd in PathGeom.cmdsForEdge(edge, True):
                            params = cmd.Parameters
                            params.update({'Z': z, 'F': self.horizFeed})
                            self.commandlist.append(Path.Command(cmd.Name, params))
                        last = edge.Vertexes[0].Point
            self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        if self.commandlist:
            self.commandlist.pop()


    def opSetDefaultValues(self, obj):
        '''opSetDefaultValues(obj) ... set depths for engraving'''
        job = PathUtils.findParentJob(obj)
        if job and job.Base:
            bb = job.Base.Shape.BoundBox
            obj.OpStartDepth = bb.ZMax
            obj.OpFinalDepth = bb.ZMax - max(obj.StepDown.Value, 0.1)
        else:
            obj.OpFinalDepth = -0.1

    def adjustWirePlacement(self, obj, shape, wires):
        job = PathUtils.findParentJob(obj)
        if hasattr(shape, 'MapMode') and 'Deactivated' != shape.MapMode:
            if hasattr(shape, 'Support') and 1 == len(shape.Support) and 1 == len(shape.Support[0][1]):
                pmntShape   = shape.Placement
                pmntSupport = shape.Support[0][0].getGlobalPlacement()
                #pmntSupport = shape.Support[0][0].Placement
                pmntBase    = job.Base.Placement
                pmnt = pmntBase.multiply(pmntSupport.inverse().multiply(pmntShape))
                #PathLog.debug("pmnt = %s" % pmnt)
                newWires = []
                for w in wires:
                    edges = []
                    for e in w.Edges:
                        e = e.copy()
                        e.Placement = FreeCAD.Placement()
                        edges.append(e)
                    w = Part.Wire(edges)
                    w.Placement = pmnt
                    newWires.append(w)
                wires = newWires
            else:
                PathLog.warning(translate("PathEngrave", "Attachment not supported by engraver"))
        else:
            PathLog.debug("MapMode: %s" % (shape.MapMode if hasattr(shape, 'MapMode') else 'None')) 
        return wires

