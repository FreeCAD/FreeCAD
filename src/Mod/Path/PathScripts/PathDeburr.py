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
import PathScripts.PathOpTools as PathOpTools
import PathScripts.PathUtil as PathUtil
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

def toolDepthAndOffset(width, extraDepth, tool):
    '''toolDepthAndOffset(width, extraDepth, tool) ... return tuple for given parameters.'''
    angle = tool.CuttingEdgeAngle
    if 0 == angle:
        angle = 180
    tan = math.tan(math.radians(angle/2))

    toolDepth = 0 if 0 == tan else width / tan
    extraDepth = extraDepth
    depth = toolDepth + extraDepth
    toolOffset = tool.FlatRadius
    extraOffset = tool.Diameter/2 - width if 180 == angle else extraDepth / tan
    offset = toolOffset + extraOffset
    return (depth, offset)

class ObjectDeburr(PathEngraveBase.ObjectOp):
    '''Proxy class for Deburr operation.'''

    def opFeatures(self, obj):
        return PathOp.FeatureTool | PathOp.FeatureHeights | PathOp.FeatureStepDown | PathOp.FeatureBaseEdges | PathOp.FeatureBaseFaces

    def initOperation(self, obj):
        PathLog.track(obj.Label)
        obj.addProperty('App::PropertyDistance',    'Width',      'Deburr', QtCore.QT_TRANSLATE_NOOP('PathDeburr', 'The desired width of the chamfer'))
        obj.addProperty('App::PropertyDistance',    'ExtraDepth', 'Deburr', QtCore.QT_TRANSLATE_NOOP('PathDeburr', 'The additional depth of the tool path'))
        obj.addProperty('App::PropertyEnumeration', 'Join',       'Deburr', QtCore.QT_TRANSLATE_NOOP('PathDeburr', 'How to join chamfer segments'))
        obj.Join = ['Round', 'Miter']
        obj.setEditorMode('Join', 2) # hide for now

    def opOnDocumentRestored(self, obj):
        obj.setEditorMode('Join', 2) # hide for now

    def opExecute(self, obj):
        PathLog.track(obj.Label)
        (depth, offset) = toolDepthAndOffset(obj.Width.Value, obj.ExtraDepth.Value, self.tool)
        PathLog.track(obj.Label, depth, offset)

        self.basewires = []
        self.adjusted_basewires = []
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
            self.edges = edges
            for edgelist in Part.sortEdges(edges):
                basewires.append(Part.Wire(edgelist))

            self.basewires.extend(basewires)

            for w in basewires:
                self.adjusted_basewires.append(w)
                wire = PathOpTools.offsetWire(w, base.Shape, offset, True)
                if wire:
                    wires.append(wire)

        zValues = []
        z = 0
        if obj.StepDown.Value != 0:
            while z + obj.StepDown.Value < depth:
                z = z + obj.StepDown.Value
                zValues.append(z)
        zValues.append(depth)
        PathLog.track(obj.Label, depth, zValues)

        self.wires = wires
        self.buildpathocc(obj, wires, zValues, True)

        # the last command is a move to clearance, which is automatically added by PathOp
        if self.commandlist:
            self.commandlist.pop()

    def opRejectAddBase(self, obj, base, sub):
        '''The chamfer op can only deal with features of the base model, all others are rejected.'''
        return not base in self.model

    def opSetDefaultValues(self, obj, job):
        PathLog.track(obj.Label, job.Label)
        obj.Width = '1 mm'
        obj.ExtraDepth = '0.1 mm'
        obj.Join = 'Round'
        obj.setExpression('StepDown', '0 mm')
        obj.StepDown = '0 mm'

def SetupProperties():
    setup = []
    setup.append('Width')
    setup.append('ExtraDepth')
    return setup

def Create(name, obj = None):
    '''Create(name) ... Creates and returns a Deburr operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectDeburr(obj, name)
    return obj

