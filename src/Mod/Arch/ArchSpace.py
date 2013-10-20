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

import FreeCAD,FreeCADGui,ArchComponent,ArchCommands,math,Draft
from DraftTools import translate
from PyQt4 import QtCore

def makeSpace(objects=None,name="Space"):
    """makeSpace([objects]): Creates a space object from the given objects. Objects can be one
    document object, in which case it becomes the base shape of the space object, or a list of
    selection objects as got from getSelectionEx(), or a list of tuples (object, subobjectname)"""
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Space(obj)
    _ViewProviderSpace(obj.ViewObject)
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

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Space")))
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
            FreeCAD.Console.PrintMessage(str(translate("Arch","Please select a base object\n")))
            FreeCADGui.Control.showDialog(ArchComponent.SelectionTaskPanel())
            FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(nextCommand="Arch_Space")
            FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)


class _Space(ArchComponent.Component):
    "A space object"
    def __init__(self,obj):
        obj.Proxy = self
        obj.addProperty("App::PropertyLink","Base","Arch",
                        str(translate("Arch","A base shape defining this space")))
        obj.addProperty("App::PropertyLinkSubList","Boundaries","Arch",
                        str(translate("Arch","The objects that make the boundaries of this space object")))
        self.Type = "Space"

    def execute(self,obj):
        self.getShape(obj)

    def onChanged(self,obj,prop):
        if prop in ["Boundaries","Base"]:
            self.getShape(obj)
            
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
            sh = obj.Shape.copy()
            cutplane,v1,v2 = ArchCommands.getCutVolume(pl,sh)
            e = sh.section(cutplane)
            e = DraftGeomUtils.sortEdges(e.Edges)
            w = Part.Wire(e)
            f = Part.Face(w)
            return round(f.Area,Draft.getParam("dimPrecision",6))
        except:
            return 0


class _ViewProviderSpace(ArchComponent.ViewProviderComponent):
    "A View Provider for Section Planes"
    def __init__(self,vobj):
        vobj.Transparency = 85
        vobj.LineWidth = 1
        vobj.LineColor = (1.0,0.0,0.0,1.0)
        vobj.DrawStyle = "Dotted"
        vobj.addProperty("App::PropertyString","Override","Base",
            "Text override. Use $area to insert the area")
        vobj.addProperty("App::PropertyColor","TextColor","Base",
            "The color of the area text")
        vobj.TextColor = (1.0,0.0,0.0,1.0)
        vobj.Override = "$area m2"
        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        
    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Space_Tree.svg"

    def claimChildren(self):
        if self.Object.Base:
            return [self.Object.Base]
        else:
            return []

    def setDisplayMode(self,mode):
        if mode == "Detailed":
            self.setAnnotation(True)
            return "Flat Lines"
        else:
            self.setAnnotation(False)
            return mode
            
    def getArea(self,obj):
        "returns a formatted area text"
        area = str(obj.Proxy.getArea(obj))
        if obj.ViewObject.Override:
            text = obj.ViewObject.Override
            area = text.replace("$area",str(area))
        return str(area)

    def setAnnotation(self,recreate=True):
        if hasattr(self,"Object"):
            if hasattr(self,"area"):
                if self.area:
                    self.Object.ViewObject.Annotation.removeChild(self.area)
                    self.area = None
                    self.coords = None
                    self.anno = None
            if recreate:
                area = self.getArea(self.Object)
                if area:
                    from pivy import coin
                    import SketcherGui
                    self.area = coin.SoSeparator()
                    self.coords = coin.SoTransform()
                    if self.Object.Shape:
                        if not self.Object.Shape.isNull():
                            c = self.Object.Shape.CenterOfMass
                            self.coords.translation.setValue([c.x,c.y,c.z])
                    self.anno = coin.SoType.fromName("SoDatumLabel").createInstance()
                    self.anno.string.setValue(area)
                    self.anno.datumtype.setValue(6)
                    color = coin.SbVec3f(self.Object.ViewObject.TextColor[:3])
                    self.anno.textColor.setValue(color)
                    self.area.addChild(self.coords)
                    self.area.addChild(self.anno)
                    self.Object.ViewObject.Annotation.addChild(self.area)
                    
    def updateData(self,obj,prop):
        if prop == "Shape":
            if hasattr(self,"area"):
                if self.area:
                    area = self.getArea(obj)
                    self.anno.string.setValue(area)
                    if not obj.Shape.isNull():
                        c = obj.Shape.CenterOfMass
                        self.coords.translation.setValue([c.x,c.y,c.z])
                        
    def onChanged(self,vobj,prop):
        if prop in ["Override","TextColor"]:
            if vobj.DisplayMode == "Detailed":
                self.setAnnotation(True)
        return


FreeCADGui.addCommand('Arch_Space',_CommandSpace())
