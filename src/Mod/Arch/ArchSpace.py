# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013                                                    *
#*   Yorik van Havre <yorik@uncreated.net>                                 *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

__title__="FreeCAD Arch Space"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

SpaceTypes = [
"Undefined",
"Exterior",
"Exterior - Terrace",
"Office",
"Office - Enclosed",
"Office - Open Plan",
"Conference / Meeting / Multipurpose",
"Classroom / Lecture / Training For Penitentiary",
"Lobby",
"Lobby - For Hotel",
"Lobby - For Performing Arts Theater",
"Lobby - For Motion Picture Theater",
"Audience/Seating Area",
"Audience/Seating Area - For Gymnasium",
"Audience/Seating Area - For Exercise Center",
"Audience/Seating Area - For Convention Center",
"Audience/Seating Area - For Penitentiary",
"Audience/Seating Area - For Religious Buildings",
"Audience/Seating Area - For Sports Arena",
"Audience/Seating Area - For Performing Arts Theater",
"Audience/Seating Area - For Motion Picture Theater",
"Audience/Seating Area - For Transportation",
"Atrium",
"Atrium - First Three Floors",
"Atrium - Each Additional Floor",
"Lounge / Recreation",
"Lounge / Recreation - For Hospital",
"Dining Area",
"Dining Area - For Penitentiary",
"Dining Area - For Hotel",
"Dining Area - For Motel",
"Dining Area - For Bar Lounge/Leisure Dining",
"Dining Area - For Family Dining",
"Food Preparation",
"Laboratory",
"Restrooms",
"Dressing / Locker / Fitting",
"Room",
"Corridor / Transition",
"Corridor / Transition - For Hospital",
"Corridor / Transition - For Manufacturing Facility",
"Stairs",
"Active Storage",
"Active Storage - For Hospital",
"Inactive Storage",
"Inactive Storage - For Museum",
"Electrical / Mechanical",
"Gymnasium / Exercise Center",
"Gymnasium / Exercise Center - Playing Area",
"Gymnasium / Exercise Center - Exercise Area",
"Courthouse / Police Station / Penitentiary",
"Courthouse / Police Station / Penitentiary - Courtroom",
"Courthouse / Police Station / Penitentiary - Confinement Cells",
"Courthouse / Police Station / Penitentiary - Judges' Chambers",
"Fire Stations",
"Fire Stations - Engine Room",
"Fire Stations - Sleeping Quarters",
"Post Office - Sorting Area",
"Convention Center - Exhibit Space",
"Library",
"Library - Card File and Cataloging",
"Library - Stacks",
"Library - Reading Area",
"Hospital",
"Hospital - Emergency",
"Hospital - Recovery",
"Hospital - Nurses' Station",
"Hospital - Exam / Treatment",
"Hospital - Pharmacy",
"Hospital - Patient Room",
"Hospital - Operating Room",
"Hospital - Nursery",
"Hospital - Medical Supply",
"Hospital - Physical Therapy",
"Hospital - Radiology",
"Hospital - Laundry-Washing",
"Automotive - Service / Repair",
"Manufacturing",
"Manufacturing - Low Bay (< 7.5m Floor to Ceiling Height)",
"Manufacturing - High Bay (> 7.5m Floor to Ceiling Height)",
"Manufacturing - Detailed Manufacturing",
"Manufacturing - Equipment Room",
"Manufacturing - Control Room",
"Hotel / Motel Guest Rooms",
"Dormitory - Living Quarters",
"Museum",
"Museum - General Exhibition",
"Museum - Restoration",
"Bank / Office - Banking Activity Area",
"Workshop",
"Sales Area",
"Religious Buildings",
"Religious Buildings - Worship Pulpit, Choir",
"Religious Buildings - Fellowship Hall",
"Retail",
"Retail - Sales Area",
"Retail - Mall Concourse",
"Sports Arena",
"Sports Arena - Ring Sports Area",
"Sports Arena - Court Sports Area",
"Sports Arena - Indoor Playing Field Area",
"Warehouse",
"Warehouse - Fine Material Storage",
"Warehouse - Medium / Bulky Material Storage",
"Parking Garage - Garage Area",
"Transportation",
"Transportation - Airport / Concourse",
"Transportation - Air / Train / Bus - Baggage Area",
"Transportation - Terminal - Ticket Counter"
]

