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

import FreeCAD
import Part
import Path
import PathScripts.PathEngraveBase as PathEngraveBase
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import math

from PySide import QtCore

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ObjectChamfer(PathEngraveBase.ObjectOp):
    '''Proxy class for Chamfer operation.'''

    def opFeatures(self, obj):
        return PathOp.FeatureTool | PathOp.FeatureHeights | PathOp.FeatureBaseEdges | PathOp.FeatureBaseFaces

    def initOperation(self, obj):
        obj.addProperty("App::PropertyDistance", "Width",      "Chamfer", QtCore.QT_TRANSLATE_NOOP("PathChamfer", "The desired width of the chamfer"))
        obj.addProperty("App::PropertyDistance", "ExtraDepth", "Chamfer", QtCore.QT_TRANSLATE_NOOP("PathChamfer", "The additional depth of the tool path"))

    def opExecute(self, obj):
        angle = self.tool.CuttingEdgeAngle
        if 0 == angle:
            angle = 180
        tan = math.tan(math.radians(angle/2))

        toolDepth = 0 if 0 == tan else obj.Width.Value / tan
        extraDepth = obj.ExtraDepth.Value
        depth = toolDepth + extraDepth
        toolOffset = self.tool.FlatRadius
        extraOffset = self.tool.Diameter/2 - obj.Width.Value if 180 == angle else obj.ExtraDepth.Value / tan
        offset = toolOffset + extraOffset

        wires = []
        for base, subs in obj.Base:
            edges = []
            basewires = []
            for f in subs:
                sub = base.Shape.getElement(f)
                if type(sub) == Part.Edge:
                    edges.append(sub)
                elif sub.Wires:
                    basewires.extend(sub.Wires)
                else:
                    basewires.append(Part.Wire(sub.Edges))
            for edgelist in Part.sortEdges(edges):
                basewires.append(Part.Wire(edgelist))

            for w in self.adjustWirePlacement(obj, base, basewires):
                wires.append(self.offsetWire(w, base, offset))
        self.wires = wires
        self.buildpathocc(obj, wires, [depth], True)

    def offsetWire(self, wire, base, offset):
        def removeInsideEdges(edges):
            def isInside(edge):
                for v in edge.Vertexes:
                    if base.Shape.isInside(v.Point, offset/2, True):
                        return True
                return False
            result = []
            return [e for e in edges if not isInside(e)]

        w = wire.makeOffset2D(offset)
        if wire.isClosed():
            if not base.Shape.isInside(w.Edges[0].Vertexes[0].Point, offset/2, True):
                return w
            return wire.makeOffset2D(-offset)
        edges = removeInsideEdges(w.Edges)
        if edges:
            return Part.Wire(edges)
        w = wire.makeOffset2D(-offset)
        edges = removeInsideEdges(w.Edges)
        return Part.Wire(edges)

    def opSetDefaultValues(self, obj):
        obj.Width = '1 mm'
        obj.ExtraDepth = '0.1 mm'

def Create(name):
    '''Create(name) ... Creates and returns a Chamfer operation.'''
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectChamfer(obj)
    return obj

