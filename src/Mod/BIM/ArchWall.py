# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD Wall"
__author__ = "Yorik van Havre"
__url__ = "https://www.freecad.org"

## @package ArchWall
#  \ingroup ARCH
#  \brief The Wall object and tools
#
#  This module provides tools to build Wall objects.  Walls are simple objects,
#  usually vertical, typically obtained by giving a thickness to a base line,
#  then extruding it vertically.

"""This module provides tools to build Wall objects.  Walls are simple
objects, usually vertical, typically obtained by giving a thickness to a base
line, then extruding it vertically.

Examples
--------
TODO put examples here.

"""

import math

import FreeCAD
import ArchCommands
import ArchComponent
import ArchSketchObject
import Draft
import DraftVecUtils

from FreeCAD import Vector
from draftutils import params

if FreeCAD.GuiUp:
    from PySide import QtCore, QtGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    import draftguitools.gui_trackers as DraftTrackers
    from draftutils.translate import translate
else:
    # \cond
    def translate(ctxt, txt):
        return txt

    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt

    # \endcond


def mergeShapes(w1, w2):
    """Not currently implemented.

    Return a Shape built on two walls that share same properties and have a
    coincident endpoint.
    """

    if not areSameWallTypes([w1, w2]):
        return None
    if (not hasattr(w1.Base, "Shape")) or (not hasattr(w2.Base, "Shape")):
        return None
    if w1.Base.Shape.Faces or w2.Base.Shape.Faces:
        return None

    # TODO fix this
    return None

    eds = w1.Base.Shape.Edges + w2.Base.Shape.Edges
    import DraftGeomUtils

    w = DraftGeomUtils.findWires(eds)
    if len(w) == 1:
        # print("found common wire")
        normal, length, width, height = w1.Proxy.getDefaultValues(w1)
        print(w[0].Edges)
        sh = w1.Proxy.getBase(w1, w[0], normal, width, height)
        print(sh)
        return sh
    return None


def areSameWallTypes(walls):
    """Check if a list of walls have the same height, width and alignment.

    Parameters
    ----------
    walls: list of <ArchComponent.Component>

    Returns
    -------
    bool
        True if the walls have the same height, width and alignment, False if
        otherwise.
    """

    for att in ["Width", "Height", "Align"]:
        value = None
        for w in walls:
            if not hasattr(w, att):
                return False
            if not value:
                value = getattr(w, att)
            else:
                if type(value) == float:
                    if round(value, Draft.precision()) != round(getattr(w, att), Draft.precision()):
                        return False
                else:
                    if value != getattr(w, att):
                        return False
    return True


