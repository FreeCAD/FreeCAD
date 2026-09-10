# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

from __future__ import annotations

__title__ = "FreeCAD Equipment"
__author__ = "Yorik van Havre"
__url__ = "https://www.freecad.org"

## @package ArchEquipment
#  \ingroup ARCH
#  \brief The Equipment object and tools
#
#  This module provides tools to build equipment objects.
#  Equipment is used to represent furniture and all kinds of electrical
#  or hydraulic appliances in a building

from typing import Any, TYPE_CHECKING

import FreeCAD
import ArchComponent
import DraftVecUtils

if TYPE_CHECKING:
    from bim_typing import ArchEquipmentObject

if FreeCAD.GuiUp:
    from PySide import QtGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    from draftutils.translate import translate
else:
    # \cond
    def translate(context: str, text: str, comment: str | None = None, /) -> str:
        return text

    def QT_TRANSLATE_NOOP(context: str, source_text: str, /) -> str:
        return source_text

    # \endcond


if FreeCAD.GuiUp:

    class EquipmentTaskPanel(ArchComponent.ComponentOptionsTaskPanel):
        """A task panel for Arch Equipment using the generic options box"""

        def __init__(self, obj: ArchEquipmentObject) -> None:
            property_definitions = [
                {"prop": "Model", "label": translate("Arch", "Model")},
                {"prop": "EquipmentPower", "label": translate("Arch", "Equipment Power")},
            ]
            super().__init__(obj, property_definitions)


class _Equipment(ArchComponent.Component):
    "The Equipment object"

    def __init__(self, obj: ArchEquipmentObject) -> None:

        ArchComponent.Component.__init__(self, obj)
        self.Type = "Equipment"
        self.setProperties(obj)
        from ArchIFC import IfcTypes

        if "Furniture" in IfcTypes:
            # IfcFurniture is new in IFC4
            obj.IfcType = "Furniture"
        elif "Furnishing Element" in IfcTypes:
            # IFC2x3 does know a IfcFurnishingElement
            obj.IfcType = "Furnishing Element"
        else:
            obj.IfcType = "Building Element Proxy"

    def setProperties(self, obj: ArchEquipmentObject) -> None:

        pl = obj.PropertiesList
        if not "Model" in pl:
            obj.addProperty(
                "App::PropertyString",
                "Model",
                "Equipment",
                QT_TRANSLATE_NOOP("App::Property", "The model description of this equipment"),
                locked=True,
            )
        if not "ProductURL" in pl:
            obj.addProperty(
                "App::PropertyString",
                "ProductURL",
                "Equipment",
                QT_TRANSLATE_NOOP("App::Property", "The URL of the product page of this equipment"),
                locked=True,
            )
        if not "StandardCode" in pl:
            obj.addProperty(
                "App::PropertyString",
                "StandardCode",
                "Equipment",
                QT_TRANSLATE_NOOP("App::Property", "A standard code (MasterFormat, OmniClass,…)"),
                locked=True,
            )
        if not "SnapPoints" in pl:
            obj.addProperty(
                "App::PropertyVectorList",
                "SnapPoints",
                "Equipment",
                QT_TRANSLATE_NOOP("App::Property", "Additional snap points for this equipment"),
                locked=True,
            )
        if not "EquipmentPower" in pl:
            obj.addProperty(
                "App::PropertyFloat",
                "EquipmentPower",
                "Equipment",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The electric power needed by this equipment in Watts"
                ),
                locked=True,
            )
        obj.setEditorMode("VerticalArea", 2)
        obj.setEditorMode("HorizontalArea", 2)
        obj.setEditorMode("PerimeterLength", 2)

    def onDocumentRestored(self, obj: ArchEquipmentObject) -> None:

        ArchComponent.Component.onDocumentRestored(self, obj)
        self.setProperties(obj)

    def loads(self, state: Any) -> None:

        self.Type = "Equipment"

    def onChanged(self, obj: ArchEquipmentObject, prop: str) -> None:

        self.hideSubobjects(obj, prop)
        ArchComponent.Component.onChanged(self, obj, prop)

    def execute(self, obj: ArchEquipmentObject) -> None:

        if self.clone(obj):
            return
        if not self.ensureBase(obj):
            return

        pl = obj.Placement
        if obj.Base:
            base_shape = getattr(obj.Base, "Shape", None)
            if base_shape is not None:
                base = base_shape.copy()
                base = self.processSubShapes(obj, base, pl)
                self.applyShape(obj, base, pl, allowinvalid=False, allownosolid=True)

        # Execute features in the SketchArch External Add-on, if present
        self.executeSketchArchFeatures(obj)

    def executeSketchArchFeatures(
        self,
        obj: ArchEquipmentObject,
        linkObj: object | None = None,
        index: int | None = None,
        linkElement: object | None = None,
    ) -> None:
        """
        To execute features in the SketchArch External Add-on  (https://github.com/paullee0/FreeCAD_SketchArch)
        -  import ArchSketchObject module, and
        -  execute features that are common to ArchObjects (including Links) and ArchSketch

        To install SketchArch External Add-on, see https://github.com/paullee0/FreeCAD_SketchArch#iv-install
        """

        # To execute features in SketchArch External Add-on, if present
        try:
            import ArchSketchObject

            # Execute SketchArch Feature - Intuitive Automatic Placement for Arch Windows/Doors, Equipment etc.
            # see https://forum.freecad.org/viewtopic.php?f=23&t=50802
            update_attachment_offset = getattr(ArchSketchObject, "updateAttachmentOffset", None)
            if callable(update_attachment_offset):
                update_attachment_offset(obj, linkObj)
        except:
            pass

    def computeAreas(self, obj: ArchEquipmentObject) -> None:
        return


class _ViewProviderEquipment(ArchComponent.ViewProviderComponent):
    "A View Provider for the Equipment object"

    Object: ArchEquipmentObject
    coords: Any

    def __init__(self, vobj: Any) -> None:

        ArchComponent.ViewProviderComponent.__init__(self, vobj)

    def getIcon(self) -> str:

        import Arch_rc  # pyright: ignore[reportMissingImports]

        if hasattr(self, "Object"):
            if hasattr(self.Object, "CloneOf"):
                if self.Object.CloneOf:
                    return ":/icons/Arch_Equipment_Clone.svg"
        return ":/icons/Arch_Equipment_Tree.svg"

    def attach(self, vobj: Any) -> None:

        self.Object = vobj.Object
        from pivy import coin  # pyright: ignore[reportMissingImports]

        sep = coin.SoSeparator()
        self.coords = coin.SoCoordinate3()
        sep.addChild(self.coords)
        self.coords.point.deleteValues(0)
        symbol = coin.SoMarkerSet()
        symbol.markerIndex = FreeCADGui.getMarkerIndex("", 5)
        sep.addChild(symbol)
        rn = vobj.RootNode
        rn.addChild(sep)
        ArchComponent.ViewProviderComponent.attach(self, vobj)

    def updateData(self, obj: ArchEquipmentObject, prop: str) -> None:

        if prop == "SnapPoints":
            if obj.SnapPoints:
                self.coords.point.setNum(len(obj.SnapPoints))
                self.coords.point.setValues([[p.x, p.y, p.z] for p in obj.SnapPoints])
            else:
                self.coords.point.deleteValues(0)
        else:
            ArchComponent.ViewProviderComponent.updateData(self, obj, prop)

    def setEdit(self, vobj: Any, mode: int) -> bool | None:
        if mode != 0:
            return None

        taskd = EquipmentTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True
