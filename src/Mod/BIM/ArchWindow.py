# ***************************************************************************
# *   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
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

import os

import FreeCAD
import ArchCommands
import ArchComponent
import Draft
import DraftVecUtils
import ArchWindowPresets
from FreeCAD import Units
from FreeCAD import Vector
from draftutils import params
from draftutils.messages import _wrn
import FreeCADGui

if FreeCAD.GuiUp:
    from PySide import QtCore, QtGui
    import draftguitools.gui_trackers as DraftTrackers

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

# \endcond

## @package ArchWindow
#  \ingroup ARCH
#  \brief The Window object and tools
#
#  This module provides tools to build Window objects.
#  Windows are Arch objects obtained by extruding a series
#  of wires, and that can be inserted into other Arch objects,
#  by defining a volume that gets subtracted from them.

__title__ = "FreeCAD Window"
__author__ = "Yorik van Havre"
__url__ = "https://www.freecad.org"

# presets
WindowPartTypes = ["Frame", "Solid panel", "Glass panel", "Louvre", "Custom"]
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

# WindowParts example:
# ["OuterFrame", "Frame,Sketch2", "Wire0,Wire1",             "100.0+V", "0.00+V",
#  "InnerFrame", "Frame",         "Wire2,Wire3,Edge8,Mode1", "100.0",   "100.0+V",
#  "InnerGlass", "Glass panel",   "Wire3,ParentInnerFrame",  "10.0",    "150.0+V"]


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
        if hasattr(obj.ViewObject, "Proxy") and hasattr(
            obj.ViewObject.Proxy, "colorize"
        ):
            obj.ViewObject.Proxy.colorize(obj)