ConditioningTypes = [
"Unconditioned",
"Heated",
"Cooled",
"HeatedAndCooled",
"Vented",
"NaturallyVentedOnly"
]

import FreeCAD,ArchComponent,ArchCommands,math,Draft,sys
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchSpace
#  \ingroup ARCH
#  \brief The Space object and tools
#
#  This module provides tools to build Space objects.
#  Spaces define an open volume inside or outside a
#  building, ie. a room.

def makeSpace(objects=None,baseobj=None,name="Space"):

    """makeSpace([objects]): Creates a space object from the given objects. Objects can be one
    document object, in which case it becomes the base shape of the space object, or a list of
    selection objects as got from getSelectionEx(), or a list of tuples (object, subobjectname)"""

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Space")
    obj.Label = translate("Arch",name)
    _Space(obj)
    if FreeCAD.GuiUp:
        _ViewProviderSpace(obj.ViewObject)
    if baseobj:
        objects = baseobj
    if objects:
        if not isinstance(objects,list):
            objects = [objects]
        if len(objects) == 1:
            obj.Base = objects[0]
            if FreeCAD.GuiUp:
                objects[0].ViewObject.hide()
        else:
            obj.Proxy.addSubobjects(obj,objects)
    return obj

def addSpaceBoundaries(space,subobjects):

    """addSpaceBoundaries(space,subobjects): adds the given subobjects to the given space"""

    import Draft
    if Draft.getType(space) == "Space":
        space.Proxy.addSubobjects(space,subobjects)

def removeSpaceBoundaries(space,objects):

    """removeSpaceBoundaries(space,objects): removes the given objects from the given spaces boundaries"""

    import Draft
    if Draft.getType(space) == "Space":
        bounds = space.Boundaries
        for o in objects:
            for b in bounds:
                if o.Name == b[0].Name:
                    bounds.remove(b)
                    break
        space.Boundaries = bounds

class _CommandSpace:

    "the Arch Space command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Space',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Space","Space"),
                'Accel': "S, P",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Space","Creates a space object from selected boundary objects")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Space"))
        FreeCADGui.addModule("Arch")
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            FreeCADGui.Control.closeDialog()
            if len(sel) == 1:
                FreeCADGui.doCommand("obj = Arch.makeSpace(FreeCADGui.Selection.getSelection())")
            else:
                FreeCADGui.doCommand("obj = Arch.makeSpace(FreeCADGui.Selection.getSelectionEx())")
            FreeCADGui.addModule("Draft")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        else:
            FreeCAD.Console.PrintMessage(translate("Arch","Please select a base object")+"\n")
            FreeCADGui.Control.showDialog(ArchComponent.SelectionTaskPanel())
            FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(nextCommand="Arch_Space")
            FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)