class _Wall(ArchComponent.Component):
    """The Wall object.

    Turns a <App::FeaturePython> into a wall object, then uses a
    <Part::Feature> to create the wall's shape.

    Walls are simple objects, usually vertical, typically obtained by giving a
    thickness to a base line, then extruding it vertically.

    Parameters
    ----------
    obj: <App::FeaturePython>
        The object to turn into a wall. Note that this is not the object that
        forms the basis for the new wall's shape. That is given later.
    """

    def __init__(self, obj):
        ArchComponent.Component.__init__(self, obj)
        self.Type = "Wall"
        self.setProperties(obj)
        obj.IfcType = "Wall"

    def setProperties(self, obj):
        """Give the wall its wall specific properties, such as its alignment."""

        lp = obj.PropertiesList
        if not "Length" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "Length",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The length of this wall. Read-only if this wall is not based on an unconstrained sketch with a single edge, or on a Draft Wire with a single edge. Refer to wiki for details how length is deduced.",
                ),
                locked=True,
            )
        if not "Width" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "Width",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The width of this wall. Not used if this wall is based on a face. Disabled and ignored if Base object (ArchSketch) provides the information.",
                ),
                locked=True,
            )
        if not "OverrideWidth" in lp:
            obj.addProperty(
                "App::PropertyFloatList",
                "OverrideWidth",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This overrides Width attribute to set width of each segment of wall.  Disabled and ignored if Base object (ArchSketch) provides Widths information, with getWidths() method  (If a value is zero, the value of 'Width' will be followed).  [ENHANCEMENT by ArchSketch] GUI 'Edit Wall Segment Width' Tool is provided in external SketchArch Add-on to let users to set the values interactively.  'Toponaming-Tolerant' if ArchSketch is used in Base (and SketchArch Add-on is installed).  Warning : Not 'Toponaming-Tolerant' if just Sketch is used.",
                ),
                locked=True,
            )
        if not "OverrideAlign" in lp:
            obj.addProperty(
                "App::PropertyStringList",
                "OverrideAlign",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This overrides Align attribute to set align of each segment of wall.  Disabled and ignored if Base object (ArchSketch) provides Aligns information, with getAligns() method  (If a value is not 'Left, Right, Center', the value of 'Align' will be followed).  [ENHANCEMENT by ArchSketch] GUI 'Edit Wall Segment Align' Tool is provided in external SketchArch Add-on to let users to set the values interactively.  'Toponaming-Tolerant' if ArchSketch is used in Base (and SketchArch Add-on is installed).  Warning : Not 'Toponaming-Tolerant' if just Sketch is used.",
                ),
                locked=True,
            )
        if not "OverrideOffset" in lp:
            obj.addProperty(
                "App::PropertyFloatList",
                "OverrideOffset",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This overrides Offset attribute to set offset of each segment of wall.  Disabled and ignored if Base object (ArchSketch) provides Offsets information, with getOffsets() method  (If a value is zero, the value of 'Offset' will be followed).  [ENHANCED by ArchSketch] GUI 'Edit Wall Segment Offset' Tool is provided in external Add-on ('SketchArch') to let users to select the edges interactively.  'Toponaming-Tolerant' if ArchSketch is used in Base (and SketchArch Add-on is installed).  Warning : Not 'Toponaming-Tolerant' if just Sketch is used. Property is ignored if Base ArchSketch provided the selected edges. ",
                ),
                locked=True,
            )
        if not "Height" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "Height",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The height of this wall. Keep 0 for automatic. Not used if this wall is based on a solid",
                ),
                locked=True,
            )
        if not "Area" in lp:
            obj.addProperty(
                "App::PropertyArea",
                "Area",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The area of this wall as a simple Height * Length calculation"
                ),
                locked=True,
            )
            obj.setEditorMode("Area", 1)
        if not "Align" in lp:
            obj.addProperty(
                "App::PropertyEnumeration",
                "Align",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The alignment of this wall on its base object, if applicable. Disabled and ignored if Base object (ArchSketch) provides the information.",
                ),
                locked=True,
            )
            obj.Align = ["Left", "Right", "Center"]
        if not "AlignLayer" in lp:
            obj.addProperty(
                "App::PropertyEnumeration",
                "AlignLayer",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Reference layer: select a specific material layer to pin to the "
                    "baseline. When set, overrides the global Align for layer positioning. "
                    "Requires a multi-material. Disabled if Base object (ArchSketch) "
                    "provides the information.",
                ),
                locked=True,
            )
            obj.AlignLayer = ["None (use Align)"]
        if not "AlignLayerMode" in lp:
            obj.addProperty(
                "App::PropertyEnumeration",
                "AlignLayerMode",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Which face of the Reference Layer to pin to the baseline: "
                    "Layer Left (nearest), Layer Center, or Layer Right (farthest). "
                    "Only active when Align Layer is set to a layer name.",
                ),
                locked=True,
            )
            obj.AlignLayerMode = ["Layer Left", "Layer Center", "Layer Right"]
        if not "AlignOffset" in lp:
            obj.addProperty(
                "App::PropertyDistance",
                "AlignOffset",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Additional lateral offset applied after all alignment calculations. "
                    "Shifts the entire wall stack from the resolved position. "
                    "Independent from the per-edge baseline Offset property.",
                ),
                locked=True,
            )
        if not "Normal" in lp:
            obj.addProperty(
                "App::PropertyVector",
                "Normal",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The normal extrusion direction of this object (keep (0,0,0) for automatic normal)",
                ),
                locked=True,
            )
        if not "Face" in lp:
            obj.addProperty(
                "App::PropertyInteger",
                "Face",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The face number of the base object used to build this wall"
                ),
                locked=True,
            )
        if not "Offset" in lp:
            obj.addProperty(
                "App::PropertyDistance",
                "Offset",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The offset between this wall and its baseline (only for left and right alignments). Disabled and ignored if Base object (ArchSketch) provides the information.",
                ),
                locked=True,
            )
        if not "MakeBlocks" in lp:
            obj.addProperty(
                "App::PropertyBool",
                "MakeBlocks",
                "Blocks",
                QT_TRANSLATE_NOOP("App::Property", "Enable this to make the wall generate blocks"),
                locked=True,
            )
        if not "BlockLength" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "BlockLength",
                "Blocks",
                QT_TRANSLATE_NOOP("App::Property", "The length of each block"),
                locked=True,
            )
        if not "BlockHeight" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "BlockHeight",
                "Blocks",
                QT_TRANSLATE_NOOP("App::Property", "The height of each block"),
                locked=True,
            )
        if not "OffsetFirst" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "OffsetFirst",
                "Blocks",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The horizontal offset of the first line of blocks"
                ),
                locked=True,
            )
        if not "OffsetSecond" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "OffsetSecond",
                "Blocks",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The horizontal offset of the second line of blocks"
                ),
                locked=True,
            )
        if not "Joint" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "Joint",
                "Blocks",
                QT_TRANSLATE_NOOP("App::Property", "The size of the joints between each block"),
                locked=True,
            )
        if not "CountEntire" in lp:
            obj.addProperty(
                "App::PropertyInteger",
                "CountEntire",
                "Blocks",
                QT_TRANSLATE_NOOP("App::Property", "The number of entire blocks"),
                locked=True,
            )
            obj.setEditorMode("CountEntire", 1)
        if not "CountBroken" in lp:
            obj.addProperty(
                "App::PropertyInteger",
                "CountBroken",
                "Blocks",
                QT_TRANSLATE_NOOP("App::Property", "The number of broken blocks"),
                locked=True,
            )
            obj.setEditorMode("CountBroken", 1)
        if not "ArchSketchData" in lp:
            obj.addProperty(
                "App::PropertyBool",
                "ArchSketchData",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Use Base ArchSketch (if used) data (e.g. widths, aligns, offsets) instead of Wall's properties",
                ),
                locked=True,
            )
            obj.ArchSketchData = True
        if not "ArchSketchEdges" in lp:
            obj.addProperty(
                "App::PropertyStringList",
                "ArchSketchEdges",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Selected edges (or group of edges) of the base Sketch/ArchSketch, to use in creating the shape of this Arch Wall (instead of using all the Base Sketch/ArchSketch's edges by default).  Input are index numbers of edges or groups.  Disabled and ignored if Base object (ArchSketch) provides selected edges (as Wall Axis) information, with getWallBaseShapeEdgesInfo() method.  [ENHANCEMENT by ArchSketch] GUI 'Edit Wall Segment' Tool is provided in external SketchArch Add-on to let users to (de)select the edges interactively.  'Toponaming-Tolerant' if ArchSketch is used in Base (and SketchArch Add-on is installed).  Warning : Not 'Toponaming-Tolerant' if just Sketch is used.",
                ),
                locked=True,
            )
        if not hasattr(obj, "ArchSketchPropertySet"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "ArchSketchPropertySet",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Select User Defined PropertySet to use in creating variant shape, layers of the Arch Wall with same ArchSketch ",
                ),
                locked=True,
            )
            obj.ArchSketchPropertySet = ["Default"]
        if not hasattr(self, "ArchSkPropSetPickedUuid"):
            self.ArchSkPropSetPickedUuid = ""
        if not hasattr(self, "ArchSkPropSetListPrev"):
            self.ArchSkPropSetListPrev = []
        self.connectEdges = []

    def get_layers(self, obj):
        """Returns a list of layers"""
        layers = []
        width = self.get_width(obj, widths=False)
        if hasattr(obj, "Material"):
            if obj.Material:
                if hasattr(obj.Material, "Materials"):
                    thicknesses = [abs(t) for t in obj.Material.Thicknesses]
                    restwidth = width - sum(thicknesses)
                    varwidth = 0
                    if restwidth > 0:
                        varwidth = [t for t in thicknesses if t == 0]
                        if varwidth:
                            varwidth = restwidth / len(varwidth)
                    for t in obj.Material.Thicknesses:
                        if t:
                            layers.append(t)
                        elif varwidth:
                            layers.append(varwidth)
        return layers

    def _update_align_layer_enum(self, obj):
        """Rebuild the AlignLayer dropdown from current multi-material layer names."""
        if getattr(self, "_updating_align_layer", False):
            return
        if not hasattr(obj, "AlignLayer"):
            return
        self._updating_align_layer = True
        try:
            entries = ["None (use Align)"]
            layers = self.get_layers(obj)
            if layers and hasattr(obj, "Material") and obj.Material:
                if hasattr(obj.Material, "Materials"):
                    for i, mat in enumerate(obj.Material.Materials):
                        if (
                            hasattr(obj.Material, "Names")
                            and i < len(obj.Material.Names)
                            and obj.Material.Names[i]
                        ):
                            name = obj.Material.Names[i]
                        elif hasattr(mat, "Label"):
                            name = mat.Label
                        else:
                            name = "Layer {}".format(i + 1)
                        entries.append(name)
            try:
                current = obj.AlignLayer
            except Exception:
                current = entries[0]
            if current not in entries:
                current = entries[0]
            try:
                existing = list(obj.getEnumerationsOfProperty("AlignLayer"))
            except Exception:
                existing = []
            if existing != entries:
                obj.AlignLayer = entries
            obj.AlignLayer = current
        finally:
            self._updating_align_layer = False

    def _compute_lateral_offset(self, obj, layers):
        """Compute the initial layeroffset for the layer loop in getExtrusionData.

        Returns 0.0 when no AlignLayer or AlignOffset is set, so existing
        behavior is completely unchanged for standard walls.
        """
        align_offset_val = 0.0
        align_offset_prop = getattr(obj, "AlignOffset", None)
        if align_offset_prop is not None:
            align_offset_val = (
                align_offset_prop.Value
                if hasattr(align_offset_prop, "Value")
                else 0.0
            )
        align_layer = getattr(obj, "AlignLayer", "None (use Align)")
        if not align_layer or align_layer == "None (use Align)":
            if getattr(obj, "Align", "Left") == "Center":
                return align_offset_val
            return align_offset_val
        layer_idx = None
        if hasattr(obj, "Material") and obj.Material:
            if hasattr(obj.Material, "Materials"):
                for i, mat in enumerate(obj.Material.Materials):
                    if (
                        hasattr(obj.Material, "Names")
                        and i < len(obj.Material.Names)
                        and obj.Material.Names[i]
                    ):
                        name = obj.Material.Names[i]
                    elif hasattr(mat, "Label"):
                        name = mat.Label
                    else:
                        name = "Layer {}".format(i + 1)
                    if name == align_layer:
                        layer_idx = i
                        break
        if layer_idx is None:
            return align_offset_val
        layer_mode = getattr(obj, "AlignLayerMode", "Layer Left")
        cum = sum(abs(l) for l in layers[:layer_idx])
        if layer_mode in ("Layer Left", "Bottom"):
            lateral = -cum
        elif layer_mode in ("Layer Center", "Center"):
            lateral = -(cum + abs(layers[layer_idx]) / 2.0)
        else:
            lateral = -(cum + abs(layers[layer_idx]))

        if getattr(obj, "Align", "Left") == "Center":
            total = sum(abs(l) for l in layers)
            lateral += total / 2.0

        return lateral + align_offset_val

    @staticmethod
    def _precompute_wire_slices(basewires, widths, aligns, offsets,
                                default_width, default_align, default_offset):
        """Pre-slice per-edge property lists into one dict per wire.

        Reads widths/aligns/offsets once, non-destructively.
        Each wire's dict is reused for every layer — no list mutation.

        Returns list of dict, one per wire:
            edgeNum, first_edge, widths[], aligns[], offsets[]
        """
        import Part
        slices = []
        pos = 0

        for wire in basewires:
            if isinstance(wire, Part.Wire):
                edgeNum = len(wire.Edges)
                first_edge = wire.Edges[0]
            else:
                edgeNum = len(wire)
                first_edge = wire[0]

            w_sl, a_sl, o_sl = [], [], []
            for n in range(edgeNum):
                idx = pos + n
                try:
                    v = widths[idx]
                    w_sl.append(v if v else default_width)
                except IndexError:
                    w_sl.append(default_width)
                try:
                    v = aligns[idx]
                    a_sl.append(v if v in ("Left", "Right", "Center") else default_align)
                except IndexError:
                    a_sl.append(default_align)
                try:
                    v = offsets[idx]
                    o_sl.append(v if v else default_offset)
                except IndexError:
                    o_sl.append(default_offset)

            slices.append(dict(edgeNum=edgeNum, first_edge=first_edge,
                               widths=w_sl, aligns=a_sl, offsets=o_sl))
            pos += edgeNum

        return slices

    def _generate_face_for_wire_layer(self, wire, layer_width, layeroffset,
                                      slc, normal, cur_align, total_width):
        """Generate one 2D face for a (wire, layer) pair.

        Parameters
        ----------
        wire        : Part.Wire or list[Part.Edge]
        layer_width : float   — absolute thickness of this layer
        layeroffset : float   — cumulative lateral offset to start of this layer
        slc         : dict    — pre-sliced wire properties
        normal      : Vector  — sketch/working-plane normal
        cur_align   : str     — "Left", "Right", or "Center"
        total_width : float   — sum of all layer thicknesses (for Center)

        Returns (face, connect_edges_list) or (None, [])
        """
        import Part, DraftGeomUtils, DraftVecUtils

        e = slc['first_edge']
        edgeNum = slc['edgeNum']

        if isinstance(e.Curve, (Part.Circle, Part.Ellipse)):
            dvec = e.Vertexes[0].Point.sub(e.Curve.Center)
        else:
            dvec = DraftGeomUtils.vec(e).cross(normal)

        if DraftVecUtils.isNull(dvec):
            return None, []
        dvec.normalize()

        curWidth = [abs(layer_width)] * edgeNum
        eff_offsets = [x + layeroffset for x in slc['offsets']]

        if cur_align == "Left":
            dvec.multiply(layer_width)
            wNe2 = DraftGeomUtils.offsetWire(
                wire, dvec, bind=False, occ=False,
                widthList=curWidth, offsetMode=None,
                alignList=slc['aligns'], normal=normal,
                basewireOffset=eff_offsets, wireNedge=True)
            wNe1 = DraftGeomUtils.offsetWire(
                wire, dvec, bind=False, occ=False,
                widthList=curWidth, offsetMode="BasewireMode",
                alignList=slc['aligns'], normal=normal,
                basewireOffset=eff_offsets, wireNedge=True)

        elif cur_align == "Right":
            dvec = dvec.negative()
            dvec.multiply(layer_width)
            wNe2 = DraftGeomUtils.offsetWire(
                wire, dvec, bind=False, occ=False,
                widthList=curWidth, offsetMode=None,
                alignList=slc['aligns'], normal=normal,
                basewireOffset=eff_offsets, wireNedge=True)
            wNe1 = DraftGeomUtils.offsetWire(
                wire, dvec, bind=False, occ=False,
                widthList=curWidth, offsetMode="BasewireMode",
                alignList=slc['aligns'], normal=normal,
                basewireOffset=eff_offsets, wireNedge=True)

        elif cur_align == "Center":
            dvec = dvec.negative()
            off = -(total_width / 2 - layeroffset)
            c_aligns = ["Right"] * edgeNum
            c_offsets = [off] * edgeNum
            wNe1 = DraftGeomUtils.offsetWire(
                wire, dvec, widthList=curWidth, offsetMode="BasewireMode",
                alignList=c_aligns, normal=normal,
                basewireOffset=c_offsets, wireNedge=True)
            wNe2 = DraftGeomUtils.offsetWire(
                wire, dvec, widthList=curWidth, offsetMode=None,
                alignList=c_aligns, normal=normal,
                basewireOffset=c_offsets, wireNedge=True)
        else:
            return None, []

        face = DraftGeomUtils.bind(wNe1[0], wNe2[0], per_segment=True)
        connect_edges = list(wNe2[2]) + list(wNe1[2])
        return face, connect_edges

    def dumps(self):
        dump = super().dumps()
        if not isinstance(dump, tuple):
            dump = (dump,)
        dump = dump + (self.ArchSkPropSetPickedUuid, self.ArchSkPropSetListPrev)
        return dump

    def loads(self, state):
        self.Type = "Wall"
        if state == None:
            return
        elif state[0] == "W":
            return
        elif state[0] == "Wall":
            self.ArchSkPropSetPickedUuid = state[1]
            self.ArchSkPropSetListPrev = state[2]
        elif state[0] != "Wall":
            self.ArchSkPropSetPickedUuid = state[0]
            self.ArchSkPropSetListPrev = state[1]

    def onDocumentRestored(self, obj):
        """Method run when the document is restored."""

        import DraftGeomUtils
        from draftutils.messages import _log

        ArchComponent.Component.onDocumentRestored(self, obj)
        self.setProperties(obj)
        self._update_align_layer_enum(obj)

        if (
            FreeCAD.ActiveDocument.getProgramVersion() < "0.22"
            and obj.Normal == Vector(0, 0, 0)
            and hasattr(obj.Base, "Shape")
            and not obj.Base.Shape.Solids
            and obj.Face == 0
            and not obj.Base.isDerivedFrom("Sketcher::SketchObject")
            and DraftGeomUtils.get_shape_normal(obj.Base.Shape) != Vector(0, 0, 1)
        ):
            obj.Normal = Vector(0, 0, 1)
            _log(
                "v1.0, "
                + obj.Name
                + ", "
                + "changed 'Normal' to [0, 0, 1] to preserve extrusion direction"
            )

        if (
            hasattr(obj, "ArchSketchData")
            and obj.ArchSketchData
            and Draft.getType(obj.Base) == "ArchSketch"
        ):
            if hasattr(obj, "Width"):
                obj.setEditorMode("Width", ["ReadOnly"])
            if hasattr(obj, "Align"):
                obj.setEditorMode("Align", ["ReadOnly"])
            if hasattr(obj, "Offset"):
                obj.setEditorMode("Offset", ["ReadOnly"])
            if hasattr(obj, "OverrideWidth"):
                obj.setEditorMode("OverrideWidth", ["ReadOnly"])
            if hasattr(obj, "OverrideAlign"):
                obj.setEditorMode("OverrideAlign", ["ReadOnly"])
            if hasattr(obj, "OverrideOffset"):
                obj.setEditorMode("OverrideOffset", ["ReadOnly"])
            if hasattr(obj, "ArchSketchEdges"):
                obj.setEditorMode("ArchSketchEdges", ["ReadOnly"])
            if hasattr(obj, "ArchSketchPropertySet"):
                obj.setEditorMode("ArchSketchPropertySet", 0)
            if hasattr(obj, "AlignLayer"):
                obj.setEditorMode("AlignLayer", ["ReadOnly"])
            if hasattr(obj, "AlignLayerMode"):
                obj.setEditorMode("AlignLayerMode", ["ReadOnly"])
            if hasattr(obj, "AlignOffset"):
                obj.setEditorMode("AlignOffset", ["ReadOnly"])
        else:
            if hasattr(obj, "Width"):
                obj.setEditorMode("Width", 0)
            if hasattr(obj, "Align"):
                obj.setEditorMode("Align", 0)
            if hasattr(obj, "Offset"):
                obj.setEditorMode("Offset", 0)
            if hasattr(obj, "OverrideWidth"):
                obj.setEditorMode("OverrideWidth", 0)
            if hasattr(obj, "OverrideAlign"):
                obj.setEditorMode("OverrideAlign", 0)
            if hasattr(obj, "OverrideOffset"):
                obj.setEditorMode("OverrideOffset", 0)
            if hasattr(obj, "ArchSketchEdges"):
                obj.setEditorMode("ArchSketchEdges", 0)
            if hasattr(obj, "ArchSketchPropertySet"):
                obj.setEditorMode("ArchSketchPropertySet", ["ReadOnly"])
            if hasattr(obj, "AlignLayer"):
                obj.setEditorMode("AlignLayer", 0)
            if hasattr(obj, "AlignOffset"):
                obj.setEditorMode("AlignOffset", 0)
            if hasattr(obj, "AlignLayerMode") and hasattr(obj, "AlignLayer"):
                align_layer = getattr(obj, "AlignLayer", "None (use Align)")
                layer_active = bool(align_layer and align_layer != "None (use Align)")
                obj.setEditorMode("AlignLayerMode", 0 if layer_active else 2)

    def execute(self, obj):
        """Method run when the object is recomputed.

        Extrude the wall from the Base shape if possible. Processe additions
        and subtractions. Assign the resulting shape as the shape of the wall.

        Add blocks if the MakeBlocks property is assigned. If the Base shape is
        a mesh, just copy the mesh.
        """

        if self.clone(obj):
            return

        import Part
        import DraftGeomUtils

        base = None
        pl = obj.Placement

        propSetPickedUuidPrev = self.ArchSkPropSetPickedUuid
        propSetListPrev = self.ArchSkPropSetListPrev
        propSetSelectedNamePrev = obj.ArchSketchPropertySet
        propSetSelectedNameCur = None
        propSetListCur = None
        if Draft.getType(obj.Base) == "ArchSketch":
            baseProxy = obj.Base.Proxy
            if hasattr(baseProxy, "getPropertySet"):
                propSetListCur = baseProxy.getPropertySet(obj.Base)
                propSetSelectedNameCur = baseProxy.getPropertySet(
                    obj.Base, propSetUuid=propSetPickedUuidPrev
                )
        if propSetSelectedNameCur:
            if propSetListPrev != propSetListCur:
                obj.ArchSketchPropertySet = propSetListCur
                obj.ArchSketchPropertySet = propSetSelectedNameCur
                self.ArchSkPropSetListPrev = propSetListCur
            elif propSetSelectedNamePrev != propSetSelectedNameCur:
                obj.ArchSketchPropertySet = propSetSelectedNameCur
        else:
            if propSetListCur:
                if propSetListPrev != propSetListCur:
                    obj.ArchSketchPropertySet = propSetListCur
                    obj.ArchSketchPropertySet = "Default"

        self._update_align_layer_enum(obj)
        extdata = self.getExtrusionData(obj)
        if extdata:
            base_faces = extdata[0]
            extv = extdata[2].Rotation.multVec(extdata[1])

            if not isinstance(base_faces, list):
                base_faces = [base_faces]

            should_fuse_solids = False
            if obj.Base and obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                is_multi_layer = (
                    hasattr(obj, "Material")
                    and obj.Material
                    and hasattr(obj.Material, "Materials")
                    and obj.Material.Materials
                )
                if not is_multi_layer:
                    should_fuse_solids = True

            # Generate and fuse per-layer solids (incorporates paullee v2)
            self.solidsNumLst = []
            layer_solids = []
            for face in base_faces:
                face.Placement = extdata[2].multiply(face.Placement)
                extruded = face.extrude(extv)
                sub_solids = extruded.Solids if hasattr(extruded, 'Solids') else [extruded]
                fused = None
                for s in sub_solids:
                    try:
                        fused = fused.fuse(s) if fused else s
                    except Exception:
                        fused = Part.makeCompound([fused, s]) if fused else s
                if fused is None:
                    fused = extruded
                layer_solids.append(fused)
                self.solidsNumLst.append(len(fused.Solids) if fused.Solids else 1)

            base = Part.makeCompound(layer_solids)
        if obj.Base:
            if hasattr(obj.Base, "Shape"):
                if obj.Base.Shape.isNull():
                    return
                if not obj.Base.Shape.isValid():
                    if not obj.Base.Shape.Solids:
                        return
                elif obj.Base.Shape.Solids:
                    base = Part.Shape(obj.Base.Shape)

            elif obj.Base.isDerivedFrom("Mesh::Feature"):
                if obj.Base.Mesh.isSolid():
                    if obj.Base.Mesh.countComponents() == 1:
                        sh = ArchCommands.getShapeFromMesh(obj.Base.Mesh)
                        if sh.isClosed() and sh.isValid() and sh.Solids and (not sh.isNull()):
                            base = sh
                        else:
                            FreeCAD.Console.PrintWarning(
                                translate("Arch", "This mesh is an invalid solid") + "\n"
                            )
                            obj.Base.ViewObject.show()
        if hasattr(obj, "MakeBlocks") and hasattr(self, "basewires"):
            if obj.MakeBlocks and self.basewires and extdata and obj.Width and obj.Height:
                if base_faces:
                    blocks = self._make_blocks(obj, base_faces[0], extv)
                    if blocks is not None:
                        base = blocks
        if not base:
            base = Part.Shape()
        base = self.processSubShapes(obj, base, pl)
        self.applyShape(obj, base, pl)
        self.apply_material_hatches(obj)

        if base.isNull() and (self.noWidths or self.noHeight):
            FreeCAD.Console.PrintWarning(
                translate(
                    "Arch",
                    f"Cannot create or update {obj.Label} as its length, height or width is zero, and there are no solids in its additions",
                )
                + "\n"
            )

        if hasattr(obj, "MakeBlocks"):
            if obj.MakeBlocks:
                fvol = obj.BlockLength.Value * obj.BlockHeight.Value * obj.Width.Value
                if fvol:
                    ents = [s for s in base.Solids if abs(s.Volume - fvol) < 1]
                    obj.CountEntire = len(ents)
                    obj.CountBroken = len(base.Solids) - len(ents)
                else:
                    obj.CountEntire = 0
                    obj.CountBroken = 0

        # Length = total length of all baseline wires in the sketch.
        # This is always correct because basewires are the original sketch
        # edges, unchanged by layer processing.
        if hasattr(self, 'basewires') and self.basewires:
            total_len = 0.0
            for wire in self.basewires:
                try:
                    if isinstance(wire, Part.Wire):
                        total_len += wire.Length
                    else:
                        for e in wire:
                            total_len += e.Length
                except Exception:
                    pass
            if total_len > 0 and obj.Length.Value != total_len:
                obj.Length = total_len
                self.oldLength = None

        obj.Area = obj.Length.Value * obj.Height.Value

    def onBeforeChange(self, obj, prop):
        """Method called before the object has a property changed.

        Specifically, this method is called before the value changes.

        If "Length" has changed, record the old length so that .onChanged() can
        be sure that the base needs to be changed.

        Also call ArchComponent.Component.onBeforeChange().

        Parameters
        ----------
        prop: string
            The name of the property that has changed.
        """

        if prop == "Length":
            self.oldLength = obj.Length.Value
        ArchComponent.Component.onBeforeChange(self, obj, prop)

    def onChanged(self, obj, prop):
        """Method called when the object has a property changed."""

        if prop == "Length":
            if (
                obj.Base
                and obj.Length.Value
                and hasattr(self, "oldLength")
                and (self.oldLength is not None)
                and (self.oldLength != obj.Length.Value)
            ):
                if hasattr(obj.Base, "Shape"):
                    if len(obj.Base.Shape.Edges) == 1:
                        import DraftGeomUtils

                        e = obj.Base.Shape.Edges[0]
                        if DraftGeomUtils.geomType(e) == "Line":
                            if e.Length != obj.Length.Value:
                                v = e.Vertexes[-1].Point.sub(e.Vertexes[0].Point)
                                v.normalize()
                                v.multiply(obj.Length.Value)
                                p2 = e.Vertexes[0].Point.add(v)
                                if Draft.getType(obj.Base) == "Wire":
                                    obj.Base.End = p2
                                elif Draft.getType(obj.Base) in [
                                    "Sketcher::SketchObject",
                                    "ArchSketch",
                                ]:
                                    obj.Base.moveGeometry(
                                        0, 2, obj.Base.Placement.inverse().multVec(p2)
                                    )
                                else:
                                    FreeCAD.Console.PrintError(
                                        translate(
                                            "Arch",
                                            "Error: Unable to modify the base object of this wall",
                                        )
                                        + "\n"
                                    )

        if prop in ["Material", "AlignLayer"]:
            self._update_align_layer_enum(obj)

        if prop == "ArchSketchPropertySet" and Draft.getType(obj.Base) == "ArchSketch":
            baseProxy = obj.Base.Proxy
            if hasattr(baseProxy, "getPropertySet"):
                uuid = baseProxy.getPropertySet(obj, propSetName=obj.ArchSketchPropertySet)
                self.ArchSkPropSetPickedUuid = uuid

        if (
            hasattr(obj, "ArchSketchData")
            and obj.ArchSketchData
            and Draft.getType(obj.Base) == "ArchSketch"
        ):
            if hasattr(obj, "Width"):
                obj.setEditorMode("Width", ["ReadOnly"])
            if hasattr(obj, "Align"):
                obj.setEditorMode("Align", ["ReadOnly"])
            if hasattr(obj, "Offset"):
                obj.setEditorMode("Offset", ["ReadOnly"])
            if hasattr(obj, "OverrideWidth"):
                obj.setEditorMode("OverrideWidth", ["ReadOnly"])
            if hasattr(obj, "OverrideAlign"):
                obj.setEditorMode("OverrideAlign", ["ReadOnly"])
            if hasattr(obj, "OverrideOffset"):
                obj.setEditorMode("OverrideOffset", ["ReadOnly"])
            if hasattr(obj, "ArchSketchEdges"):
                obj.setEditorMode("ArchSketchEdges", ["ReadOnly"])
            if hasattr(obj, "ArchSketchPropertySet"):
                obj.setEditorMode("ArchSketchPropertySet", 0)
            if hasattr(obj, "AlignLayer"):
                obj.setEditorMode("AlignLayer", ["ReadOnly"])
            if hasattr(obj, "AlignLayerMode"):
                obj.setEditorMode("AlignLayerMode", ["ReadOnly"])
            if hasattr(obj, "AlignOffset"):
                obj.setEditorMode("AlignOffset", ["ReadOnly"])
        else:
            if hasattr(obj, "Width"):
                if hasattr(self, "multimaterialsWidth") and self.multimaterialsWidth:
                    obj.setEditorMode("Width", ["ReadOnly"])
                else:
                    obj.setEditorMode("Width", 0)
            if hasattr(obj, "Align"):
                obj.setEditorMode("Align", 0)
            if hasattr(obj, "Offset"):
                obj.setEditorMode("Offset", 0)
            if hasattr(obj, "OverrideWidth"):
                obj.setEditorMode("OverrideWidth", 0)
            if hasattr(obj, "OverrideAlign"):
                obj.setEditorMode("OverrideAlign", 0)
            if hasattr(obj, "OverrideOffset"):
                obj.setEditorMode("OverrideOffset", 0)
            if hasattr(obj, "ArchSketchEdges"):
                obj.setEditorMode("ArchSketchEdges", 0)
            if hasattr(obj, "ArchSketchPropertySet"):
                obj.setEditorMode("ArchSketchPropertySet", ["ReadOnly"])
            if hasattr(obj, "AlignLayer"):
                obj.setEditorMode("AlignLayer", 0)
            if hasattr(obj, "AlignOffset"):
                obj.setEditorMode("AlignOffset", 0)
            if hasattr(obj, "AlignLayerMode") and hasattr(obj, "AlignLayer"):
                align_layer = getattr(obj, "AlignLayer", "None (use Align)")
                layer_active = bool(align_layer and align_layer != "None (use Align)")
                obj.setEditorMode("AlignLayerMode", 0 if layer_active else 2)

        self.hideSubobjects(obj, prop)
        ArchComponent.Component.onChanged(self, obj, prop)

    def getFootprint(self, obj):
        """Get the faces that make up the base/foot of the wall.

        Returns
        -------
        list of <Part.Face>
            The faces that make up the foot of the wall.
        """

        faces = []
        if obj.Shape:
            for f in obj.Shape.Faces:
                if f.normalAt(0, 0).getAngle(FreeCAD.Vector(0, 0, -1)) < 0.01:
                    if abs(abs(f.CenterOfMass.z) - abs(obj.Shape.BoundBox.ZMin)) < 0.001:
                        faces.append(f)
        return faces

    def getExtrusionData(self, obj):
        """Get data needed to extrude the wall from a base object.

        take the Base object, and find a base face to extrude
        out, a vector to define the extrusion direction and distance.

        Rebase the base face to the (0,0,0) origin.

        Return the base face, rebased, with the extrusion vector, and the
        <Base.Placement> needed to return the face back to its original
        position.

        Returns
        -------
        tuple of (<Part.Face>, <Base.Vector>, <Base.Placement>)
            Tuple containing the base face, the vector for extrusion, and the
            placement needed to move the face back from the (0,0,0) origin.
        """

        import Part
        import DraftGeomUtils
        import ArchSketchObject

        propSetUuid = self.ArchSkPropSetPickedUuid

        data = ArchComponent.Component.getExtrusionData(self, obj)
        if data:
            if not isinstance(data[0], list):
                return data
        length = obj.Length.Value

        self.noWidths = False
        self.noHeight = False
        width = 0
        widths = []
        if (
            hasattr(obj, "ArchSketchData")
            and obj.ArchSketchData
            and Draft.getType(obj.Base) == "ArchSketch"
        ):
            if hasattr(obj.Base, "Proxy"):
                if hasattr(obj.Base.Proxy, "getWidths"):
                    widths = obj.Base.Proxy.getWidths(obj.Base, propSetUuid=propSetUuid)
        if not widths:
            if obj.OverrideWidth:
                if obj.Base and obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                    try:
                        import ArchSketchObject
                    except Exception:
                        print("ArchSketchObject add-on module is not installed yet")
                    try:
                        widths = ArchSketchObject.sortSketchWidth(
                            obj.Base, obj.OverrideWidth, obj.ArchSketchEdges
                        )
                    except Exception:
                        widths = obj.OverrideWidth
                else:
                    widths = obj.OverrideWidth
            elif obj.Width:
                widths = [obj.Width.Value]
            else:
                self.noWidths = True

        if obj.Width.Value:
            width = obj.Width.Value
        else:
            width = 200

        height = obj.Height.Value
        if not height:
            height = self.getParentHeight(obj)
        if not height:
            self.noHeight = True

        if self.noWidths or self.noHeight:
            return None

        aligns = []
        if (
            hasattr(obj, "ArchSketchData")
            and obj.ArchSketchData
            and Draft.getType(obj.Base) == "ArchSketch"
        ):
            if hasattr(obj.Base, "Proxy"):
                if hasattr(obj.Base.Proxy, "getAligns"):
                    aligns = obj.Base.Proxy.getAligns(obj.Base, propSetUuid=propSetUuid)
        if not aligns:
            if obj.OverrideAlign:
                if obj.Base and obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                    try:
                        import ArchSketchObject
                    except Exception:
                        print("ArchSketchObject add-on module is not installed yet")
                    try:
                        aligns = ArchSketchObject.sortSketchAlign(
                            obj.Base, obj.OverrideAlign, obj.ArchSketchEdges
                        )
                    except Exception:
                        aligns = obj.OverrideAlign
                else:
                    aligns = obj.OverrideAlign
            else:
                aligns = [obj.Align]

        align = obj.Align

        offsets = []
        if (
            hasattr(obj, "ArchSketchData")
            and obj.ArchSketchData
            and Draft.getType(obj.Base) == "ArchSketch"
        ):
            if hasattr(obj.Base, "Proxy"):
                if hasattr(obj.Base.Proxy, "getOffsets"):
                    offsets = obj.Base.Proxy.getOffsets(obj.Base, propSetUuid=propSetUuid)
        if not offsets:
            if obj.OverrideOffset:
                if obj.Base and obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                    if hasattr(ArchSketchObject, "sortSketchOffset"):
                        offsets = ArchSketchObject.sortSketchOffset(
                            obj.Base, obj.OverrideOffset, obj.ArchSketchEdges
                        )
                    else:
                        offsets = obj.OverrideOffset
                else:
                    offsets = obj.OverrideOffset
            elif obj.Offset:
                offsets = [obj.Offset.Value]

        offset = obj.Offset.Value

        if obj.Normal == Vector(0, 0, 0):
            if obj.Base and hasattr(obj.Base, "Shape"):
                normal = DraftGeomUtils.get_shape_normal(obj.Base.Shape)
                if normal is None:
                    normal = Vector(0, 0, 1)
            else:
                normal = Vector(0, 0, 1)
        else:
            normal = Vector(obj.Normal)
        base = None
        placement = None
        self.basewires = None

        self.multimaterialsWidth = False
        layers = self.get_layers(obj)
        if layers:
            total = sum(layers)
            if obj.Width.Value != total:
                obj.Width = total
            if not (0 in obj.Material.Thicknesses):
                self.multimaterialsWidth = True
        if self.multimaterialsWidth:
            obj.setEditorMode("Width", ["ReadOnly"])
        else:
            obj.setEditorMode("Width", 0)
        if self.ensureBase(obj):
            if hasattr(obj.Base, "Shape"):
                if obj.Base.Shape:
                    if obj.Base.Shape.Solids:
                        return None

                    elif obj.Face > 0:
                        if len(obj.Base.Shape.Faces) >= obj.Face:
                            face = obj.Base.Shape.Faces[obj.Face - 1]
                            if obj.Normal != Vector(0, 0, 0):
                                normal = face.normalAt(0, 0)
                            if normal.getAngle(Vector(0, 0, 1)) > math.pi / 4:
                                normal.multiply(width)
                                base = face.extrude(normal)
                                if obj.Align == "Center":
                                    base.translate(normal.negative().multiply(0.5))
                                elif obj.Align == "Right":
                                    base.translate(normal.negative())
                            else:
                                normal.multiply(height)
                                base = face.extrude(normal)
                            base, placement = self.rebase(base)
                            return (base, normal, placement)

                    elif obj.Base.Shape.Faces:
                        if not DraftGeomUtils.isCoplanar(obj.Base.Shape.Faces):
                            return None
                        else:
                            base, placement = self.rebase(obj.Base.Shape)

                    elif (
                        hasattr(obj.Base, "Proxy")
                        and obj.ArchSketchData
                        and hasattr(obj.Base.Proxy, "getWallBaseShapeEdgesInfo")
                    ):
                        wallBaseShapeEdgesInfo = obj.Base.Proxy.getWallBaseShapeEdgesInfo(
                            obj.Base, propSetUuid=propSetUuid
                        )
                        if wallBaseShapeEdgesInfo:
                            self.basewires = wallBaseShapeEdgesInfo.get("wallAxis")

                    elif obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                        self.basewires = []
                        skGeom = obj.Base.GeometryFacadeList
                        skGeomEdges = []
                        skPlacement = obj.Base.Placement
                        for ig, geom in enumerate(skGeom):
                            if (not obj.ArchSketchEdges and not geom.Construction) or str(
                                ig
                            ) in obj.ArchSketchEdges:
                                if isinstance(
                                    geom.Geometry,
                                    (Part.LineSegment, Part.Circle, Part.ArcOfCircle, Part.Ellipse),
                                ):
                                    skGeomEdgesI = geom.Geometry.toShape()
                                    skGeomEdges.append(skGeomEdgesI)
                        for cluster in Part.getSortedClusters(skGeomEdges):
                            clusterTransformed = []
                            for edge in cluster:
                                edge.Placement = edge.Placement.multiply(skPlacement)
                                clusterTransformed.append(edge)
                            self.basewires.append(clusterTransformed)

                        normal = obj.Base.getGlobalPlacement().Rotation.multVec(
                            FreeCAD.Vector(0, 0, 1)
                        )

                    else:
                        self.basewires = []
                        clusters = Part.getSortedClusters(obj.Base.Shape.Edges)
                        self.basewires = clusters

                    if self.basewires:
                        self.connectEdges = []
                        self.wiresNum = len(self.basewires)
                        self.layersNum = len(layers) if layers else 0

                        wire_slices = _Wall._precompute_wire_slices(
                            self.basewires,
                            widths, aligns, offsets,
                            width, align, offset,
                        )

                        cur_align = wire_slices[0]['aligns'][0] if wire_slices else align

                        total_width = sum(abs(l) for l in layers) if layers else width

                        layeroffset = (self._compute_lateral_offset(obj, layers)
                                       if layers else 0)

                        basefaces_per_layer = []

                        if layers:
                            for layer in layers:
                                layer_faces = []
                                if layer < 0:
                                    layeroffset += abs(layer)
                                    continue
                                for wire, slc in zip(self.basewires, wire_slices):
                                    face, ce = self._generate_face_for_wire_layer(
                                        wire, abs(layer), layeroffset,
                                        slc, normal, cur_align, total_width,
                                    )
                                    if face:
                                        layer_faces.append(face)
                                    self.connectEdges.extend(ce)

                                if layer_faces:
                                    if len(layer_faces) == 1:
                                        basefaces_per_layer.append(layer_faces[0])
                                    else:
                                        import Part as _Part
                                        basefaces_per_layer.append(
                                            _Part.Compound(layer_faces)
                                        )
                                layeroffset += abs(layer)
                        else:
                            for wire, slc in zip(self.basewires, wire_slices):
                                face, ce = self._generate_face_for_wire_layer(
                                    wire, width, 0,
                                    slc, normal, cur_align, total_width,
                                )
                                if face:
                                    basefaces_per_layer.append(face)
                                self.connectEdges.extend(ce)

                        if basefaces_per_layer:
                            base, placement = self.rebase(basefaces_per_layer)

        else:
            base, placement = self.build_base_from_scratch(obj)

        if base and placement:
            normal.normalize()
            extrusion = normal.multiply(height)
            if placement.Rotation.Angle > 0:
                extrusion = placement.inverse().Rotation.multVec(extrusion)
            return (base, extrusion, placement)
        return None

    def calc_endpoints(self, obj):
        """Returns the global start and end points of a baseless wall's centerline."""
        p1_local = FreeCAD.Vector(-obj.Length.Value / 2, 0, 0)
        p2_local = FreeCAD.Vector(obj.Length.Value / 2, 0, 0)

        p1_global = obj.Placement.multVec(p1_local)
        p2_global = obj.Placement.multVec(p2_local)

        return [p1_global, p2_global]

    def set_from_endpoints(self, obj, pts):
        """Sets the Length and Placement of a baseless wall from two global points."""
        if len(pts) < 2:
            return

        p1 = pts[0]
        p2 = pts[1]

        new_length = p1.distanceToPoint(p2)
        new_midpoint = (p1 + p2) * 0.5
        new_direction = (p2 - p1).normalize()

        new_rotation = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), new_direction)

        obj.Length = new_length
        obj.Placement.Base = new_midpoint
        obj.Placement.Rotation = new_rotation

    def handleComponentRemoval(self, obj, subobject):
        """
        Overrides the default component removal to implement smart debasing
        when the Base object is being removed.
        """
        import Arch
        from PySide import QtGui

        if hasattr(obj, "Base") and obj.Base == subobject:
            if Arch.is_debasable(obj):
                Arch.debaseWall(obj)
            else:
                if FreeCAD.GuiUp:
                    from PySide import QtGui

                    msg_box = QtGui.QMessageBox()
                    msg_box.setWindowTitle(translate("ArchComponent", "Unsupported Base"))
                    msg_box.setText(
                        translate(
                            "ArchComponent", "The base of this wall is not a single straight line."
                        )
                    )
                    msg_box.setInformativeText(
                        translate(
                            "ArchComponent",
                            "Removing the base of this complex wall will alter its shape and reset its position.\n\n"
                            "Do you want to proceed?",
                        )
                    )
                    msg_box.setStandardButtons(QtGui.QMessageBox.Yes | QtGui.QMessageBox.Cancel)
                    msg_box.setDefaultButton(QtGui.QMessageBox.Cancel)
                    if msg_box.exec_() == QtGui.QMessageBox.Yes:
                        super(_Wall, self).handleComponentRemoval(obj, subobject)
                else:
                    FreeCAD.Console.PrintWarning(
                        f"Skipping removal of complex base for wall '{obj.Label}'. "
                        "This interactive action is not supported in headless mode.\n"
                    )
        else:
            super(_Wall, self).handleComponentRemoval(obj, subobject)

    def get_width(self, obj, widths=True):
        """Returns a width and a list of widths for this wall.
        If widths is False, only the main width is returned"""
        import ArchSketchObject

        if obj.Width.Value:
            width = obj.Width.Value
        else:
            width = 200
        if not widths:
            return width

        lwidths = []
        if (
            hasattr(obj, "ArchSketchData")
            and obj.ArchSketchData
            and Draft.getType(obj.Base) == "ArchSketch"
        ):
            if hasattr(obj.Base, "Proxy"):
                if hasattr(obj.Base.Proxy, "getWidths"):
                    lwidths = obj.Base.Proxy.getWidths(
                        obj.Base, propSetUuid=self.ArchSkPropSetPickedUuid
                    )
        if not lwidths:
            if obj.OverrideWidth:
                if (
                    obj.Base
                    and obj.Base.isDerivedFrom("Sketcher::SketchObject")
                    and hasattr(ArchSketchObject, "sortSketchWidth")
                ):
                    lwidths = ArchSketchObject.sortSketchWidth(
                        obj.Base, obj.OverrideWidth, obj.ArchSketchEdges
                    )
                else:
                    lwidths = obj.OverrideWidth
            elif obj.Width:
                lwidths = [obj.Width.Value]
            else:
                return None
        return width, lwidths

    def _make_blocks(self, obj, base_face, extv):
        """Cut a wall's base face into block-sized pieces and stack them.

        Uses self.basewires (a list of lists of edges representing the wall's centerline) to compute
        vertical cutting planes at block boundaries. The base face is then cut and extruded in
        alternating rows to produce a brick-like pattern.

        Parameters
        ----------
        obj : FreeCAD.DocumentObject
            The wall object. Its block properties (BlockLength, BlockHeight, Joint, OffsetFirst,
            OffsetSecond) control the block layout.
        base_face : Part.Face
            The 2D cross-section face to be cut into blocks and extruded.
        extv : FreeCAD.Vector
            The extrusion vector (direction and magnitude of the wall height).

        Returns
        -------
        Part.Compound or None
            A compound of block solids, or None if blocks could not be computed.
        """
        import Part

        if len(self.basewires) != 1:
            FreeCAD.Console.PrintWarning(
                translate("Arch", "Cannot compute blocks for wall") + obj.Label + "\n"
            )
            return None

        n = FreeCAD.Vector(extv)
        n.normalize()
        cuts1 = []
        cuts2 = []
        if obj.BlockLength.Value:
            for i in range(2):
                if i == 0:
                    offset = obj.OffsetFirst.Value
                else:
                    offset = obj.OffsetSecond.Value

                baseEdges = self.basewires[0]

                for edge in baseEdges:
                    while offset < (edge.Length - obj.Joint.Value):
                        if offset:
                            t = edge.tangentAt(offset)
                            p = t.cross(n)
                            p.multiply(1.1 * obj.Width.Value + obj.Offset.Value)
                            p1 = edge.valueAt(offset).add(p)
                            p2 = edge.valueAt(offset).add(p.negative())
                            sh = Part.LineSegment(p1, p2).toShape()
                            if obj.Joint.Value:
                                sh = sh.extrude(-t.multiply(obj.Joint.Value))
                            sh = sh.extrude(n)
                            if i == 0:
                                cuts1.append(sh)
                            else:
                                cuts2.append(sh)
                        offset += obj.BlockLength.Value + obj.Joint.Value
                    offset -= edge.Length

        blocks = []
        if obj.BlockHeight.Value:
            fsize = obj.BlockHeight.Value + obj.Joint.Value
            bh = obj.BlockHeight.Value
        else:
            fsize = obj.Height.Value
            bh = obj.Height.Value
        bvec = FreeCAD.Vector(n)
        bvec.multiply(bh)
        svec = FreeCAD.Vector(n)
        svec.multiply(fsize)
        if cuts1:
            faces1 = base_face.cut(cuts1).Faces
        else:
            faces1 = base_face.Faces
        blocks1 = Part.makeCompound([f.extrude(bvec) for f in faces1])
        if cuts2:
            faces2 = base_face.cut(cuts2).Faces
        else:
            faces2 = base_face.Faces
        blocks2 = Part.makeCompound([f.extrude(bvec) for f in faces2])
        interval = extv.Length / (fsize)
        entire = int(interval)
        rest = interval - entire
        for i in range(entire):
            if i % 2:
                b = Part.Shape(blocks2)
            else:
                b = Part.Shape(blocks1)
            if i:
                t = FreeCAD.Vector(svec)
                t.multiply(i)
                b.translate(t)
            blocks.append(b)
        if rest:
            rest = extv.Length - (entire * fsize)
            rvec = FreeCAD.Vector(n)
            rvec.multiply(rest)
            if entire % 2:
                b = Part.makeCompound([f.extrude(rvec) for f in faces2])
            else:
                b = Part.makeCompound([f.extrude(rvec) for f in faces1])
            t = FreeCAD.Vector(svec)
            t.multiply(entire)
            b.translate(t)
            blocks.append(b)
        if blocks:
            return Part.makeCompound(blocks)
        return None

    def build_base_from_scratch(self, obj):
        """Generate the 2D profile for extruding a baseless Arch Wall.

        This function creates the rectangular face or faces that form the wall's cross-section,
        which is then used by the caller for extrusion. It handles both single- and multi-layer wall
        configurations.

        Parameters
        ----------
        obj : FreeCAD.DocumentObject
            The wall object being built. Its Length, Align, and layer
            properties are used.

        Returns
        -------
        tuple of (list of Part.Face, FreeCAD.Placement)
            A tuple containing two elements:
            1. A list of one or more Part.Face objects representing the cross-section.
            2. An identity Placement, as the geometry is in local coordinates.

        Notes
        -----
        The geometry follows the convention where `Align='Left'` offsets the wall in the negative-Y
        direction (the geometric right).
        """
        import Part

        def _create_face_from_coords(half_length, y_min, y_max):
            """Creates a rectangular Part.Face centered on the X-axis, defined by Y coordinates."""
            bottom_left = Vector(-half_length, y_min, 0)
            bottom_right = Vector(half_length, y_min, 0)
            top_right = Vector(half_length, y_max, 0)
            top_left = Vector(-half_length, y_max, 0)
            return Part.Face(
                Part.makePolygon([bottom_left, bottom_right, top_right, top_left, bottom_left])
            )

        layers = self.get_layers(obj)
        width = self.get_width(obj, widths=False)
        align = obj.Align

        safe_length = obj.Length.Value or 0.5

        if not layers:
            safe_width = width or 0.5
            layers = [safe_width]

        base_faces = []

        totalwidth = sum([abs(layer) for layer in layers])

        offset = 0
        if align == "Center":
            offset = -totalwidth / 2
        elif align == "Left":
            offset = -totalwidth

        for layer in layers:
            if layer > 0:
                half_length = safe_length / 2
                layer_y_min = offset
                layer_y_max = offset + layer
                face = _create_face_from_coords(half_length, layer_y_min, layer_y_max)
                base_faces.append(face)

            offset += abs(layer)

        placement = FreeCAD.Placement()

        p1 = Vector(-safe_length / 2, 0, 0)
        p2 = Vector(safe_length / 2, 0, 0)
        self.basewires = [[Part.LineSegment(p1, p2).toShape()]]

        return base_faces, placement