class Window(ArchComponent.Component):
    "The Window object"

    def __init__(self, obj):

        ArchComponent.Component.__init__(self, obj)
        self.setProperties(obj)
        obj.IfcType = "Window"
        obj.MoveWithHost = True

        # Add features in the SketchArch External Add-on
        self.addSketchArchFeatures(obj)

    def addSketchArchFeatures(self, obj, linkObj=None, mode=None):
        """
        To add features in the SketchArch External Add-on
        (https://github.com/paullee0/FreeCAD_SketchArch)
        -  import ArchSketchObject module, and
        -  set properties that are common to ArchObjects (including Links) and ArchSketch
           to support the additional features

        To install SketchArch External Add-on, see
        https://github.com/paullee0/FreeCAD_SketchArch#iv-install
        """

        try:
            import ArchSketchObject

            ArchSketchObject.ArchSketch.setPropertiesLinkCommon(
                self, obj, linkObj, mode
            )
        except:
            pass

    def setProperties(self, obj):

        lp = obj.PropertiesList
        if "Hosts" not in lp:
            obj.addProperty(
                "App::PropertyLinkList",
                "Hosts",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The objects that host this window"),
            )
        if "WindowParts" not in lp:
            obj.addProperty(
                "App::PropertyStringList",
                "WindowParts",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The components of this window"),
            )
            obj.setEditorMode("WindowParts", 2)
        if "HoleDepth" not in lp:
            obj.addProperty(
                "App::PropertyLength",
                "HoleDepth",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The depth of the hole that this window makes in its host object."
                    "If 0, the value will be calculated automatically.",
                ),
            )
        if "Subvolume" not in lp:
            obj.addProperty(
                "App::PropertyLink",
                "Subvolume",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "An optional object that defines a volume"
                    "to be subtracted from hosts of this window",
                ),
            )
        if "Width" not in lp:
            obj.addProperty(
                "App::PropertyLength",
                "Width",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The width of this window"),
            )
        if "Height" not in lp:
            obj.addProperty(
                "App::PropertyLength",
                "Height",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The height of this window"),
            )
        if "Normal" not in lp:
            obj.addProperty(
                "App::PropertyVector",
                "Normal",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The normal direction of this window"
                ),
            )
        if "Preset" not in lp:
            obj.addProperty(
                "App::PropertyInteger",
                "Preset",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The preset number this window is based on"
                ),
            )
            obj.setEditorMode("Preset", 2)
        if "Frame" not in lp:
            obj.addProperty(
                "App::PropertyLength",
                "Frame",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The frame size of this window"),
            )
        if "Offset" not in lp:
            obj.addProperty(
                "App::PropertyLength",
                "Offset",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The offset size of this window"),
            )
        if "Area" not in lp:
            obj.addProperty(
                "App::PropertyArea",
                "Area",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The area of this window"),
            )
        if "LouvreWidth" not in lp:
            obj.addProperty(
                "App::PropertyLength",
                "LouvreWidth",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The width of louvre elements"),
            )
        if "LouvreSpacing" not in lp:
            obj.addProperty(
                "App::PropertyLength",
                "LouvreSpacing",
                "Window",
                QT_TRANSLATE_NOOP("App::Property", "The space between louvre elements"),
            )
        if "Opening" not in lp:
            obj.addProperty(
                "App::PropertyPercent",
                "Opening",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Opens the subcomponents that have a hinge defined"
                ),
            )
        if "HoleWire" not in lp:
            obj.addProperty(
                "App::PropertyInteger",
                "HoleWire",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The number of the wire that defines the hole."
                    "If 0, the value will be calculated automatically",
                ),
            )
        if "SymbolPlan" not in lp:
            obj.addProperty(
                "App::PropertyBool",
                "SymbolPlan",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Shows plan opening symbols if available"
                ),
            )
        if "SymbolElevation" not in lp:
            obj.addProperty(
                "App::PropertyBool",
                "SymbolElevation",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Show elevation opening symbols if available"
                ),
            )
        if "SymbolAnnotations" not in lp:
            obj.addProperty(
                "App::PropertyLinkList",
                "SymbolAnnotations",
                "Window",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "A list of shapes or texts that represent additional"
                    "symbols to be added to this window",
                ),
            )
        obj.setEditorMode("VerticalArea", 2)
        obj.setEditorMode("HorizontalArea", 2)
        obj.setEditorMode("PerimeterLength", 2)
        self.Type = "Window"

    def onDocumentRestored(self, obj):

        ArchComponent.Component.onDocumentRestored(self, obj)
        self.setProperties(obj)

        # Add features in the SketchArch External Add-on
        self.addSketchArchFeatures(obj, mode="ODR")

    def onBeforeChange(self, obj, prop):

        if prop in [
            "Base",
            "WindowParts",
            "Placement",
            "HoleDepth",
            "Height",
            "Width",
            "Hosts",
        ]:
            setattr(self, prop, getattr(obj, prop))
        if prop in ["Height", "Width"] and obj.CloneOf is None:
            self.TouchOnShapeChange = True  # touch hosts after next "Shape" change

    def onChanged(self, obj, prop):

        self.hideSubobjects(obj, prop)
        if "Restore" not in obj.State:
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

    def get_float(self, val):
        """Returns a float from the given value"""

        return FreeCAD.Units.Quantity(val.strip()).Value

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
                norm = shape.normalAt(0, 0)
                if hasattr(obj, "Normal"):
                    if obj.Normal:
                        if not DraftVecUtils.isNull(obj.Normal):
                            norm = obj.Normal
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
                            v2 = v1.add(
                                DraftVecUtils.rotate(chord, -math.pi / 4, enorm)
                            )
                            v3 = v1.add(
                                DraftVecUtils.rotate(chord, -math.pi / 2, enorm)
                            )
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
                            v2 = v1.add(
                                DraftVecUtils.rotate(chord, -math.pi / 8, enorm)
                            )
                            v3 = v1.add(
                                DraftVecUtils.rotate(chord, -math.pi / 4, enorm)
                            )
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
                            v2 = v1.add(
                                DraftVecUtils.rotate(chord, -math.pi / 2, enorm)
                            )
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
                            v2 = v1.add(
                                DraftVecUtils.rotate(chord, -math.pi / 2, enorm)
                            )
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
                thk = self.get_float(thk) + V
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
                    zof = self.get_float(zof) + V
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
                                box = Part.makeBox(
                                    bb.XLength, bb.YLength, obj.LouvreSpacing.Value
                                )
                                boxes = []
                                for i in range(int(bb.ZLength / step) + 1):
                                    b = box.copy()
                                    b.translate(
                                        FreeCAD.Vector(
                                            bb.XMin, bb.YMin, bb.ZMin + i * step
                                        )
                                    )
                                    boxes.append(b)
                                self.boxes = Part.makeCompound(boxes)
                                self.boxes.translate(
                                    shape.BoundBox.Center.sub(
                                        self.boxes.BoundBox.Center
                                    )
                                )
                                shape = shape.cut(self.boxes)
                if rotdata:
                    shape.rotate(rotdata[0], rotdata[1], rotdata[2])
                shapes.append(shape)
                shapes.extend(self.add_annotations(obj))
                self.sshapes.extend(ssymbols)
                self.vshapes.extend(vsymbols)
        return shapes

    def execute(self, obj):

        if self.clone(obj):
            clonedProxy = obj.CloneOf.Proxy
            if not (
                hasattr(clonedProxy, "sshapes") and hasattr(clonedProxy, "vshapes")
            ):
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

    def executeSketchArchFeatures(
        self, obj, linkObj=None, index=None, linkElement=None
    ):
        """
        To execute features in the SketchArch External Add-on
        (https://github.com/paullee0/FreeCAD_SketchArch)
        -  import ArchSketchObject module, and
        -  execute features that are common to ArchObjects (including Links) and ArchSketch

        To install SketchArch External Add-on, see
        https://github.com/paullee0/FreeCAD_SketchArch#iv-install
        """

        # To execute features in SketchArch External Add-on
        try:
            # Why needed ? Should have try: addSketchArchFeatures() before !  Need 'per method' ?
            import ArchSketchObject

            # Execute SketchArch Feature - Intuitive Automatic Placement
            # for Arch Windows/Doors, Equipment etc.
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

        # Add features in the SketchArch External Add-on
        self.addSketchArchFeatures(obj, linkObj)

        # Execute features in the SketchArch External Add-on
        self.executeSketchArchFeatures(obj, linkObj)

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
        ):  # if this is a clone, the original's HoleDepth is overridden if HoleDepth is
            # set in the clone  # TODO To support Links
            if obj.HoleDepth.Value:
                width = obj.HoleDepth.Value
        if not width:
            if orig and hasattr(orig, "HoleDepth"):
                if orig.HoleDepth.Value:
                    width = orig.HoleDepth.Value
        if not width:
            if host and Draft.getType(host) == "Wall":
                # TODO More robust approach :  With ArchSketch, on which wall segment
                # an ArchObject is attached to is declared by user and saved.
                #      The extrusion of each wall segment could be done per segment, and
                # punch hole in the exact wall segment before fusing them all. No need to care
                # about each wall segment thickness.
                # TODO Consider to turn below codes to getWidths/getSortedWidths() in ArchWall
                # (below codes copied and modified from ArchWall)
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
                            widths = host.Base.Proxy.getWidths(
                                host.Base, propSetUuid=propSetUuid
                            )
                if not widths:
                    if host.OverrideWidth:
                        # TODO No need to test as in ArchWall if host.Base is Sketch
                        # and sortSketchWidth(), just need the max value
                        widths = host.OverrideWidth
                    elif host.Width:
                        widths = [host.Width.Value]
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
        ):  # TODO Should not happen, it means there is no Base (Sketch or
            # another FC object) in Clone either
            width = 1.1112
            # some weird value to have little chance
            # to overlap with an existing face

        # setup base
        if orig:
            base = (
                orig.Base
            )
            # always use original's base; clone's base should not
            # be used to supersede original's base
        else:
            base = obj.Base

        # finding which wire to use to drill the hole
        f = None
        if hasattr(
            obj, "HoleWire"
        ):  # if this is a clone, the original's HoleWire is overridden if HoleWire is set in
            # the clone  # TODO To support Links
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

    def add_annotations(self, obj):
        """Adds annotation shapes to this object"""

        shapes = []
        if hasattr(obj.ViewObject, "Annotation"):
            obj.ViewObject.Annotation.removeAllChildren()
        for ann in getattr(obj, "SymbolAnnotations", []):
            shape = getattr(ann, "Shape", None)
            if shape:
                shape = shape.copy()
                shape.Placement = shape.Placement.multiply(obj.Placement)
                shapes.append(shape)
            elif hasattr(ann, "Text"):
                if hasattr(ann.ViewObject, "RootNode"):
                    node = ann.ViewObject.RootNode.copy()
                    obj.ViewObject.Annotation.addChild(node)
        return shapes


