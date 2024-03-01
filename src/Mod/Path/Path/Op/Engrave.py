# -*- coding: utf-8 -*-
# ***************************************************************************
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

import FreeCAD
import Path
import Path.Op.Base as PathOp
import Path.Op.EngraveBase as PathEngraveBase
import PathScripts.PathUtils as PathUtils

from PySide.QtCore import QT_TRANSLATE_NOOP

__doc__ = "Class and implementation of CAM Engrave operation"

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")


class ObjectEngrave(PathEngraveBase.ObjectOp):
    """Proxy class for Engrave operation."""

    def __init__(self, obj, name, parentJob):
        super(ObjectEngrave, self).__init__(obj, name, parentJob)
        self.wires = []

    def opFeatures(self, obj):
        """opFeatures(obj) ... return all standard features and edges based geometries"""
        return (
            PathOp.FeatureTool
            | PathOp.FeatureDepths
            | PathOp.FeatureHeights
            | PathOp.FeatureStepDown
            | PathOp.FeatureBaseEdges
            | PathOp.FeatureCoolant
        )

    def setupAdditionalProperties(self, obj):
        if not hasattr(obj, "BaseShapes"):
            obj.addProperty(
                "App::PropertyLinkList",
                "BaseShapes",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Additional base objects to be engraved"
                ),
            )
        obj.setEditorMode("BaseShapes", 2)  # hide
        if not hasattr(obj, "BaseObject"):
            obj.addProperty(
                "App::PropertyLink",
                "BaseObject",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Additional base objects to be engraved"
                ),
            )
        obj.setEditorMode("BaseObject", 2)  # hide

    def initOperation(self, obj):
        """initOperation(obj) ... create engraving specific properties."""
        obj.addProperty(
            "App::PropertyInteger",
            "StartVertex",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The vertex index to start the toolpath from"
            ),
        )
        self.setupAdditionalProperties(obj)

    def opOnDocumentRestored(self, obj):
        # upgrade ...
        self.setupAdditionalProperties(obj)

    def opExecute(self, obj):
        """opExecute(obj) ... process engraving operation"""
        Path.Log.track()

        jobshapes = []

        if len(obj.Base) >= 1:  # user has selected specific subelements
            Path.Log.track(len(obj.Base))
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
                jobshapes.append(Part.makeCompound(wires))

        elif len(obj.BaseShapes) > 0:  # user added specific shapes
            jobshapes.extend([base.Shape for base in obj.BaseShapes])
        else:
            Path.Log.track(self.model)
            for base in self.model:
                Path.Log.track(base.Label)
                if base.isDerivedFrom("Part::Part2DObject"):
                    jobshapes.append(base.Shape)
                elif base.isDerivedFrom("Sketcher::SketchObject"):
                    jobshapes.append(base.Shape)
                elif hasattr(base, "ArrayType"):
                    jobshapes.append(base.Shape)

        if len(jobshapes) > 0:
            Path.Log.debug("processing {} jobshapes".format(len(jobshapes)))
            wires = []
            for shape in jobshapes:
                shapeWires = shape.Wires
                Path.Log.debug("jobshape has {} edges".format(len(shape.Edges)))
                self.commandlist.append(
                    Path.Command(
                        "G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid}
                    )
                )
                self.buildpathocc(obj, shapeWires, self.getZValues(obj))
                wires.extend(shapeWires)
            self.wires = wires
            Path.Log.debug(
                "processing {} jobshapes -> {} wires".format(len(jobshapes), len(wires))
            )
        # the last command is a move to clearance, which is automatically added by PathOp
        if self.commandlist:
            self.commandlist.pop()

    def opUpdateDepths(self, obj):
        """updateDepths(obj) ... engraving is always done at the top most z-value"""
        job = PathUtils.findParentJob(obj)
        self.opSetDefaultValues(obj, job)


def SetupProperties():
    return ["StartVertex"]


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns an Engrave operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectEngrave(obj, name, parentJob)
    return obj