if FreeCAD.GuiUp:

    class WallTaskPanel(ArchComponent.ComponentTaskPanel):
        def __init__(self, obj):
            ArchComponent.ComponentTaskPanel.__init__(self)
            self.obj = obj
            self.wallWidget = QtGui.QWidget()
            self.wallWidget.setWindowTitle(translate("Arch", "Wall Options"))

            layout = QtGui.QFormLayout(self.wallWidget)
            loader = FreeCADGui.UiLoader()

            self.length = loader.createWidget("Gui::QuantitySpinBox")
            FreeCADGui.ExpressionBinding(self.length).bind(self.obj, "Length")
            self.length.setProperty("value", self.obj.Length)
            layout.addRow(translate("Arch", "Length"), self.length)

            self.width = loader.createWidget("Gui::QuantitySpinBox")
            FreeCADGui.ExpressionBinding(self.width).bind(self.obj, "Width")
            self.width.setProperty("value", self.obj.Width)
            layout.addRow(translate("Arch", "Width"), self.width)

            self.height = loader.createWidget("Gui::QuantitySpinBox")
            FreeCADGui.ExpressionBinding(self.height).bind(self.obj, "Height")
            self.height.setProperty("value", self.obj.Height)
            layout.addRow(translate("Arch", "Height"), self.height)

            self.alignLayout = QtGui.QHBoxLayout()
            self.alignLeft = QtGui.QRadioButton(translate("Arch", "Left"))
            self.alignCenter = QtGui.QRadioButton(translate("Arch", "Center"))
            self.alignRight = QtGui.QRadioButton(translate("Arch", "Right"))
            self.alignLayout.addWidget(self.alignLeft)
            self.alignLayout.addWidget(self.alignCenter)
            self.alignLayout.addWidget(self.alignRight)
            self.alignLayout.addStretch()

            self.alignGroup = QtGui.QButtonGroup(self.wallWidget)
            self.alignGroup.addButton(self.alignLeft)
            self.alignGroup.addButton(self.alignCenter)
            self.alignGroup.addButton(self.alignRight)
            self.alignGroup.buttonClicked.connect(self.setAlign)

            if obj.Align == "Left":
                self.alignLeft.setChecked(True)
            elif obj.Align == "Right":
                self.alignRight.setChecked(True)
            else:
                self.alignCenter.setChecked(True)

            layout.addRow(translate("Arch", "Alignment"), self.alignLayout)

            self.form = [self.wallWidget, self.form]

        def setAlign(self, button):
            if button == self.alignLeft:
                self.obj.Align = "Left"
            elif button == self.alignRight:
                self.obj.Align = "Right"
            else:
                self.obj.Align = "Center"
            self.obj.recompute()

        def accept(self):
            self.obj.Length = self.length.property("value")
            self.obj.Width = self.width.property("value")
            self.obj.Height = self.height.property("value")
            return super().accept()