class ViewProviderWindow(ArchComponent.ViewProviderComponent):
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
                            obj.ViewObject.DiffuseColor = (
                                obj.Base.ViewObject.DiffuseColor
                            )
                            obj.ViewObject.update()
            self.colorize(obj)
        elif prop == "CloneOf":
            if hasattr(obj, "CloneOf") and obj.CloneOf:
                mat = None
                if hasattr(obj, "Material"):
                    if obj.Material:
                        mat = obj.Material
                if not mat:
                    if (
                        obj.ViewObject.DiffuseColor
                        != obj.CloneOf.ViewObject.DiffuseColor
                    ):
                        if len(obj.CloneOf.ViewObject.DiffuseColor) > 1:
                            obj.ViewObject.DiffuseColor = (
                                obj.CloneOf.ViewObject.DiffuseColor
                            )
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
                        if (
                            hasattr(child, "Shape")
                            and child.Shape
                            and child.Shape.Solids
                        ):
                            sol2 = self.getSolidSignature(child.Shape)
                            if sol1 == sol2:
                                color = self.getSolidMaterial(
                                    obj, arch_mat, child.Label
                                )
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
                            [
                                float(f)
                                for f in mat.Material["DiffuseColor"]
                                .strip("()")
                                .split(",")
                            ]
                        )
                if color and ("Transparency" in mat.Material):
                    t = float(mat.Material["Transparency"]) / 100.0
                    color = color[:3] + (t,)
        return color

    def getHingeEdgeIndices(self):
        """returns a list of hinge edge indices (0-based)"""

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

        import ArchWindowTaskPanel

        self.tasks = ArchWindowTaskPanel.window_task_panel(vobj.Object)
        self.sets = [vobj.DisplayMode, vobj.Transparency]
        vobj.DisplayMode = "Shaded"
        vobj.Transparency = 80
        if self.Object.Base:
            self.Object.Base.ViewObject.show()
        FreeCADGui.Control.showDialog(self.tasks)
        return True

    def unsetEdit(self, vobj, mode=0):
        if mode != 0:
            return None

        vobj.DisplayMode = self.sets[0]
        vobj.Transparency = self.sets[1]
        vobj.DiffuseColor = vobj.DiffuseColor  # reset face colors
        if self.Object.Base:
            self.Object.Base.ViewObject.hide()
        FreeCADGui.Control.closeDialog()
        if hasattr(self, "tasks"):
            del self.tasks
        return True

    def setupContextMenu(self, vobj, menu):

        if FreeCADGui.activeWorkbench().name() != "BIMWorkbench":
            return

        hingeIdxs = self.getHingeEdgeIndices()

        super().contextMenuAddEdit(menu)

        if len(hingeIdxs) > 0:
            actionInvertOpening = QtGui.QAction(
                QtGui.QIcon(":/icons/Arch_Window_Tree.svg"),
                translate("Arch", "Invert opening direction"),
                menu,
            )
            QtCore.QObject.connect(
                actionInvertOpening, QtCore.SIGNAL("triggered()"), self.invertOpening
            )
            menu.addAction(actionInvertOpening)

        if len(hingeIdxs) == 1:
            actionInvertHinge = QtGui.QAction(
                QtGui.QIcon(":/icons/Arch_Window_Tree.svg"),
                translate("Arch", "Invert hinge position"),
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
            ["Mode" + str(i), "Mode" + str(i + 1)]
            for i in range(1, len(WindowOpeningModes), 2)
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


# backwards compatibility
_Window = Window
_ViewProviderWindow = ViewProviderWindow
