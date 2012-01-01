#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
#*   Yorik van Havre <yorik@uncreated.net>                                 *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
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

import FreeCAD,FreeCADGui,Draft,ArchComponent
from draftlibs import fcvec
from FreeCAD import Vector
from PyQt4 import QtCore

__title__="FreeCAD Structure"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeStructure(baseobj=None,length=None,width=None,height=None,name="Structure"):
    '''makeStructure([obj],[length],[width],[heigth],[swap]): creates a
    structure element based on the given profile object and the given
    extrusion height. If no base object is given, you can also specify
    length and width for a cubic object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Structure(obj)
    _ViewProviderStructure(obj.ViewObject)
    if baseobj: obj.Base = baseobj
    if length: obj.Length = length
    if width: obj.Width = width
    if height: obj.Height = height
    if obj.Base:
        obj.Base.ViewObject.hide()
    else:
        if (not obj.Width) and (not obj.Length):
            obj.Width = 1
            obj.Height = 1
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    c = p.GetUnsigned("StructureColor")
    r = float((c>>24)&0xFF)/255.0
    g = float((c>>16)&0xFF)/255.0
    b = float((c>>8)&0xFF)/255.0
    obj.ViewObject.ShapeColor = (r,g,b,1.0)
    return obj

class _CommandStructure:
    "the Arch Structure command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Structure',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Structure","Structure"),
                'Accel': "S, T",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Structure","Creates a structure object from scratch or from a selected object (sketch, wire, face or solid)")}
        
    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            FreeCAD.ActiveDocument.openTransaction("Structure")
            for obj in sel:
                makeStructure(obj)
            FreeCAD.ActiveDocument.commitTransaction()
        else:
            makeStructure()
       
class _Structure(ArchComponent.Component):
    "The Structure object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLength","Length","Base",
                        "The length of this element, if not based on a profile")
        obj.addProperty("App::PropertyLength","Width","Base",
                        "The width of this element, if not based on a profile")
        obj.addProperty("App::PropertyLength","Height","Base",
                        "The height or extrusion depth of this element. Keep 0 for automatic")
        self.Type = "Structure"
        
    def execute(self,obj):
        self.createGeometry(obj)
        
    def onChanged(self,obj,prop):
        if prop in ["Base","Length","Width","Height","Normal","Additions","Subtractions"]:
            self.createGeometry(obj)

    def createGeometry(self,obj):
        import Part
        from draftlibs import fcgeo
        # getting default values
        height = normal = None
        if obj.Length:
            length = obj.Length
        else:
            length = 1
        if obj.Width:
            width = obj.Width
        else:
            width = 1
        if obj.Height:
            height = obj.Height
        else:
            for p in obj.InList:
                if Draft.getType(p) == "Floor":
                    height = p.Height
        if not height: height = 1
        if obj.Normal == Vector(0,0,0):
            normal = Vector(0,0,1)
        else:
            normal = Vector(obj.Normal)

        # creating shape
        pl = obj.Placement
        norm = normal.multiply(height)
        base = None
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                base = obj.Base.Shape.copy()
                if base.Solids:
                    pass
                elif base.Faces:
                    base = base.extrude(normal)
                elif (len(base.Wires) == 1) and base.Wires[0].isClosed():
                    base = Part.Face(base.Wires[0])
                    base = base.extrude(normal)                
        else:
            l2 = length/2 or 0.5
            w2 = width/2 or 0.5
            v1 = Vector(-l2,-w2,0)
            v2 = Vector(l2,-w2,0)
            v3 = Vector(l2,w2,0)
            v4 = Vector(-l2,w2,0)
            base = Part.makePolygon([v1,v2,v3,v4,v1])
            base = Part.Face(base)
            base = base.extrude(normal)
        for app in obj.Additions:
            base = base.oldFuse(app.Shape)
            app.ViewObject.hide() # to be removed
        for hole in obj.Subtractions:
            cut = False
            if hasattr(hole,"Proxy"):
                if hasattr(hole.Proxy,"Subvolume"):
                    if hole.Proxy.Subvolume:
                        print "cutting subvolume",hole.Proxy.Subvolume
                        base = base.cut(hole.Proxy.Subvolume)
                        cut = True
            if not cut:
                if hasattr(obj,"Shape"):
                    base = base.cut(hole.Shape)
                    hole.ViewObject.hide() # to be removed
        if base:
            obj.Shape = base
            if not fcgeo.isNull(pl): obj.Placement = pl

class _ViewProviderStructure(ArchComponent.ViewProviderComponent):
    "A View Provider for the Structure object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):          
        return ":/icons/Arch_Structure_Tree.svg"

FreeCADGui.addCommand('Arch_Structure',_CommandStructure())
