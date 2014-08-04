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

import FreeCAD,ArchComponent,ArchCommands,math,Draft
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

def makeSpace(objects=None,baseobj=None,name=translate("Arch","Space")):
    """makeSpace([objects]): Creates a space object from the given objects. Objects can be one
    document object, in which case it becomes the base shape of the space object, or a list of
    selection objects as got from getSelectionEx(), or a list of tuples (object, subobjectname)"""
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
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
        FreeCADGui.doCommand("import Arch")
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
        obj.addProperty("App::PropertyLinkSubList","Boundaries","Arch",translate("Arch","The objects that make the boundaries of this space object"))
        obj.addProperty("App::PropertyFloat","Area","Arch",translate("Arch","The computed floor area of this space"))
        self.Type = "Space"

    def execute(self,obj):
        self.getShape(obj)

    def onChanged(self,obj,prop):
        if prop in ["Boundaries","Base"]:
            self.getShape(obj)
            obj.Area = self.getArea(obj)
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
        except:
            return 0


class _ViewProviderSpace(ArchComponent.ViewProviderComponent):
    "A View Provider for Section Planes"
    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        vobj.Transparency = 85
        vobj.LineWidth = 1
        vobj.LineColor = (1.0,0.0,0.0,1.0)
        vobj.DrawStyle = "Dotted"
        vobj.addProperty("App::PropertyStringList","Text",        "Arch","Text to show. Use $area, $label or $tag to insert the area, label or tag")
        vobj.addProperty("App::PropertyString",    "FontName",    "Arch","Font name")
        vobj.addProperty("App::PropertyColor",     "TextColor",   "Arch","The color of the area text")
        vobj.addProperty("App::PropertyLength",    "FontSize",    "Arch","Font size")
        vobj.addProperty("App::PropertyFloat",     "LineSpacing", "Arch","The space between the lines of text")
        vobj.addProperty("App::PropertyVector",    "TextPosition","Arch","The position of the text. Leave (0,0,0) for automatic position")
        vobj.TextColor = (0.0,0.0,0.0,1.0)
        vobj.Text = ["$label","$area"]
        vobj.FontSize = Draft.getParam("textheight",10)
        vobj.FontName = Draft.getParam("textfont","")
        vobj.LineSpacing = 1.0
        
    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Space_Tree.svg"

    def attach(self,vobj):
        ArchComponent.ViewProviderComponent.attach(self,vobj)
        from pivy import coin
        self.color = coin.SoBaseColor()
        self.font = coin.SoFont()
        self.text = coin.SoAsciiText()
        self.text.string = "d"
        self.coords = coin.SoTransform()
        self.text.justification = coin.SoAsciiText.LEFT
        label = coin.SoSeparator()
        label.addChild(self.coords)
        label.addChild(self.color)
        label.addChild(self.font)
        label.addChild(self.text)
        vobj.Annotation.addChild(label)
        self.onChanged(vobj,"TextColor")
        self.onChanged(vobj,"FontSize")
        self.onChanged(vobj,"LineSpacing")
        self.onChanged(vobj,"FontName")
    
    def updateData(self,obj,prop):
        if prop in ["Shape","Label","Tag"]:
            self.onChanged(obj.ViewObject,"Text")
            self.onChanged(obj.ViewObject,"TextPosition")
                        
    def onChanged(self,vobj,prop):
        if prop == "Text":
            if hasattr(self,"text") and hasattr(vobj,"Text"):
                self.text.string.deleteValues(0)
                texts = []
                for t in vobj.Text:
                    if t:
                        if hasattr(vobj.Object,"Area"):
                            from FreeCAD import Units
                            q = Units.Quantity(vobj.Object.Area,Units.Area)
                            q = q.getUserPreferred()[0].replace("^2","²")
                            t = t.replace("$area",q.decode("utf8"))
                        t = t.replace("$label",vobj.Object.Label.decode("utf8"))
                        if hasattr(vobj.Object,"Tag"):
                            t = t.replace("$tag",vobj.Object.Tag.decode("utf8"))
                        texts.append(t.encode("utf8"))
                if texts:
                    self.text.string.setValues(texts)
            
        elif prop == "FontName":
            if hasattr(self,"font") and hasattr(vobj,"FontName"):
                self.font.name = str(vobj.FontName)
                
        elif (prop == "FontSize"):
            if hasattr(self,"font") and hasattr(vobj,"FontSize"):
                self.font.size = vobj.FontSize.Value
            
        elif prop == "TextColor":
            if hasattr(self,"color") and hasattr(vobj,"TextColor"):
                c = vobj.TextColor
                self.color.rgb.setValue(c[0],c[1],c[2])
                
        elif prop == "TextPosition":
            if hasattr(self,"coords") and hasattr(vobj,"TextPosition"):
                import DraftVecUtils
                if DraftVecUtils.isNull(vobj.TextPosition):
                    try:
                        pos = vobj.Object.Shape.CenterOfMass
                        z = vobj.Object.Shape.BoundBox.ZMin
                        pos = FreeCAD.Vector(pos.x,pos.y,z)
                    except:
                        pos = FreeCAD.Vector()
                else:
                    pos = vobj.TextPosition
                self.coords.translation.setValue([pos.x,pos.y,pos.z])
                    
        elif prop == "LineSpacing":
            if hasattr(self,"text") and hasattr(vobj,"LineSpacing"):
                self.text.spacing = vobj.LineSpacing
                    





if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Space',_CommandSpace())
