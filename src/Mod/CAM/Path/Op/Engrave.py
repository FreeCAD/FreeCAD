# SPDX-License-Identifier: LGPL-2.1-or-later

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
                QT_TRANSLATE_NOOP("App::Property", "Additional base objects to be engraved"),
            )
        obj.setEditorMode("BaseShapes", 2)  # hide

    def initOperation(self, obj):
        """initOperation(obj) ... create engraving specific properties."""
        obj.addProperty(
            "App::PropertyInteger",
            "StartVertex",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The vertex index to start the toolpath from"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "Reverse",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Reverse wires"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "Pattern",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Direction of the path\n"
                "\nDirectional - use direction from wire"
                "\nBidirectional - direction can be swapped while step down or optimizing path",
            ),
        )
        pattern = [
            QT_TRANSLATE_NOOP("CAM_Engrave", "Directional"),
            QT_TRANSLATE_NOOP("CAM_Engrave", "Bidirectional"),
        ]
        obj.Pattern = pattern
        obj.Pattern = "Bidirectional"
        obj.addProperty(
            "App::PropertyEnumeration",
            "SortingMode",
            "Sorting",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Order processing of the wires\n"
                "\nManual - Using order from selection without sorting"
                "\nAutomatic - Sorting wires by the nearest neighbor method"
                "\nAutomatic2 - Sorting wires by the nearest neighbour method, further improved with 2-opt",
            ),
        )
        obj.SortingMode = ("Automatic", "Automatic2", "Manual")
        obj.SortingMode = "Automatic2"

        obj.addProperty(
            "App::PropertyVectorDistance",
            "StartPoint",
            "Sorting",
            QT_TRANSLATE_NOOP("App::Property", "The start point for sorting"),
        )
        obj.addProperty(
            "App::PropertyVectorDistance",
            "EndPoint",
            "Sorting",
            QT_TRANSLATE_NOOP("App::Property", "The end point for sorting"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "UseEndPoint",
            "Sorting",
            QT_TRANSLATE_NOOP("App::Property", "Use end point for sorting"),
        )
        obj.setEditorMode("StartPoint", 2)  # hide
        obj.setEditorMode("EndPoint", 2)  # hide
        obj.setEditorMode("UseEndPoint", 2)  # hide
        self.setupAdditionalProperties(obj)

    def opOnDocumentRestored(self, obj):
        # upgrade ...
        if not hasattr(obj, "Reverse"):
            obj.addProperty(
                "App::PropertyBool",
                "Reverse",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Reverse wires"),
            )
        if not hasattr(obj, "Pattern"):
            pattern = [
                QT_TRANSLATE_NOOP("CAM_Engrave", "Directional"),
                QT_TRANSLATE_NOOP("CAM_Engrave", "Bidirectional"),
            ]
            obj.addProperty(
                "App::PropertyEnumeration",
                "Pattern",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Direction of the path\n"
                    "\nDirectional - use direction from wire"
                    "\nBidirectional - direction can be swapped while step down or optimizing path",
                ),
            )
            obj.Pattern = pattern
        if not hasattr(obj, "SortingMode"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "SortingMode",
                "Sorting",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Order processing of the wires\n"
                    "\nManual - Using order from selection without sorting"
                    "\nAutomatic - Sorting wires by the nearest neighbor method"
                    "\nAutomatic2 - Sorting wires by the nearest neighbour method, further improved with 2-opt",
                ),
            )
            obj.SortingMode = ("Automatic", "Automatic2", "Automatic3", "Manual")
            obj.SortingMode = "Automatic2"
        if not hasattr(obj, "StartPoint"):
            obj.addProperty(
                "App::PropertyVectorDistance",
                "StartPoint",
                "Sorting",
                QT_TRANSLATE_NOOP("App::Property", "The start point of this path"),
            )
            obj.setEditorMode("StartPoint", 2)  # hide
        if not hasattr(obj, "EndPoint"):
            obj.addProperty(
                "App::PropertyVectorDistance",
                "EndPoint",
                "Sorting",
                QT_TRANSLATE_NOOP("App::Property", "The end point of this path"),
            )
            obj.setEditorMode("EndPoint", 2)  # hide
        if not hasattr(obj, "UseEndPoint"):
            obj.addProperty(
                "App::PropertyBool",
                "UseEndPoint",
                "Sorting",
                QT_TRANSLATE_NOOP("App::Property", "Make True, if specifying a End Point"),
            )
            obj.setEditorMode("UseEndPoint", 2)  # hide

        self.setupAdditionalProperties(obj)

    def opExecute(self, obj):
        """opExecute(obj) ... process engraving operation"""
        Path.Log.track()

        SortingMode = 0 if "Automatic2" in obj.SortingMode else 2
        obj.setEditorMode("StartPoint", SortingMode)
        obj.setEditorMode("EndPoint", SortingMode)
        obj.setEditorMode("UseEndPoint", SortingMode)

        jobshapes = []

        if obj.Base:
            # user has selected specific subelements
            Path.Log.track(len(obj.Base))
            for base, subs in obj.Base:
                edges = []
                wires = []
                for feature in subs:
                    sub = base.Shape.getElement(feature)
                    if isinstance(sub, Part.Edge):
                        edges.append(sub)
                    elif sub.Wires:
                        wires.extend(sub.Wires)
                    else:
                        wires.append(Part.Wire(sub.Edges))

                for sortedEdges in Part.sortEdges(edges):
                    wires.append(Part.Wire(sortedEdges))

                jobshapes.append(Part.makeCompound(wires))

        elif obj.BaseShapes:
            # user added specific shapes
            jobshapes.extend([base.Shape for base in obj.BaseShapes])
        else:
            # process all objects in Job.Model.Group
            Path.Log.track(self.model)
            for base in self.model:
                Path.Log.track(base.Label)
                if base.isDerivedFrom("Part::Part2DObject"):
                    jobshapes.append(base.Shape)
                elif base.isDerivedFrom("Sketcher::SketchObject"):
                    jobshapes.append(base.Shape)
                elif hasattr(base, "ArrayType"):
                    jobshapes.append(base.Shape)

        if jobshapes:
            Path.Log.debug("processing {} jobshapes".format(len(jobshapes)))
            wires = []
            for shape in jobshapes:
                if isinstance(shape, Part.Edge):
                    shapeWires = [Part.Wire(shape)]
                else:
                    shapeWires = shape.Wires
                Path.Log.debug("jobshape has {} edges".format(len(shape.Edges)))
                self.commandlist.append(
                    Path.Command("G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid})
                )
                self.buildpathocc(
                    obj,
                    shapeWires,
                    self.getZValues(obj),
                    forward=not obj.Reverse,
                    start_idx=obj.StartVertex,
                )
                wires.extend(shapeWires)
            self.wires = wires
            Path.Log.debug("processing {} jobshapes -> {} wires".format(len(jobshapes), len(wires)))
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