class _ViewProviderWall(ArchComponent.ViewProviderComponent):
    """The view provider for the wall object.

    Parameters
    ----------
    vobj: <Gui.ViewProviderDocumentObject>
        The view provider to turn into a wall view provider.
    """

    def __init__(self, vobj):
        ArchComponent.ViewProviderComponent.__init__(self, vobj)
        vobj.ShapeColor = ArchCommands.getDefaultColor("Wall")

    def getIcon(self):
        """Return the path to the appropriate icon.

        If a clone, return the cloned wall icon path. Otherwise return the
        Arch wall icon.

        Returns
        -------
        str
            Path to the appropriate icon .svg file.
        """

        import Arch_rc

        if hasattr(self, "Object"):
            if self.Object.CloneOf:
                return ":/icons/Arch_Wall_Clone.svg"
            elif (not self.Object.Base) and self.Object.Additions and not self.Object.Length.Value:
                return ":/icons/Arch_Wall_Tree_Assembly.svg"
        return ":/icons/Arch_Wall_Tree.svg"

    def attach(self, vobj):
        """Add display modes' data to the coin scenegraph.

        Add each display mode as a coin node, whose parent is this view
        provider.

        Each display mode's node includes the data needed to display the object
        in that mode. This might include colors of faces, or the draw style of
        lines. This data is stored as additional coin nodes which are children
        of the display mode node.

        Add the textures used in the Footprint display mode.
        """

        self.Object = vobj.Object
        from pivy import coin

        tex = coin.SoTexture2()
        image = Draft.loadTexture(Draft.svgpatterns()["simple"][1], 128)
        if not image is None:
            tex.image = image
        texcoords = coin.SoTextureCoordinatePlane()
        s = params.get_param_arch("patternScale")
        texcoords.directionS.setValue(s, 0, 0)
        texcoords.directionT.setValue(0, s, 0)
        self.fcoords = coin.SoCoordinate3()
        self.fset = coin.SoIndexedFaceSet()
        sep = coin.SoSeparator()
        sep.addChild(tex)
        sep.addChild(texcoords)
        sep.addChild(self.fcoords)
        sep.addChild(self.fset)
        vobj.RootNode.addChild(sep)
        ArchComponent.ViewProviderComponent.attach(self, vobj)

    def updateData(self, obj, prop):
        """Method called when the host object has a property changed.

        If the host object's Placement, Shape, or Material has changed, and the
        host object has a Material assigned, give the shape the color and
        transparency of the Material.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The host object that has changed.
        prop: string
            The name of the property that has changed.
        """

        if prop in ["Placement", "Shape", "Material"]:
            if obj.ViewObject.DisplayMode == "Footprint":
                obj.ViewObject.Proxy.setDisplayMode("Footprint")
            if hasattr(obj, "Material"):
                if obj.Material and obj.Shape:
                    if hasattr(obj.Material, "Materials"):
                        activematerials = [
                            obj.Material.Materials[i]
                            for i in range(len(obj.Material.Materials))
                            if obj.Material.Thicknesses[i] >= 0
                        ]

                        if hasattr(obj.Proxy, 'solidsNumLst'):
                            expanded = []
                            for mat, count in zip(activematerials,
                                                  obj.Proxy.solidsNumLst):
                                expanded.extend([mat] * count)
                            activematerials = expanded

                        if len(activematerials) == len(obj.Shape.Solids):
                            cols = []
                            for i, mat in enumerate(activematerials):
                                c = obj.ViewObject.ShapeColor
                                c = (c[0], c[1], c[2],
                                     1.0 - obj.ViewObject.Transparency / 100.0)
                                if "DiffuseColor" in mat.Material:
                                    if "(" in mat.Material["DiffuseColor"]:
                                        c = tuple(float(f) for f in
                                                  mat.Material["DiffuseColor"]
                                                  .strip("()").split(","))
                                if "Transparency" in mat.Material:
                                    c = (c[0], c[1], c[2],
                                         1.0 - float(mat.Material["Transparency"]))
                                cols.extend([c] * len(obj.Shape.Solids[i].Faces))
                            obj.ViewObject.DiffuseColor = cols
        ArchComponent.ViewProviderComponent.updateData(self, obj, prop)
        if len(obj.ViewObject.DiffuseColor) > 1:
            obj.ViewObject.DiffuseColor = obj.ViewObject.DiffuseColor

    def getDisplayModes(self, vobj):
        """Define the display modes unique to the Arch Wall.

        Define mode Footprint, which only displays the footprint of the wall.
        Also add the display modes of the Arch Component.

        Returns
        -------
        list of str
            List containing the names of the new display modes.
        """

        modes = ArchComponent.ViewProviderComponent.getDisplayModes(self, vobj) + ["Footprint"]
        return modes

    def setDisplayMode(self, mode):
        """Method called when the display mode changes.

        Called when the display mode changes, this method can be used to set
        data that wasn't available when .attach() was called.

        When Footprint is set as display mode, find the faces that make up the
        footprint of the wall, and give them a lined texture. Then display
        the wall as a wireframe.

        Then pass the displaymode onto Arch Component's .setDisplayMode().

        Parameters
        ----------
        mode: str
            The name of the display mode the view provider has switched to.

        Returns
        -------
        str:
            The name of the display mode the view provider has switched to.
        """

        self.fset.coordIndex.deleteValues(0)
        self.fcoords.point.deleteValues(0)
        if mode == "Footprint":
            if hasattr(self, "Object"):
                faces = self.Object.Proxy.getFootprint(self.Object)
                if faces:
                    verts = []
                    fdata = []
                    idx = 0
                    for face in faces:
                        tri = face.tessellate(1)
                        for v in tri[0]:
                            verts.append([v.x, v.y, v.z])
                        for f in tri[1]:
                            fdata.extend([f[0] + idx, f[1] + idx, f[2] + idx, -1])
                        idx += len(tri[0])
                    self.fcoords.point.setValues(verts)
                    self.fset.coordIndex.setValues(0, len(fdata), fdata)
            return "Wireframe"
        return ArchComponent.ViewProviderComponent.setDisplayMode(self, mode)

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None
        taskd = WallTaskPanel(vobj.Object)
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True

    def setupContextMenu(self, vobj, menu):

        if FreeCADGui.activeWorkbench().name() != "BIMWorkbench":
            return

        super().contextMenuAddEdit(menu)

        actionFlipDirection = QtGui.QAction(
            QtGui.QIcon(":/icons/Arch_Wall_Tree.svg"), translate("Arch", "Flip Direction"), menu
        )
        QtCore.QObject.connect(
            actionFlipDirection, QtCore.SIGNAL("triggered()"), self.flipDirection
        )
        menu.addAction(actionFlipDirection)

        super().contextMenuAddToggleSubcomponents(menu)

    def flipDirection(self):

        if hasattr(self, "Object") and self.Object:
            obj = self.Object
            if obj.Align == "Left":
                obj.Align = "Right"
                FreeCAD.ActiveDocument.recompute()
            elif obj.Align == "Right":
                obj.Align = "Left"
                FreeCAD.ActiveDocument.recompute()