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

__title__ = "FreeCAD Window"
__author__ = "Yorik van Havre"
__url__ = "https://www.freecad.org"

## @package ArchWindow
#  \ingroup ARCH
#  \brief The Window object and tools
#
#  This module provides tools to build Window objects.
#  Windows are Arch objects obtained by extruding a series
#  of wires, and that can be inserted into other Arch objects,
#  by defining a volume that gets subtracted from them.

import os

import FreeCAD
import ArchCommands
import ArchComponent
import ArchWindowPresets
import Draft
import DraftVecUtils

from FreeCAD import Units
from FreeCAD import Vector
from draftutils import params
from draftutils.messages import _wrn

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

# presets
WindowPartTypes = ["Frame", "Solid panel", "Glass panel", "Louvre"]
WindowOpeningModes = [
    "None",
    "Arc 90",
    "Arc 90 inv",
    "Arc 45",
    "Arc 45 inv",
    "Arc 180",
    "Arc 180 inv",
    "Triangle",
    "Triangle inv",
    "Sliding",
    "Sliding inv",
]
WindowPresets = ArchWindowPresets.WindowPresets


def recolorize(attr):  # names is [docname,objname]
    """Recolorizes an object or a [documentname,objectname] list
    This basically calls the Proxy.colorize(obj) methods of objects that
    have one."""

    if isinstance(attr, list):
        if attr[0] in FreeCAD.listDocuments():
            doc = FreeCAD.getDocument(attr[0])
            obj = doc.getObject(attr[1])
            if obj:
                if obj.ViewObject:
                    if obj.ViewObject.Proxy:
                        obj.ViewObject.Proxy.colorize(obj)
    elif hasattr(attr, "ViewObject") and attr.ViewObject:
        obj = attr
        if hasattr(obj.ViewObject, "Proxy") and hasattr(obj.ViewObject.Proxy, "colorize"):
            obj.ViewObject.Proxy.colorize(obj)


