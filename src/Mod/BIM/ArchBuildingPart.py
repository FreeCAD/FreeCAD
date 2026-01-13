# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
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

__title__ = "FreeCAD Arch BuildingPart"
__author__ = "Yorik van Havre"
__url__ = "https://www.freecad.org"

## @package ArchBuildingPart
#  \ingroup ARCH
#  \brief The BuildingPart object and tools
#
#  This module provides tools to build BuildingPart objects.
#  BuildingParts are used to group different Arch objects

import os
import tempfile

import FreeCAD
import Arch
import ArchCommands
import ArchIFC
import Draft
import DraftVecUtils

from draftutils import params

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    from draftutils.translate import translate
    import draftutils.units as units
else:
    # \cond
    def translate(ctxt, txt):
        return txt

    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt

    # \endcond
unicode = str


# fmt: off
BuildingTypes = ['Undefined',
'Agricultural - Barn',
'Agricultural - Chicken coop or chickenhouse',
'Agricultural - Cow-shed',
'Agricultural - Farmhouse',
'Agricultural - Granary',
'Agricultural - Greenhouse',
'Agricultural - Hayloft',
'Agricultural - Pigpen or sty',
'Agricultural - Root cellar',
'Agricultural - Shed',
'Agricultural - Silo',
'Agricultural - Stable',
'Agricultural - Storm cellar',
'Agricultural - Well house',
'Agricultural - Underground pit',

'Commercial - Automobile repair shop',
'Commercial - Bank',
'Commercial - Car wash',
'Commercial - Convention center',
'Commercial - Forum',
'Commercial - Gas station',
'Commercial - Hotel',
'Commercial - Market',
'Commercial - Market house',
'Commercial - Skyscraper',
'Commercial - Shop',
'Commercial - Shopping mall',
'Commercial - Supermarket',
'Commercial - Warehouse',
'Commercial - Restaurant',

'Residential - Apartment block',
'Residential - Asylum',
'Residential - Condominium',
'Residential - Dormitory',
'Residential - Duplex',
'Residential - House',
'Residential - Nursing home',
'Residential - Townhouse',
'Residential - Villa',
'Residential - Bungalow',

'Educational - Archive',
'Educational - College classroom building',
'Educational - College gymnasium',
'Educational - College students union',
'Educational - School',
'Educational - Library',
'Educational - Museum',
'Educational - Art gallery',
'Educational - Theater',
'Educational - Amphitheater',
'Educational - Concert hall',
'Educational - Cinema',
'Educational - Opera house',
'Educational - Boarding school',

'Government - Capitol',
'Government - City hall',
'Government - Consulate',
'Government - Courthouse',
'Government - Embassy',
'Government - Fire station',
'Government - Meeting house',
'Government - Moot hall',
'Government - Palace',
'Government - Parliament',
'Government - Police station',
'Government - Post office',
'Government - Prison',

'Industrial - Brewery',
'Industrial - Factory',
'Industrial - Foundry',
'Industrial - Power plant',
'Industrial - Mill',

'Military - Arsenal',
'Military -Barracks',

'Parking - Boathouse',
'Parking - Garage',
'Parking - Hangar',

'Storage - Silo',
'Storage - Hangar',

'Religious - Church',
'Religious - Basilica',
'Religious - Cathedral',
'Religious - Chapel',
'Religious - Oratory',
'Religious - Martyrium',
'Religious - Mosque',
'Religious - Mihrab',
'Religious - Surau',
'Religious - Imambargah',
'Religious - Monastery',
'Religious - Mithraeum',
'Religious - Fire temple',
'Religious - Shrine',
'Religious - Synagogue',
'Religious - Temple',
'Religious - Pagoda',
'Religious - Gurdwara',
'Religious - Hindu temple',

'Transport - Airport terminal',
'Transport - Bus station',
'Transport - Metro station',
'Transport - Taxi station',
'Transport - Railway station',
'Transport - Signal box',
'Transport - Lighthouse',

'Infrastructure - Data centre',

'Power station - Fossil-fuel power station',
'Power station - Nuclear power plant',
'Power station - Geothermal power',
'Power station - Biomass-fuelled power plant',
'Power station - Waste heat power plant',
'Power station - Renewable energy power station',
'Power station - Atomic energy plant',

'Other - Apartment',
'Other - Clinic',
'Other - Community hall',
'Other - Eatery',
'Other - Folly',
'Other - Food court',
'Other - Hospice',
'Other - Hospital',
'Other - Hut',
'Other - Bathhouse',
'Other - Workshop',
'Other - World trade centre'
]
# fmt: on


