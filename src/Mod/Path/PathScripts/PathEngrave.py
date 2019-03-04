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
import FreeCAD
import Part
import Path
import PathScripts.PathEngraveBase as PathEngraveBase
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils
import traceback

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

class ObjectEngrave(PathEngraveBase.ObjectOp):
    '''Proxy class for Engrave operation.'''

    def opFeatures(self, obj):
        '''opFeatures(obj) ... return all standard features and edges based geomtries'''
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureStepDown | PathOp.FeatureBaseEdges  | PathOp.FeatureBaseFaces;

    def setupAdditionalProperties(self, obj):
        if not hasattr(obj, 'BaseShapes'):
            obj.addProperty("App::PropertyLinkList", "BaseShapes", "Path", QtCore.QT_TRANSLATE_NOOP("PathEngrave", "Additional base objects to be engraved"))
        obj.setEditorMode('BaseShapes', 2) # hide
        if not hasattr(obj, 'BaseObject'):
            obj.addProperty("App::PropertyLink", "BaseObject", "Path", QtCore.QT_TRANSLATE_NOOP("PathEngrave", "Additional base objects to be engraved"))
        obj.setEditorMode('BaseObject', 2) # hide

    def initOperation(self, obj):
        '''initOperation(obj) ... create engraving specific properties.'''
        obj.addProperty("App::PropertyInteger", "StartVertex", "Path", QtCore.QT_TRANSLATE_NOOP("PathEngrave", "The vertex index to start the path from"))
        self.setupAdditionalProperties(obj)

    def opOnDocumentRestored(self, obj):
        # upgrade ...
        self.setupAdditionalProperties(obj)

    def opExecute(self, obj):
        '''opExecute(obj) ... process engraving operation'''
        PathLog.track()

        job = PathUtils.findParentJob(obj)

        jobshapes = []
        zValues = self.getZValues(obj)

        try:
            if len(self.model) == 1 and self.model[0].isDerivedFrom('Sketcher::SketchObject') or \
                    self.model[0].isDerivedFrom('Part::Part2DObject') or \
                    hasattr(self.model[0], 'ArrayType'):
                PathLog.track()

                self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))

                # we only consider the outer wire if this is a Face
                wires = []
                for w in self.model[0].Shape.Wires:
                    wires.append(Part.Wire(w.Edges))
                self.buildpathocc(obj, wires, zValues)
                self.wires = wires

            elif len(self.model) == 1 and isinstance(self.model[0].Proxy, ArchPanel.PanelSheet):  # process the sheet
                PathLog.track()
                wires = []
                for tag in self.model[0].Proxy.getTags(self.model[0], transform=True):
                    self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
                    tagWires = []
                    for w in tag.Wires:
                        tagWires.append(Part.Wire(w.Edges))
                    self.buildpathocc(obj, tagWires, zValues)
                    wires.extend(tagWires)
                self.wires = wires
            elif obj.Base:
                PathLog.track()
                wires = []
                for base, subs in obj.Base:
                    edges = []
                    basewires = []
                    for feature in subs:
                        sub = base.Shape.getElement(feature)
                        if type(sub) == Part.Edge:
                            edges.append(sub)
                        elif sub.Wires:
                            basewires.extend(sub.Wires)
                        else:
                            basewires.append(Part.Wire(sub.Edges))

                    for edgelist in Part.sortEdges(edges):
                        basewires.append(Part.Wire(edgelist))

                    wires.extend(basewires)
                self.buildpathocc(obj, wires, zValues)
                self.wires = wires
            elif not obj.BaseShapes:
                PathLog.track()
                if not obj.Base and not obj.BaseShapes:
                    for base in self.model:
                        PathLog.track(base.Label)
                        if base.isDerivedFrom('Part::Part2DObject'):
                            jobshapes.append(base)

                if not jobshapes:
                    raise ValueError(translate('PathEngrave', "Unknown baseobject type for engraving (%s)") % (obj.Base))

            if obj.BaseShapes or jobshapes:
                PathLog.track()
                wires = []
                for shape in obj.BaseShapes + jobshapes:
                    PathLog.track(shape.Label)
                    shapeWires = shape.Shape.Wires
                    self.buildpathocc(obj, shapeWires, zValues)
                    wires.extend(shapeWires)
                self.wires = wires
            # the last command is a move to clearance, which is automatically added by PathOp
            if self.commandlist:
                self.commandlist.pop()

        except Exception as e:
            PathLog.error(e)
            traceback.print_exc()
            PathLog.error(translate('PathEngrave', 'The Job Base Object has no engraveable element.  Engraving operation will produce no output.'))

    def opUpdateDepths(self, obj, ignoreErrors=False):
        '''updateDepths(obj) ... engraving is always done at the top most z-value'''
        job = PathUtils.findParentJob(obj)
        self.opSetDefaultValues(obj, job)

def SetupProperties():
    return [ "StartVertex" ]

def Create(name, obj = None):
    '''Create(name) ... Creates and returns an Engrave operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectEngrave(obj, name)
    return obj