class _Window(ArchComponent.Component):
    "The Window object"

    def __init__(self, obj):

        ArchComponent.Component.__init__(self, obj)
        self.Type = "Window"
        self.setProperties(obj)
        obj.IfcType = "Window"
        obj.MoveWithHost = True

        # Add features in the SketchArch External Add-on
        self.addSketchArchFeatures(obj)

    def addSketchArchFeatures(self, obj, linkObj=None, mode=None):
        """
        To add features in the SketchArch External Add-on  (https://github.com/paullee0/FreeCAD_SketchArch)
        -  import ArchSketchObject module, and
        -  set properties that are common to ArchObjects (including Links) and ArchSketch
           to support the additional features

        To install SketchArch External Add-on, see https://github.com/paullee0/FreeCAD_SketchArch#iv-install
        """

        try:
            import ArchSketchObject

            ArchSketchObject.ArchSketch.setPropertiesLinkCommon(self, obj, linkObj, mode)
        except:
            pass

    def setProperties(self, obj, mode=None):

        lp = obj.PropertiesList
        if not "Hosts" in lp:
            obj.addProperty(
                "App::PropertyLinkList",
                "Hosts",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The objects that host this window"),
                locked=True,
            )
        if not "WindowParts" in lp:
            obj.addProperty(
                "App::PropertyStringList",
                "WindowParts",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The components of this window"),
                locked=True,
            )
            obj.setEditorMode("WindowParts", 2)
        if not "HoleDepth" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "HoleDepth",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The depth of the hole that this window makes in its host object. If 0, the value will be calculated automatically.",
                ),
                locked=True,
            )
        if not "Subvolume" in lp:
            obj.addProperty(
                "App::PropertyLink",
                "Subvolume",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "An optional object that defines a volume to be subtracted from hosts of this window",
                ),
                locked=True,
            )
        if not "Width" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "Width",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The width of this window"),
                locked=True,
            )
        if not "Height" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "Height",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The height of this window"),
                locked=True,
            )
        if not "Normal" in lp:
            obj.addProperty(
                "App::PropertyVector",
                "Normal",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The normal direction of this window"),
                locked=True,
            )
        # Automatic Normal Reverse
        if not "AutoNormalReversed" in lp:
            obj.addProperty(
                "App::PropertyBool",
                "AutoNormalReversed",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "When normal direction is in auto mode (0,0,0), use reversed normal direction of the Base Sketch, i.e. -z.",
                ),
                locked=True,
            )
            if mode == "ODR":
                obj.AutoNormalReversed = False  # To maintain auto extrusion behaviour before introduction of this flag, this remains False if this is called by onDocumentRestored()
            elif mode == None:
                obj.AutoNormalReversed = True  # To enable new extrusion behaviour which is consistent with Window intuitive creation tool after introduction of this flag, this is set True.
        if not "Preset" in lp:
            obj.addProperty(
                "App::PropertyInteger",
                "Preset",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The preset number this window is based on"),
                locked=True,
            )
            obj.setEditorMode("Preset", 2)
        if not "Frame" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "Frame",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The frame depth of this window. Measured from front face to back face horizontally (i.e. perpendicular to the window elevation plane).",
                ),
                locked=True,
            )
        if not "Offset" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "Offset",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The offset size of this window"),
                locked=True,
            )
        if not "Area" in lp:
            obj.addProperty(
                "App::PropertyArea",
                "Area",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The area of this window"),
                locked=True,
            )
        if not "LouvreWidth" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "LouvreWidth",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The width of louvre elements"),
                locked=True,
            )
        if not "LouvreSpacing" in lp:
            obj.addProperty(
                "App::PropertyLength",
                "LouvreSpacing",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The space between louvre elements"),
                locked=True,
            )
        if not "Opening" in lp:
            obj.addProperty(
                "App::PropertyPercent",
                "Opening",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Opens the subcomponents that have a hinge defined"
                ),
                locked=True,
            )
        if not "HoleWire" in lp:
            obj.addProperty(
                "App::PropertyInteger",
                "HoleWire",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The number of the wire that defines the hole. If 0, the value will be calculated automatically",
                ),
                locked=True,
            )
        if not "SymbolPlan" in lp:
            obj.addProperty(
                "App::PropertyBool",
                "SymbolPlan",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "Shows plan opening symbols if available"),
                locked=True,
            )
        if not "SymbolElevation" in lp:
            obj.addProperty(
                "App::PropertyBool",
                "SymbolElevation",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "Show elevation opening symbols if available"),
                locked=True,
            )
        obj.setEditorMode("VerticalArea", 2)
        obj.setEditorMode("HorizontalArea", 2)
        obj.setEditorMode("PerimeterLength", 2)

        # SillHeight change related properties
        self.setSillProperties(obj)

    def setSillProperties(self, orgObj, linkObj=None):
        """Set properties which support SillHeight change.
        Support both Arch Window and Link of Arch Window.
        """

        if linkObj:
            obj = linkObj
        else:
            obj = orgObj

        prop = obj.PropertiesList

        # 'Sill' support
        if not "SillHeight" in prop:
            obj.addProperty(
                "App::PropertyLength",
                "SillHeight",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The height of this window's sill"),
                locked=True,
            )

        # Link has no Proxy, so needs to use PropertyPythonObject
        sillProp = ["baseSill", "basePosZ", "atthOffZ"]
        for i in sillProp:
            if i not in prop:
                obj.addProperty("App::PropertyPythonObject", i)

    def onDocumentRestored(self, obj):

        ArchComponent.Component.onDocumentRestored(self, obj)
        self.setProperties(obj, mode="ODR")

        # Add features in the SketchArch External Add-on
        self.addSketchArchFeatures(obj, mode="ODR")

        # TODO 2025.6.27 : Seems SillHeight already triggered onChanged() upon document restored - NO need codes below in onDocumentRestored()
        # Need to restore 'initial' settings as corresponding codes in onChanged() does upon object creation
        # self.baseSill = obj.SillHeight.Value
        # self.basePos = obj.Base.Placement.Base
        # self.atthOff = None
        # if hasattr(obj, 'AttachmentOffsetXyzAndRotation'):
        #    self.atthOff = obj.AttachmentOffsetXyzAndRotation.Base

        # Sill -> SillHeight property rename migration
        if hasattr(obj, "Sill"):
            obj.SillHeight = obj.Sill
            obj.setPropertyStatus("Sill", "-LockDynamic")
            obj.removeProperty("Sill")

    def loads(self, state):

        self.Type = "Window"

    def onBeforeChange(self, obj, prop):

        if prop in ["Base", "WindowParts", "Placement", "HoleDepth", "Height", "Width", "Hosts"]:
            setattr(self, prop, getattr(obj, prop))
        if prop in ["Height", "Width"] and obj.CloneOf is None:
            self.TouchOnShapeChange = True  # touch hosts after next "Shape" change

    def onChanged(self, obj, prop):

        self.hideSubobjects(obj, prop)
        if prop == "SillHeight":
            self.setSillProperties(obj)  # Can't wait until onDocumentRestored
            self.onSillHeightChanged(obj)
        elif not "Restore" in obj.State:
            if prop in [
                "Base",
                "WindowParts",
                "Placement",
                "HoleDepth",
                "Height",
                "Width",
                "Hosts",
                "Shape",
            ]:
                # anti-recursive loops, bc the base sketch will touch the Placement all the time
                touchhosts = False
                if prop == "Shape":
                    if hasattr(self, "TouchOnShapeChange") and self.TouchOnShapeChange:
                        self.TouchOnShapeChange = False
                        touchhosts = True
                elif hasattr(self, prop) and getattr(self, prop) != getattr(obj, prop):
                    touchhosts = True
                if touchhosts:
                    hosts = self.Hosts if hasattr(self, "Hosts") else []
                    hosts += obj.Hosts if hasattr(obj, "Hosts") else []
                    for host in set(hosts):  # use set to remove duplicates
                        # mark host to recompute so it can detect this object
                        host.touch()
            if prop in ["Width", "Height", "Frame"]:
                if obj.Base:
                    if hasattr(obj.Base, "Constraints") and (
                        prop in [c.Name for c in obj.Base.Constraints]
                    ):
                        val = getattr(obj, prop).Value
                        if val > 0:
                            obj.Base.setDatum(prop, val)
            else:
                ArchComponent.Component.onChanged(self, obj, prop)

    def buildShapes(self, obj):

        import Part
        import DraftGeomUtils
        import math

        self.sshapes = []
        self.vshapes = []
        shapes = []
        rotdata = None
        for i in range(int(len(obj.WindowParts) / 5)):
            wires = []
            hinge = None
            omode = None
            ssymbols = []
            vsymbols = []
            wstr = obj.WindowParts[(i * 5) + 2].split(",")
            for s in wstr:
                if "Wire" in s:
                    j = int(s[4:])
                    if obj.Base.Shape.Wires:
                        if len(obj.Base.Shape.Wires) >= j:
                            wires.append(obj.Base.Shape.Wires[j])
                elif "Edge" in s:
                    hinge = int(s[4:]) - 1
                elif "Mode" in s:
                    omode = int(s[4:])
                    if omode >= len(WindowOpeningModes):
                        # Ignore modes not listed in WindowOpeningModes
                        omode = None
            if wires:
                max_length = 0
                for w in wires:
                    if w.BoundBox.DiagonalLength > max_length:
                        max_length = w.BoundBox.DiagonalLength
                        ext = w
                wires.remove(ext)
                shape = Part.Face(ext)
                norm = None
                if hasattr(obj, "Normal"):  # TODO Any reason need this test?
                    if (
                        obj.Normal
                    ):  # TODO v=Vector(0,0,0), if v: print('true') - true: It always return True?  Why this test?
                        if not DraftVecUtils.isNull(obj.Normal):
                            norm = obj.Normal
                if not norm:
                    if not obj.AutoNormalReversed:
                        norm = shape.normalAt(
                            0, 0
                        )  # TODO Should use Sketch's normal, to avoid possible difference in edge direction of various wires, for consistency?
                    else:  # elif obj.AutoNormalReversed:
                        norm = obj.Base.getGlobalPlacement().Rotation.multVec(
                            FreeCAD.Vector(0, 0, 1)
                        )
                        norm = norm.negative()
                if hinge and omode:
                    opening = None
                    if hasattr(obj, "Opening"):
                        if obj.Opening:
                            opening = obj.Opening / 100.0
                    e = obj.Base.Shape.Edges[hinge]
                    ev1 = e.Vertexes[0].Point
                    ev2 = e.Vertexes[-1].Point
                    # choose the one with lowest z to draw the symbol
                    if ev2.z < ev1.z:
                        ev1, ev2 = ev2, ev1
                    # find the point most distant from the hinge
                    p = None
                    d = 0
                    for v in shape.Vertexes:
                        dist = v.Point.distanceToLine(ev1, ev2.sub(ev1))
                        if dist > d:
                            d = dist
                            p = v.Point
                    if p:
                        # bring that point to the level of ev1 if needed
                        chord = p.sub(ev1)
                        enorm = ev2.sub(ev1)
                        proj = DraftVecUtils.project(chord, enorm)
                        v1 = ev1
                        if proj.Length > 0:
                            # chord = p.sub(ev1.add(proj))
                            # p = v1.add(chord)
                            p = p.sub(proj)
                            chord = p.sub(ev1)
                        # calculate symbols
                        v4 = p.add(DraftVecUtils.scale(enorm, 0.5))
                        if omode == 1:  # Arc 90
                            v2 = v1.add(DraftVecUtils.rotate(chord, math.pi / 4, enorm))
                            v3 = v1.add(DraftVecUtils.rotate(chord, math.pi / 2, enorm))
                            ssymbols.append(Part.Arc(p, v2, v3).toShape())
                            ssymbols.append(Part.LineSegment(v3, v1).toShape())
                            vsymbols.append(Part.LineSegment(v1, v4).toShape())
                            vsymbols.append(Part.LineSegment(v4, ev2).toShape())
                            if opening:
                                rotdata = [v1, ev2.sub(ev1), 90 * opening]
                        elif omode == 2:  # Arc -90
                            v2 = v1.add(DraftVecUtils.rotate(chord, -math.pi / 4, enorm))
                            v3 = v1.add(DraftVecUtils.rotate(chord, -math.pi / 2, enorm))
                            ssymbols.append(Part.Arc(p, v2, v3).toShape())
                            ssymbols.append(Part.LineSegment(v3, v1).toShape())
                            vsymbols.append(Part.LineSegment(v1, v4).toShape())
                            vsymbols.append(Part.LineSegment(v4, ev2).toShape())
                            if opening:
                                rotdata = [v1, ev2.sub(ev1), -90 * opening]
                        elif omode == 3:  # Arc 45
                            v2 = v1.add(DraftVecUtils.rotate(chord, math.pi / 8, enorm))
                            v3 = v1.add(DraftVecUtils.rotate(chord, math.pi / 4, enorm))
                            ssymbols.append(Part.Arc(p, v2, v3).toShape())
                            ssymbols.append(Part.LineSegment(v3, v1).toShape())
                            vsymbols.append(Part.LineSegment(v1, v4).toShape())
                            vsymbols.append(Part.LineSegment(v4, ev2).toShape())
                            if opening:
                                rotdata = [v1, ev2.sub(ev1), 45 * opening]
                        elif omode == 4:  # Arc -45
                            v2 = v1.add(DraftVecUtils.rotate(chord, -math.pi / 8, enorm))
                            v3 = v1.add(DraftVecUtils.rotate(chord, -math.pi / 4, enorm))
                            ssymbols.append(Part.Arc(p, v2, v3).toShape())
                            ssymbols.append(Part.LineSegment(v3, v1).toShape())
                            vsymbols.append(Part.LineSegment(v1, v4).toShape())
                            vsymbols.append(Part.LineSegment(v4, ev2).toShape())
                            if opening:
                                rotdata = [v1, ev2.sub(ev1), -45 * opening]
                        elif omode == 5:  # Arc 180
                            v2 = v1.add(DraftVecUtils.rotate(chord, math.pi / 2, enorm))
                            v3 = v1.add(DraftVecUtils.rotate(chord, math.pi, enorm))
                            ssymbols.append(Part.Arc(p, v2, v3).toShape())
                            ssymbols.append(Part.LineSegment(v3, v1).toShape())
                            vsymbols.append(Part.LineSegment(v1, v4).toShape())
                            vsymbols.append(Part.LineSegment(v4, ev2).toShape())
                            if opening:
                                rotdata = [v1, ev2.sub(ev1), 180 * opening]
                        elif omode == 6:  # Arc -180
                            v2 = v1.add(DraftVecUtils.rotate(chord, -math.pi / 2, enorm))
                            v3 = v1.add(DraftVecUtils.rotate(chord, -math.pi, enorm))
                            ssymbols.append(Part.Arc(p, v2, v3).toShape())
                            ssymbols.append(Part.LineSegment(v3, v1).toShape())
                            vsymbols.append(Part.LineSegment(v1, v4).toShape())
                            vsymbols.append(Part.LineSegment(v4, ev2).toShape())
                            if opening:
                                rotdata = [ev1, ev2.sub(ev1), -180 * opening]
                        elif omode == 7:  # tri
                            v2 = v1.add(DraftVecUtils.rotate(chord, math.pi / 2, enorm))
                            ssymbols.append(Part.LineSegment(p, v2).toShape())
                            ssymbols.append(Part.LineSegment(v2, v1).toShape())
                            vsymbols.append(Part.LineSegment(v1, v4).toShape())
                            vsymbols.append(Part.LineSegment(v4, ev2).toShape())
                            if opening:
                                rotdata = [v1, ev2.sub(ev1), 90 * opening]
                        elif omode == 8:  # -tri
                            v2 = v1.add(DraftVecUtils.rotate(chord, -math.pi / 2, enorm))
                            ssymbols.append(Part.LineSegment(p, v2).toShape())
                            ssymbols.append(Part.LineSegment(v2, v1).toShape())
                            vsymbols.append(Part.LineSegment(v1, v4).toShape())
                            vsymbols.append(Part.LineSegment(v4, ev2).toShape())
                            if opening:
                                rotdata = [v1, ev2.sub(ev1), -90 * opening]
                        elif omode == 9:  # sliding
                            pass
                        elif omode == 10:  # -sliding
                            pass
                exv = FreeCAD.Vector()
                zov = FreeCAD.Vector()
                V = 0
                thk = obj.WindowParts[(i * 5) + 3]
                if "+V" in thk:
                    thk = thk[:-2]
                    V = obj.Frame.Value
                thk = float(thk) + V
                if thk:
                    exv = DraftVecUtils.scaleTo(norm, thk)
                    shape = shape.extrude(exv)
                    for w in wires:
                        f = Part.Face(w)
                        f = f.extrude(exv)
                        shape = shape.cut(f)
                if obj.WindowParts[(i * 5) + 4]:
                    V = 0
                    zof = obj.WindowParts[(i * 5) + 4]
                    if "+V" in zof:
                        zof = zof[:-2]
                        V = obj.Offset.Value
                    zof = float(zof) + V
                    if zof:
                        zov = DraftVecUtils.scaleTo(norm, zof)
                        shape.translate(zov)
                if hinge and omode and 0 < omode < 9:
                    if DraftVecUtils.angle(chord, norm, enorm) < 0:
                        if omode % 2 == 0:
                            zov = zov.add(exv)
                    else:
                        if omode % 2 == 1:
                            zov = zov.add(exv)
                    for symb in ssymbols:
                        symb.translate(zov)
                    for symb in vsymbols:
                        symb.translate(zov)
                    if rotdata:
                        rotdata[0] = rotdata[0].add(zov)
                if obj.WindowParts[(i * 5) + 1] == "Louvre":
                    if hasattr(obj, "LouvreWidth"):
                        if obj.LouvreWidth and obj.LouvreSpacing:
                            bb = shape.BoundBox
                            bb.enlarge(10)
                            step = obj.LouvreWidth.Value + obj.LouvreSpacing.Value
                            if step < bb.ZLength:
                                box = Part.makeBox(bb.XLength, bb.YLength, obj.LouvreSpacing.Value)
                                boxes = []
                                for i in range(int(bb.ZLength / step) + 1):
                                    b = box.copy()
                                    b.translate(
                                        FreeCAD.Vector(bb.XMin, bb.YMin, bb.ZMin + i * step)
                                    )
                                    boxes.append(b)
                                self.boxes = Part.makeCompound(boxes)
                                # rot = obj.Base.Placement.Rotation
                                # self.boxes.rotate(self.boxes.BoundBox.Center,rot.Axis,math.degrees(rot.Angle))
                                self.boxes.translate(
                                    shape.BoundBox.Center.sub(self.boxes.BoundBox.Center)
                                )
                                shape = shape.cut(self.boxes)
                if rotdata:
                    shape.rotate(rotdata[0], rotdata[1], rotdata[2])
                shapes.append(shape)
                self.sshapes.extend(ssymbols)
                self.vshapes.extend(vsymbols)
        return shapes

    def execute(self, obj):

        if self.clone(obj):
            clonedProxy = obj.CloneOf.Proxy
            if not (hasattr(clonedProxy, "sshapes") and hasattr(clonedProxy, "vshapes")):
                clonedProxy.buildShapes(obj.CloneOf)
            self.sshapes = clonedProxy.sshapes
            self.vshapes = clonedProxy.vshapes
            if hasattr(clonedProxy, "boxes"):
                self.boxes = clonedProxy.boxes
            return
        if not self.ensureBase(obj):
            return

        import Part
        import DraftGeomUtils
        import math

        pl = obj.Placement
        base = None
        self.sshapes = []
        self.vshapes = []
        if obj.Base:
            if hasattr(obj, "Shape"):
                if hasattr(obj, "WindowParts"):
                    if obj.WindowParts and (len(obj.WindowParts) % 5 == 0):
                        shapes = self.buildShapes(obj)
                        if shapes:
                            base = Part.makeCompound(shapes)
                    elif not obj.WindowParts:
                        if obj.Base.Shape.Solids:
                            base = obj.Base.Shape.copy()
                            # obj placement is already added by applyShape() below
                            # if not DraftGeomUtils.isNull(pl):
                            #    base.Placement = base.Placement.multiply(pl)
                    else:
                        print("Arch: Bad formatting of window parts definitions")

        base = self.processSubShapes(obj, base)
        if base:
            if not base.isNull():
                b = []
                if self.sshapes:
                    if hasattr(obj, "SymbolPlan"):
                        if obj.SymbolPlan:
                            b.extend(self.sshapes)
                    else:
                        b.extend(self.sshapes)
                if self.vshapes:
                    if hasattr(obj, "SymbolElevation"):
                        if obj.SymbolElevation:
                            b.extend(self.vshapes)
                    else:
                        b.extend(self.vshapes)
                if b:
                    base = Part.makeCompound([base] + b)
                    # base = Part.makeCompound([base]+self.sshapes+self.vshapes)
                self.applyShape(obj, base, pl, allowinvalid=True, allownosolid=True)
                obj.Placement = pl
        else:
            obj.Shape = Part.Shape()
        if hasattr(obj, "Area"):
            obj.Area = obj.Width.Value * obj.Height.Value

        self.executeSketchArchFeatures(obj)

    def executeSketchArchFeatures(self, obj, linkObj=None, index=None, linkElement=None):
        """
        To execute features in the SketchArch External Add-on  (https://github.com/paullee0/FreeCAD_SketchArch)
        -  import ArchSketchObject module, and
        -  execute features that are common to ArchObjects (including Links) and ArchSketch

        To install SketchArch External Add-on, see https://github.com/paullee0/FreeCAD_SketchArch#iv-install
        """

        # To execute features in SketchArch External Add-on
        try:
            import ArchSketchObject  # Why needed ? Should have try: addSketchArchFeatures() before !  Need 'per method' ?

            # Execute SketchArch Feature - Intuitive Automatic Placement for Arch Windows/Doors, Equipment etc.
            # see https://forum.freecad.org/viewtopic.php?f=23&t=50802
            ArchSketchObject.updateAttachmentOffset(obj, linkObj)
        except:
            pass

    def appLinkExecute(self, obj, linkObj, index, linkElement):
        """
        Default Link Execute method() -
        See https://forum.freecad.org/viewtopic.php?f=22&t=42184&start=10#p361124
        @realthunder added support to Links to run Linked Scripted Object's methods()
        """

        # SillHeight change support
        self.setSillProperties(obj, linkObj)

        # Add features in the SketchArch External Add-on
        self.addSketchArchFeatures(obj, linkObj)

        # Execute features in the SketchArch External Add-on
        self.executeSketchArchFeatures(obj, linkObj)

        # SillHeight change feature
        self.onSillHeightChanged(obj, linkObj)

    def onSillHeightChanged(self, orgObj, linkObj=None, index=None, linkElement=None):

        if linkObj:
            obj = linkObj
        else:
            obj = orgObj

        val = getattr(obj, "SillHeight").Value
        if (
            getattr(obj, "baseSill", None) is None
            and getattr(obj, "basePosZ", None) is None
            and getattr(obj, "atthOffZ", None) is None
        ):  # TODO Any cases only 1 or 2 are not None?
            obj.baseSill = val
            # Not to change Base's Placement, would change all Clones and
            # Link's disposition unexpectedly to users, undesirable.
            #
            # self.basePos = obj.Base.Placement.Base
            obj.basePosZ = obj.Placement.Base.z
            obj.atthOffZ = None
            if hasattr(obj, "AttachmentOffsetXyzAndRotation"):
                obj.atthOffZ = obj.AttachmentOffsetXyzAndRotation.Base.z
            return

        import ArchSketchObject  # Need to import per method

        host = None
        if obj.Hosts:
            host = obj.Hosts[0]
        if (
            hasattr(obj, "AttachToAxisOrSketch")
            and obj.AttachToAxisOrSketch == "Host"
            and host
            and Draft.getType(host.Base) == "ArchSketch"
            and hasattr(ArchSketchObject, "updateAttachmentOffset")
        ):
            SketchArch = True
        else:
            SketchArch = False

        # Keep track of change whether SketchArch is True or False (i.e.
        # even Window object is not currently parametrically attached to
        # a Wall or other Arch object at the moment).
        #
        # SketchArch or Not
        if hasattr(obj, "AttachmentOffsetXyzAndRotation"):
            objAttOff = obj.AttachmentOffsetXyzAndRotation
            objAttOff.Base.z = obj.atthOffZ + (obj.SillHeight.Value - obj.baseSill)
            obj.AttachmentOffsetXyzAndRotation = objAttOff
        if not SketchArch:
            # Not to change Base's Placement
            # obj.Base.Placement.Base.z = self.basePos.z + (obj.Sill.Value - self.baseSill)
            obj.Placement.Base.z = obj.basePosZ + (obj.SillHeight.Value - obj.baseSill)

    def getSubFace(self):
        "returns a subface for creation of subvolume for cutting in a base object"
        # creation of subface from HoleWire (getSubWire)
        raise NotImplementedError

    def getSubVolume(self, obj, plac=None, host=None):
        "returns a subvolume for cutting in a base object"

        # check if this is a clone or not, setup orig if positive
        orig = None
        if Draft.isClone(obj, "Window"):
            if hasattr(obj, "CloneOf"):  # TODO need to check this?
                orig = obj.CloneOf

        # TODO Why always need tests e.g. hasattr(obj,"Subvolme"), hasattr(obj,"ClonOf") etc.?

        # check if we have a custom subvolume
        if hasattr(obj, "Subvolume"):  # TODO To support Links
            if obj.Subvolume:
                if hasattr(obj.Subvolume, "Shape"):
                    if not obj.Subvolume.Shape.isNull():
                        sh = obj.Subvolume.Shape.copy()
                        pl = FreeCAD.Placement(sh.Placement)
                        pl = obj.Placement.multiply(pl)
                        if plac:
                            pl = plac.multiply(pl)
                        sh.Placement = pl
                        return sh

        # getting extrusion depth
        width = 0
        if hasattr(
            obj, "HoleDepth"
        ):  # if this is a clone, the original's HoleDepth is overridden if HoleDepth is set in the clone  # TODO To support Links
            if obj.HoleDepth.Value:
                width = obj.HoleDepth.Value
        if not width:
            if orig and hasattr(orig, "HoleDepth"):
                if orig.HoleDepth.Value:
                    width = orig.HoleDepth.Value
        if not width:
            if host and Draft.getType(host) == "Wall":
                # TODO More robust approach :  With ArchSketch, on which wall segment an ArchObject is attached to is declared by user and saved.
                #      The extrusion of each wall segment could be done per segment, and punch hole in the exact wall segment before fusing them all. No need to care about each wall segment thickness.
                # TODO Consider to turn below codes to getWidths/getSortedWidths() in ArchWall (below codes copied and modified from ArchWall)
                propSetUuid = host.Proxy.ArchSkPropSetPickedUuid
                widths = []  # [] or None are both False
                if (
                    hasattr(host, "ArchSketchData")
                    and host.ArchSketchData
                    and Draft.getType(host.Base) == "ArchSketch"
                ):
                    if hasattr(host.Base, "Proxy"):  # TODO Any need to test ?
                        if hasattr(host.Base.Proxy, "getWidths"):
                            # Return a list of Width corresponding to indexes
                            # of sorted edges of Sketch.
                            widths = host.Base.Proxy.getWidths(host.Base, propSetUuid=propSetUuid)
                if not widths:
                    if host.OverrideWidth:
                        # TODO No need to test as in ArchWall if host.Base is Sketch and sortSketchWidth(), just need the max value
                        widths = host.OverrideWidth
                    elif host.Width:
                        widths = [host.Width.Value]

                    # TODO Below codes copied and adopted from ArchWall.py.
                    #      Consider adding a variable to store the layer's
                    #      thickness as deduced, so the figure there could be
                    #      used directly without re-calculated here below.
                    if hasattr(host, "Material"):
                        if host.Material:
                            if hasattr(host.Material, "Materials"):
                                thicknesses = [abs(t) for t in host.Material.Thicknesses]
                                totalThk = sum(thicknesses)
                                # Append totalThk to widths, find max below
                                widths.append(totalThk)

                if widths:
                    width = max(widths)
                    # +100mm to ensure subtract is through at the moment
                    width += 100
            elif obj.Base:  # If host is not Wall
                b = obj.Base.Shape.BoundBox
                width = max(
                    b.XLength, b.YLength, b.ZLength
                )  # TODO Fix this, the width would be too much in many cases
        if (
            not width
        ):  # TODO Should not happen, it means there is no Base (Sketch or another FC object) in Clone either
            width = (
                1.1112  # some weird value to have little chance to overlap with an existing face
            )

        # setup base
        if orig:
            base = (
                orig.Base
            )  # always use original's base; clone's base should not be used to supersede original's base
        else:
            base = obj.Base

        # finding which wire to use to drill the hole
        f = None
        if hasattr(
            obj, "HoleWire"
        ):  # if this is a clone, the original's HoleWire is overridden if HoleWire is set in the clone  # TODO To support Links
            if obj.HoleWire > 0:
                if obj.HoleWire <= len(base.Shape.Wires):
                    f = base.Shape.Wires[obj.HoleWire - 1]
        if not f:
            if orig and hasattr(orig, "HoleDepth"):
                # check original's HoleWire
                if orig.HoleWire > 0:
                    if orig.HoleWire <= len(base.Shape.Wires):
                        f = base.Shape.Wires[obj.HoleWire - 1]
        if not f:
            # finding biggest wire in the base shape
            max_length = 0
            for w in base.Shape.Wires:
                if w.BoundBox.DiagonalLength > max_length:
                    max_length = w.BoundBox.DiagonalLength
                    f = w
        if f:
            import Part

            f = Part.Face(f)
            norm = f.normalAt(0, 0)
            if hasattr(obj, "Normal"):
                if obj.Normal:
                    if not DraftVecUtils.isNull(obj.Normal):
                        norm = obj.Normal
            v1 = DraftVecUtils.scaleTo(norm, width)
            f.translate(v1)
            v2 = v1.negative()
            v2 = Vector(v1).multiply(-2)
            f = f.extrude(v2)
            if plac:
                f.Placement = plac
            else:
                f.Placement = obj.Placement
            return f
        return None

    def computeAreas(self, obj):
        return


