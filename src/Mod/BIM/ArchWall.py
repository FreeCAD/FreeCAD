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
        if not hasattr(self, "ArchSkPropSetPickedUuid"):  # 'obj.Proxy', 'self' not works ?
            self.ArchSkPropSetPickedUuid = ""
        if not hasattr(self, "ArchSkPropSetListPrev"):
            self.ArchSkPropSetListPrev = []
        self.connectEdges = []

    def dumps(self):
        dump = super().dumps()
        if not isinstance(dump, tuple):
            dump = (dump,)  # Python Tuple With One Item
        dump = dump + (self.ArchSkPropSetPickedUuid, self.ArchSkPropSetListPrev)
        return dump

    def loads(self, state):
        self.Type = "Wall"
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
            bplates = extdata[0]
            extv = extdata[2].Rotation.multVec(extdata[1])
            if isinstance(bplates, list):
                shps = []
                # Test : if base is Sketch, then fuse all solid; otherwise, makeCompound
                sketchBaseToFuse = obj.Base.getLinkedObject().isDerivedFrom(
                    "Sketcher::SketchObject"
                )
                # but turn this off if we have layers, otherwise layers get merged
                if (
                    hasattr(obj, "Material")
                    and obj.Material
                    and hasattr(obj.Material, "Materials")
                    and obj.Material.Materials
                ):
                    sketchBaseToFuse = False
                for b in bplates:
                    b.Placement = extdata[2].multiply(b.Placement)
                    b = b.extrude(extv)

                    # See getExtrusionData() - not fusing baseplates there - fuse solids here
                    # Remarks - If solids are fused, but exportIFC.py use underlying baseplates w/o fuse, the result in ifc look slightly different from in FC.

                    if sketchBaseToFuse:
                        if shps:
                            shps = shps.fuse(b)  # shps.fuse(b)
                        else:
                            shps = b
                    else:
                        shps.append(b)
                    # TODO - To let user to select whether to fuse (slower) or to do a compound (faster) only ?

                if sketchBaseToFuse:
                    base = shps
                else:
                    base = Part.makeCompound(shps)
            else:
                bplates.Placement = extdata[2].multiply(bplates.Placement)
                base = bplates.extrude(extv)
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
                # blocks calculation
                elif hasattr(obj, "MakeBlocks") and hasattr(self, "basewires"):
                    if obj.MakeBlocks and self.basewires and extdata and obj.Width and obj.Height:
                        # print "calculating blocks"
                        if len(self.basewires) == 1:
                            blocks = []
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

                            if isinstance(bplates, list):
                                bplates = bplates[0]
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
                                plate1 = bplates.cut(cuts1).Faces
                            else:
                                plate1 = bplates.Faces
                            blocks1 = Part.makeCompound([f.extrude(bvec) for f in plate1])
                            if cuts2:
                                plate2 = bplates.cut(cuts2).Faces
                            else:
                                plate2 = bplates.Faces
                            blocks2 = Part.makeCompound([f.extrude(bvec) for f in plate2])
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
                                    b = Part.makeCompound([f.extrude(rvec) for f in plate2])
                                else:
                                    b = Part.makeCompound([f.extrude(rvec) for f in plate1])
                                t = FreeCAD.Vector(svec)
                                t.multiply(entire)
                                b.translate(t)
                                blocks.append(b)
                            if blocks:
                                base = Part.makeCompound(blocks)

                        else:
                            FreeCAD.Console.PrintWarning(
                                translate("Arch", "Cannot compute blocks for wall")
                                + obj.Label
                                + "\n"
                            )

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
        if not base:
            # FreeCAD.Console.PrintError(translate("Arch","Error: Invalid base object")+"\n")
            # return
            # walls can be made of only a series of additions and have no base shape
            base = Part.Shape()

        base = self.processSubShapes(obj, base, pl)

        self.applyShape(obj, base, pl)

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
        if self.connectEdges:
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

        # If ArchComponent.Component.getExtrusionData() can successfully get
        # extrusion data, just use that.
        data = ArchComponent.Component.getExtrusionData(self, obj)
        if data:
            if not isinstance(data[0], list):
                # multifuses not considered here
                return data
        length = obj.Length.Value
        # TODO currently layers were not supported when len(basewires) > 0	##( or 1 ? )
        width = 0
        # Get width of each edge segment from Base Objects if they store it
        # (Adding support in SketchFeaturePython, DWire...)
        widths = []  # [] or None are both False
        if (
            hasattr(obj, "ArchSketchData")
            and obj.ArchSketchData
            and Draft.getType(obj.Base) == "ArchSketch"
        ):
            if hasattr(obj.Base, "Proxy"):  # TODO Any need to test ?
                if hasattr(obj.Base.Proxy, "getWidths"):
                    # Return a list of Width corresponding to indexes of sorted
                    # edges of Sketch.
                    widths = obj.Base.Proxy.getWidths(obj.Base, propSetUuid=propSetUuid)
        # Get width of each edge/wall segment from ArchWall.OverrideWidth if
        # Base Object does not provide it
        if not widths:
            if obj.OverrideWidth:
                if obj.Base and obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                    # If Base Object is ordinary Sketch (or when ArchSketch.getWidth() not implemented yet):-
                    # sort the width list in OverrrideWidth to correspond to indexes of sorted edges of Sketch
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
                    # If Base Object is not Sketch, but e.g. DWire, the width
                    # list in OverrrideWidth just correspond to sequential
                    # order of edges
                    widths = obj.OverrideWidth
            elif obj.Width:
                widths = [obj.Width.Value]
            else:
                # having no width is valid for walls so the user doesn't need to be warned
                # it just disables extrusions and return none
                # print ("Width & OverrideWidth & base.getWidths() should not be all 0 or None or [] empty list ")
                return None

        # Set 'default' width - for filling in any item in the list == 0 or None
        if obj.Width.Value:
            width = obj.Width.Value
        else:
            width = 200  # 'Default' width value

        # Get align of each edge segment from Base Objects if they store it.
        # (Adding support in SketchFeaturePython, DWire...)
        aligns = []
        if (
            hasattr(obj, "ArchSketchData")
            and obj.ArchSketchData
            and Draft.getType(obj.Base) == "ArchSketch"
        ):
            if hasattr(obj.Base, "Proxy"):
                if hasattr(obj.Base.Proxy, "getAligns"):
                    # Return a list of Align corresponds to indexes of sorted
                    # edges of Sketch.
                    aligns = obj.Base.Proxy.getAligns(obj.Base, propSetUuid=propSetUuid)
        # Get align of each edge/wall segment from ArchWall.OverrideAlign if
        # Base Object does not provide it
        if not aligns:
            if obj.OverrideAlign:
                if obj.Base and obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                    # If Base Object is ordinary Sketch (or when
                    # ArchSketch.getAligns() not implemented yet):- sort the
                    # align list in OverrideAlign to correspond to indexes of
                    # sorted edges of Sketch
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
                    # If Base Object is not Sketch, but e.g. DWire, the align
                    # list in OverrideAlign just correspond to sequential order
                    # of edges
                    aligns = obj.OverrideAlign
            else:
                aligns = [obj.Align]

        # set 'default' align - for filling in any item in the list == 0 or None
        align = obj.Align  # or aligns[0]

        # Get offset of each edge segment from Base Objects if they store it
        # (Adding support in SketchFeaturePython, DWire...)
        offsets = []  # [] or None are both False
        if (
            hasattr(obj, "ArchSketchData")
            and obj.ArchSketchData
            and Draft.getType(obj.Base) == "ArchSketch"
        ):
            if hasattr(obj.Base, "Proxy"):
                if hasattr(obj.Base.Proxy, "getOffsets"):
                    # Return a list of Offset corresponding to indexes of sorted
                    # edges of Sketch.
                    offsets = obj.Base.Proxy.getOffsets(obj.Base, propSetUuid=propSetUuid)
        # Get offset of each edge/wall segment from ArchWall.OverrideOffset if
        # Base Object does not provide it
        if not offsets:
            if obj.OverrideOffset:
                if obj.Base and obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                    # If Base Object is ordinary Sketch (or when ArchSketch.getOffsets() not implemented yet):-
                    # sort the offset list in OverrideOffset to correspond to indexes of sorted edges of Sketch
                    if hasattr(ArchSketchObject, "sortSketchOffset"):
                        offsets = ArchSketchObject.sortSketchOffset(
                            obj.Base, obj.OverrideOffset, obj.ArchSketchEdges
                        )
                    else:
                        offsets = obj.OverrideOffset
                else:
                    # If Base Object is not Sketch, but e.g. DWire, the width
                    # list in OverrrideWidth just correspond to sequential
                    # order of edges
                    offsets = obj.OverrideOffset
            elif obj.Offset:
                offsets = [obj.Offset.Value]

        # Set 'default' offset - for filling in any item in the list == 0 or None
        offset = obj.Offset.Value  # could be 0

        height = obj.Height.Value
        if not height:
            height = self.getParentHeight(obj)
        if not height:
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

        # build wall layers
        layers = []
        if hasattr(obj, "Material"):
            if obj.Material:
                if hasattr(obj.Material, "Materials"):
                    thicknesses = [abs(t) for t in obj.Material.Thicknesses]
                    # multimaterials
                    varwidth = 0
                    restwidth = width - sum(thicknesses)
                    if restwidth > 0:
                        varwidth = [t for t in thicknesses if t == 0]
                        if varwidth:
                            varwidth = restwidth / len(varwidth)
                    for t in obj.Material.Thicknesses:
                        if t:
                            layers.append(t)
                        elif varwidth:
                            layers.append(varwidth)

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
                                    (Part.LineSegment, Part.Circle, Part.ArcOfCircle, Part.Ellipse),
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
                        clusters = Part.getSortedClusters(obj.Base.Shape.Edges)
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
                        else:
                            self.layersNum = 0
                        layeroffset = 0
                        baseface = None
                        self.connectEdges = []
                        for i, wire in enumerate(self.basewires):

                            # Check number of edges per 'wire' and get the 1st edge
                            if isinstance(wire, Part.Wire):
                                edgeNum = len(wire.Edges)
                                e = wire.Edges[0]
                            elif isinstance(wire[0], Part.Edge):
                                edgeNum = len(wire)
                                e = wire[0]

                            for n in range(
                                0, edgeNum, 1
                            ):  # why these not work - range(edgeNum), range(0,edgeNum) ...

                                # Fill the aligns list with ArchWall's default
                                # align entry and with same number of items as
                                # number of edges
                                try:
                                    if aligns[n] not in ["Left", "Right", "Center"]:
                                        aligns[n] = align
                                except Exception:
                                    aligns.append(align)

                                # Fill the widths List with ArchWall's default
                                # width entry and with same number of items as
                                # number of edges
                                try:
                                    if not widths[n]:
                                        widths[n] = width
                                except Exception:
                                    widths.append(width)
                                # Fill the offsets List with ArchWall's default
                                # offset entry and with same number of items as
                                # number of edges
                                try:
                                    if not offsets[n]:
                                        offsets[n] = offset
                                except Exception:
                                    offsets.append(offset)

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

        # Build Wall if there is no obj.Base or even obj.Base is not valid
        else:
            if layers:
                totalwidth = sum([abs(l) for l in layers])
                offset = 0
                base = []
                for l in layers:
                    if l > 0:
                        l2 = length / 2 or 0.5
                        w1 = -totalwidth / 2 + offset
                        w2 = w1 + l
                        v1 = Vector(-l2, w1, 0)
                        v2 = Vector(l2, w1, 0)
                        v3 = Vector(l2, w2, 0)
                        v4 = Vector(-l2, w2, 0)
                        base.append(Part.Face(Part.makePolygon([v1, v2, v3, v4, v1])))
                    offset += abs(l)
            else:
                l2 = length / 2 or 0.5
                w2 = width / 2 or 0.5
                v1 = Vector(-l2, -w2, 0)
                v2 = Vector(l2, -w2, 0)
                v3 = Vector(l2, w2, 0)
                v4 = Vector(-l2, w2, 0)
                base = Part.Face(Part.makePolygon([v1, v2, v3, v4, v1]))
            placement = FreeCAD.Placement()
        if base and placement:
            normal.normalize()
            extrusion = normal.multiply(height)
            if placement.Rotation.Angle > 0:
                extrusion = placement.inverse().Rotation.multVec(extrusion)
            return (base, extrusion, placement)
        return None

    def handleComponentRemoval(self, obj, subobject):
        """
        Overrides the default component removal to implement smart debasing
        when the Base object is being removed.
        """
        import Arch
        from PySide import QtGui

        # Check if the component being removed is this wall's Base
        if hasattr(obj, "Base") and obj.Base == subobject:
            if Arch.is_debasable(obj):
                # This is a valid, single-line wall. Perform a clean debase.
                Arch.debaseWall(obj)
            else:
                # This is a complex wall. Behavior depends on GUI availability.
                if FreeCAD.GuiUp:
                    # --- GUI Path: Warn the user and ask for confirmation. ---
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
            elif (not self.Object.Base) and self.Object.Additions:
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
                                c = (c[0], c[1], c[2], 1.0 - obj.ViewObject.Transparency / 100.0)
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