class _Space(ArchComponent.Component):

    "A space object"

    def __init__(self,obj):

        ArchComponent.Component.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Space"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Boundaries" in pl:
            obj.addProperty("App::PropertyLinkSubList","Boundaries",    "Space",QT_TRANSLATE_NOOP("App::Property","The objects that make the boundaries of this space object"))
        if not "Area" in pl:
            obj.addProperty("App::PropertyArea",       "Area",          "Space",QT_TRANSLATE_NOOP("App::Property","The computed floor area of this space"))
        if not "FinishFloor" in pl:
            obj.addProperty("App::PropertyString",     "FinishFloor",   "Space",QT_TRANSLATE_NOOP("App::Property","The finishing of the floor of this space"))
        if not "FinishWalls" in pl:
            obj.addProperty("App::PropertyString",     "FinishWalls",   "Space",QT_TRANSLATE_NOOP("App::Property","The finishing of the walls of this space"))
        if not "FinishCeiling" in pl:
            obj.addProperty("App::PropertyString",     "FinishCeiling", "Space",QT_TRANSLATE_NOOP("App::Property","The finishing of the ceiling of this space"))
        if not "Group" in pl:
            obj.addProperty("App::PropertyLinkList",   "Group",         "Space",QT_TRANSLATE_NOOP("App::Property","Objects that are included inside this space, such as furniture"))
        if not "SpaceType" in pl:
            obj.addProperty("App::PropertyEnumeration","SpaceType",     "Space",QT_TRANSLATE_NOOP("App::Property","The type of this space"))
            obj.SpaceType = SpaceTypes
        if not "FloorThickness" in pl:
            obj.addProperty("App::PropertyLength",     "FloorThickness","Space",QT_TRANSLATE_NOOP("App::Property","The thickness of the floor finish"))
        if not "Zone" in pl:
            obj.addProperty("App::PropertyLink",       "Zone",          "Space",QT_TRANSLATE_NOOP("App::Property","A zone this space is part of"))
        if not "NumberOfPeople" in pl:
            obj.addProperty("App::PropertyInteger",    "NumberOfPeople","Space",QT_TRANSLATE_NOOP("App::Property","The number of people who typically occupy this space"))
        if not "LightingPower" in pl:
            obj.addProperty("App::PropertyFloat",      "LightingPower", "Space",QT_TRANSLATE_NOOP("App::Property","The electric power needed to light this space in Watts"))
        if not "EquipmentPower" in pl:
            obj.addProperty("App::PropertyFloat",      "EquipmentPower","Space",QT_TRANSLATE_NOOP("App::Property","The electric power needed by the equipment of this space in Watts"))
        if not "AutoPower" in pl:
            obj.addProperty("App::PropertyBool",       "AutoPower",     "Space",QT_TRANSLATE_NOOP("App::Property","If True, Equipment Power will be automatically filled by the equipment included in this space"))
        if not "Conditioning" in pl:
            obj.addProperty("App::PropertyEnumeration","Conditioning",  "Space",QT_TRANSLATE_NOOP("App::Property","The type of air conditioning of this space"))
            obj.Conditioning = ConditioningTypes
        if not "Internal" in pl:
            obj.addProperty("App::PropertyBool",       "Internal",     "Space",QT_TRANSLATE_NOOP("App::Property","Specifies if this space is internal or external"))
            obj.Internal = True
        self.Type = "Space"
        obj.setEditorMode("HorizontalArea",2)

    def onDocumentRestored(self,obj):

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def execute(self,obj):

        if self.clone(obj):
            return
        self.getShape(obj)

    def onChanged(self,obj,prop):

        if prop == "Group":
            if hasattr(obj,"EquipmentPower"):
                if obj.AutoPower:
                    p = 0
                    for o in Draft.getObjectsOfType(Draft.getGroupContents(obj.Group,addgroups=True),"Equipment"):
                        if hasattr(o,"EquipmentPower"):
                            p += o.EquipmentPower
                    if p != obj.EquipmentPower:
                        obj.EquipmentPower = p
        elif prop == "Zone":
            if obj.Zone:
                if obj.Zone.ViewObject:
                    if hasattr(obj.Zone.ViewObject,"Proxy"):
                        if hasattr(obj.Zone.ViewObject.Proxy,"claimChildren"):
                            obj.Zone.ViewObject.Proxy.claimChildren()
        if hasattr(obj,"Area"):
            obj.setEditorMode('Area',1)
        ArchComponent.Component.onChanged(self,obj,prop)

    def addSubobjects(self,obj,subobjects):

        "adds subobjects to this space"
        objs = obj.Boundaries
        for o in subobjects:
            if isinstance(o,tuple) or isinstance(o,list):
                if o[0].Name != obj.Name:
                    objs.append(tuple(o))
            else:
                for el in o.SubElementNames:
                    if "Face" in el:
                        if o.Object.Name != obj.Name:
                            objs.append((o.Object,el))
        obj.Boundaries = objs

    def getShape(self,obj):

        "computes a shape from a base shape and/or bounday faces"
        import Part
        shape = None
        faces = []

        pl = obj.Placement

        #print("starting compute")
        # 1: if we have a base shape, we use it

        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.Solids:
                    shape = obj.Base.Shape.copy()
                    shape = shape.removeSplitter()

        # 2: if not, add all bounding boxes of considered objects and build a first shape
        if shape:
            #print("got shape from base object")
            bb = shape.BoundBox
        else:
            bb = None
            for b in obj.Boundaries:
                if b[0].isDerivedFrom("Part::Feature"):
                    if not bb:
                        bb = b[0].Shape.BoundBox
                    else:
                        bb.add(b[0].Shape.BoundBox)
            if not bb:
                return
            shape = Part.makeBox(bb.XLength,bb.YLength,bb.ZLength,FreeCAD.Vector(bb.XMin,bb.YMin,bb.ZMin))
            #print("created shape from boundbox")

        # 3: identifying boundary faces
        goodfaces = []
        for b in obj.Boundaries:
                if b[0].isDerivedFrom("Part::Feature"):
                    for sub in b[1]:
                        if "Face" in sub:
                            fn = int(sub[4:])-1
                            faces.append(b[0].Shape.Faces[fn])
                            #print("adding face ",fn," of object ",b[0].Name)

        #print("total: ", len(faces), " faces")

        # 4: get cutvolumes from faces
        cutvolumes = []
        for f in faces:
            f = f.copy()
            f.reverse()
            cutface,cutvolume,invcutvolume = ArchCommands.getCutVolume(f,shape)
            if cutvolume:
                #print("generated 1 cutvolume")
                cutvolumes.append(cutvolume.copy())
                #Part.show(cutvolume)
        for v in cutvolumes:
            #print("cutting")
            shape = shape.cut(v)

        # 5: get the final shape
        if shape:
            if shape.Solids:
                #print("setting objects shape")
                shape = shape.Solids[0]
                obj.Shape = shape
                #pl = pl.multiply(obj.Placement)
                obj.Placement = pl
                if hasattr(obj.Area,"Value"):
                    a = self.getArea(obj)
                    if obj.Area.Value != a:
                        obj.Area = a
                return

        print("Arch: error computing space boundary")

    def getArea(self,obj,notouch=False):

        "returns the horizontal area at the center of the space"

        import Part,DraftGeomUtils
        if not hasattr(obj.Shape,"CenterOfMass"):
            return 0
        try:
            pl = Part.makePlane(1,1)
            pl.translate(obj.Shape.CenterOfMass)
            sh = obj.Shape.copy()
            cutplane,v1,v2 = ArchCommands.getCutVolume(pl,sh)
            e = sh.section(cutplane)
            e = Part.__sortEdges__(e.Edges)
            w = Part.Wire(e)
            self.face = Part.Face(w)
        except Part.OCCError:
            return 0
        else:
            if not notouch:
                if hasattr(obj,"PerimeterLength"):
                    if w.Length != obj.PerimeterLength.Value:
                        obj.PerimeterLength = w.Length
                if hasattr(obj,"VerticalArea"):
                    a = 0
                    for f in sh.Faces:
                        ang = f.normalAt(0,0).getAngle(FreeCAD.Vector(0,0,1))
                        if (ang > 1.57) and (ang < 1.571):
                            a += f.Area
                        if a != obj.VerticalArea.Value:
                            obj.VerticalArea = a
            #print "area of ",obj.Label," : ",f.Area
            return self.face.Area


