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

"""

import math

import FreeCAD
import ArchCommands
import ArchComponent
import ArchSketchObject
import ArchWallGeometry
import ArchWallEndpoint
import ArchWallEndCondition
import ArchWallTrimming
import ArchWallRelation
import ArchWallRelationResolver
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

    _SECTION_ALIGNMENTS = ("Left", "Right", "Center")

    def __init__(self, obj):
        ArchComponent.Component.__init__(self, obj)
        self.Type = "Wall"
        self._normalizing_end_condition_order = False
        self._resolved_geometry_signatures = {}
        self._invalidating_wall_relations = False
        self.setProperties(obj)
        obj.IfcType = "Wall"

    def setProperties(self, obj):
        """Give the wall its wall specific properties, such as its alignment.

        You can learn more about properties here:
        https://wiki.freecad.org/property

        parameters
        ----------
        obj: <part::featurepython>
            The object to turn into a wall.
        """

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

        # To be combined into Width when PropertyLengthList is available
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
            )  # see DraftGeomUtils.offsetwire()
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
            )  # see DraftGeomUtils.offsetwire()
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
            )  # see DraftGeomUtils.offsetwire()
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
                    "App::Property",
                    "The area of this wall as a simple Height * Length calculation",
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
                    "App::Property",
                    "The face number of the base object used to build this wall",
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

        # See getExtrusionData(), removeSplitters are no longer used
        # if not "Refine" in lp:
        #    obj.addProperty("App::PropertyEnumeration","Refine","Wall",QT_TRANSLATE_NOOP("App::Property","Select whether or not and the method to remove splitter of the Wall. Currently Draft removeSplitter and Part removeSplitter available but may not work on complex sketch."), locked=True)
        #    obj.Refine = ['No','DraftRemoveSplitter','PartRemoveSplitter']
        # TODO - To implement in Arch Component ?

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
                    "App::Property",
                    "The horizontal offset of the second line of blocks",
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
        if not hasattr(self, "ArchSkPropSetPickedUuid"):  # 'obj.Proxy', 'self' not works ?
            self.ArchSkPropSetPickedUuid = ""
        if not hasattr(self, "ArchSkPropSetListPrev"):
            self.ArchSkPropSetListPrev = []
        if "EndingStart" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyPlacement",
                "EndingStart",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "A placement, relative to the main wall placement, describing "
                    "a plane that cuts the end of the wall, at start position.",
                ),
            )
        if "EndingEnd" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyPlacement",
                "EndingEnd",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "A placement, relative to the main wall placement, describing "
                    "a plane that cuts the end of the wall, at end position.",
                ),
            )
        if "EndConditionOrderStart" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "EndConditionOrderStart",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Ordered trim providers for the start end of the wall. Valid entries are Relation and Manual.",
                ),
            )
            obj.EndConditionOrderStart = list(ArchWallEndCondition.DEFAULT_END_CONDITION_ORDER)
        if "EndConditionOrderEnd" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "EndConditionOrderEnd",
                "Wall",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Ordered trim providers for the end end of the wall. Valid entries are Relation and Manual.",
                ),
            )
            obj.EndConditionOrderEnd = list(ArchWallEndCondition.DEFAULT_END_CONDITION_ORDER)
        self.connectEdges = []

    def dumps(self):
        dump = super().dumps()
        if not isinstance(dump, tuple):
            dump = (dump,)  # Python Tuple With One Item
        dump = dump + (self.ArchSkPropSetPickedUuid, self.ArchSkPropSetListPrev)
        return dump

    def loads(self, state):
        self.Type = "Wall"
        self._normalizing_end_condition_order = False
        if state == None:
            return
        elif state[0] == "W":  # state[1] == 'a', behaviour before 2024.11.28
            return
        elif state[0] == "Wall":
            self.ArchSkPropSetPickedUuid = state[1]
            self.ArchSkPropSetListPrev = state[2]
        elif state[0] != "Wall":  # model before merging super.dumps/loads()
            self.ArchSkPropSetPickedUuid = state[0]
            self.ArchSkPropSetListPrev = state[1]

    def onDocumentRestored(self, obj):
        """Method run when the document is restored. Re-adds the Arch component, and Arch wall properties."""

        import DraftGeomUtils
        from draftutils.messages import _log

        ArchComponent.Component.onDocumentRestored(self, obj)
        self._normalizing_end_condition_order = False
        self.setProperties(obj)

        # In V1.0 the handling of wall normals has changed. As a result existing
        # walls with their Normal set to [0, 0, 0], based on wires or faces with
        # a shape normal pointing towards -Z, would be extruded in that direction
        # instead of towards +Z as before. To avoid this their Normal property is
        # changed to [0, 0, 1].
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
            if hasattr(obj, "Width"):  # TODO need test?
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

    def execute(self, obj):
        """Method run when the object is recomputed.

        Extrude the wall from the Base shape if possible. Processe additions
        and subtractions. Assign the resulting shape as the shape of the wall.

        Add blocks if the MakeBlocks property is assigned. If the Base shape is
        a mesh, just copy the mesh.
        """

        if self.clone(obj):
            return

        self._invalidate_relations_if_geometry_changed(obj)

        # Wall can do without Base, validity to be tested in getExtrusionData()
        # Remarked out ensureBase() below
        # if not self.ensureBase(obj):
        #    return

        import Part
        import DraftGeomUtils

        base = None
        pl = obj.Placement

        # PropertySet support
        propSetPickedUuidPrev = self.ArchSkPropSetPickedUuid
        propSetListPrev = self.ArchSkPropSetListPrev
        propSetSelectedNamePrev = obj.ArchSketchPropertySet
        propSetSelectedNameCur = None
        propSetListCur = None
        if Draft.getType(obj.Base) == "ArchSketch":
            baseProxy = obj.Base.Proxy
            if hasattr(baseProxy, "getPropertySet"):
                # get full list of PropertySet
                propSetListCur = baseProxy.getPropertySet(obj.Base)
                # get updated name (if any) of the selected PropertySet
                propSetSelectedNameCur = baseProxy.getPropertySet(
                    obj.Base, propSetUuid=propSetPickedUuidPrev
                )
        if propSetSelectedNameCur:  # True if selection is not deleted
            if propSetListPrev != propSetListCur:
                obj.ArchSketchPropertySet = propSetListCur
                obj.ArchSketchPropertySet = propSetSelectedNameCur
                self.ArchSkPropSetListPrev = propSetListCur
            # elif propSetListPrev == propSetListCur:
            # pass  #nothing to do in this case
            # but if below, though (propSetListPrev == propSetListCur)
            elif propSetSelectedNamePrev != propSetSelectedNameCur:
                obj.ArchSketchPropertySet = propSetSelectedNameCur
        else:  # True if selection is deleted
            if propSetListCur:
                if propSetListPrev != propSetListCur:
                    obj.ArchSketchPropertySet = propSetListCur
                    obj.ArchSketchPropertySet = "Default"
                # else:  # Seems no need ...
                # obj.PropertySet = 'Default'

        extdata = self.getExtrusionData(obj)
        if extdata:
            base_faces = extdata[0]
            extv = extdata[2].Rotation.multVec(extdata[1])

            # Normalize geometry: getExtrusionData can return a single face or a list of faces.
            # Normalize it to always be a list to simplify the logic below.
            if not isinstance(base_faces, list):
                base_faces = [base_faces]

            # Determine the fusion strategy: solids should only be fused if the base is a Sketch and
            # it is not a multi-layer wall.
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

            # Generate solids
            solids = []
            for face in base_faces:
                face.Placement = extdata[2].multiply(face.Placement)
                solids.append(face.extrude(extv))

            # Apply the fusion strategy
            if should_fuse_solids:
                fused_shape = None
                for solid in solids:
                    fused_shape = fused_shape.fuse(solid) if fused_shape else solid
                base = fused_shape
            else:
                base = Part.makeCompound(solids)
        if obj.Base:
            if hasattr(obj.Base, "Shape"):
                if obj.Base.Shape.isNull():
                    return
                if not obj.Base.Shape.isValid():
                    if not obj.Base.Shape.Solids:
                        # let pass invalid objects if they have solids...
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
        # Blocks calculation
        if hasattr(obj, "MakeBlocks") and hasattr(self, "basewires"):
            if obj.MakeBlocks and self.basewires and extdata and obj.Width and obj.Height:
                blocks = self._make_blocks(obj, base_faces[0], extv)
                if blocks is not None:
                    base = blocks
        if not base:
            # FreeCAD.Console.PrintError(translate("Arch","Error: Invalid base object")+"\n")
            # return
            # walls can be made of only a series of additions and have no base shape
            base = Part.Shape()

        relation_endings = ArchWallRelationResolver.collect_wall_relation_endings(obj)
        end_conditions = {
            end_name: self._resolve_end_condition(obj, end_name, relation_endings)
            for end_name in ("Start", "End")
        }
        baseline = self.get_global_baseline(obj)
        if baseline is not None:
            for end_name, condition in end_conditions.items():
                if condition and condition.source == "Relation":
                    base = ArchWallTrimming.extend_solid_along_baseline(
                        base,
                        baseline,
                        pl,
                        end_name,
                        condition.extension,
                    )
        base = self.processSubShapes(obj, base, pl)
        trimmed_base = self.process_endings(obj, base, pl, end_conditions)
        base = trimmed_base
        self.applyShape(obj, base, pl)

        # Check if there is base, and if width and height is provided or not
        # Provide users message below to check the setting of the Wall object
        if base.isNull() and (self.noWidths or self.noHeight):
            FreeCAD.Console.PrintWarning(
                translate(
                    "Arch",
                    f"Cannot create or update {obj.Label} as its length, height or width is zero, and there are no solids in its additions",
                )
                + "\n"
            )

        # count blocks
        if hasattr(obj, "MakeBlocks"):
            if obj.MakeBlocks:
                fvol = obj.BlockLength.Value * obj.BlockHeight.Value * obj.Width.Value
                if fvol:
                    # print("base volume:",fvol)
                    # for s in base.Solids:
                    # print(abs(s.Volume - fvol))
                    ents = [s for s in base.Solids if abs(s.Volume - fvol) < 1]
                    obj.CountEntire = len(ents)
                    obj.CountBroken = len(base.Solids) - len(ents)
                else:
                    obj.CountEntire = 0
                    obj.CountBroken = 0

        # set the length property
        if hasattr(self, "connectEdges") and self.connectEdges:
            l = float(0)
            for e in self.connectEdges:
                l += e.Length
            l = l / 2
            if self.layersNum:
                l = l / self.layersNum
            if obj.Length.Value != l:
                obj.Length = l
                self.oldLength = (
                    None  # delete the stored value to prevent triggering base change below
                )

        # set the Area property
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
        """Method called when the object has a property changed.

        If length has changed, extend the length of the Base object, if the
        Base object only has a single edge to extend.

        Also hide subobjects.

        Also call ArchComponent.Component.onChanged().

        Parameters
        ----------
        prop: string
            The name of the property that has changed.
        """

        if prop in ("EndConditionOrderStart", "EndConditionOrderEnd"):
            self._normalize_end_condition_order_property(obj, prop)

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
                                    # print "modifying p2"
                                    obj.Base.End = p2
                                elif Draft.getType(obj.Base) in [
                                    "Sketcher::SketchObject",
                                    "ArchSketch",
                                ]:
                                    # obj.Base.recompute() # Fix for the 'GeoId index out range' error. Not required in V1.1.
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

        self.hideSubobjects(obj, prop)
        ArchComponent.Component.onChanged(self, obj, prop)

    def _invalidate_relations_if_geometry_changed(self, obj):
        """Touch relation dependents when resolved wall geometry changes.

        A wall can depend on a Draft line or sketch without receiving an
        ``onChanged('Base')`` callback when that source moves.  Comparing the
        resolved baseline and section at the wall execution boundary covers
        both direct property edits and changes propagated from a linked base,
        without maintaining a fragile list of property names.
        """
        signature = self._resolved_geometry_signature(obj)
        previous = self._resolved_geometry_signatures.get(obj.Name)
        self._resolved_geometry_signatures[obj.Name] = signature
        if previous is None or previous == signature:
            return
        self._touch_wall_relations(obj)

    def _touch_wall_relations(self, obj):
        if self._invalidating_wall_relations:
            return
        self._invalidating_wall_relations = True
        touched = set()
        touched_walls = {obj.Name}
        try:
            for relation in ArchWallRelation.iter_wall_relations(obj):
                if relation.Name in touched:
                    continue
                touched.add(relation.Name)
                relation.touch()
                for linked_wall in ArchWallRelation.get_relation_walls(relation):
                    if not linked_wall or linked_wall.Name in touched_walls:
                        continue
                    touched_walls.add(linked_wall.Name)
                    linked_wall.touch()
        finally:
            self._invalidating_wall_relations = False

    def _resolved_geometry_signature(self, obj):
        baseline = self.get_global_baseline(obj)
        normal = baseline.normal if baseline is not None else None
        section = self.get_resolved_section(obj)
        return (
            self._edge_signature(baseline),
            self._vector_signature(normal),
            (
                tuple((layer.raw_thickness, layer.y_min, layer.y_max) for layer in section.layers)
                if section is not None
                else None
            ),
        )

    @staticmethod
    def _edge_signature(baseline):
        if baseline is None:
            return None
        return (
            _Wall._vector_signature(baseline.start_point),
            _Wall._vector_signature(baseline.end_point),
        )

    @staticmethod
    def _vector_signature(vector):
        if vector is None:
            return None
        return tuple(round(value, 9) for value in (vector.x, vector.y, vector.z))

    def _normalize_end_condition_order_property(self, obj, prop):
        """Normalize wall end-condition order without recursively handling our own write."""
        if self._normalizing_end_condition_order:
            return

        current = list(getattr(obj, prop))
        normalized = ArchWallEndCondition.normalize_end_condition_order(current)
        if current == normalized:
            return

        self._normalizing_end_condition_order = True
        try:
            setattr(obj, prop, normalized)
        finally:
            self._normalizing_end_condition_order = False

    @staticmethod
    def _base_object(obj):
        """Return the wall base object safely for legacy or partially initialized walls."""
        return getattr(obj, "Base", None)

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

    def requires_brep_export(self, obj):
        """Return whether IFC must use the wall's processed shape."""
        manual_endings = (
            getattr(obj, "EndingStart", FreeCAD.Placement()),
            getattr(obj, "EndingEnd", FreeCAD.Placement()),
        )
        if any(
            not ArchWallEndCondition.is_null_placement(placement) for placement in manual_endings
        ):
            return True
        relation_endings = ArchWallRelationResolver.collect_wall_relation_endings(obj)
        return any(relation_endings.get(end_name) for end_name in ("Start", "End"))

    def isStandardCase(self, obj):
        """Return whether this wall can use the IFC standard-case form."""
        if self.requires_brep_export(obj):
            return False
        return super().isStandardCase(obj)

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

        propSetUuid = self.ArchSkPropSetPickedUuid

        # If ArchComponent.Component.getExtrusionData() can successfully get
        # extrusion data, just use that.
        data = ArchComponent.Component.getExtrusionData(self, obj)
        if data:
            if not isinstance(data[0], list):
                # multifuses not considered here
                return data
        length = obj.Length.Value
        # TODO currently layers were not supported when len(basewires) > 0	##( or 1 ? )

        self.noWidths = False
        self.noHeight = False
        widths, aligns, offsets = self._resolved_section_lists(obj)
        default_width, default_align, default_offset = self._section_defaults(obj)
        width, _align, _offset = self._resolve_section_value_at(obj, 0)
        if not widths and not self._resolve_material_layers(obj):
            self.noWidths = True

        # Check height.
        height = obj.Height.Value
        if not height:
            height = self.getParentHeight(obj)
        if not height:
            self.noHeight = True
        if self.noWidths or self.noHeight:
            return None

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

        # Width, alignment, and offset have already been resolved above.

        # Check and build wall layers
        self.multimaterialsWidth = False
        layers = self.get_layers(obj)
        # check total width and update Wall's Width
        if layers:
            total = sum(abs(layer) for layer in layers)
            if obj.Width.Value != total:
                obj.Width = total
            # If there is no 0 (zero) in any of the layers, the total thickness
            # is driven by the multi-materials itself.  Otherwise, user should
            # be able in any time change the Width and drive the total thickness
            # - in the latter case, Width property should not be changed to
            # ready-only.
            if not (0 in obj.Material.Thicknesses):
                self.multimaterialsWidth = True
        if self.multimaterialsWidth:
            obj.setEditorMode("Width", ["ReadOnly"])
        else:
            obj.setEditorMode("Width", 0)
        # Check if there is obj.Base and its validity to proceed
        if self.ensureBase(obj):
            if hasattr(obj.Base, "Shape"):
                if obj.Base.Shape:
                    if obj.Base.Shape.Solids:
                        return None

                    # If the user has defined a specific face of the Base
                    # object to build the wall from, extrude from that face,
                    # and return the extrusion moved to (0,0,0), normal of the
                    # face, and placement to move the extrusion back to its
                    # original position.
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

                    # If the Base has faces, but no specific one has been
                    # selected, rebase the faces and continue.
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
                        # get wall edges (not wires); use original edges if getWallBaseShapeEdgesInfo() provided none
                        if wallBaseShapeEdgesInfo:
                            self.basewires = wallBaseShapeEdgesInfo.get(
                                "wallAxis"
                            )  # 'wallEdges'  # widths, aligns, offsets?

                    # Sort Sketch edges consistently with below procedures
                    # without using Sketch.Shape.Edges - found the latter order
                    # in some corner case != getSortedClusters()
                    elif obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                        self.basewires = []
                        skGeom = obj.Base.GeometryFacadeList
                        skGeomEdges = []
                        skPlacement = obj.Base.Placement  # Get Sketch's placement to restore later
                        # Get ArchSketch edges to construct ArchWall
                        # No need to test obj.ArchSketchData ...
                        for ig, geom in enumerate(skGeom):
                            # Construction mode edges should be ignored if
                            # ArchSketchEdges, otherwise, ArchSketchEdges data
                            # needs to take out those in Construction before
                            # using as parameters.
                            if (not obj.ArchSketchEdges and not geom.Construction) or str(
                                ig
                            ) in obj.ArchSketchEdges:
                                # support Line, Arc, Circle, Ellipse for Sketch
                                # as Base at the moment
                                if isinstance(
                                    geom.Geometry,
                                    (
                                        Part.LineSegment,
                                        Part.Circle,
                                        Part.ArcOfCircle,
                                        Part.Ellipse,
                                    ),
                                ):
                                    skGeomEdgesI = geom.Geometry.toShape()
                                    skGeomEdges.append(skGeomEdgesI)
                        for cluster in Part.getSortedClusters(skGeomEdges):
                            clusterTransformed = []
                            for edge in cluster:
                                # TODO 2023.11.26: Multiplication order should be switched?
                                # So far 'no problem' as 'edge.placement' is always '0,0,0' ?
                                edge.Placement = edge.Placement.multiply(
                                    skPlacement
                                )  ## TODO add attribute to skip Transform...
                                clusterTransformed.append(edge)
                            # Only use cluster of edges rather than turning into wire
                            self.basewires.append(clusterTransformed)

                        # Use Sketch's Normal for all edges/wires generated
                        # from sketch for consistency. Discussion on checking
                        # normal of sketch.Placement vs
                        # sketch.getGlobalPlacement() -
                        # https://forum.freecad.org/viewtopic.php?f=22&t=39341&p=334275#p334275
                        # normal = obj.Base.Placement.Rotation.multVec(FreeCAD.Vector(0,0,1))
                        normal = obj.Base.getGlobalPlacement().Rotation.multVec(
                            FreeCAD.Vector(0, 0, 1)
                        )

                    else:  # For all objects except Sketch, single edge or more
                        # See discussion - https://forum.freecad.org/viewtopic.php?t=86365
                        # See discussion - https://forum.freecad.org/viewtopic.php?t=82207&start=10
                        # self.basewires = obj.Base.Shape.Wires
                        #
                        # Now, adopt approach same as for Sketch
                        self.basewires = []
                        shpEdges = obj.Base.Shape.Edges
                        baseEdges = []
                        for edge in shpEdges:
                            if (
                                isinstance(edge.Curve, (Part.Line, Part.Circle))
                                or isinstance(edge.Curve, Part.Ellipse)
                                and edge.isClosed()
                            ):
                                baseEdges.append(edge)
                        clusters = Part.getSortedClusters(baseEdges)
                        self.basewires = clusters
                        # Previously :
                        # Found case that after sorting below, direction of
                        # edges sorted are not as 'expected' thus resulted in
                        # bug - e.g. a Dwire with edges/vertexes in clockwise
                        # order, 1st vertex is Forward as expected.  After
                        # sorting below, edges sorted still in clockwise order
                        # - no problem, but 1st vertex of each edge become
                        # Reverse rather than Forward.

                        # See FC discussion -
                        # https://forum.freecad.org/viewtopic.php?f=23&t=48275&p=413745#p413745

                        # self.basewires = []
                        # for cluster in Part.getSortedClusters(obj.Base.Shape.Edges):
                        #    for c in Part.sortEdges(cluster):
                        #        self.basewires.append(Part.Wire(c))
                        # if not sketch, e.g. Dwire, can have wire which is 3d
                        # so not on the placement's working plane - below
                        # applied to Sketch not applicable here
                        # normal = obj.Base.getGlobalPlacement().Rotation.multVec(FreeCAD.Vector(0,0,1))
                        # normal = obj.Base.Placement.Rotation.multVec(FreeCAD.Vector(0,0,1))

                    if self.basewires:
                        if (len(self.basewires) == 1) and layers:
                            self.basewires = [self.basewires[0] for l in layers]
                            self.layersNum = len(layers)
                        if not ((len(self.basewires) == 1) and layers):
                            self.layersNum = 0
                        layeroffset = 0
                        baseface = None
                        self.connectEdges = []
                        section_index = 0
                        for i, wire in enumerate(self.basewires):

                            # Check number of edges per 'wire' and get the 1st edge
                            if isinstance(wire, Part.Wire):
                                edgeNum = len(wire.Edges)
                                e = wire.Edges[0]
                            elif isinstance(wire[0], Part.Edge):
                                edgeNum = len(wire)
                                e = wire[0]

                            if not layers:
                                resolved_widths = []
                                resolved_aligns = []
                                resolved_offsets = []
                                for segment in range(edgeNum):
                                    section = self.get_resolved_section(
                                        obj, section_index + segment
                                    )
                                    if section is None or not section.visible_layers:
                                        continue
                                    resolved_widths.append(section.y_max - section.y_min)
                                    _, align, offset = self._resolve_section_value_at(
                                        obj, section_index + segment
                                    )
                                    resolved_aligns.append(align)
                                    resolved_offsets.append(offset)
                                if resolved_widths:
                                    widths = resolved_widths
                                    aligns = resolved_aligns
                                    offsets = resolved_offsets
                                    width = widths[0]
                                section_index += edgeNum

                            for n in range(
                                0, edgeNum, 1
                            ):  # why these not work - range(edgeNum), range(0,edgeNum) ...

                                # Fill the aligns list with ArchWall's default
                                # align entry and with same number of items as
                                # number of edges
                                try:
                                    if aligns[n] not in self._SECTION_ALIGNMENTS:
                                        aligns[n] = default_align
                                except Exception:
                                    aligns.append(default_align)

                                # Fill the widths List with ArchWall's default
                                # width entry and with same number of items as
                                # number of edges
                                try:
                                    if not widths[n]:
                                        widths[n] = default_width
                                except Exception:
                                    widths.append(default_width)
                                # Fill the offsets List with ArchWall's default
                                # offset entry and with same number of items as
                                # number of edges
                                try:
                                    if not offsets[n]:
                                        offsets[n] = default_offset
                                except Exception:
                                    offsets.append(default_offset)

                            # Get a direction vector orthogonal to both the
                            # normal of the face/sketch and the direction the
                            # wire was drawn in. IE: along the width direction
                            # of the wall.
                            if isinstance(e.Curve, (Part.Circle, Part.Ellipse)):
                                dvec = e.Vertexes[0].Point.sub(e.Curve.Center)
                            else:
                                dvec = DraftGeomUtils.vec(e).cross(normal)

                            if not DraftVecUtils.isNull(dvec):
                                dvec.normalize()
                            face = None

                            curAligns = aligns[0]
                            # off = obj.Offset.Value  # off is no longer used

                            if curAligns == "Left":

                                if layers:
                                    curWidth = []
                                    for n in range(edgeNum):
                                        curWidth.append(abs(layers[i]))
                                    # off = off+layeroffset  # off is no longer used
                                    offsets = [x + layeroffset for x in offsets]
                                    dvec.multiply(curWidth[0])
                                    layeroffset += abs(curWidth[0])
                                else:
                                    curWidth = widths
                                    dvec.multiply(width)

                                # Now DraftGeomUtils.offsetWire() support
                                # similar effect as ArchWall Offset
                                #
                                # if off:
                                #    dvec2 = DraftVecUtils.scaleTo(dvec,off)
                                #    wire = DraftGeomUtils.offsetWire(wire,dvec2)

                                # Get the 'offseted' wire taking into account
                                # of Width and Align of each edge, and overall
                                # Offset
                                wNe2 = DraftGeomUtils.offsetWire(
                                    wire,
                                    dvec,
                                    bind=False,
                                    occ=False,
                                    widthList=curWidth,
                                    offsetMode=None,
                                    alignList=aligns,
                                    normal=normal,
                                    basewireOffset=offsets,
                                    wireNedge=True,
                                )
                                # Get the 'base' wire taking into account of
                                # width and align of each edge
                                wNe1 = DraftGeomUtils.offsetWire(
                                    wire,
                                    dvec,
                                    bind=False,
                                    occ=False,
                                    widthList=curWidth,
                                    offsetMode="BasewireMode",
                                    alignList=aligns,
                                    normal=normal,
                                    basewireOffset=offsets,
                                    wireNedge=True,
                                )
                            elif curAligns == "Right":
                                dvec = dvec.negative()

                                if layers:
                                    curWidth = []
                                    for n in range(edgeNum):
                                        curWidth.append(abs(layers[i]))
                                    # off = off+layeroffset  # off is no longer used
                                    offsets = [x + layeroffset for x in offsets]
                                    dvec.multiply(curWidth[0])
                                    layeroffset += abs(curWidth[0])
                                else:
                                    curWidth = widths
                                    dvec.multiply(width)

                                # Now DraftGeomUtils.offsetWire() support similar effect as ArchWall Offset
                                #
                                # if off:
                                #    dvec2 = DraftVecUtils.scaleTo(dvec,off)
                                #    wire = DraftGeomUtils.offsetWire(wire,dvec2)
                                wNe2 = DraftGeomUtils.offsetWire(
                                    wire,
                                    dvec,
                                    bind=False,
                                    occ=False,
                                    widthList=curWidth,
                                    offsetMode=None,
                                    alignList=aligns,
                                    normal=normal,
                                    basewireOffset=offsets,
                                    wireNedge=True,
                                )
                                wNe1 = DraftGeomUtils.offsetWire(
                                    wire,
                                    dvec,
                                    bind=False,
                                    occ=False,
                                    widthList=curWidth,
                                    offsetMode="BasewireMode",
                                    alignList=aligns,
                                    normal=normal,
                                    basewireOffset=offsets,
                                    wireNedge=True,
                                )
                            elif curAligns == "Center":
                                if layers:
                                    # TODO Current, the order of layers follow
                                    # "Right" align.  Option for the order of
                                    # layers follow "Left" align should be
                                    # provided for users.
                                    dvec = dvec.negative()
                                    totalwidth = sum([abs(l) for l in layers])
                                    off = totalwidth / 2 - layeroffset
                                    # TODO To consider offset per edge?
                                    #
                                    # Offset follows direction of align "Right".
                                    # Needs to be reversed in this case.
                                    off = -off
                                    # d1 = Vector(dvec).multiply(off)
                                    curWidth = []
                                    alignListC = []
                                    offsetListC = []
                                    for n in range(edgeNum):
                                        curWidth.append(abs(layers[i]))
                                        alignListC.append("Right")  # ("Left")
                                        offsetListC.append(off)
                                    # wNe1 = DraftGeomUtils.offsetWire(wire, d1, wireNedge=True)
                                    # See https://github.com/FreeCAD/FreeCAD/issues/25485#issuecomment-3566734050
                                    # d1 may be Vector (0,0,0), offsetWire()
                                    # in draftgeoutils\offsets.py:-
                                    # v1 = App.Vector(dvec).normalize() return
                                    # error.  Provide widthList, alignList etc.
                                    # so no need to run above code to deduce
                                    # v1 in offsetWire()
                                    wNe1 = DraftGeomUtils.offsetWire(
                                        wire,
                                        dvec,
                                        widthList=curWidth,
                                        offsetMode="BasewireMode",
                                        alignList=alignListC,
                                        normal=normal,
                                        basewireOffset=offsetListC,
                                        wireNedge=True,
                                    )
                                    wNe2 = DraftGeomUtils.offsetWire(
                                        wire,
                                        dvec,
                                        widthList=curWidth,
                                        offsetMode=None,
                                        alignList=alignListC,
                                        normal=normal,
                                        basewireOffset=offsetListC,
                                        wireNedge=True,
                                    )
                                    layeroffset += abs(curWidth[0])
                                else:
                                    dvec.multiply(width)
                                    wNe2 = DraftGeomUtils.offsetWire(
                                        wire,
                                        dvec,
                                        bind=False,
                                        occ=False,
                                        widthList=widths,
                                        offsetMode=None,
                                        alignList=aligns,
                                        normal=normal,
                                        basewireOffset=offsets,
                                        wireNedge=True,
                                    )
                                    wNe1 = DraftGeomUtils.offsetWire(
                                        wire,
                                        dvec,
                                        bind=False,
                                        occ=False,
                                        widthList=widths,
                                        offsetMode="BasewireMode",
                                        alignList=aligns,
                                        normal=normal,
                                        basewireOffset=offsets,
                                        wireNedge=True,
                                    )
                            w2 = wNe2[0]
                            w1 = wNe1[0]
                            face = DraftGeomUtils.bind(w1, w2, per_segment=True)
                            cEdgesF2 = wNe2[1]
                            cEdges2 = wNe2[2]
                            oEdges2 = wNe2[3]
                            cEdgesF1 = wNe1[1]
                            cEdges1 = wNe1[2]
                            oEdges1 = wNe1[3]
                            self.connectEdges.extend(cEdges2)
                            self.connectEdges.extend(cEdges1)

                            del widths[0:edgeNum]
                            del aligns[0:edgeNum]
                            del offsets[0:edgeNum]

                            if face:

                                if layers and (layers[i] < 0):
                                    # layers with negative values are not drawn
                                    continue

                                if baseface:

                                    # To allow exportIFC.py to work properly on
                                    # sketch, which use only 1st face / wire,
                                    # do not fuse baseface here So for a sketch
                                    # with multiple wires, each returns
                                    # individual face (rather than fusing
                                    # together) for exportIFC.py to work
                                    # properly
                                    # "ArchWall - Based on Sketch Issues" - https://forum.freecad.org/viewtopic.php?f=39&t=31235

                                    # "Bug #2408: [PartDesign] .fuse is splitting edges it should not"
                                    # - https://forum.freecad.org/viewtopic.php?f=10&t=20349&p=346237#p346237
                                    # - bugtracker - https://freecad.org/tracker/view.php?id=2408

                                    # Try Part.Shell before removeSplitter
                                    # - https://forum.freecad.org/viewtopic.php?f=10&t=20349&start=10
                                    # - 1st finding : if a rectangle + 1 line, can't removesSplitter properly...
                                    # - 2nd finding : if 2 faces do not touch, can't form a shell; then, subsequently for remaining faces even though touch each faces, can't form a shell

                                    baseface.append(face)
                                    # The above make Refine methods below (in else) useless, regardless removeSpitters yet to be improved for cases do not work well
                                    """  Whether layers or not, all baseface.append(face) """

                                else:
                                    baseface = [face]

                                    """  Whether layers or not, all baseface = [face] """

                        if baseface:
                            base, placement = self.rebase(baseface)

                else:  # if not self.basewires:
                    FreeCAD.Console.PrintWarning(
                        translate(
                            "Arch",
                            f"No supported edges in Base object of {obj.Label} (line, circle, arc, ellipse)",
                        )
                        + "\n"
                    )

        # Build Wall from scratch if there is no obj.Base or even obj.Base is not valid
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
        """Return the global endpoints of the canonical wall baseline.

        The baseline resolver handles both based and baseless straight walls.
        Unsupported base topology produces an empty list rather than exposing
        local coordinates or making callers interpret the wall placement.
        """
        baseline = self.get_global_baseline(obj)
        if baseline:
            return [baseline.start_point, baseline.end_point]
        return []

    def set_from_endpoints(self, obj, pts):
        """Set a straight wall from two global points.

        Endpoint editing is defined in global coordinates and updates the
        wall's length, midpoint, and direction.  A straight based wall is
        debased first when the normal Arch wall rules allow it; unsupported or
        non-debasable bases are left unchanged.
        """
        if len(pts) < 2:
            return

        edit = ArchWallEndpoint.resolve_endpoint_edit(obj, pts)
        if edit is not None:
            ArchWallEndpoint.apply_endpoint_edit(obj, edit)

    def handleComponentRemoval(self, obj, subobject):
        """
        Overrides the default component removal to implement smart debasing
        when the Base object is being removed.
        """
        from PySide import QtGui

        # Check if the component being removed is this wall's Base
        if hasattr(obj, "Base") and obj.Base == subobject:
            if ArchWallEndpoint.is_debasable(obj):
                # This is a valid, single-line wall. Perform a clean debase.
                ArchWallEndpoint.debaseWall(obj)
            else:
                # This is a complex wall. Behavior depends on GUI availability.
                if FreeCAD.GuiUp:
                    # --- GUI Path: Warn the user and ask for confirmation. ---
                    from PySide import QtGui

                    msg_box = QtGui.QMessageBox()
                    msg_box.setWindowTitle(translate("ArchComponent", "Unsupported Base"))
                    msg_box.setText(
                        translate(
                            "ArchComponent",
                            "The base of this wall is not a single straight line.",
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
                        # User confirmed, perform the standard removal
                        super(_Wall, self).handleComponentRemoval(obj, subobject)
                else:
                    # --- Headless Path: Do not perform the destructive action. Print a warning. ---
                    FreeCAD.Console.PrintWarning(
                        f"Skipping removal of complex base for wall '{obj.Label}'. "
                        "This interactive action is not supported in headless mode.\n"
                    )
        else:
            # If it's not the base (e.g., an Addition), use the default behavior
            # from the parent Component class.
            super(_Wall, self).handleComponentRemoval(obj, subobject)

    def get_width(self, obj, widths=True):
        """Return the legacy wall width API while using resolved sources.

        With ``widths=False`` this returns the scalar wall default.  With
        ``widths=True`` it returns ``(default_width, override_widths)`` when
        segment values exist, or ``None`` otherwise.  The return shape and
        fallback behavior are retained for existing Arch callers.
        """
        default_width, _default_align, _default_offset = self._section_defaults(obj)
        if not widths:
            return default_width
        widths_list = self._resolved_section_lists(obj)[0]
        return None if not widths_list else (default_width, widths_list)

    def get_layers(self, obj):
        """Return the wall-global material stack used by shape generation.

        The legacy based-wall builder obtains one material stack for the wall
        and reuses it for every baseline segment.  Section resolution follows
        that same rule so its geometry cannot describe a different shape.
        """
        return self._resolve_material_layers(obj)

    def _resolve_section_values(self, obj):
        """Resolve the effective width, alignment, and offset sources.

        The returned lists are intentionally kept as the shape-builder input;
        ``get_resolved_section`` turns one indexed entry into immutable section
        geometry.  Base-provider data wins over wall overrides, matching the
        existing ArchSketch contract.
        """
        default_width, default_align, default_offset = self._section_defaults(obj)

        widths, aligns, offsets = self._resolved_section_lists(obj)

        width = self._section_value(widths, 0, default_width)
        align = self._section_value(aligns, 0, default_align, valid_values=self._SECTION_ALIGNMENTS)
        offset = self._section_value(offsets, 0, default_offset)
        return width, align, offset, widths

    def _resolve_section_value_at(self, obj, segment_index, section_lists=None):
        """Return width, alignment, and offset for one segment.

        A segment entry is selected from the already chosen provider or
        override lists.  Missing, zero, empty, or invalid entries fall back
        to the wall properties, preserving the short-list behavior of the
        legacy shape builder.
        """
        default_width, default_align, default_offset = self._section_defaults(obj)
        widths, aligns, offsets = section_lists or self._resolved_section_lists(obj)
        width = self._section_value(widths, segment_index, default_width)
        align = self._section_value(
            aligns, segment_index, default_align, valid_values=self._SECTION_ALIGNMENTS
        )
        offset = self._section_value(offsets, segment_index, default_offset)
        return width, align, offset

    def _resolved_section_lists(self, obj):
        """Return per-segment width, alignment, and offset source lists.

        Optional ArchSketch values take precedence over wall override
        properties.  If no provider or override list is available, the wall's
        Width, Align, and Offset properties become one-entry fallback lists.
        These lists remain raw resolution inputs; scalar fallback and material
        expansion happen in the section resolver.
        """
        provider_widths, provider_aligns, provider_offsets = self._base_section_values(obj)
        widths = provider_widths or list(obj.OverrideWidth)
        aligns = provider_aligns or list(obj.OverrideAlign)
        offsets = provider_offsets or list(obj.OverrideOffset)
        if not widths and obj.Width.Value:
            widths = [obj.Width.Value]
        if not aligns:
            aligns = [obj.Align]
        if not offsets:
            offsets = [obj.Offset.Value]
        return list(widths), list(aligns), list(offsets)

    def get_resolved_section(self, obj, segment_index=0):
        """Return the immutable section that the wall shape actually builds.

        Width, alignment, and offset are resolved for the requested segment.
        Material layers are resolved once from the wall's default width,
        matching the legacy builder's wall-global layer stack even when a
        segment has a width override.  Negative layers remain invisible
        construction layers while moving the cursor for following layers.
        """
        section_lists = self._resolved_section_lists(obj)
        widths, _aligns, _offsets = section_lists
        width, align, offset = self._resolve_section_value_at(obj, segment_index, section_lists)
        layers = self._resolve_material_layers(obj)
        if not layers:
            if not widths:
                return None
            layers = [width]

        total = sum(abs(layer) for layer in layers)
        if align == "Center":
            cursor = -total / 2.0
        elif align == "Left":
            cursor = -total - offset
        else:  # Right, and the legacy fallback for invalid values.
            cursor = offset

        resolved_layers = []
        for raw_thickness in layers:
            raw_thickness = float(raw_thickness)
            thickness = abs(raw_thickness)
            resolved_layers.append(
                ArchWallGeometry.WallSectionLayer(
                    raw_thickness=raw_thickness,
                    y_min=cursor,
                    y_max=cursor + thickness,
                )
            )
            cursor += thickness
        return ArchWallGeometry.WallSection(tuple(resolved_layers))

    def _base_section_values(self, obj):
        """Read optional section lists from an ArchSketch base provider.

        ArchSketch is an optional object protocol: its proxy may provide any
        of the three getters, and a missing getter contributes an empty list.
        The provider is considered only when ArchSketch data is selected on a
        wall with an ArchSketch base.
        """
        base = obj.Base
        if not obj.ArchSketchData or not base or Draft.getType(base) != "ArchSketch":
            return [], [], []
        proxy = base.Proxy
        kwargs = {"propSetUuid": self.ArchSkPropSetPickedUuid}
        return (
            proxy.getWidths(base, **kwargs) if hasattr(proxy, "getWidths") else [],
            proxy.getAligns(base, **kwargs) if hasattr(proxy, "getAligns") else [],
            proxy.getOffsets(base, **kwargs) if hasattr(proxy, "getOffsets") else [],
        )

    @staticmethod
    def _section_value(values, index, fallback, valid_values=None):
        """Select one list entry, applying the wall-level fallback rules."""
        if not values or index >= len(values):
            return fallback
        value = values[index]
        value = getattr(value, "Value", value)
        if value in (None, 0, ""):
            return fallback
        if valid_values is not None and value not in valid_values:
            return fallback
        return value

    @staticmethod
    def _section_defaults(obj):
        """Return the wall defaults used when no segment value is present."""
        return obj.Width.Value or 200.0, obj.Align, obj.Offset.Value

    @staticmethod
    def _resolve_material_layers(obj):
        """Resolve the material thickness stack used by every wall segment.

        Positive thicknesses are preserved.  Zero-thickness layers divide the
        remaining width equally, using the wall's default Width rather than a
        segment override because the existing shape builder is wall-global.
        Signed values are retained so negative layers can act as invisible
        cursor steps in ``WallSection``.
        """
        material = obj.Material
        if not material or not hasattr(material, "Materials"):
            return []
        raw_thicknesses = [float(value) for value in material.Thicknesses]
        if not raw_thicknesses:
            return []
        width = obj.Width.Value or 200.0
        rest_width = width - sum(abs(value) for value in raw_thicknesses)
        zero_count = sum(value == 0 for value in raw_thicknesses)
        variable_width = rest_width / zero_count if rest_width > 0 and zero_count else 0.0
        return [
            variable_width if value == 0 and variable_width else value for value in raw_thicknesses
        ]

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

                # only 1 wire (first) is supported
                # TODO - Can support multiple wires?

                # self.basewires was list of list of edges,
                # no matter Base is DWire, Sketch or else
                # See discussion - https://forum.freecad.org/viewtopic.php?t=86365
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
            if i % 2:  # odd
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

        # Use a small default for zero dimensions to ensure a valid shape can be created.
        safe_length = obj.Length.Value or 0.5
        section = self.get_resolved_section(obj)
        if section is None:
            return [], FreeCAD.Placement()

        # --- Calculate and Create Geometry ---
        base_faces = []

        # Loop through the already-resolved layers.  Invisible layers retain
        # their cursor position but never become faces.
        for layer in section.layers:
            if layer.visible:
                half_length = safe_length / 2
                face = _create_face_from_coords(half_length, layer.y_min, layer.y_max)
                base_faces.append(face)

        placement = FreeCAD.Placement()

        # Set basewires so blocks calculation can use the centerline edge.
        p1 = Vector(-safe_length / 2, 0, 0)
        p2 = Vector(safe_length / 2, 0, 0)
        self.basewires = [[Part.LineSegment(p1, p2).toShape()]]

        return base_faces, placement

    def get_global_baseline(self, obj):
        """Resolve one supported wall baseline into global coordinates.

        Based walls must expose exactly one straight edge.  Its semantic
        provider orientation is resolved before the wall placement is applied
        and copied into a fresh ``Part.Edge`` so relation code never has to
        interpret wall placement.
        A baseless wall is derived directly from its local length and
        placement.  Unsupported topology returns ``None``.
        """
        import Part
        import DraftGeomUtils

        base = obj.Base
        placement = obj.Placement
        if base:
            if not hasattr(base, "Shape") or len(base.Shape.Edges) != 1:
                return None
            source_edge = base.Shape.Edges[0]
            if source_edge.Curve.TypeId != "Part::GeomLine":
                return None
            points = [
                placement.multVec(point)
                for point in ArchWallEndpoint.get_oriented_base_points(base)
            ]
            if len(points) != 2 or points[0].distanceToPoint(points[1]) <= 1e-9:
                return None
            edge = Part.makeLine(points[0], points[1])
        else:
            half_length = obj.Length.Value / 2.0
            points = [
                placement.multVec(FreeCAD.Vector(-half_length, 0, 0)),
                placement.multVec(FreeCAD.Vector(half_length, 0, 0)),
            ]
            if points[0].distanceToPoint(points[1]) <= 1e-9:
                return None
            edge = Part.makeLine(points[0], points[1])

        if obj.Normal == Vector(0, 0, 0):
            local_normal = None
            if base and hasattr(base, "Shape"):
                local_normal = DraftGeomUtils.get_shape_normal(base.Shape)
            local_normal = local_normal or Vector(0, 0, 1)
        else:
            local_normal = Vector(obj.Normal)
        normal = placement.Rotation.multVec(local_normal)
        if normal.Length <= 1e-9:
            return None
        normal.normalize()
        if edge.Vertexes[0].Point.sub(edge.Vertexes[-1].Point).cross(normal).Length <= 1e-9:
            return None
        return ArchWallGeometry.WallBaseline(edge, normal, points[0], points[1])

    def process_endings(self, obj, base_solid, wall_placement, end_conditions=None):
        """Trim a wall solid using the winning end-condition providers.

        Each end is resolved independently from its normalized provider list.
        Relation extensions are applied to the construction solid before
        subshape processing.  This stage applies only the selected cutting
        planes to the already processed solid.
        """
        if base_solid.isNull():
            return base_solid

        solid_to_trim = base_solid
        min_tool_size = base_solid.BoundBox.DiagonalLength * 2
        if end_conditions is None:
            relation_endings = ArchWallRelationResolver.collect_wall_relation_endings(obj)
            end_conditions = {
                end_name: self._resolve_end_condition(obj, end_name, relation_endings)
                for end_name in ("Start", "End")
            }
        baseline = self.get_global_baseline(obj)
        if baseline is None:
            return solid_to_trim
        endpoints = [baseline.start_point, baseline.end_point]
        start_condition = end_conditions["Start"]
        if start_condition:
            solid_to_trim = ArchWallTrimming.apply_cutting_plane(
                obj,
                solid_to_trim,
                wall_placement,
                start_condition.placement,
                endpoints[1],
                min_tool_size,
                is_global=start_condition.is_global,
            )

        end_condition = end_conditions["End"]
        if end_condition:
            solid_to_trim = ArchWallTrimming.apply_cutting_plane(
                obj,
                solid_to_trim,
                wall_placement,
                end_condition.placement,
                endpoints[0],
                min_tool_size,
                is_global=end_condition.is_global,
            )
        return solid_to_trim

    def _resolve_end_condition(self, obj, end_name, relation_endings):
        conditions = [
            ArchWallEndCondition.WallEndCondition(
                source="Manual", placement=getattr(obj, "Ending" + end_name)
            )
        ]
        relation_condition = relation_endings.get(end_name)
        if relation_condition is not None:
            conditions.append(relation_condition)
        return ArchWallEndCondition.select_end_condition(
            conditions, getattr(obj, "EndConditionOrder" + end_name)
        )


if FreeCAD.GuiUp:

    class WallTaskPanel(ArchComponent.ComponentTaskPanel):
        def __init__(self, obj):
            ArchComponent.ComponentTaskPanel.__init__(self)
            self.obj = obj
            self.wallWidget = QtGui.QWidget()
            self.wallWidget.setWindowTitle(translate("Arch", "Wall Options"))

            layout = QtGui.QFormLayout(self.wallWidget)
            loader = FreeCADGui.UiLoader()

            # Length
            self.length = loader.createWidget("Gui::QuantitySpinBox")
            FreeCADGui.ExpressionBinding(self.length).bind(self.obj, "Length")
            self.length.setProperty("value", self.obj.Length)
            layout.addRow(translate("Arch", "Length"), self.length)

            # Width
            self.width = loader.createWidget("Gui::QuantitySpinBox")
            FreeCADGui.ExpressionBinding(self.width).bind(self.obj, "Width")
            self.width.setProperty("value", self.obj.Width)
            layout.addRow(translate("Arch", "Width"), self.width)

            # Height
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

            # Wall Options first, then Components (inherited self.form)
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
                # The wall is an assembly: it is built from additions only, yet it is not
                # strictly a baseless wall, since baseless walls are parametric.
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
                        if len(activematerials) == len(obj.Shape.Solids):
                            cols = []
                            for i, mat in enumerate(activematerials):
                                c = obj.ViewObject.ShapeColor
                                c = (
                                    c[0],
                                    c[1],
                                    c[2],
                                    1.0 - obj.ViewObject.Transparency / 100.0,
                                )
                                if "DiffuseColor" in mat.Material:
                                    if "(" in mat.Material["DiffuseColor"]:
                                        c = tuple(
                                            [
                                                float(f)
                                                for f in mat.Material["DiffuseColor"]
                                                .strip("()")
                                                .split(",")
                                            ]
                                        )
                                if "Transparency" in mat.Material:
                                    c = (
                                        c[0],
                                        c[1],
                                        c[2],
                                        1.0 - float(mat.Material["Transparency"]),
                                    )
                                cols.extend([c for j in range(len(obj.Shape.Solids[i].Faces))])
                            obj.ViewObject.DiffuseColor = cols
        ArchComponent.ViewProviderComponent.updateData(self, obj, prop)
        if len(obj.ViewObject.DiffuseColor) > 1:
            # force-reset colors if changed
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
            QtGui.QIcon(":/icons/Arch_Wall_Tree.svg"),
            translate("Arch", "Flip Direction"),
            menu,
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