class _ViewProviderWindow(ArchComponent.ViewProviderComponent):
    "A View Provider for the Window object"

    def __init__(self, vobj):

        ArchComponent.ViewProviderComponent.__init__(self, vobj)

    def getIcon(self):

        import Arch_rc

        if hasattr(self, "Object"):
            if hasattr(self.Object, "CloneOf"):
                if self.Object.CloneOf:
                    return ":/icons/Arch_Window_Clone.svg"
        return ":/icons/Arch_Window_Tree.svg"

    def updateData(self, obj, prop):

        if prop == "Shape":
            if obj.Base:
                if obj.Base.isDerivedFrom("Part::Compound"):
                    if obj.ViewObject.DiffuseColor != obj.Base.ViewObject.DiffuseColor:
                        if len(obj.Base.ViewObject.DiffuseColor) > 1:
                            obj.ViewObject.DiffuseColor = obj.Base.ViewObject.DiffuseColor
                            obj.ViewObject.update()
            self.colorize(obj)
        elif prop == "CloneOf":
            if hasattr(obj, "CloneOf") and obj.CloneOf:
                mat = None
                if hasattr(obj, "Material"):
                    if obj.Material:
                        mat = obj.Material
                if not mat:
                    if obj.ViewObject.DiffuseColor != obj.CloneOf.ViewObject.DiffuseColor:
                        if len(obj.CloneOf.ViewObject.DiffuseColor) > 1:
                            obj.ViewObject.DiffuseColor = obj.CloneOf.ViewObject.DiffuseColor
                            obj.ViewObject.update()

    def onDelete(self, vobj, subelements):

        for o in vobj.Object.Hosts:
            o.touch()
        return True

    def onChanged(self, vobj, prop):

        if prop == "ShapeAppearance":
            self.colorize(vobj.Object)
        ArchComponent.ViewProviderComponent.onChanged(self, vobj, prop)

    def colorize(self, obj):

        def _shapeAppearanceMaterialIsSame(sapp_mat1, sapp_mat2):
            for prop in (
                "AmbientColor",
                "DiffuseColor",
                "EmissiveColor",
                "Shininess",
                "SpecularColor",
                "Transparency",
            ):
                if getattr(sapp_mat1, prop) != getattr(sapp_mat2, prop):
                    return False
            return True

        def _shapeAppearanceIsSame(sapp1, sapp2):
            if len(sapp1) != len(sapp2):
                return False
            for sapp_mat1, sapp_mat2 in zip(sapp1, sapp2):
                if not _shapeAppearanceMaterialIsSame(sapp_mat1, sapp_mat2):
                    return False
            return True

        if not obj.Shape:
            return
        if not obj.Shape.Solids:
            return

        # setting different part colors
        if hasattr(obj, "CloneOf") and obj.CloneOf:
            obj, clone = obj.CloneOf, obj
            base_sapp_mat = clone.ViewObject.ShapeAppearance[0]
            arch_mat = getattr(clone, "Material", None)
        else:
            clone = None
            base_sapp_mat = obj.ViewObject.ShapeAppearance[0]
            arch_mat = getattr(obj, "Material", None)

        solids = obj.Shape.copy().Solids
        sapp = []
        for i in range(len(solids)):
            color = None
            if obj.WindowParts and len(obj.WindowParts) > i * 5:
                # WindowParts-based window
                name = obj.WindowParts[(i * 5)]
                mtype = obj.WindowParts[(i * 5) + 1]
                color = self.getSolidMaterial(obj, arch_mat, name, mtype)
            elif obj.Base and hasattr(obj.Base, "Shape"):
                # Type-based window: obj.Base furnishes the window solids
                sol1 = self.getSolidSignature(solids[i])
                # here we look for all the ways to retrieve a name for each
                # solid. Currently we look for similar solids in the
                if hasattr(obj.Base, "Group"):
                    for child in obj.Base.Group:
                        if hasattr(child, "Shape") and child.Shape and child.Shape.Solids:
                            sol2 = self.getSolidSignature(child.Shape)
                            if sol1 == sol2:
                                color = self.getSolidMaterial(obj, arch_mat, child.Label)
                                break
            if color is None:
                typeidx = (i * 5) + 1
                if typeidx < len(obj.WindowParts):
                    typ = obj.WindowParts[typeidx]
                    if typ == WindowPartTypes[2]:  # "Glass panel"
                        color = ArchCommands.getDefaultColor("WindowGlass")

            if color is None:
                sapp_mat = base_sapp_mat
            else:
                # color is an RGBA tuple (0.0-1.0)
                sapp_mat = (
                    FreeCAD.Material()
                )  # ShapeAppearance material with default v0.21 properties.
                sapp_mat.DiffuseColor = color[:3] + (1.0,)
                sapp_mat.Transparency = 1.0 - color[3]
            sapp.extend((sapp_mat,) * len(solids[i].Faces))

        if clone is not None:
            obj = clone
        if not _shapeAppearanceIsSame(obj.ViewObject.ShapeAppearance, sapp):
            obj.ViewObject.ShapeAppearance = sapp

    def getSolidSignature(self, solid):
        """Returns a tuple defining as uniquely as possible a solid"""

        return (
            solid.ShapeType,
            round(solid.Volume, 3),
            round(solid.Area, 3),
            round(solid.Length, 3),
        )

    def getSolidMaterial(self, obj, arch_mat, name, mtype=None):
        """returns an RGBA tuple of floats (0.0 - 1.0)"""

        color = None
        if arch_mat is not None and hasattr(arch_mat, "Materials") and arch_mat.Names:
            mat = None
            if name in arch_mat.Names:
                mat = arch_mat.Materials[arch_mat.Names.index(name)]
            elif mtype is not None and (mtype in arch_mat.Names):
                mat = arch_mat.Materials[arch_mat.Names.index(mtype)]
            if mat:
                if "DiffuseColor" in mat.Material:
                    if "(" in mat.Material["DiffuseColor"]:
                        color = tuple(
                            [float(f) for f in mat.Material["DiffuseColor"].strip("()").split(",")]
                        )
                if color and ("Transparency" in mat.Material):
                    t = float(mat.Material["Transparency"]) / 100.0
                    color = color[:3] + (1.0 - t,)
        return color

    def getHingeEdgeIndices(self):
        """returns a list of hinge edge indices (0-based)"""

        # WindowParts example:
        # ["OuterFrame", "Frame",       "Wire0,Wire1",             "100.0+V", "0.00+V",
        #  "InnerFrame", "Frame",       "Wire2,Wire3,Edge8,Mode1", "100.0",   "100.0+V",
        #  "InnerGlass", "Glass panel", "Wire3",                   "10.0",    "150.0+V"]

        idxs = []
        parts = self.Object.WindowParts
        for i in range(len(parts) // 5):
            for s in parts[(i * 5) + 2].split(","):
                if "Edge" in s:
                    idxs.append(int(s[4:]) - 1)  # Edge indices in string are 1-based.
        return idxs

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None

        taskd = _ArchWindowTaskPanel()
        taskd.obj = self.Object
        self.sets = [vobj.DisplayMode, vobj.Transparency]
        vobj.DisplayMode = "Shaded"
        vobj.Transparency = 80
        if self.Object.Base:
            self.Object.Base.ViewObject.show()
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None

        vobj.DisplayMode = self.sets[0]
        vobj.Transparency = self.sets[1]
        vobj.DiffuseColor = vobj.DiffuseColor  # reset face colors
        if self.Object.Base:
            self.Object.Base.ViewObject.hide()
        FreeCADGui.Control.closeDialog()
        return True

    def setupContextMenu(self, vobj, menu):

        if FreeCADGui.activeWorkbench().name() != "BIMWorkbench":
            return

        hingeIdxs = self.getHingeEdgeIndices()

        super().contextMenuAddEdit(menu)

        if len(hingeIdxs) > 0:
            actionInvertOpening = QtGui.QAction(
                QtGui.QIcon(":/icons/Arch_Window_Tree.svg"),
                translate("Arch", "Invert Opening Direction"),
                menu,
            )
            QtCore.QObject.connect(
                actionInvertOpening, QtCore.SIGNAL("triggered()"), self.invertOpening
            )
            menu.addAction(actionInvertOpening)

        if len(hingeIdxs) == 1:
            actionInvertHinge = QtGui.QAction(
                QtGui.QIcon(":/icons/Arch_Window_Tree.svg"),
                translate("Arch", "Invert Hinge Position"),
                menu,
            )
            QtCore.QObject.connect(
                actionInvertHinge, QtCore.SIGNAL("triggered()"), self.invertHinge
            )
            menu.addAction(actionInvertHinge)

        super().contextMenuAddToggleSubcomponents(menu)

    def invertOpening(self):
        """swaps the opening modes found in this window"""

        pairs = [
            ["Mode" + str(i), "Mode" + str(i + 1)] for i in range(1, len(WindowOpeningModes), 2)
        ]
        self.invertPairs(pairs)

    def invertHinge(self):
        """swaps the hinge edge of a single hinge edge window"""

        idxs = self.getHingeEdgeIndices()
        if len(idxs) != 1:
            return

        idx = idxs[0]
        end = 0
        for wire in self.Object.Base.Shape.Wires:
            sta = end
            end += len(wire.Edges)
            if sta <= idx < end:
                new = idx + 2  # A rectangular wire is assumed.
                if not (sta <= new < end):
                    new = idx - 2
                break

        pairs = [["Edge" + str(idx + 1), "Edge" + str(new + 1)]]
        self.invertPairs(pairs)
        # Also invert opening direction, so the door still opens towards
        # the same side of the wall
        self.invertOpening()

    def invertPairs(self, pairs):
        """scans the WindowParts of this window and swaps the two elements of each pair, if found"""

        if hasattr(self, "Object"):
            windowparts = self.Object.WindowParts
            nparts = []
            for part in windowparts:
                for pair in pairs:
                    if pair[0] in part:
                        part = part.replace(pair[0], pair[1])
                        break
                    elif pair[1] in part:
                        part = part.replace(pair[1], pair[0])
                        break
                nparts.append(part)
            if nparts != self.Object.WindowParts:
                self.Object.WindowParts = nparts
                FreeCAD.ActiveDocument.recompute()
            else:
                FreeCAD.Console.PrintWarning(
                    translate("Arch", "This window has no defined opening") + "\n"
                )


class _ArchWindowTaskPanel:
    """The TaskPanel for Arch Windows"""

    def __init__(self):

        self.obj = None
        self.baseform = QtGui.QWidget()
        self.baseform.setObjectName("TaskPanel")
        self.grid = QtGui.QGridLayout(self.baseform)
        self.grid.setObjectName("grid")
        self.title = QtGui.QLabel(self.baseform)
        self.grid.addWidget(self.title, 0, 0, 1, 7)
        self.basepanel = ArchComponent.ComponentTaskPanel()
        self.form = [self.baseform, self.basepanel.baseform]

        # base object
        self.tree = QtGui.QTreeWidget(self.baseform)
        self.grid.addWidget(self.tree, 1, 0, 1, 7)
        self.tree.setColumnCount(1)
        self.tree.setMaximumSize(QtCore.QSize(500, 24))
        self.tree.header().hide()

        # hole
        self.holeLabel = QtGui.QLabel(self.baseform)
        self.grid.addWidget(self.holeLabel, 2, 0, 1, 1)

        self.holeNumber = QtGui.QLineEdit(self.baseform)
        self.grid.addWidget(self.holeNumber, 2, 2, 1, 3)

        self.holeButton = QtGui.QPushButton(self.baseform)
        self.grid.addWidget(self.holeButton, 2, 6, 1, 1)
        self.holeButton.setEnabled(True)

        # trees
        self.wiretree = QtGui.QTreeWidget(self.baseform)
        self.grid.addWidget(self.wiretree, 3, 0, 1, 3)
        self.wiretree.setColumnCount(1)
        self.wiretree.setSelectionMode(QtGui.QAbstractItemView.MultiSelection)

        self.comptree = QtGui.QTreeWidget(self.baseform)
        self.grid.addWidget(self.comptree, 3, 4, 1, 3)
        self.comptree.setColumnCount(1)

        # buttons
        self.addButton = QtGui.QPushButton(self.baseform)
        self.addButton.setObjectName("addButton")
        self.addButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.addButton, 4, 0, 1, 1)
        self.addButton.setMaximumSize(QtCore.QSize(70, 40))

        self.editButton = QtGui.QPushButton(self.baseform)
        self.editButton.setObjectName("editButton")
        self.editButton.setIcon(QtGui.QIcon(":/icons/Draft_Edit.svg"))
        self.grid.addWidget(self.editButton, 4, 2, 1, 3)
        self.editButton.setMaximumSize(QtCore.QSize(60, 40))
        self.editButton.setEnabled(False)

        self.delButton = QtGui.QPushButton(self.baseform)
        self.delButton.setObjectName("delButton")
        self.delButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.grid.addWidget(self.delButton, 4, 6, 1, 1)
        self.delButton.setMaximumSize(QtCore.QSize(70, 40))
        self.delButton.setEnabled(False)

        # invert buttons
        self.invertOpeningButton = QtGui.QPushButton(self.baseform)
        self.invertOpeningButton.setIcon(QtGui.QIcon(":/icons/Arch_Window_Tree.svg"))
        self.invertOpeningButton.clicked.connect(self.invertOpening)
        self.grid.addWidget(self.invertOpeningButton, 5, 0, 1, 7)
        self.invertOpeningButton.setEnabled(False)
        self.invertHingeButton = QtGui.QPushButton(self.baseform)
        self.invertHingeButton.setIcon(QtGui.QIcon(":/icons/Arch_Window_Tree.svg"))
        self.invertHingeButton.clicked.connect(self.invertHinge)
        self.grid.addWidget(self.invertHingeButton, 6, 0, 1, 7)
        self.invertHingeButton.setEnabled(False)

        # add new

        ui = FreeCADGui.UiLoader()
        self.newtitle = QtGui.QLabel(self.baseform)
        self.new1 = QtGui.QLabel(self.baseform)
        self.new2 = QtGui.QLabel(self.baseform)
        self.new3 = QtGui.QLabel(self.baseform)
        self.new4 = QtGui.QLabel(self.baseform)
        self.new5 = QtGui.QLabel(self.baseform)
        self.new6 = QtGui.QLabel(self.baseform)
        self.new7 = QtGui.QLabel(self.baseform)
        self.field1 = QtGui.QLineEdit(self.baseform)
        self.field2 = QtGui.QComboBox(self.baseform)
        self.field3 = QtGui.QLineEdit(self.baseform)
        self.field4 = ui.createWidget("Gui::InputField")
        self.field5 = ui.createWidget("Gui::InputField")
        self.field6 = QtGui.QPushButton(self.baseform)
        self.field7 = QtGui.QComboBox(self.baseform)
        self.addp4 = QtGui.QCheckBox(self.baseform)
        self.addp5 = QtGui.QCheckBox(self.baseform)
        self.createButton = QtGui.QPushButton(self.baseform)
        self.createButton.setObjectName("createButton")
        self.createButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.newtitle, 7, 0, 1, 7)
        self.grid.addWidget(self.new1, 8, 0, 1, 1)
        self.grid.addWidget(self.field1, 8, 2, 1, 5)
        self.grid.addWidget(self.new2, 9, 0, 1, 1)
        self.grid.addWidget(self.field2, 9, 2, 1, 5)
        self.grid.addWidget(self.new3, 10, 0, 1, 1)
        self.grid.addWidget(self.field3, 10, 2, 1, 5)
        self.grid.addWidget(self.new4, 11, 0, 1, 1)
        self.grid.addWidget(self.field4, 11, 2, 1, 4)
        self.grid.addWidget(self.addp4, 11, 6, 1, 1)
        self.grid.addWidget(self.new5, 12, 0, 1, 1)
        self.grid.addWidget(self.field5, 12, 2, 1, 4)
        self.grid.addWidget(self.addp5, 12, 6, 1, 1)
        self.grid.addWidget(self.new6, 13, 0, 1, 1)
        self.grid.addWidget(self.field6, 13, 2, 1, 5)
        self.grid.addWidget(self.new7, 14, 0, 1, 1)
        self.grid.addWidget(self.field7, 14, 2, 1, 5)
        self.grid.addWidget(self.createButton, 15, 0, 1, 7)
        self.newtitle.setVisible(False)
        self.new1.setVisible(False)
        self.new2.setVisible(False)
        self.new3.setVisible(False)
        self.new4.setVisible(False)
        self.new5.setVisible(False)
        self.new6.setVisible(False)
        self.new7.setVisible(False)
        self.field1.setVisible(False)
        self.field2.setVisible(False)
        for t in WindowPartTypes:
            self.field2.addItem("")
        self.field3.setVisible(False)
        self.field3.setReadOnly(True)
        self.field4.setVisible(False)
        self.field5.setVisible(False)
        self.field6.setVisible(False)
        self.field7.setVisible(False)
        self.addp4.setVisible(False)
        self.addp5.setVisible(False)
        for t in WindowOpeningModes:
            self.field7.addItem("")
        self.createButton.setVisible(False)

        QtCore.QObject.connect(self.holeButton, QtCore.SIGNAL("clicked()"), self.selectHole)
        QtCore.QObject.connect(
            self.holeNumber, QtCore.SIGNAL("textEdited(QString)"), self.setHoleNumber
        )
        QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        QtCore.QObject.connect(self.delButton, QtCore.SIGNAL("clicked()"), self.removeElement)
        QtCore.QObject.connect(self.editButton, QtCore.SIGNAL("clicked()"), self.editElement)
        QtCore.QObject.connect(self.createButton, QtCore.SIGNAL("clicked()"), self.create)
        QtCore.QObject.connect(
            self.comptree, QtCore.SIGNAL("itemClicked(QTreeWidgetItem*,int)"), self.check
        )
        QtCore.QObject.connect(
            self.wiretree, QtCore.SIGNAL("itemClicked(QTreeWidgetItem*,int)"), self.select
        )
        QtCore.QObject.connect(self.field6, QtCore.SIGNAL("clicked()"), self.addEdge)
        self.update()

        FreeCADGui.Selection.clearSelection()

    def isAllowedAlterSelection(self):

        return True

    def isAllowedAlterView(self):

        return True

    def getStandardButtons(self):

        return QtGui.QDialogButtonBox.Close

    def check(self, wid, col):

        self.editButton.setEnabled(True)
        self.delButton.setEnabled(True)

    def select(self, wid, col):

        FreeCADGui.Selection.clearSelection()
        ws = ""
        for it in self.wiretree.selectedItems():
            if ws:
                ws += ","
            ws += str(it.text(0))
            w = int(str(it.text(0)[4:]))
            if self.obj:
                if self.obj.Base:
                    edges = self.obj.Base.Shape.Wires[w].Edges
                    for e in edges:
                        for i in range(len(self.obj.Base.Shape.Edges)):
                            if e.hashCode() == self.obj.Base.Shape.Edges[i].hashCode():
                                FreeCADGui.Selection.addSelection(
                                    self.obj.Base, "Edge" + str(i + 1)
                                )
        self.field3.setText(ws)

    def selectHole(self):
        "takes a selected edge to determine current Hole Wire"

        s = FreeCADGui.Selection.getSelectionEx()
        if s and self.obj:
            if s[0].SubElementNames:
                if "Edge" in s[0].SubElementNames[0]:
                    for i, w in enumerate(self.obj.Base.Shape.Wires):
                        for e in w.Edges:
                            if e.hashCode() == s[0].SubObjects[0].hashCode():
                                self.holeNumber.setText(str(i + 1))
                                self.setHoleNumber(str(i + 1))
                                break

    def setHoleNumber(self, val):
        "sets the HoleWire obj property"

        if val.isdigit():
            val = int(val)
            if self.obj:
                if not hasattr(self.obj, "HoleWire"):
                    self.obj.addProperty(
                        "App::PropertyInteger",
                        "HoleWire",
                        "Arch",
                        QT_TRANSLATE_NOOP(
                            "App::Property",
                            "The number of the wire that defines the hole. A value of 0 means automatic",
                        ),
                        locked=True,
                    )
                self.obj.HoleWire = val

    def getIcon(self, obj):

        if hasattr(obj.ViewObject, "Proxy"):
            if hasattr(obj.ViewObject.Proxy, "getIcon"):
                return QtGui.QIcon(obj.ViewObject.Proxy.getIcon())
        elif obj.isDerivedFrom("Sketcher::SketchObject"):
            return QtGui.QIcon(":/icons/Sketcher_Sketch.svg")
        elif hasattr(obj.ViewObject, "Icon"):
            return QtGui.QIcon(obj.ViewObject.Icon)
        return QtGui.QIcon(":/icons/Part_3D_object.svg")

    def update(self):
        "fills the tree widgets"

        self.tree.clear()
        self.wiretree.clear()
        self.comptree.clear()
        if self.obj:
            if self.obj.Base:
                item = QtGui.QTreeWidgetItem(self.tree)
                item.setText(0, self.obj.Base.Name)
                item.setIcon(0, self.getIcon(self.obj.Base))
                if hasattr(self.obj.Base, "Shape"):
                    i = 0
                    for w in self.obj.Base.Shape.Wires:
                        if w.isClosed():
                            item = QtGui.QTreeWidgetItem(self.wiretree)
                            item.setText(0, "Wire" + str(i))
                            item.setIcon(0, QtGui.QIcon(":/icons/Draft_Draft.svg"))
                        i += 1
                if self.obj.WindowParts:
                    for p in range(0, len(self.obj.WindowParts), 5):
                        item = QtGui.QTreeWidgetItem(self.comptree)
                        item.setText(0, self.obj.WindowParts[p])
                        item.setIcon(0, QtGui.QIcon(":/icons/Part_3D_object.svg"))
                if hasattr(self.obj, "HoleWire"):
                    self.holeNumber.setText(str(self.obj.HoleWire))
                else:
                    self.holeNumber.setText("0")

            self.retranslateUi(self.baseform)
            self.basepanel.obj = self.obj
            self.basepanel.update()
            for wp in self.obj.WindowParts:
                if ("Edge" in wp) and ("Mode" in wp):
                    self.invertOpeningButton.setEnabled(True)
                    self.invertHingeButton.setEnabled(True)
                    break

    def addElement(self):
        "opens the component creation dialog"

        self.field1.setText("")
        self.field3.setText("")
        self.field4.setText("")
        self.field5.setText("")
        self.field6.setText(QtGui.QApplication.translate("Arch", "Get selected edge", None))
        self.field7.setCurrentIndex(0)
        self.addp4.setChecked(False)
        self.addp5.setChecked(False)
        self.newtitle.setVisible(True)
        self.new1.setVisible(True)
        self.new2.setVisible(True)
        self.new3.setVisible(True)
        self.new4.setVisible(True)
        self.new5.setVisible(True)
        self.new6.setVisible(True)
        self.new7.setVisible(True)
        self.field1.setVisible(True)
        self.field2.setVisible(True)
        self.field3.setVisible(True)
        self.field4.setVisible(True)
        self.field5.setVisible(True)
        self.field6.setVisible(True)
        self.field7.setVisible(True)
        self.addp4.setVisible(True)
        self.addp5.setVisible(True)
        self.createButton.setVisible(True)
        self.addButton.setEnabled(False)
        self.editButton.setEnabled(False)
        self.delButton.setEnabled(False)

    def removeElement(self):

        for it in self.comptree.selectedItems():
            comp = str(it.text(0))
            if self.obj:
                p = self.obj.WindowParts
                if comp in self.obj.WindowParts:
                    ind = self.obj.WindowParts.index(comp)
                    for i in range(5):
                        p.pop(ind)
                    self.obj.WindowParts = p
                    self.update()
                    self.editButton.setEnabled(False)
                    self.delButton.setEnabled(False)

    def editElement(self):

        for it in self.comptree.selectedItems():
            self.addElement()
            comp = str(it.text(0))
            if self.obj:
                if comp in self.obj.WindowParts:
                    ind = self.obj.WindowParts.index(comp)
                    self.field6.setText(
                        QtGui.QApplication.translate("Arch", "Get selected edge", None)
                    )
                    self.field7.setCurrentIndex(0)
                    for i in range(5):
                        f = getattr(self, "field" + str(i + 1))
                        t = self.obj.WindowParts[ind + i]
                        if i == 1:
                            # special behaviour for types
                            if t in WindowPartTypes:
                                f.setCurrentIndex(WindowPartTypes.index(t))
                            else:
                                f.setCurrentIndex(0)
                        elif i == 2:
                            wires = []
                            for l in t.split(","):
                                if "Wire" in l:
                                    wires.append(l)
                                elif "Edge" in l:
                                    self.field6.setText(l)
                                elif "Mode" in l:
                                    if int(l[4:]) < len(WindowOpeningModes):
                                        self.field7.setCurrentIndex(int(l[4:]))
                                    else:
                                        # Ignore modes not listed in WindowOpeningModes
                                        self.field7.setCurrentIndex(0)
                            if wires:
                                f.setText(",".join(wires))

                        elif i in [3, 4]:
                            if "+V" in t:
                                t = t[:-2]
                                if i == 3:
                                    self.addp4.setChecked(True)
                                else:
                                    self.addp5.setChecked(True)
                            else:
                                if i == 3:
                                    self.addp4.setChecked(False)
                                else:
                                    self.addp5.setChecked(False)
                            f.setProperty(
                                "text",
                                FreeCAD.Units.Quantity(float(t), FreeCAD.Units.Length).UserString,
                            )
                        else:
                            f.setText(t)

    def create(self):
        "adds a new component"

        # testing if fields are ok
        ok = True
        ar = []
        for i in range(5):
            if i == 1:  # type (1)
                n = getattr(self, "field" + str(i + 1)).currentIndex()
                if n in range(len(WindowPartTypes)):
                    t = WindowPartTypes[n]
                else:
                    # if type was not specified or is invalid, we set a default
                    t = WindowPartTypes[0]
            else:  # name (0)
                t = str(getattr(self, "field" + str(i + 1)).property("text"))
                if t in WindowPartTypes:
                    t = t + "_"  # avoiding part names similar to types
            if t == "":
                if not (i in [1, 5]):
                    ok = False
            else:
                if i > 2:  # thickness (3), offset (4)
                    try:
                        q = FreeCAD.Units.Quantity(t)
                        t = str(q.Value)
                        if i == 3:
                            if self.addp4.isChecked():
                                t += "+V"
                        if i == 4:
                            if self.addp5.isChecked():
                                t += "+V"
                    except (ValueError, TypeError):
                        ok = False
                elif i == 2:
                    # check additional opening parameters
                    hinge = self.field6.property("text")
                    n = self.field7.currentIndex()
                    if (hinge.startswith("Edge")) and (n > 0):
                        # remove accelerator added by Qt
                        hinge = hinge.replace("&", "")
                        t += "," + hinge + ",Mode" + str(n)
            ar.append(t)

        if ok:
            if self.obj:
                parts = self.obj.WindowParts
                if ar[0] in parts:
                    b = parts.index(ar[0])
                    for i in range(5):
                        parts[b + i] = ar[i]
                else:
                    parts.extend(ar)
                self.obj.WindowParts = parts
                self.update()
        else:
            FreeCAD.Console.PrintWarning(translate("Arch", "Unable to create component") + "\n")

        self.newtitle.setVisible(False)
        self.new1.setVisible(False)
        self.new2.setVisible(False)
        self.new3.setVisible(False)
        self.new4.setVisible(False)
        self.new5.setVisible(False)
        self.new6.setVisible(False)
        self.new7.setVisible(False)
        self.field1.setVisible(False)
        self.field2.setVisible(False)
        self.field3.setVisible(False)
        self.field4.setVisible(False)
        self.field5.setVisible(False)
        self.field6.setVisible(False)
        self.field7.setVisible(False)
        self.addp4.setVisible(False)
        self.addp5.setVisible(False)
        self.createButton.setVisible(False)
        self.addButton.setEnabled(True)

    def addEdge(self):

        for sel in FreeCADGui.Selection.getSelectionEx():
            for sub in sel.SubElementNames:
                if "Edge" in sub:
                    self.field6.setText(sub)
                    return

    def reject(self):

        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def retranslateUi(self, TaskPanel):

        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Window elements", None))
        self.holeLabel.setText(QtGui.QApplication.translate("Arch", "Hole wire", None))
        self.holeNumber.setToolTip(
            QtGui.QApplication.translate(
                "Arch",
                "The number of the wire that defines a hole in the host object. A value of zero will automatically adopt the largest wire",
                None,
            )
        )
        self.holeButton.setText(QtGui.QApplication.translate("Arch", "Pick Selected", None))
        self.delButton.setText(QtGui.QApplication.translate("Arch", "Remove", None))
        self.addButton.setText(QtGui.QApplication.translate("Arch", "Add", None))
        self.editButton.setText(QtGui.QApplication.translate("Arch", "Edit", None))
        self.createButton.setText(
            QtGui.QApplication.translate("Arch", "Create/Update Component", None)
        )
        self.title.setText(QtGui.QApplication.translate("Arch", "Base 2D object", None))
        self.wiretree.setHeaderLabels([QtGui.QApplication.translate("Arch", "Wires", None)])
        self.comptree.setHeaderLabels([QtGui.QApplication.translate("Arch", "Components", None)])
        self.newtitle.setText(QtGui.QApplication.translate("Arch", "Create new Component", None))
        self.new1.setText(QtGui.QApplication.translate("Arch", "Name", None))
        self.new2.setText(QtGui.QApplication.translate("Arch", "Type", None))
        self.new3.setText(QtGui.QApplication.translate("Arch", "Wires", None))
        self.new4.setText(QtGui.QApplication.translate("Arch", "Frame depth", None))
        self.new5.setText(QtGui.QApplication.translate("Arch", "Offset", None))
        self.new6.setText(QtGui.QApplication.translate("Arch", "Hinge", None))
        self.new7.setText(QtGui.QApplication.translate("Arch", "Opening mode", None))
        self.addp4.setText(QtGui.QApplication.translate("Arch", "+ Frame property", None))
        self.addp4.setToolTip(
            QtGui.QApplication.translate(
                "Arch",
                "If this is checked, the window's Frame property value will be added to the value entered here",
                None,
            )
        )
        self.addp5.setText(QtGui.QApplication.translate("Arch", "+ Offset property", None))
        self.addp5.setToolTip(
            QtGui.QApplication.translate(
                "Arch",
                "If this is checked, the window's Offset property value will be added to the value entered here",
                None,
            )
        )
        self.field6.setText(QtGui.QApplication.translate("Arch", "Get Selected Edge", None))
        self.field6.setToolTip(
            QtGui.QApplication.translate("Arch", "Press to retrieve the selected edge", None)
        )
        self.invertOpeningButton.setText(
            QtGui.QApplication.translate("Arch", "Invert Opening Direction", None)
        )
        self.invertHingeButton.setText(
            QtGui.QApplication.translate("Arch", "Invert Hinge Position", None)
        )
        for i in range(len(WindowPartTypes)):
            self.field2.setItemText(
                i, QtGui.QApplication.translate("Arch", WindowPartTypes[i], None)
            )
        for i in range(len(WindowOpeningModes)):
            self.field7.setItemText(
                i, QtGui.QApplication.translate("Arch", WindowOpeningModes[i], None)
            )

    def invertOpening(self):

        if self.obj:
            self.obj.ViewObject.Proxy.invertOpening()

    def invertHinge(self):

        if self.obj:
            self.obj.ViewObject.Proxy.invertHinge()