class _ViewProviderSpace(ArchComponent.ViewProviderComponent):

    "A View Provider for Section Planes"
    def __init__(self,vobj):

        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        self.setProperties(vobj)
        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        vobj.Transparency = prefs.GetInt("defaultSpaceTransparency",85)
        vobj.LineWidth = Draft.getParam("linewidth")
        vobj.LineColor = ArchCommands.getDefaultColor("Space")
        vobj.DrawStyle = ["Solid","Dashed","Dotted","Dashdot"][prefs.GetInt("defaultSpaceStyle",2)]
        if prefs.GetInt("defaultSpaceTransparency",85) == 100:
            vobj.DisplayMode = "Wireframe"

    def setProperties(self,vobj):

        pl = vobj.PropertiesList
        if not "Text" in pl:
            vobj.addProperty("App::PropertyStringList",    "Text",        "Space",QT_TRANSLATE_NOOP("App::Property","The text to show. Use $area, $label, $tag, $floor, $walls, $ceiling to insert the respective data"))
            vobj.Text = ["$label","$area"]
        if not "FontName" in pl:
            vobj.addProperty("App::PropertyFont",          "FontName",    "Space",QT_TRANSLATE_NOOP("App::Property","The name of the font"))
            vobj.FontName = Draft.getParam("textfont","")
        if not "TextColor" in pl:
            vobj.addProperty("App::PropertyColor",         "TextColor",   "Space",QT_TRANSLATE_NOOP("App::Property","The color of the area text"))
            vobj.TextColor = (0.0,0.0,0.0,1.0)
        if not "FontSize" in pl:
            vobj.addProperty("App::PropertyLength",        "FontSize",    "Space",QT_TRANSLATE_NOOP("App::Property","The size of the text font"))
            vobj.FontSize = Draft.getParam("textheight",10)
        if not "FirstLine" in pl:
            vobj.addProperty("App::PropertyLength",        "FirstLine",   "Space",QT_TRANSLATE_NOOP("App::Property","The size of the first line of text"))
            vobj.FirstLine = Draft.getParam("textheight",10)
        if not "LineSpacing" in pl:
            vobj.addProperty("App::PropertyFloat",         "LineSpacing", "Space",QT_TRANSLATE_NOOP("App::Property","The space between the lines of text"))
            vobj.LineSpacing = 1.0
        if not "TextPosition" in pl:
            vobj.addProperty("App::PropertyVectorDistance","TextPosition","Space",QT_TRANSLATE_NOOP("App::Property","The position of the text. Leave (0,0,0) for automatic position"))
        if not "TextAlign" in pl:
            vobj.addProperty("App::PropertyEnumeration",   "TextAlign",   "Space",QT_TRANSLATE_NOOP("App::Property","The justification of the text"))
            vobj.TextAlign = ["Left","Center","Right"]
        if not "Decimals" in pl:
            vobj.addProperty("App::PropertyInteger",       "Decimals",    "Space",QT_TRANSLATE_NOOP("App::Property","The number of decimals to use for calculated texts"))
            vobj.Decimals = Draft.getParam("dimPrecision",2)
        if not "ShowUnit" in pl:
            vobj.addProperty("App::PropertyBool",          "ShowUnit",    "Space",QT_TRANSLATE_NOOP("App::Property","Show the unit suffix"))
            vobj.ShowUnit = Draft.getParam("showUnit",True)

    def onDocumentRestored(self,vobj):

        self.setProperties(vobj)

    def getIcon(self):

        import Arch_rc
        if hasattr(self,"Object"):
            if hasattr(self.Object,"CloneOf"):
                if self.Object.CloneOf:
                    return ":/icons/Arch_Space_Clone.svg"
        return ":/icons/Arch_Space_Tree.svg"

    def attach(self,vobj):

        ArchComponent.ViewProviderComponent.attach(self,vobj)
        from pivy import coin
        self.color = coin.SoBaseColor()
        self.font = coin.SoFont()
        self.text1 = coin.SoAsciiText()
        self.text1.string = " "
        self.text1.justification = coin.SoAsciiText.LEFT
        self.text2 = coin.SoAsciiText()
        self.text2.string = " "
        self.text2.justification = coin.SoAsciiText.LEFT
        self.coords = coin.SoTransform()
        self.header = coin.SoTransform()
        self.label = coin.SoSwitch()
        sep = coin.SoSeparator()
        self.label.whichChild = 0
        sep.addChild(self.coords)
        sep.addChild(self.color)
        sep.addChild(self.font)
        sep.addChild(self.text2)
        sep.addChild(self.header)
        sep.addChild(self.text1)
        self.label.addChild(sep)
        vobj.Annotation.addChild(self.label)
        self.onChanged(vobj,"TextColor")
        self.onChanged(vobj,"FontSize")
        self.onChanged(vobj,"FirstLine")
        self.onChanged(vobj,"LineSpacing")
        self.onChanged(vobj,"FontName")

    def updateData(self,obj,prop):

        if prop in ["Shape","Label","Tag","Area"]:
            self.onChanged(obj.ViewObject,"Text")
            self.onChanged(obj.ViewObject,"TextPosition")

    def getTextPosition(self,vobj):

        pos = FreeCAD.Vector()
        if hasattr(vobj,"TextPosition"):
            import DraftVecUtils
            if DraftVecUtils.isNull(vobj.TextPosition):
                try:
                    pos = vobj.Object.Shape.CenterOfMass
                    z = vobj.Object.Shape.BoundBox.ZMin
                    pos = FreeCAD.Vector(pos.x,pos.y,z)
                except (AttributeError, RuntimeError):
                    pos = FreeCAD.Vector()
            else:
                pos = vobj.TextPosition
        # placement's displacement will be already added by the coin node
        pos = vobj.Object.Placement.inverse().multVec(pos)
        return pos

    def onChanged(self,vobj,prop):

        if prop in ["Text","Decimals","ShowUnit"]:
            if hasattr(self,"text1") and hasattr(self,"text2") and hasattr(vobj,"Text"):
                self.text1.string.deleteValues(0)
                self.text2.string.deleteValues(0)
                text1 = []
                text2 = []
                first = True
                for t in vobj.Text:
                    if t:
                        if hasattr(vobj.Object,"Area"):
                            from FreeCAD import Units
                            q = Units.Quantity(vobj.Object.Area.Value,Units.Area).getUserPreferred()
                            qt = vobj.Object.Area.Value/q[1]
                            if hasattr(vobj,"Decimals"):
                                if vobj.Decimals == 0:
                                    qt = str(int(qt))
                                else:
                                    f = "%."+str(abs(vobj.Decimals))+"f"
                                    qt = f % qt
                            else:
                                qt = str(qt)
                            if hasattr(vobj,"ShowUnit"):
                                if vobj.ShowUnit:
                                    qt = qt + q[2].replace("^2",u"\xb2") # square symbol
                            t = t.replace("$area",qt)
                        t = t.replace("$label",vobj.Object.Label)
                        if hasattr(vobj.Object,"Tag"):
                            t = t.replace("$tag",vobj.Object.Tag)
                        if hasattr(vobj.Object,"FinishFloor"):
                            t = t.replace("$floor",vobj.Object.FinishFloor)
                        if hasattr(vobj.Object,"FinishWalls"):
                            t = t.replace("$walls",vobj.Object.FinishWalls)
                        if hasattr(vobj.Object,"FinishCeiling"):
                            t = t.replace("$ceiling",vobj.Object.FinishCeiling)
                        if sys.version_info.major < 3:
                            t = t.encode("utf8")
                        if first:
                            text1.append(t)
                        else:
                            text2.append(t)
                    first = False
                if text1:
                    self.text1.string.setValues(text1)
                if text2:
                    self.text2.string.setValues(text2)

        elif prop == "FontName":
            if hasattr(self,"font") and hasattr(vobj,"FontName"):
                self.font.name = str(vobj.FontName)

        elif (prop == "FontSize"):
            if hasattr(self,"font") and hasattr(vobj,"FontSize"):
                self.font.size = vobj.FontSize.Value
                if hasattr(vobj,"FirstLine"):
                    scale = vobj.FirstLine.Value/vobj.FontSize.Value
                    self.header.scaleFactor.setValue([scale,scale,scale])

        elif (prop == "FirstLine"):
            if hasattr(self,"header") and hasattr(vobj,"FontSize") and hasattr(vobj,"FirstLine"):
                scale = vobj.FirstLine.Value/vobj.FontSize.Value
                self.header.scaleFactor.setValue([scale,scale,scale])

        elif prop == "TextColor":
            if hasattr(self,"color") and hasattr(vobj,"TextColor"):
                c = vobj.TextColor
                self.color.rgb.setValue(c[0],c[1],c[2])

        elif prop == "TextPosition":
            if hasattr(self,"coords") and hasattr(self,"header") and hasattr(vobj,"TextPosition") and hasattr(vobj,"FirstLine"):
                pos = self.getTextPosition(vobj)
                self.coords.translation.setValue([pos.x,pos.y,pos.z])
                up = vobj.FirstLine.Value * vobj.LineSpacing
                self.header.translation.setValue([0,up,0])

        elif prop == "LineSpacing":
            if hasattr(self,"text1") and hasattr(self,"text2") and hasattr(vobj,"LineSpacing"):
                self.text1.spacing = vobj.LineSpacing
                self.text2.spacing = vobj.LineSpacing
                self.onChanged(vobj,"TextPosition")

        elif prop == "TextAlign":
            if hasattr(self,"text1") and hasattr(self,"text2") and hasattr(vobj,"TextAlign"):
                from pivy import coin
                if vobj.TextAlign == "Center":
                    self.text1.justification = coin.SoAsciiText.CENTER
                    self.text2.justification = coin.SoAsciiText.CENTER
                elif vobj.TextAlign == "Right":
                    self.text1.justification = coin.SoAsciiText.RIGHT
                    self.text2.justification = coin.SoAsciiText.RIGHT
                else:
                    self.text1.justification = coin.SoAsciiText.LEFT
                    self.text2.justification = coin.SoAsciiText.LEFT

        elif prop == "Visibility":
            if vobj.Visibility:
                self.label.whichChild = 0
            else:
                self.label.whichChild = -1

    def setEdit(self,vobj,mode):
        taskd = SpaceTaskPanel()
        taskd.obj = self.Object
        taskd.update()
        taskd.updateBoundaries()
        FreeCADGui.Control.showDialog(taskd)
        return True


