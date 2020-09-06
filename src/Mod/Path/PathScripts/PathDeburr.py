# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2020 Schildkroet                                        *
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
import PathScripts.PathEngraveBase as PathEngraveBase
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathOpTools as PathOpTools
import math

from PySide import QtCore

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader
Part = LazyLoader('Part', globals(), 'Part')

__title__ = "Path Deburr Operation"
__author__ = "sliptonic (Brad Collette), Schildkroet"
__url__ = "http://www.freecadweb.org"
__doc__ = "Deburr operation."

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
#PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


def toolDepthAndOffset(width, extraDepth, tool):
    '''toolDepthAndOffset(width, extraDepth, tool) ... return tuple for given parameters.'''
    angle = float(tool.CuttingEdgeAngle)
    if 0 == angle:
        angle = 180
    tan = math.tan(math.radians(angle / 2))

    toolDepth = 0 if 0 == tan else width / tan
    depth = toolDepth + extraDepth
    toolOffset = float(tool.FlatRadius)
    extraOffset = float(tool.Diameter) / 2 - width if 180 == angle else extraDepth / tan
    offset = toolOffset + extraOffset
    
    return (depth, offset)


class ObjectDeburr(PathEngraveBase.ObjectOp):
    '''Proxy class for Deburr operation.'''

    def opFeatures(self, obj):
        return PathOp.FeatureTool | PathOp.FeatureHeights | PathOp.FeatureStepDown | PathOp.FeatureBaseEdges | PathOp.FeatureBaseFaces | PathOp.FeatureCoolant 

    def initOperation(self, obj):
        PathLog.track(obj.Label)
        obj.addProperty('App::PropertyDistance', 'Width', 'Deburr', QtCore.QT_TRANSLATE_NOOP('PathDeburr', 'The desired width of the chamfer'))
        obj.addProperty('App::PropertyDistance', 'ExtraDepth', 'Deburr', QtCore.QT_TRANSLATE_NOOP('PathDeburr', 'The additional depth of the tool path'))
        obj.addProperty('App::PropertyEnumeration', 'Join', 'Deburr', QtCore.QT_TRANSLATE_NOOP('PathDeburr', 'How to join chamfer segments'))
        obj.Join = ['Round', 'Miter']
        obj.setEditorMode('Join', 2)  # hide for now
        obj.addProperty('App::PropertyEnumeration', 'Direction',  'Deburr', QtCore.QT_TRANSLATE_NOOP('PathDeburr', 'Direction of Operation'))
        obj.Direction = ['CW', 'CCW']
        obj.addProperty('App::PropertyEnumeration', 'Side',  'Deburr', QtCore.QT_TRANSLATE_NOOP('PathDeburr', 'Side of Operation'))
        obj.Side = ['Outside', 'Inside']
        obj.setEditorMode('Side', 2) # Hide property, it's calculated by op
        obj.addProperty('App::PropertyInteger', 'EntryPoint',  'Deburr', QtCore.QT_TRANSLATE_NOOP('PathDeburr', 'Select the segment, there the operations starts'))

    def opOnDocumentRestored(self, obj):
        obj.setEditorMode('Join', 2)  # hide for now

    def opExecute(self, obj):
        PathLog.track(obj.Label)
        (depth, offset) = toolDepthAndOffset(obj.Width.Value, obj.ExtraDepth.Value, self.tool)
        PathLog.track(obj.Label, depth, offset)

        self.basewires = [] # pylint: disable=attribute-defined-outside-init
        self.adjusted_basewires = [] # pylint: disable=attribute-defined-outside-init
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
            self.edges = edges # pylint: disable=attribute-defined-outside-init
            for edgelist in Part.sortEdges(edges):
                basewires.append(Part.Wire(edgelist))

            self.basewires.extend(basewires)
            
            # Set default value
            side = ["Outside"]
            
            for w in basewires:
                self.adjusted_basewires.append(w)
                wire = PathOpTools.offsetWire(w, base.Shape, offset, True, side)
                if wire:
                    wires.append(wire)
        
        # Save Outside or Inside
        obj.Side = side[0]
        
        # Set direction of op
        forward = True
        if obj.Direction == 'CCW':
            forward = False
        
        zValues = []
        z = 0
        if obj.StepDown.Value != 0:
            while z + obj.StepDown.Value < depth:
                z = z + obj.StepDown.Value
                zValues.append(z)
        zValues.append(depth)
        PathLog.track(obj.Label, depth, zValues)
        
        if obj.EntryPoint < 0:
            obj.EntryPoint = 0;
        
        self.wires = wires # pylint: disable=attribute-defined-outside-init
        self.buildpathocc(obj, wires, zValues, True, forward, obj.EntryPoint)
        

    def opRejectAddBase(self, obj, base, sub):
        '''The chamfer op can only deal with features of the base model, all others are rejected.'''
        return base not in self.model

    def opSetDefaultValues(self, obj, job):
        PathLog.track(obj.Label, job.Label)
        obj.Width = '1 mm'
        obj.ExtraDepth = '0.5 mm'
        obj.Join = 'Round'
        obj.setExpression('StepDown', '0 mm')
        obj.StepDown = '0 mm'
        obj.Direction = 'CW'
        obj.Side = "Outside"
        obj.EntryPoint = 0;


def SetupProperties():
    setup = []
    setup.append('Width')
    setup.append('ExtraDepth')
    return setup


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Deburr operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectDeburr(obj, name)
    return obj
