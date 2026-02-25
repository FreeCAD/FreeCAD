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

import FreeCAD
import ArchComponent
import DraftVecUtils

if FreeCAD.GuiUp:
    from PySide import QtGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    from draftutils.translate import translate
else:
    # \cond
    def translate(ctxt, txt):
        return txt

    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt

    # \endcond


class _Equipment(ArchComponent.Component):
    "The Equipment object"

    def __init__(self, obj):

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
        # Add features in the SketchArch External Add-on, if present
        self.addSketchArchFeatures(obj)

    def addSketchArchFeatures(self, obj, linkObj=None, mode=None):
        """
        To add features in the SketchArch External Add-on, if present (https://github.com/paullee0/FreeCAD_SketchArch)
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

    def setProperties(self, obj):

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

    def onDocumentRestored(self, obj):

        ArchComponent.Component.onDocumentRestored(self, obj)
        self.setProperties(obj)

        # Add features in the SketchArch External Add-on, if present
        self.addSketchArchFeatures(obj)

    def loads(self, state):

        self.Type = "Equipment"

    def onChanged(self, obj, prop):

        self.hideSubobjects(obj, prop)
        ArchComponent.Component.onChanged(self, obj, prop)

    def execute(self, obj):

        if self.clone(obj):
            return
        if not self.ensureBase(obj):
            return

        pl = obj.Placement
        if obj.Base:
            base = None
            if hasattr(obj.Base, "Shape"):
                base = obj.Base.Shape.copy()
                base = self.processSubShapes(obj, base, pl)
                self.applyShape(obj, base, pl, allowinvalid=False, allownosolid=True)

        # Execute features in the SketchArch External Add-on, if present
        self.executeSketchArchFeatures(obj)

    def executeSketchArchFeatures(self, obj, linkObj=None, index=None, linkElement=None):
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
            ArchSketchObject.updateAttachmentOffset(obj, linkObj)
        except:
            pass

    def appLinkExecute(self, obj, linkObj, index, linkElement):
        """
        Default Link Execute method() -
        See https://forum.freecad.org/viewtopic.php?f=22&t=42184&start=10#p361124
        @realthunder added support to Links to run Linked Scripted Object's methods()
        """

        # Add features in the SketchArch External Add-on, if present
        self.addSketchArchFeatures(obj, linkObj)

        # Execute features in the SketchArch External Add-on, if present
        self.executeSketchArchFeatures(obj, linkObj)

    def computeAreas(self, obj):
        return


class _ViewProviderEquipment(ArchComponent.ViewProviderComponent):
    "A View Provider for the Equipment object"

    def __init__(self, vobj):

        ArchComponent.ViewProviderComponent.__init__(self, vobj)

    def getIcon(self):

        import Arch_rc

        if hasattr(self, "Object"):
            if hasattr(self.Object, "CloneOf"):
                if self.Object.CloneOf:
                    return ":/icons/Arch_Equipment_Clone.svg"
        return ":/icons/Arch_Equipment_Tree.svg"

    def attach(self, vobj):

        self.Object = vobj.Object
        from pivy import coin

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

    def updateData(self, obj, prop):

        if prop == "SnapPoints":
            if obj.SnapPoints:
                self.coords.point.setNum(len(obj.SnapPoints))
                self.coords.point.setValues([[p.x, p.y, p.z] for p in obj.SnapPoints])
            else:
                self.coords.point.deleteValues(0)
        else:
            ArchComponent.ViewProviderComponent.updateData(self, obj, prop)
