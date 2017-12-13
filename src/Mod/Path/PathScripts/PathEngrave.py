# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

import ArchPanel
import DraftGeomUtils
import FreeCAD
import Part
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils

from PySide import QtCore

__doc__ = "Class and implementation of Path Engrave operation"

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectEngrave(PathOp.ObjectOp):
    '''Proxy class for Engrave operation.'''

    def opFeatures(self, obj):
        '''opFeatures(obj) ... return all standard features and edges based geomtries'''
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureStepDown | PathOp.FeatureBaseEdges;

    def initOperation(self, obj):
        '''initOperation(obj) ... create engraving specific properties.'''
        obj.addProperty("App::PropertyInteger", "StartVertex", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The vertex index to start the path from"))

    def opExecute(self, obj):
        '''opExecute(obj) ... process engraving operation'''
        PathLog.track()

        zValues = []
        if obj.StepDown.Value != 0:
            z = obj.StartDepth.Value - obj.StepDown.Value

            while z > obj.FinalDepth.Value:
                zValues.append(z)
                z -= obj.StepDown.Value
        zValues.append(obj.FinalDepth.Value)
        self.zValues = zValues

        output = ''
        try:
            if self.baseobject.isDerivedFrom('Sketcher::SketchObject') or \
                    self.baseobject.isDerivedFrom('Part::Part2DObject') or \
                    hasattr(self.baseobject, 'ArrayType'):

                output += "G0 Z" + PathUtils.fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"
                self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))

                # we only consider the outer wire if this is a Face
                wires = []
                for w in self.baseobject.Shape.Wires:
                    tempedges = PathUtils.cleanedges(w.Edges, 0.5)
                    wires.append(Part.Wire(tempedges))
                output += self.buildpathocc(obj, wires, zValues)
                self.wires = wires

            elif isinstance(self.baseobject.Proxy, ArchPanel.PanelSheet):  # process the sheet
                wires = []
                for tag in self.baseobject.Proxy.getTags(self.baseobject, transform=True):
                    output += "G0 Z" + PathUtils.fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"
                    self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
                    tagWires = []
                    for w in tag.Wires:
                        tempedges = PathUtils.cleanedges(w.Edges, 0.5)
                        tagWires.append(Part.Wire(tempedges))
                    output += self.buildpathocc(obj, tagWires, zValues)
                    wires.extend(tagWires)
                self.wires = wires
            else:
                raise ValueError('Unknown baseobject type for engraving')

            output += "G0 Z" + PathUtils.fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"
            self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))

        except Exception as e:
            #PathLog.error("Exception: %s" % e)
            PathLog.error(translate("Path", "The Job Base Object has no engraveable element.  Engraving operation will produce no output."))

    def buildpathocc(self, obj, wires, zValues):
        '''buildpathocc(obj, wires, zValues) ... internal helper function to generate engraving commands.'''
        PathLog.track()

        output = ""

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
                        output += "G0" + " X" + PathUtils.fmt(last.x) + " Y" + PathUtils.fmt(last.y) + " Z" + PathUtils.fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.horizRapid)  # Rapid to starting position
                        self.commandlist.append(Path.Command('G0', {'X': last.x, 'Y': last.y, 'Z': obj.ClearanceHeight.Value, 'F': self.horizRapid}))
                        output += "G0" + " X" + PathUtils.fmt(last.x) + " Y" + PathUtils.fmt(last.y) + " Z" + PathUtils.fmt(obj.SafeHeight.Value) + "F " + PathUtils.fmt(self.horizRapid)  # Rapid to safe height
                        self.commandlist.append(Path.Command('G0', {'X': last.x, 'Y': last.y, 'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
                        output += "G1" + " X" + PathUtils.fmt(last.x) + " Y" + PathUtils.fmt(last.y) + " Z" + PathUtils.fmt(z) + "F " + PathUtils.fmt(self.vertFeed) + "\n"  # Vertical feed to depth
                        self.commandlist.append(Path.Command('G0', {'X': last.x, 'Y': last.y, 'Z': z, 'F': self.vertFeed}))

                    if isinstance(edge.Curve, Part.Circle):
                        point = edge.Vertexes[-1].Point
                        if point == last:  # edges can come flipped
                            point = edge.Vertexes[0].Point
                        center = edge.Curve.Center
                        relcenter = center.sub(last)
                        v1 = last.sub(center)
                        v2 = point.sub(center)
                        if v1.cross(v2).z < 0:
                            output += "G2"
                        else:
                            output += "G3"
                        output += " X" + PathUtils.fmt(point.x) + " Y" + PathUtils.fmt(point.y) + " Z" + PathUtils.fmt(z)
                        output += " I" + PathUtils.fmt(relcenter.x) + " J" + PathUtils.fmt(relcenter.y) + " K" + PathUtils.fmt(relcenter.z)
                        output += " F " + PathUtils.fmt(self.horizFeed)
                        output += "\n"

                        cmd = 'G2' if v1.cross(v2).z < 0 else 'G3'
                        args = {}
                        args['X'] = point.x
                        args['Y'] = point.y
                        args['Z'] = z
                        args['I'] = relcenter.x
                        args['J'] = relcenter.y
                        args['K'] = relcenter.z
                        args['F'] = self.horizFeed
                        self.commandlist.append(Path.Command(cmd, args))
                        last = point
                    else:
                        point = edge.Vertexes[-1].Point
                        if point == last:  # edges can come flipped
                            point = edge.Vertexes[0].Point
                        output += "G1 X" + PathUtils.fmt(point.x) + " Y" + PathUtils.fmt(point.y) + " Z" + PathUtils.fmt(z)
                        output += " F " + PathUtils.fmt(self.horizFeed)
                        output += "\n"
                        self.commandlist.append(Path.Command('G1', {'X': point.x, 'Y': point.y, 'Z': z, 'F': self.horizFeed}))
                        last = point
            output += "G0 Z " + PathUtils.fmt(obj.ClearanceHeight.Value)
            self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        return output

    def opSetDefaultValues(self, obj):
        '''opSetDefaultValues(obj) ... set depths for engraving'''
        job = PathUtils.findParentJob(obj)
        if job and job.Base:
            bb = job.Base.Shape.BoundBox
            obj.OpStartDepth = bb.ZMax
            obj.OpFinalDepth = bb.ZMin
        else:
            obj.OpFinalDepth = -0.1

def Create(name):
    '''Create(name) ... Creates and returns a Engrave operation.'''
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectEngrave(obj)
    return obj