class BuildingPart(ArchIFC.IfcProduct):
    "The BuildingPart object"

    def __init__(self, obj):

        obj.Proxy = self
        self.Type = "BuildingPart"
        obj.addExtension("App::GroupExtensionPython")
        # obj.addExtension('App::OriginGroupExtensionPython')
        self.setProperties(obj)

    def setProperties(self, obj):
        ArchIFC.IfcProduct.setProperties(self, obj)

        pl = obj.PropertiesList
        if not "Height" in pl:
            obj.addProperty(
                "App::PropertyLength",
                "Height",
                "BuildingPart",
                QT_TRANSLATE_NOOP("App::Property", "The height of this object"),
                locked=True,
            )
        if not "HeightPropagate" in pl:
            obj.addProperty(
                "App::PropertyBool",
                "HeightPropagate",
                "Children",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If true, the height value propagates to contained objects if the height of those objects is set to 0",
                ),
                locked=True,
            )
            obj.HeightPropagate = True
        if not "LevelOffset" in pl:
            obj.addProperty(
                "App::PropertyDistance",
                "LevelOffset",
                "BuildingPart",
                QT_TRANSLATE_NOOP("App::Property", "The level of the (0,0,0) point of this level"),
                locked=True,
            )
        if not "Area" in pl:
            obj.addProperty(
                "App::PropertyArea",
                "Area",
                "BuildingPart",
                QT_TRANSLATE_NOOP("App::Property", "The computed floor area of this floor"),
                locked=True,
            )
        if not "Description" in pl:
            obj.addProperty(
                "App::PropertyString",
                "Description",
                "Component",
                QT_TRANSLATE_NOOP("App::Property", "An optional description for this component"),
                locked=True,
            )
        if not "Tag" in pl:
            obj.addProperty(
                "App::PropertyString",
                "Tag",
                "Component",
                QT_TRANSLATE_NOOP("App::Property", "An optional tag for this component"),
                locked=True,
            )
        if not "Shape" in pl:
            obj.addProperty(
                "Part::PropertyPartShape",
                "Shape",
                "BuildingPart",
                QT_TRANSLATE_NOOP("App::Property", "The shape of this object"),
                locked=True,
            )
        if not "SavedInventor" in pl:
            obj.addProperty(
                "App::PropertyFileIncluded",
                "SavedInventor",
                "BuildingPart",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This property stores an OpenInventor representation for this object",
                ),
                locked=True,
            )
            obj.setEditorMode("SavedInventor", 2)
        if not "OnlySolids" in pl:
            obj.addProperty(
                "App::PropertyBool",
                "OnlySolids",
                "BuildingPart",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If true, only solids will be collected by this object when referenced from other files",
                ),
                locked=True,
            )
            obj.OnlySolids = True
        if not "MaterialsTable" in pl:
            obj.addProperty(
                "App::PropertyMap",
                "MaterialsTable",
                "BuildingPart",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "A MaterialName:SolidIndexesList map that relates material names with solid indexes to be used when referencing this object from other files",
                ),
                locked=True,
            )

    def onDocumentRestored(self, obj):

        self.setProperties(obj)

    def dumps(self):

        return None

    def loads(self, state):

        self.Type = "BuildingPart"

    def onBeforeChange(self, obj, prop):

        if prop == "Placement":
            self.oldPlacement = FreeCAD.Placement(obj.Placement)

    def onChanged(self, obj, prop):

        import math

        ArchIFC.IfcProduct.onChanged(self, obj, prop)

        # clean svg cache if needed
        if prop in ["Placement", "Group"]:
            self.svgcache = None
            self.shapecache = None

        if (prop == "Height" or prop == "HeightPropagate") and obj.Height.Value:
            self.touchChildren(obj)

        elif prop == "Placement":
            if hasattr(self, "oldPlacement") and self.oldPlacement != obj.Placement:
                deltap = obj.Placement.Base.sub(self.oldPlacement.Base)
                if deltap.Length == 0:
                    deltap = None
                deltar = obj.Placement.Rotation * self.oldPlacement.Rotation.inverted()
                if deltar.Angle < 0.0001:
                    deltar = None
                for child in self.getMovableChildren(obj):
                    if deltar:
                        child.Placement.rotate(
                            self.oldPlacement.Base,
                            deltar.Axis,
                            math.degrees(deltar.Angle),
                            comp=True,
                        )
                    if deltap:
                        child.Placement.move(deltap)

    def execute(self, obj):
        "gather all the child shapes into a compound"

        pl = obj.Placement
        shapes, materialstable = self.getShapes(obj)
        if shapes:
            import Part

            if obj.OnlySolids:
                f = []
                for s in shapes:
                    f.extend(s.Solids)
                # print("faces before compound:",len(f))
                obj.Shape = Part.makeCompound(f)
                # print("faces after compound:",len(obj.Shape.Faces))
                # print("recomputing ",obj.Label)
            else:
                obj.Shape = Part.makeCompound(shapes)
            obj.Placement = pl
        obj.Area = self.getArea(obj)
        obj.MaterialsTable = materialstable
        if obj.ViewObject:
            # update the autogroup box if needed
            obj.ViewObject.Proxy.onChanged(obj.ViewObject, "AutoGroupBox")

    def getMovableChildren(self, obj):
        "recursively get movable children"

        result = []
        for child in obj.Group:
            if child.isDerivedFrom("App::DocumentObjectGroup"):
                result.extend(self.getMovableChildren(child))
            if not hasattr(child, "MoveWithHost") or child.MoveWithHost:
                if hasattr(child, "Placement"):
                    result.append(child)
        return result

    def getArea(self, obj):
        "computes the area of this floor by adding its inner spaces"

        area = 0
        if hasattr(obj, "Group"):
            for child in obj.Group:
                if (Draft.get_type(child) in ["Space", "BuildingPart"]) and hasattr(
                    child, "IfcType"
                ):
                    area += child.Area.Value
        return area

    def getShapes(self, obj):
        "recursively get the shapes of objects inside this BuildingPart"

        shapes = []
        solidindex = 0
        materialstable = {}
        for child in Draft.get_group_contents(obj, walls=True):
            if not Draft.get_type(child) in ["Space"]:
                if hasattr(child, "Shape") and child.Shape:
                    shapes.append(child.Shape)
                    for solid in child.Shape.Solids:
                        matname = "Undefined"
                        if hasattr(child, "Material") and child.Material:
                            matname = child.Material.Name
                        if matname in materialstable:
                            materialstable[matname] = (
                                materialstable[matname] + "," + str(solidindex)
                            )
                        else:
                            materialstable[matname] = str(solidindex)
                        solidindex += 1
        return shapes, materialstable

    def getSpaces(self, obj):
        "gets the list of Spaces that have this object as their Zone property"

        g = []
        for o in obj.OutList:
            if hasattr(o, "Zone"):
                if o.Zone == obj:
                    g.append(o)
        return g

    def touchChildren(self, obj):
        "Touches all descendents where applicable"

        g = []
        if hasattr(obj, "Group"):
            g = obj.Group
        elif Draft.getType(obj) in ["Wall", "Structure"]:
            g = obj.Additions
        for child in g:
            if Draft.getType(child) in ["Wall", "Structure"]:
                if not child.Height.Value:
                    FreeCAD.Console.PrintLog("Auto-updating Height of " + child.Name + "\n")
                    self.touchChildren(child)
                    child.Proxy.execute(child)
            elif Draft.getType(child) in ["App::DocumentObjectGroup", "Group", "BuildingPart"]:
                self.touchChildren(child)

    def addObject(self, obj, child):
        "Adds an object to the group of this BuildingPart"

        if not child in obj.Group:
            g = obj.Group
            g.append(child)
            obj.Group = g

    def autogroup(self, obj, child):
        "Adds an object to the group of this BuildingPart automatically"

        if obj.ViewObject:
            if hasattr(obj.ViewObject.Proxy, "autobbox") and obj.ViewObject.Proxy.autobbox:
                if hasattr(child, "Shape") and child.Shape:
                    abb = obj.ViewObject.Proxy.autobbox
                    cbb = child.Shape.BoundBox
                    if abb.isValid():
                        if not cbb.isValid():
                            FreeCAD.ActiveDocument.recompute()
                        if not cbb.isValid():
                            cbb = FreeCAD.BoundBox()
                            for v in child.Shape.Vertexes:
                                print(v.Point)
                                cbb.add(v.Point)
                        if cbb.isValid() and abb.isInside(cbb):
                            self.addObject(obj, child)
                            return True
        return False


