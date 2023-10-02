# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
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

"""Used to create material stock around a machined part - for visualization"""

import FreeCAD
import Path
import math
from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtCore

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class StockType:

    NoStock = "None"
    FromBase = "FromBase"
    CreateBox = "CreateBox"
    CreateCylinder = "CreateCylinder"
    Unknown = "Unknown"

    @classmethod
    def FromStock(cls, stock):
        """FromStock(stock) ... Answer a string representing the type of stock."""
        if not stock:
            return cls.NoStock
        if hasattr(stock, "StockType"):
            return stock.StockType

        # fallback in case somebody messed with internals
        if hasattr(stock, "ExtXneg") and hasattr(stock, "ExtZpos"):
            return cls.FromBase
        if hasattr(stock, "Length") and hasattr(stock, "Width"):
            return cls.CreateBox
        if hasattr(stock, "Radius") and hasattr(stock, "Height"):
            return cls.CreateCylinder
        return cls.Unknown


def shapeBoundBox(obj):
    Path.Log.track(type(obj))
    if list == type(obj) and obj:
        bb = FreeCAD.BoundBox()
        for o in obj:
            bb.add(shapeBoundBox(o))
        return bb

    if hasattr(obj, "Shape"):
        return obj.Shape.BoundBox
    if obj and "App::Part" == obj.TypeId:
        bounds = [shapeBoundBox(o) for o in obj.Group]
        if bounds:
            bb = bounds[0]
            for b in bounds[1:]:
                bb = bb.united(b)
            return bb
    if obj:
        Path.Log.error(
            translate("PathStock", "Invalid base object %s - no shape found") % obj.Name
        )
    return None


class Stock(object):
    def onDocumentRestored(self, obj):
        if hasattr(obj, "StockType"):
            obj.setEditorMode("StockType", 2)  # hide


class StockFromBase(Stock):
    def __init__(self, obj, base):
        "Make stock"
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Base",
            QT_TRANSLATE_NOOP(
                "App::Property", "The base object this stock is derived from"
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "ExtXneg",
            "Stock",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Extra allowance from part bound box in negative X direction",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "ExtXpos",
            "Stock",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Extra allowance from part bound box in positive X direction",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "ExtYneg",
            "Stock",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Extra allowance from part bound box in negative Y direction",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "ExtYpos",
            "Stock",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Extra allowance from part bound box in positive Y direction",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "ExtZneg",
            "Stock",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Extra allowance from part bound box in negative Z direction",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "ExtZpos",
            "Stock",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Extra allowance from part bound box in positive Z direction",
            ),
        )
        obj.addProperty(
            "App::PropertyLink",
            "Material",
            "Component",
            QT_TRANSLATE_NOOP("App::Property", "A material for this object"),
        )

        obj.Base = base
        obj.ExtXneg = 1.0
        obj.ExtXpos = 1.0
        obj.ExtYneg = 1.0
        obj.ExtYpos = 1.0
        obj.ExtZneg = 1.0
        obj.ExtZpos = 1.0

        # placement is only tracked on creation
        bb = shapeBoundBox(base.Group) if base else None
        if bb:
            obj.Placement = FreeCAD.Placement(
                FreeCAD.Vector(bb.XMin, bb.YMin, bb.ZMin), FreeCAD.Rotation()
            )
        else:
            Path.Log.track(obj.Label, base.Label)
        obj.Proxy = self

        # debugging aids
        self.origin = None
        self.length = None
        self.width = None
        self.height = None

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def execute(self, obj):
        bb = (
            shapeBoundBox(obj.Base.Group)
            if obj.Base and hasattr(obj.Base, "Group")
            else None
        )
        Path.Log.track(obj.Label, bb)

        # Sometimes, when the Base changes it's temporarily not assigned when
        # Stock.execute is triggered - it'll be set correctly the next time around.
        if bb:
            self.origin = FreeCAD.Vector(
                -obj.ExtXneg.Value, -obj.ExtYneg.Value, -obj.ExtZneg.Value
            )

            self.length = bb.XLength + obj.ExtXneg.Value + obj.ExtXpos.Value
            self.width = bb.YLength + obj.ExtYneg.Value + obj.ExtYpos.Value
            self.height = bb.ZLength + obj.ExtZneg.Value + obj.ExtZpos.Value

            shape = Part.makeBox(self.length, self.width, self.height, self.origin)
            shape.Placement = obj.Placement
            obj.Shape = shape

    def onChanged(self, obj, prop):
        if (
            prop in ["ExtXneg", "ExtXpos", "ExtYneg", "ExtYpos", "ExtZneg", "ExtZpos"]
            and not "Restore" in obj.State
        ):
            self.execute(obj)


