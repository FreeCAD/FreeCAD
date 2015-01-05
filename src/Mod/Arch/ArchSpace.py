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

Roles = ["Space"]

import FreeCAD,ArchComponent,ArchCommands,math,Draft
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

def makeSpace(objects=None,baseobj=None,name="Space"):
    """makeSpace([objects]): Creates a space object from the given objects. Objects can be one
    document object, in which case it becomes the base shape of the space object, or a list of
    selection objects as got from getSelectionEx(), or a list of tuples (object, subobjectname)"""
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
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
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Space","Space"),
                'Accel': "S, P",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Space","Creates a space object from selected boundary objects")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Space"))
        FreeCADGui.addModule("Arch")
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            FreeCADGui.Control.closeDialog()
            if len(sel) == 1:
                FreeCADGui.doCommand("Arch.makeSpace(FreeCADGui.Selection.getSelection())")
            else:
                FreeCADGui.doCommand("Arch.makeSpace(FreeCADGui.Selection.getSelectionEx())")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        else:
            FreeCAD.Console.PrintMessage(translate("Arch","Please select a base object\n"))
            FreeCADGui.Control.showDialog(ArchComponent.SelectionTaskPanel())
            FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(nextCommand="Arch_Space")
            FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)


class _Space(ArchComponent.Component):
    "A space object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLinkSubList","Boundaries",   "Arch",translate("Arch","The objects that make the boundaries of this space object"))
        obj.addProperty("App::PropertyFloat",      "Area",         "Arch",translate("Arch","The computed floor area of this space"))
        obj.addProperty("App::PropertyString",     "FinishFloor",  "Arch",translate("Arch","The finishing of the floor of this space"))
        obj.addProperty("App::PropertyString",     "FinishWalls",  "Arch",translate("Arch","The finishing of the walls of this space"))
        obj.addProperty("App::PropertyString",     "FinishCeiling","Arch",translate("Arch","The finishing of the ceiling of this space"))
        obj.addProperty("App::PropertyLinkList",   "Group",        "Arch",translate("Arch","Objects that are included inside this space, such as furniture"))
        self.Type = "Space"
        obj.Role = Roles

    def execute(self,obj):
        self.getShape(obj)

    def onChanged(self,obj,prop):
        if prop in ["Boundaries","Base"]:
            self.getShape(obj)
            obj.Area = self.getArea(obj)
        if hasattr(obj,"Area"):
            obj.setEditorMode('Area',1)

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

        #print "starting compute"
        # 1: if we have a base shape, we use it

        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.Solids:
                    shape = obj.Base.Shape.Solids[0].copy()
                    shape = shape.removeSplitter()

        # 2: if not, add all bounding boxes of considered objects and build a first shape
        if shape:
            #print "got shape from base object"
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
            #print "created shape from boundbox"

        # 3: identifing boundary faces
        goodfaces = []
        for b in obj.Boundaries:
                if b[0].isDerivedFrom("Part::Feature"):
                    if "Face" in b[1]:
                        fn = int(b[1][4:])-1
                        faces.append(b[0].Shape.Faces[fn])
                        #print "adding face ",fn," of object ",b[0].Name

        #print "total: ", len(faces), " faces"

        # 4: get cutvolumes from faces
        cutvolumes = []
        for f in faces:
            f = f.copy()
            f.reverse()
            cutface,cutvolume,invcutvolume = ArchCommands.getCutVolume(f,shape)
            if cutvolume:
                #print "generated 1 cutvolume"
                cutvolumes.append(cutvolume.copy())
                #Part.show(cutvolume)
        for v in cutvolumes:
            #print "cutting"
            shape = shape.cut(v)

        # 5: get the final shape
        if shape:
            if shape.Solids:
                #print "setting objects shape"
                shape = shape.Solids[0]
                obj.Shape = shape
                return

        print "Arch: error computing space boundary"

    def getArea(self,obj):
        "returns the horizontal area at the center of the space"
        import Part,DraftGeomUtils
        try:
            pl = Part.makePlane(1,1)
            pl.translate(obj.Shape.CenterOfMass)
            sh = obj.Shape.copy()
            cutplane,v1,v2 = ArchCommands.getCutVolume(pl,sh)
            e = sh.section(cutplane)
            e = DraftGeomUtils.sortEdges(e.Edges)
            w = Part.Wire(e)
            f = Part.Face(w)
            return f.Area
        except Part.OCCError:
            return 0


class _ViewProviderSpace(ArchComponent.ViewProviderComponent):
    "A View Provider for Section Planes"
    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        vobj.Transparency = 85
        vobj.LineWidth = 1
        vobj.LineColor = (1.0,0.0,0.0,1.0)
        vobj.DrawStyle = "Dotted"
        vobj.addProperty("App::PropertyStringList", "Text",        "Arch",translate("Arch","The text to show. Use $area, $label, $tag, $floor, $walls, $ceiling to insert the respective data"))
        vobj.addProperty("App::PropertyString",     "FontName",    "Arch",translate("Arch","The name of the font"))
        vobj.addProperty("App::PropertyColor",      "TextColor",   "Arch",translate("Arch","The color of the area text"))
        vobj.addProperty("App::PropertyLength",     "FontSize",    "Arch",translate("Arch","The size of the text font"))
        vobj.addProperty("App::PropertyLength",     "FirstLine",  "Arch",translate("Arch","The size of the first line of text"))
        vobj.addProperty("App::PropertyFloat",      "LineSpacing", "Arch",translate("Arch","The space between the lines of text"))
        vobj.addProperty("App::PropertyVector",     "TextPosition","Arch",translate("Arch","The position of the text. Leave (0,0,0) for automatic position"))
        vobj.addProperty("App::PropertyEnumeration","TextAlign",   "Arch",translate("Arch","The justification of the text"))
        vobj.addProperty("App::PropertyInteger",    "Decimals",    "Arch",translate("Arch","The number of decimals to use for calculated texts"))
        vobj.addProperty("App::PropertyBool",       "ShowUnit",    "Arch",translate("Arch","Show the unit suffix"))
        vobj.TextColor = (0.0,0.0,0.0,1.0)
        vobj.Text = ["$label","$area"]
        vobj.TextAlign = ["Left","Center","Right"]
        vobj.FontSize = Draft.getParam("textheight",10)
        vobj.FirstLine = Draft.getParam("textheight",10)
        vobj.FontName = Draft.getParam("textfont","")
        vobj.Decimals = Draft.getParam("dimPrecision",2)
        vobj.ShowUnit = Draft.getParam("showUnit",True)
        vobj.LineSpacing = 1.0

    def getDefaultDisplayMode(self):
        return "Wireframe"

    def getIcon(self):
        import Arch_rc
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
        label = coin.SoSeparator()
        label.addChild(self.coords)
        label.addChild(self.color)
        label.addChild(self.font)
        label.addChild(self.text2)
        label.addChild(self.header)
        label.addChild(self.text1)
        vobj.Annotation.addChild(label)
        self.onChanged(vobj,"TextColor")
        self.onChanged(vobj,"FontSize")
        self.onChanged(vobj,"FirstLine")
        self.onChanged(vobj,"LineSpacing")
        self.onChanged(vobj,"FontName")

    def updateData(self,obj,prop):
        if prop in ["Shape","Label","Tag"]:
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
                            q = Units.Quantity(vobj.Object.Area,Units.Area).getUserPreferred()
                            qt = vobj.Object.Area/q[1]
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
                        if first:
                            text1.append(t.encode("utf8"))
                        else:
                            text2.append(t.encode("utf8"))
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






if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Space',_CommandSpace())