class ViewProviderBuildingPart:
    "A View Provider for the BuildingPart object"

    def __init__(self, vobj):

        if vobj:
            vobj.addExtension("Gui::ViewProviderGroupExtensionPython")
            vobj.Proxy = self
            self.setProperties(vobj)
            vobj.ShapeColor = ArchCommands.getDefaultColor("Helpers")
            self.Object = vobj.Object

    def setProperties(self, vobj):

        pl = vobj.PropertiesList
        if not "LineWidth" in pl:
            vobj.addProperty(
                "App::PropertyFloat",
                "LineWidth",
                "BuildingPart",
                QT_TRANSLATE_NOOP("App::Property", "The line width of this object"),
                locked=True,
            )
            vobj.LineWidth = 1
        if not "OverrideUnit" in pl:
            vobj.addProperty(
                "App::PropertyString",
                "OverrideUnit",
                "BuildingPart",
                QT_TRANSLATE_NOOP("App::Property", "An optional unit to express levels"),
                locked=True,
            )
        if not "DisplayOffset" in pl:
            vobj.addProperty(
                "App::PropertyPlacement",
                "DisplayOffset",
                "BuildingPart",
                QT_TRANSLATE_NOOP("App::Property", "A transformation to apply to the level mark"),
                locked=True,
            )
            vobj.DisplayOffset = FreeCAD.Placement(
                FreeCAD.Vector(0, 0, 0), FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), 90)
            )
        if not "ShowLevel" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "ShowLevel",
                "BuildingPart",
                QT_TRANSLATE_NOOP("App::Property", "If true, show the level"),
                locked=True,
            )
            vobj.ShowLevel = True
        if not "ShowUnit" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "ShowUnit",
                "BuildingPart",
                QT_TRANSLATE_NOOP("App::Property", "If true, show the unit on the level tag"),
                locked=True,
            )
        if not "OriginOffset" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "OriginOffset",
                "BuildingPart",
                QT_TRANSLATE_NOOP(
                    "App::Property", "If true, display offset will affect the origin mark too"
                ),
                locked=True,
            )
        if not "ShowLabel" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "ShowLabel",
                "BuildingPart",
                QT_TRANSLATE_NOOP("App::Property", "If true, the object's label is displayed"),
                locked=True,
            )
            vobj.ShowLabel = True
        if not "FontName" in pl:
            vobj.addProperty(
                "App::PropertyFont",
                "FontName",
                "BuildingPart",
                QT_TRANSLATE_NOOP("App::Property", "The font to be used for texts"),
                locked=True,
            )
            vobj.FontName = params.get_param("textfont")
        if not "FontSize" in pl:
            vobj.addProperty(
                "App::PropertyLength",
                "FontSize",
                "BuildingPart",
                QT_TRANSLATE_NOOP("App::Property", "The font size of texts"),
                locked=True,
            )
            vobj.FontSize = params.get_param("textheight") * params.get_param(
                "DefaultAnnoScaleMultiplier"
            )
        if not "DiffuseColor" in pl:
            vobj.addProperty(
                "App::PropertyColorList",
                "DiffuseColor",
                "BuildingPart",
                QT_TRANSLATE_NOOP("App::Property", "The individual face colors"),
                locked=True,
            )

        # Interaction properties
        if not "SetWorkingPlane" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "SetWorkingPlane",
                "Interaction",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If true, when activated, the working plane will automatically adapt to this level",
                ),
                locked=True,
            )
            vobj.SetWorkingPlane = True
        if not "AutoWorkingPlane" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "AutoWorkingPlane",
                "Interaction",
                QT_TRANSLATE_NOOP(
                    "App::Property", "If set to True, the working plane will be kept on Auto mode"
                ),
                locked=True,
            )
        if not "ViewData" in pl:
            vobj.addProperty(
                "App::PropertyFloatList",
                "ViewData",
                "Interaction",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Camera position data associated with this object"
                ),
                locked=True,
            )
            vobj.setEditorMode("ViewData", 2)
        if not "RestoreView" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "RestoreView",
                "Interaction",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If set, the view stored in this object will be restored on double-click",
                ),
                locked=True,
            )
        if not "DoubleClickActivates" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "DoubleClickActivates",
                "Interaction",
                QT_TRANSLATE_NOOP(
                    "App::Property", "If True, double-clicking this object in the tree activates it"
                ),
                locked=True,
            )
            vobj.DoubleClickActivates = True

        # inventor saving
        if not "SaveInventor" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "SaveInventor",
                "Interaction",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If this is enabled, the OpenInventor representation of this object will be saved in the FreeCAD file, allowing to reference it in other files in lightweight mode.",
                ),
                locked=True,
            )
        if not "SavedInventor" in pl:
            vobj.addProperty(
                "App::PropertyFileIncluded",
                "SavedInventor",
                "Interaction",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "A slot to save the OpenInventor representation of this object, if enabled",
                ),
                locked=True,
            )
            vobj.setEditorMode("SavedInventor", 2)

        # children properties
        if not "ChildrenOverride" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "ChildrenOverride",
                "Children",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If true, show the objects contained in this Building Part will adopt these line, color and transparency settings",
                ),
                locked=True,
            )
        if not "ChildrenLineWidth" in pl:
            vobj.addProperty(
                "App::PropertyFloat",
                "ChildrenLineWidth",
                "Children",
                QT_TRANSLATE_NOOP("App::Property", "The line width of child objects"),
                locked=True,
            )
            vobj.ChildrenLineWidth = params.get_param_view("DefaultShapeLineWidth")
        if not "ChildrenLineColor" in pl:
            vobj.addProperty(
                "App::PropertyColor",
                "ChildrenLineColor",
                "Children",
                QT_TRANSLATE_NOOP("App::Property", "The line color of child objects"),
                locked=True,
            )
            vobj.ChildrenLineColor = params.get_param_view("DefaultShapeLineColor") & 0xFFFFFF00
        if not "ChildrenShapeColor" in pl:
            vobj.addProperty(
                "App::PropertyMaterial",
                "ChildrenShapeColor",
                "Children",
                QT_TRANSLATE_NOOP("App::Property", "The shape appearance of child objects"),
                locked=True,
            )
            vobj.ChildrenShapeColor = params.get_param_view("DefaultShapeColor") & 0xFFFFFF00
        if not "ChildrenTransparency" in pl:
            vobj.addProperty(
                "App::PropertyPercent",
                "ChildrenTransparency",
                "Children",
                QT_TRANSLATE_NOOP("App::Property", "The transparency of child objects"),
                locked=True,
            )
            vobj.ChildrenTransparency = params.get_param_view("DefaultShapeTransparency")

        # clip properties
        if not "CutView" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "CutView",
                "Clip",
                QT_TRANSLATE_NOOP("App::Property", "Cut the view above this level"),
                locked=True,
            )
        if not "CutMargin" in pl:
            vobj.addProperty(
                "App::PropertyLength",
                "CutMargin",
                "Clip",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The distance between the level plane and the cut line"
                ),
                locked=True,
            )
            vobj.CutMargin = 1600
        if not "AutoCutView" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "AutoCutView",
                "Clip",
                QT_TRANSLATE_NOOP("App::Property", "Turn cutting on when activating this level"),
                locked=True,
            )

        # autogroup properties
        if not "AutogroupSize" in pl:
            vobj.addProperty(
                "App::PropertyIntegerList",
                "AutogroupSize",
                "AutoGroup",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The capture box for newly created objects expressed as [XMin,YMin,ZMin,XMax,YMax,ZMax]",
                ),
                locked=True,
            )
        if not "AutogroupBox" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "AutogroupBox",
                "AutoGroup",
                QT_TRANSLATE_NOOP("App::Property", "Turns auto group box on/off"),
                locked=True,
            )
        if not "AutogroupAutosize" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "AutogroupAutosize",
                "AutoGroup",
                QT_TRANSLATE_NOOP("App::Property", "Automatically set size from contents"),
                locked=True,
            )
        if not "AutogroupMargin" in pl:
            vobj.addProperty(
                "App::PropertyLength",
                "AutogroupMargin",
                "AutoGroup",
                QT_TRANSLATE_NOOP("App::Property", "A margin to use when autosize is turned on"),
                locked=True,
            )

    def onDocumentRestored(self, vobj):

        self.setProperties(vobj)

    def getIcon(self):

        import Arch_rc

        if hasattr(self, "Object"):
            if self.Object.IfcType == "Building Storey":
                return ":/icons/Arch_Floor_Tree.svg"
            elif self.Object.IfcType == "Building":
                return ":/icons/Arch_Building_Tree.svg"
            elif self.Object.IfcType == "Annotation":
                return ":/icons/BIM_ArchView.svg"
            elif hasattr(self.Object, "IfcClass"):
                from nativeifc import ifc_viewproviders

                return ifc_viewproviders.get_icon(self)
        return ":/icons/Arch_BuildingPart_Tree.svg"

    def attach(self, vobj):

        self.Object = vobj.Object
        self.clip = None
        from pivy import coin

        self.sep = coin.SoGroup()
        self.mat = coin.SoMaterial()
        self.sep.addChild(self.mat)
        self.dst = coin.SoDrawStyle()
        self.sep.addChild(self.dst)
        self.lco = coin.SoCoordinate3()
        self.sep.addChild(self.lco)
        import PartGui  # Required for "SoBrepEdgeSet" (because a BuildingPart is not a Part::FeaturePython object).

        lin = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        if lin:
            lin.coordIndex.setValues([0, 1, -1, 2, 3, -1, 4, 5, -1])
        self.sep.addChild(lin)
        self.bbox = coin.SoSwitch()
        self.bbox.whichChild = -1
        bboxsep = coin.SoSeparator()
        self.bbox.addChild(bboxsep)
        drawstyle = coin.SoDrawStyle()
        drawstyle.style = coin.SoDrawStyle.LINES
        drawstyle.lineWidth = 3
        drawstyle.linePattern = 0x0F0F  # 0xaa
        bboxsep.addChild(drawstyle)
        self.bbco = coin.SoCoordinate3()
        bboxsep.addChild(self.bbco)
        lin = coin.SoIndexedLineSet()
        lin.coordIndex.setValues(
            [0, 1, 2, 3, 0, -1, 4, 5, 6, 7, 4, -1, 0, 4, -1, 1, 5, -1, 2, 6, -1, 3, 7, -1]
        )
        bboxsep.addChild(lin)
        self.sep.addChild(self.bbox)
        self.tra = coin.SoTransform()
        self.tra.rotation.setValue(FreeCAD.Rotation(0, 0, 90).Q)
        self.sep.addChild(self.tra)
        self.fon = coin.SoFont()
        self.sep.addChild(self.fon)
        self.txt = coin.SoAsciiText()
        self.txt.justification = coin.SoText2.LEFT
        self.txt.string.setValue("level")
        self.sep.addChild(self.txt)
        vobj.addDisplayMode(self.sep, "Default")
        self.onChanged(vobj, "ShapeColor")
        self.onChanged(vobj, "FontName")
        self.onChanged(vobj, "ShowLevel")
        self.onChanged(vobj, "FontSize")
        self.onChanged(vobj, "AutogroupBox")
        self.setProperties(vobj)
        return

    def getDisplayModes(self, vobj):

        return ["Default"]

    def getDefaultDisplayMode(self):

        return "Default"

    def setDisplayMode(self, mode):

        return mode

    def updateData(self, obj, prop):

        if prop in ["Placement", "LevelOffset"]:
            self.onChanged(obj.ViewObject, "OverrideUnit")
        elif prop == "Shape":
            # gather all the child shapes
            colors = self.getColors(obj)
            if colors and hasattr(obj.ViewObject, "DiffuseColor"):
                if len(colors) == len(obj.Shape.Faces):
                    if colors != obj.ViewObject.DiffuseColor:
                        obj.ViewObject.DiffuseColor = colors
                        self.writeInventor(obj)
                # else:
                # print("color mismatch:",len(colors),"colors,",len(obj.Shape.Faces),"faces")
        elif prop == "Group":
            self.onChanged(obj.ViewObject, "ChildrenOverride")
        elif prop == "Label":
            self.onChanged(obj.ViewObject, "ShowLabel")

    def getColors(self, obj):
        "recursively get the colors of objects inside this BuildingPart"

        colors = []
        for child in Draft.get_group_contents(obj, walls=True):
            if not Draft.get_type(child) in ["Space"]:
                if hasattr(child, "Shape") and (
                    hasattr(child.ViewObject, "DiffuseColor")
                    or hasattr(child.ViewObject, "ShapeColor")
                ):
                    if hasattr(child.ViewObject, "DiffuseColor") and len(
                        child.ViewObject.DiffuseColor
                    ) == len(child.Shape.Faces):
                        colors.extend(child.ViewObject.DiffuseColor)
                    else:
                        c = child.ViewObject.ShapeColor[:3] + (
                            1.0 - child.ViewObject.Transparency / 100.0,
                        )
                        for i in range(len(child.Shape.Faces)):
                            colors.append(c)
        return colors

    def onChanged(self, vobj, prop):

        # print(vobj.Object.Label," - ",prop)

        if prop == "ShapeColor":
            if hasattr(vobj, "ShapeColor"):
                l = vobj.ShapeColor
                self.mat.diffuseColor.setValue([l[0], l[1], l[2]])
        elif prop == "LineWidth":
            if hasattr(vobj, "LineWidth"):
                self.dst.lineWidth = vobj.LineWidth
        elif prop == "FontName":
            if hasattr(vobj, "FontName") and hasattr(self, "fon"):
                if vobj.FontName:
                    self.fon.name = vobj.FontName
        elif prop in ["FontSize", "DisplayOffset", "OriginOffset"]:
            if (
                hasattr(vobj, "FontSize")
                and hasattr(vobj, "DisplayOffset")
                and hasattr(vobj, "OriginOffset")
                and hasattr(self, "fon")
            ):
                fs = vobj.FontSize.Value
                if fs:
                    self.fon.size = fs
                    b = vobj.DisplayOffset.Base
                    self.tra.translation.setValue([b.x + fs / 8, b.y, b.z + fs / 8])
                    r = vobj.DisplayOffset.Rotation
                    self.tra.rotation.setValue(r.Q)
                    if vobj.OriginOffset:
                        self.lco.point.setValues(
                            [
                                [b.x - fs, b.y, b.z],
                                [b.x + fs, b.y, b.z],
                                [b.x, b.y - fs, b.z],
                                [b.x, b.y + fs, b.z],
                                [b.x, b.y, b.z - fs],
                                [b.x, b.y, b.z + fs],
                            ]
                        )
                    else:
                        self.lco.point.setValues(
                            [
                                [-fs, 0, 0],
                                [fs, 0, 0],
                                [0, -fs, 0],
                                [0, fs, 0],
                                [0, 0, -fs],
                                [0, 0, fs],
                            ]
                        )
        elif prop in ["OverrideUnit", "ShowUnit", "ShowLevel", "ShowLabel"]:
            if (
                hasattr(vobj, "OverrideUnit")
                and hasattr(vobj, "ShowUnit")
                and hasattr(vobj, "ShowLevel")
                and hasattr(vobj, "ShowLabel")
                and hasattr(self, "txt")
            ):
                offset = getattr(vobj.Object, "LevelOffset", 0)
                if hasattr(offset, "Value"):
                    offset = offset.Value
                z = vobj.Object.Placement.Base.z + offset
                q = FreeCAD.Units.Quantity(z, FreeCAD.Units.Length)
                txt = ""
                if vobj.ShowLabel:
                    txt += vobj.Object.Label
                if vobj.ShowLevel:
                    if txt:
                        txt += " "
                    if z >= 0:
                        txt += "+"
                    if vobj.OverrideUnit:
                        u = vobj.OverrideUnit
                    else:
                        u = q.getUserPreferred()[2]
                    try:
                        txt += units.display_external(float(q), None, "Length", vobj.ShowUnit, u)
                    except Exception:
                        q = q.getValueAs(q.getUserPreferred()[2])
                        d = params.get_param("Decimals", path="Units")
                        fmt = "{0:." + str(d) + "f}"
                        if not vobj.ShowUnit:
                            u = ""
                        txt += fmt.format(float(q)) + str(u)
                if not txt:
                    txt = " "  # empty texts make coin crash...
                if isinstance(txt, unicode):
                    txt = txt.encode("utf8")
                self.txt.string.setValue(txt)
        elif prop in [
            "ChildrenOverride",
            "ChildenLineWidth",
            "ChildrenLineColor",
            "ChildrenShapeColor",
            "ChildrenTransparency",
        ]:
            if hasattr(vobj, "ChildrenOverride") and vobj.ChildrenOverride:
                props = [
                    "ChildenLineWidth",
                    "ChildrenLineColor",
                    "ChildrenShapeColor",
                    "ChildrenTransparency",
                ]
                for child in vobj.Object.Group:
                    for prop in props:
                        if (
                            hasattr(vobj, prop)
                            and hasattr(child.ViewObject, prop[8:])
                            and not hasattr(child, "ChildrenOverride")
                        ):
                            setattr(child.ViewObject, prop[8:], getattr(vobj, prop))
        elif prop in ["CutView", "CutMargin"]:
            if (
                hasattr(vobj, "CutView")
                and FreeCADGui.ActiveDocument.ActiveView
                and hasattr(FreeCADGui.ActiveDocument.ActiveView, "getSceneGraph")
            ):
                sg = FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()
                if vobj.CutView:
                    from pivy import coin

                    if self.clip:
                        sg.removeChild(self.clip)
                        self.clip = None
                    for o in Draft.get_group_contents(vobj.Object.Group, walls=True):
                        if hasattr(o.ViewObject, "Lighting"):
                            o.ViewObject.Lighting = "One side"
                    self.clip = coin.SoClipPlane()
                    self.clip.on.setValue(True)
                    norm = vobj.Object.Placement.multVec(FreeCAD.Vector(0, 0, 1))
                    mp = vobj.Object.Placement.Base
                    mp = DraftVecUtils.project(mp, norm)
                    dist = mp.Length  # - 0.1 # to not clip exactly on the section object
                    norm = norm.negative()
                    marg = 1
                    if hasattr(vobj, "CutMargin"):
                        marg = vobj.CutMargin.Value
                    if mp.getAngle(norm) > 1:
                        dist += marg
                        dist = -dist
                    else:
                        dist -= marg
                    plane = coin.SbPlane(coin.SbVec3f(norm.x, norm.y, norm.z), dist)
                    self.clip.plane.setValue(plane)
                    sg.insertChild(self.clip, 0)
                else:
                    if getattr(self, "clip", None):
                        sg.removeChild(self.clip)
                        self.clip = None
                    for o in Draft.get_group_contents(vobj.Object.Group, walls=True):
                        if hasattr(o.ViewObject, "Lighting"):
                            o.ViewObject.Lighting = "Two side"
        elif prop == "Visibility":
            # turn clipping off when turning the object off
            if hasattr(vobj, "Visibility") and not (vobj.Visibility) and hasattr(vobj, "CutView"):
                vobj.CutView = False
        elif prop == "SaveInventor":
            self.writeInventor(vobj.Object)
        elif prop in ["AutogroupBox", "AutogroupSize"]:
            if hasattr(vobj, "AutogroupBox") and hasattr(vobj, "AutogroupSize"):
                if vobj.AutogroupBox:
                    if len(vobj.AutogroupSize) >= 6:
                        self.autobbox = FreeCAD.BoundBox(*vobj.AutogroupSize[0:6])
                        self.autobbox.move(vobj.Object.Placement.Base)
                        pts = [list(self.autobbox.getPoint(i)) for i in range(8)]
                        self.bbco.point.setValues(pts)
                        self.bbox.whichChild = 0
                else:
                    self.autobbox = None
                    self.bbox.whichChild = -1
        elif prop in ["AutogroupAutosize", "AutogroupMargin"]:
            if hasattr(vobj, "AutogroupAutosize") and vobj.AutogroupAutosize:
                bbox = vobj.Object.Shape.BoundBox
                bbox.enlarge(vobj.AutogroupMargin.Value)
                vobj.AutogroupSize = [
                    int(i)
                    for i in [bbox.XMin, bbox.YMin, bbox.ZMin, bbox.XMax, bbox.YMax, bbox.ZMax]
                ]

    def onDelete(self, vobj, subelements):

        if self.clip:
            sg = FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()
            sg.removeChild(self.clip)
            self.clip = None
        for o in Draft.get_group_contents(vobj.Object.Group, walls=True):
            if hasattr(o.ViewObject, "Lighting"):
                o.ViewObject.Lighting = "Two side"
        return True

    def setEdit(self, vobj, mode):
        # mode == 1 if Transform is selected in the Tree view context menu.
        # mode == 2 has been added for consistency.
        if mode == 1 or mode == 2:
            return None
        # For some reason mode is always 0 if the object is double-clicked in
        # the Tree view. Using FreeCADGui.getUserEditMode() as a workaround.
        if FreeCADGui.getUserEditMode() in ("Transform", "Cutting"):
            return None

        self.activate()
        return False  # Return `False` as we don't want to enter edit mode.

    def unsetEdit(self, vobj, mode):
        if mode == 1 or mode == 2:
            return None
        if FreeCADGui.getUserEditMode() in ("Transform", "Cutting"):
            return None

        return True

    def setupContextMenu(self, vobj, menu):
        from PySide import QtCore, QtGui
        import Draft_rc

        if FreeCADGui.activeWorkbench().name() != "BIMWorkbench":
            return
        if (not hasattr(vobj, "DoubleClickActivates")) or vobj.DoubleClickActivates:
            menuTxt = translate("Arch", "Active")
            actionActivate = QtGui.QAction(menuTxt, menu)
            actionActivate.setCheckable(True)
            if FreeCADGui.ActiveDocument.ActiveView.getActiveObject("Arch") == self.Object:
                actionActivate.setChecked(True)
            else:
                actionActivate.setChecked(False)
            actionActivate.triggered.connect(lambda _: self.activate(actionActivate))
            menu.addAction(actionActivate)

        actionSetWorkingPlane = QtGui.QAction(
            QtGui.QIcon(":/icons/Draft_SelectPlane.svg"),
            translate("Arch", "Set Working Plane"),
            menu,
        )
        QtCore.QObject.connect(
            actionSetWorkingPlane, QtCore.SIGNAL("triggered()"), self.setWorkingPlane
        )
        menu.addAction(actionSetWorkingPlane)

        actionWriteCamera = QtGui.QAction(
            QtGui.QIcon(":/icons/Draft_SelectPlane.svg"),
            translate("Arch", "Write Camera Position"),
            menu,
        )
        QtCore.QObject.connect(actionWriteCamera, QtCore.SIGNAL("triggered()"), self.writeCamera)
        menu.addAction(actionWriteCamera)

        actionCreateGroup = QtGui.QAction(translate("Arch", "New Group"), menu)
        QtCore.QObject.connect(actionCreateGroup, QtCore.SIGNAL("triggered()"), self.createGroup)
        menu.addAction(actionCreateGroup)

        actionReorder = QtGui.QAction(translate("Arch", "Reorder Children Alphabetically"), menu)
        QtCore.QObject.connect(actionReorder, QtCore.SIGNAL("triggered()"), self.reorder)
        menu.addAction(actionReorder)

        actionCloneUp = QtGui.QAction(translate("Arch", "Clone Level Up"), menu)
        QtCore.QObject.connect(actionCloneUp, QtCore.SIGNAL("triggered()"), self.cloneUp)
        menu.addAction(actionCloneUp)

    def activate(self, action=None):
        from draftutils.gui_utils import toggle_working_plane

        vobj = self.Object.ViewObject

        if (not hasattr(vobj, "DoubleClickActivates")) or vobj.DoubleClickActivates:
            if toggle_working_plane(self.Object, action, restore=True):
                print("Setting active working plane to: ", self.Object.Label)
            else:
                print("Deactivating working plane from: ", self.Object.Label)

        FreeCADGui.Selection.clearSelection()

    def setWorkingPlane(self, restore=False):
        vobj = self.Object.ViewObject

        import WorkingPlane

        wp = WorkingPlane.get_working_plane(update=False)
        autoclip = False
        if hasattr(vobj, "AutoCutView"):
            autoclip = vobj.AutoCutView
        if restore:
            if wp.label.rstrip("*") == self.Object.Label:
                prev_data = wp._previous()
                if prev_data:
                    prev_label = prev_data.get("label", "").rstrip("*")
                    prev_obj = None
                    for obj in FreeCAD.ActiveDocument.Objects:
                        if hasattr(obj, "Label") and obj.Label == prev_label:
                            prev_obj = obj
                            break

                    if prev_obj:
                        # check in which context we need to set the active object
                        context = "Arch"
                        obj_type = Draft.getType(prev_obj)
                        if obj_type == "IfcBuildingStorey":
                            context = "NativeIFC"
                        FreeCADGui.ActiveDocument.ActiveView.setActiveObject(context, prev_obj)
                        print(f"Set active object to: {prev_obj.Label} (context: {context})")

            if autoclip:
                vobj.CutView = False
        else:
            wp.align_to_selection()
            if autoclip:
                vobj.CutView = True

    def writeCamera(self):

        if hasattr(self, "Object"):
            from pivy import coin

            n = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
            FreeCAD.Console.PrintMessage(
                QT_TRANSLATE_NOOP("Draft", "Writing camera position") + "\n"
            )
            cdata = list(n.position.getValue().getValue())
            cdata.extend(list(n.orientation.getValue().getValue()))
            cdata.append(n.nearDistance.getValue())
            cdata.append(n.farDistance.getValue())
            cdata.append(n.aspectRatio.getValue())
            cdata.append(n.focalDistance.getValue())
            if isinstance(n, coin.SoOrthographicCamera):
                cdata.append(n.height.getValue())
                cdata.append(0.0)  # orthograhic camera
            elif isinstance(n, coin.SoPerspectiveCamera):
                cdata.append(n.heightAngle.getValue())
                cdata.append(1.0)  # perspective camera
            self.Object.ViewObject.ViewData = cdata

    def createGroup(self):

        if hasattr(self, "Object"):
            s = (
                'FreeCAD.ActiveDocument.getObject("%s").newObject("App::DocumentObjectGroup","Group")'
                % self.Object.Name
            )
            FreeCADGui.doCommand(s)

    def reorder(self):

        if hasattr(self, "Object"):
            if hasattr(self.Object, "Group") and self.Object.Group:
                g = self.Object.Group
                g.sort(key=lambda obj: obj.Label)
                self.Object.Group = g
                FreeCAD.ActiveDocument.recompute()

    def cloneUp(self):

        if hasattr(self, "Object"):
            if not self.Object.Height.Value:
                FreeCAD.Console.PrintError(
                    "This level has no height value. Define a height before using this function.\n"
                )
                return
            height = self.Object.Height.Value
            ng = []
            if hasattr(self.Object, "Group") and self.Object.Group:
                for o in self.Object.Group:
                    no = Draft.clone(o)
                    Draft.move(no, FreeCAD.Vector(0, 0, height))
                    ng.append(no)
            nobj = Arch.makeBuildingPart()
            Draft.formatObject(nobj, self.Object)
            nobj.Placement = self.Object.Placement
            nobj.Placement.move(FreeCAD.Vector(0, 0, height))
            nobj.IfcType = self.Object.IfcType
            nobj.Height = height
            nobj.Label = self.Object.Label
            nobj.Group = ng
            for parent in self.Object.InList:
                if (
                    hasattr(parent, "Group")
                    and hasattr(parent, "addObject")
                    and (self.Object in parent.Group)
                ):
                    parent.addObject(nobj)
            FreeCAD.ActiveDocument.recompute()
            # fix for missing IFC attributes
            for no in ng:
                if (
                    hasattr(no, "LongName")
                    and hasattr(no, "CloneOf")
                    and no.CloneOf
                    and hasattr(no.CloneOf, "LongName")
                ):
                    no.LongName = no.CloneOf.LongName
            FreeCAD.ActiveDocument.recompute()

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def writeInventor(self, obj):

        def callback(match):
            return next(callback.v)

        if hasattr(obj.ViewObject, "SaveInventor") and obj.ViewObject.SaveInventor:
            if obj.Shape and obj.Shape.Faces and hasattr(obj, "SavedInventor"):
                colors = obj.ViewObject.DiffuseColor
                if len(colors) != len(obj.Shape.Faces):
                    print("Debug: Colors mismatch in", obj.Label)
                    colors = None
                iv = self.Object.Shape.writeInventor()
                import re

                if colors:
                    if len(re.findall(r"IndexedFaceSet", iv)) == len(obj.Shape.Faces):
                        # convert colors to iv representations
                        colors = [
                            "Material { diffuseColor "
                            + str(color[0])
                            + " "
                            + str(color[1])
                            + " "
                            + str(color[2])
                            + "}\n    IndexedFaceSet"
                            for color in colors
                        ]
                        # replace
                        callback.v = iter(colors)
                        iv = re.sub(r"IndexedFaceSet", callback, iv)
                    else:
                        print("Debug: IndexedFaceSet mismatch in", obj.Label)
                # save embedded file
                tf = tempfile.mkstemp(prefix=obj.Name, suffix=".iv")[1]
                f = open(tf, "w")
                f.write(iv)
                f.close()
                obj.SavedInventor = tf
                os.remove(tf)