class StockCreateBox(Stock):
    MinExtent = 0.001

    def __init__(self, obj):
        obj.addProperty(
            "App::PropertyLength",
            "Length",
            "Stock",
            QT_TRANSLATE_NOOP("App::Property", "Length of this stock box"),
        )
        obj.addProperty(
            "App::PropertyLength",
            "Width",
            "Stock",
            QT_TRANSLATE_NOOP("App::Property", "Width of this stock box"),
        )
        obj.addProperty(
            "App::PropertyLength",
            "Height",
            "Stock",
            QT_TRANSLATE_NOOP("App::Property", "Height of this stock box"),
        )

        obj.Length = 10
        obj.Width = 10
        obj.Height = 10

        obj.Proxy = self

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def execute(self, obj):
        if obj.Length < self.MinExtent:
            obj.Length = self.MinExtent
        if obj.Width < self.MinExtent:
            obj.Width = self.MinExtent
        if obj.Height < self.MinExtent:
            obj.Height = self.MinExtent

        shape = Part.makeBox(obj.Length, obj.Width, obj.Height)
        shape.Placement = obj.Placement
        obj.Shape = shape

    def onChanged(self, obj, prop):
        if prop in ["Length", "Width", "Height"] and not "Restore" in obj.State:
            self.execute(obj)


class StockCreateCylinder(Stock):
    MinExtent = 0.001

    def __init__(self, obj):
        obj.addProperty(
            "App::PropertyLength",
            "Radius",
            "Stock",
            QT_TRANSLATE_NOOP("App::Property", "Radius of this stock cylinder"),
        )
        obj.addProperty(
            "App::PropertyLength",
            "Height",
            "Stock",
            QT_TRANSLATE_NOOP("App::Property", "Height of this stock cylinder"),
        )

        obj.Radius = 2
        obj.Height = 10

        obj.Proxy = self

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def execute(self, obj):
        if obj.Radius < self.MinExtent:
            obj.Radius = self.MinExtent
        if obj.Height < self.MinExtent:
            obj.Height = self.MinExtent

        shape = Part.makeCylinder(obj.Radius, obj.Height)
        shape.Placement = obj.Placement
        obj.Shape = shape

    def onChanged(self, obj, prop):
        if prop in ["Radius", "Height"] and not "Restore" in obj.State:
            self.execute(obj)


def SetupStockObject(obj, stockType):
    Path.Log.track(obj.Label, stockType)
    if FreeCAD.GuiUp and obj.ViewObject:
        obj.addProperty(
            "App::PropertyString",
            "StockType",
            "Stock",
            QT_TRANSLATE_NOOP("App::Property", "Internal representation of stock type"),
        )
        obj.StockType = stockType
        obj.setEditorMode("StockType", 2)  # hide

        # If I don't rename the module then usage as Path.Base.Gui.IconViewProvider below
        # 'causes above Path.Log.track(...) to fail with - claiming that Path is accessed
        # before it's assigned.
        # Alternative _another_ `import Path` statement in front of `Path.Log.track(...)`
        # also prevents the issue from happening.
        # Go figure.
        import Path.Base.Gui.IconViewProvider as PathIconViewProvider

        PathIconViewProvider.ViewProvider(obj.ViewObject, "Stock")
        obj.ViewObject.Transparency = 90
        obj.ViewObject.DisplayMode = "Wireframe"


class FakeJob(object):
    def __init__(self, base):
        self.Group = [base]