class SpaceTaskPanel(ArchComponent.ComponentTaskPanel):

    "A modified version of the Arch component task panel"

    def __init__(self):

        ArchComponent.ComponentTaskPanel.__init__(self)
        self.editButton = QtGui.QPushButton(self.form)
        self.editButton.setObjectName("editButton")
        self.editButton.setIcon(QtGui.QIcon(":/icons/Draft_Edit.svg"))
        self.grid.addWidget(self.editButton, 4, 0, 1, 2)
        self.editButton.setText(QtGui.QApplication.translate("Arch", "Set text position", None))
        QtCore.QObject.connect(self.editButton, QtCore.SIGNAL("clicked()"), self.setTextPos)
        boundLabel = QtGui.QLabel(self.form)
        self.grid.addWidget(boundLabel, 5, 0, 1, 2)
        boundLabel.setText(QtGui.QApplication.translate("Arch", "Space boundaries", None))
        self.boundList = QtGui.QListWidget(self.form)
        self.grid.addWidget(self.boundList, 6, 0, 1, 2)
        self.addCompButton = QtGui.QPushButton(self.form)
        self.addCompButton.setObjectName("addCompButton")
        self.addCompButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        self.grid.addWidget(self.addCompButton, 7, 0, 1, 1)
        self.addCompButton.setText(QtGui.QApplication.translate("Arch", "Add", None))
        QtCore.QObject.connect(self.addCompButton, QtCore.SIGNAL("clicked()"), self.addBoundary)
        self.delCompButton = QtGui.QPushButton(self.form)
        self.delCompButton.setObjectName("delCompButton")
        self.delCompButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        self.grid.addWidget(self.delCompButton, 7, 1, 1, 1)
        self.delCompButton.setText(QtGui.QApplication.translate("Arch", "Remove", None))
        QtCore.QObject.connect(self.delCompButton, QtCore.SIGNAL("clicked()"), self.delBoundary)

    def updateBoundaries(self):

        self.boundList.clear()
        if self.obj:
            for b in self.obj.Boundaries:
                s = b[0].Label
                for n in b[1]:
                    s += ", " + n
                it = QtGui.QListWidgetItem(s)
                it.setToolTip(b[0].Name)
                self.boundList.addItem(it)

    def setTextPos(self):

        FreeCADGui.runCommand("Draft_Edit")

    def addBoundary(self):

        if self.obj:
            if FreeCADGui.Selection.getSelectionEx():
                self.obj.Proxy.addSubobjects(self.obj,FreeCADGui.Selection.getSelectionEx())
                self.updateBoundaries()

    def delBoundary(self):

        if self.boundList.currentRow() >= 0:
            it = self.boundList.item(self.boundList.currentRow())
            if it and self.obj:
                on = it.toolTip()
                bounds = self.obj.Boundaries
                for b in bounds:
                    if b[0].Name == on:
                        bounds.remove(b)
                        break
                self.obj.Boundaries = bounds
                self.updateBoundaries()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Space',_CommandSpace())
