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

import FreeCAD,FreeCADGui,Part,Draft,Component
from draftlibs import fcgeo,fcvec
from FreeCAD import Vector
from PyQt4 import QtCore

__title__="FreeCAD Wall"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeWall(baseobj=None,width=None,height=None,align="Center",name="Wall"):
    '''makeWall(obj,[width],[height],[align],[name]): creates a wall based on the
    given object'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Wall(obj)
    _ViewProviderWall(obj.ViewObject)
    if baseobj: obj.Base = baseobj
    if width: obj.Width = width
    if height: obj.Height = height
    obj.Align = align
    if obj.Base: obj.Base.ViewObject.hide()
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    c = p.GetUnsigned("WallColor")
    r = float((c>>24)&0xFF)/255.0
    g = float((c>>16)&0xFF)/255.0
    b = float((c>>8)&0xFF)/255.0
    obj.ViewObject.ShapeColor = (r,g,b,1.0)
    return obj

class _CommandWall:
    "the Arch Wall command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Wall',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Wall","Wall"),
                'Accel': "W, A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Wall","Creates a wall object from scratch or from a selected object (wire, face or solid)")}
        
    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            FreeCAD.ActiveDocument.openTransaction("Wall")
            for obj in sel:
                makeWall(obj)
            FreeCAD.ActiveDocument.commitTransaction()
        else:
            wall = makeWall()
       
class _Wall(Component.Component):
    "The Wall object"
    def __init__(self,obj):
        Component.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLength","Width","Base",
                        "The width of this wall. Not used if this wall is based on a face")
        obj.addProperty("App::PropertyLength","Height","Base",
                        "The height of this wall. Keep 0 for automatic. Not used if this wall is based on a solid")
        obj.addProperty("App::PropertyLength","Length","Base",
                        "The length of this wall. Not used if this wall is based on a shape")
        obj.addProperty("App::PropertyEnumeration","Align","Base",
                        "The alignment of this wall on its base object, if applicable")
        obj.Align = ['Left','Right','Center']
        self.Type = "Wall"
        obj.Width = 0.1
        obj.Length = 1
        obj.Height = 0
        
    def execute(self,obj):
        self.createGeometry(obj)
        
    def onChanged(self,obj,prop):
        if prop in ["Base","Height","Width","Align","Additions","Subtractions"]:
            self.createGeometry(obj)

    def createGeometry(self,obj):
        
        def getbase(wire):
            "returns a full shape from a base wire"
            dvec = fcgeo.vec(wire.Edges[0]).cross(normal)
            dvec.normalize()
            if obj.Align == "Left":
                dvec = dvec.multiply(obj.Width)
                w2 = fcgeo.offsetWire(wire,dvec)
                sh = fcgeo.bind(wire,w2)
            elif obj.Align == "Right":
                dvec = dvec.multiply(obj.Width)
                dvec = fcvec.neg(dvec)
                w2 = fcgeo.offsetWire(wire,dvec)
                sh = fcgeo.bind(wire,w2)
            elif obj.Align == "Center":
                dvec = dvec.multiply(obj.Width/2)
                w1 = fcgeo.offsetWire(wire,dvec)
                dvec = fcvec.neg(dvec)
                w2 = fcgeo.offsetWire(wire,dvec)
                sh = fcgeo.bind(w1,w2)
            if height:
                norm = Vector(normal).multiply(height)
                sh = sh.extrude(norm)
            return sh
        
        pl = obj.Placement

        # getting default values
        height = normal = None
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

        # computing shape
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                base = obj.Base.Shape.copy()
                if base.Solids:
                    pass
                elif base.Faces:
                    if height:
                        norm = normal.multiply(height)
                        base = base.extrude(norm)
                elif base.Wires:
                    temp = None
                    for wire in obj.Base.Shape.Wires:
                        sh = getbase(wire)
                        if temp:
                            temp = temp.oldFuse(sh)
                        else:
                            temp = sh
                    base = temp
        else:
            if obj.Length == 0:
                return
            v1 = Vector(0,0,0)
            v2 = Vector(obj.Length,0,0)
            w = Part.Wire(Part.Line(v1,v2).toShape())
            base = getbase(w)
            
        for app in obj.Additions:
            base = base.oldFuse(app.Shape)
            app.ViewObject.hide() #to be removed
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
        obj.Shape = base
        if not fcgeo.isNull(pl):
            obj.Placement = pl

class _ViewProviderWall(Component.ViewProviderComponent):
    "A View Provider for the Wall object"

    def __init__(self,vobj):
        Component.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):          
        return """
                /* XPM */
                static char * Arch_Wall_xpm[] = {
                "16 16 9 1",
                " 	c None",
                ".	c #543016",
                "+	c #6D2F08",
                "@	c #954109",
                "#	c #874C24",
                "$	c #AE6331",
                "%	c #C86423",
                "&	c #FD7C26",
                "*	c #F5924F",
                "                ",
                "                ",
                "       #        ",
                "      ***$#     ",
                "    .*******.   ",
                "   *##$****#+   ",
                " #**%&&##$#@@   ",
                ".$**%&&&&+@@+   ",
                "@&@#$$%&&@@+..  ",
                "@&&&%#.#$#+..#$.",
                " %&&&&+%#.$**$@+",
                "   @%&+&&&$##@@+",
                "     @.&&&&&@@@ ",
                "        @%&&@@  ",
                "           @+   ",
                "                "};
		"""

FreeCADGui.addCommand('Arch_Wall',_CommandWall())