def _getBase(job):
    if job and hasattr(job, "Model"):
        return job.Model
    if job:
        import PathScripts.PathUtils as PathUtils

        job = PathUtils.findParentJob(job)
        return job.Model if job else None
    return None


def CreateFromBase(job, neg=None, pos=None, placement=None):
    Path.Log.track(job.Label, neg, pos, placement)
    base = _getBase(job)
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Stock")
    obj.Proxy = StockFromBase(obj, base)

    if neg:
        obj.ExtXneg = neg.x
        obj.ExtYneg = neg.y
        obj.ExtZneg = neg.z

    if pos:
        obj.ExtXpos = pos.x
        obj.ExtYpos = pos.y
        obj.ExtZpos = pos.z

    if placement:
        obj.Placement = placement

    SetupStockObject(obj, StockType.FromBase)
    obj.Proxy.execute(obj)
    obj.purgeTouched()
    return obj


def CreateBox(job, extent=None, placement=None):
    base = _getBase(job)
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Stock")
    obj.Proxy = StockCreateBox(obj)

    if extent:
        obj.Length = extent.x
        obj.Width = extent.y
        obj.Height = extent.z
    elif base:
        bb = shapeBoundBox(base.Group)
        obj.Length = max(bb.XLength, 1)
        obj.Width = max(bb.YLength, 1)
        obj.Height = max(bb.ZLength, 1)

    if placement:
        obj.Placement = placement
    elif base:
        bb = shapeBoundBox(base.Group)
        origin = FreeCAD.Vector(bb.XMin, bb.YMin, bb.ZMin)
        obj.Placement = FreeCAD.Placement(origin, FreeCAD.Vector(), 0)

    SetupStockObject(obj, StockType.CreateBox)
    return obj


def CreateCylinder(job, radius=None, height=None, placement=None):
    base = _getBase(job)
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "Stock")
    obj.Proxy = StockCreateCylinder(obj)

    if radius:
        obj.Radius = radius

    if height:
        obj.Height = height
    elif base:
        bb = shapeBoundBox(base.Group)
        obj.Radius = math.sqrt(bb.XLength**2 + bb.YLength**2) / 2.0
        obj.Height = max(bb.ZLength, 1)

    if placement:
        obj.Placement = placement
    elif base:
        bb = shapeBoundBox(base.Group)
        origin = FreeCAD.Vector(
            (bb.XMin + bb.XMax) / 2, (bb.YMin + bb.YMax) / 2, bb.ZMin
        )
        obj.Placement = FreeCAD.Placement(origin, FreeCAD.Vector(), 0)

    SetupStockObject(obj, StockType.CreateCylinder)
    return obj


def TemplateAttributes(stock, includeExtent=True, includePlacement=True):
    attrs = {}
    if stock:
        attrs["version"] = 1
        stockType = StockType.FromStock(stock)
        attrs["create"] = stockType

        if includeExtent:
            if stockType == StockType.FromBase:
                attrs["xneg"] = "%s" % stock.ExtXneg
                attrs["xpos"] = "%s" % stock.ExtXpos
                attrs["yneg"] = "%s" % stock.ExtYneg
                attrs["ypos"] = "%s" % stock.ExtYpos
                attrs["zneg"] = "%s" % stock.ExtZneg
                attrs["zpos"] = "%s" % stock.ExtZpos
            if stockType == StockType.CreateBox:
                attrs["length"] = "%s" % stock.Length
                attrs["width"] = "%s" % stock.Width
                attrs["height"] = "%s" % stock.Height
            if stockType == StockType.CreateCylinder:
                attrs["radius"] = "%s" % stock.Radius
                attrs["height"] = "%s" % stock.Height

        if includePlacement:
            pos = stock.Placement.Base
            attrs["posX"] = pos.x
            attrs["posY"] = pos.y
            attrs["posZ"] = pos.z
            rot = stock.Placement.Rotation
            attrs["rotX"] = rot.Q[0]
            attrs["rotY"] = rot.Q[1]
            attrs["rotZ"] = rot.Q[2]
            attrs["rotW"] = rot.Q[3]

    return attrs


def CreateFromTemplate(job, template):
    if template.get("version") and 1 == int(template["version"]):
        stockType = template.get("create")
        if stockType:
            placement = None
            posX = template.get("posX")
            posY = template.get("posY")
            posZ = template.get("posZ")
            rotX = template.get("rotX")
            rotY = template.get("rotY")
            rotZ = template.get("rotZ")
            rotW = template.get("rotW")
            if (
                posX is not None
                and posY is not None
                and posZ is not None
                and rotX is not None
                and rotY is not None
                and rotZ is not None
                and rotW is not None
            ):
                pos = FreeCAD.Vector(float(posX), float(posY), float(posZ))
                rot = FreeCAD.Rotation(
                    float(rotX), float(rotY), float(rotZ), float(rotW)
                )
                placement = FreeCAD.Placement(pos, rot)
            elif (
                posX is not None
                or posY is not None
                or posZ is not None
                or rotX is not None
                or rotY is not None
                or rotZ is not None
                or rotW is not None
            ):
                Path.Log.warning(
                    "Corrupted or incomplete placement information in template - ignoring"
                )

            if stockType == StockType.FromBase:
                xneg = template.get("xneg")
                xpos = template.get("xpos")
                yneg = template.get("yneg")
                ypos = template.get("ypos")
                zneg = template.get("zneg")
                zpos = template.get("zpos")
                neg = None
                pos = None
                if (
                    xneg is not None
                    and xpos is not None
                    and yneg is not None
                    and ypos is not None
                    and zneg is not None
                    and zpos is not None
                ):
                    neg = FreeCAD.Vector(
                        FreeCAD.Units.Quantity(xneg).Value,
                        FreeCAD.Units.Quantity(yneg).Value,
                        FreeCAD.Units.Quantity(zneg).Value,
                    )
                    pos = FreeCAD.Vector(
                        FreeCAD.Units.Quantity(xpos).Value,
                        FreeCAD.Units.Quantity(ypos).Value,
                        FreeCAD.Units.Quantity(zpos).Value,
                    )
                elif (
                    xneg is not None
                    or xpos is not None
                    or yneg is not None
                    or ypos is not None
                    or zneg is not None
                    or zpos is not None
                ):
                    Path.Log.error(
                        "Corrupted or incomplete specification for creating stock from base - ignoring extent"
                    )
                return CreateFromBase(job, neg, pos, placement)

            if stockType == StockType.CreateBox:
                Path.Log.track(" create box")
                length = template.get("length")
                width = template.get("width")
                height = template.get("height")
                extent = None
                if length is not None and width is not None and height is not None:
                    Path.Log.track("  have extent")
                    extent = FreeCAD.Vector(
                        FreeCAD.Units.Quantity(length).Value,
                        FreeCAD.Units.Quantity(width).Value,
                        FreeCAD.Units.Quantity(height).Value,
                    )
                elif length is not None or width is not None or height is not None:
                    Path.Log.error(
                        "Corrupted or incomplete size for creating a stock box - ignoring size"
                    )
                else:
                    Path.Log.track(
                        "  take placement (%s) and extent (%s) from model"
                        % (placement, extent)
                    )
                return CreateBox(job, extent, placement)

            if stockType == StockType.CreateCylinder:
                radius = template.get("radius")
                height = template.get("height")
                if radius is not None and height is not None:
                    pass
                elif radius is not None or height is not None:
                    radius = None
                    height = None
                    Path.Log.error(
                        "Corrupted or incomplete size for creating a stock cylinder - ignoring size"
                    )
                return CreateCylinder(job, radius, height, placement)

            Path.Log.error(
                translate("PathStock", "Unsupported stock type named {}").format(
                    stockType
                )
            )
        else:
            Path.Log.error(
                translate(
                    "PathStock", "Unsupported PathStock template version {}"
                ).format(template.get("version"))
            )
        return None


FreeCAD.Console.PrintLog("Loading PathStock... done\n")
